#include "Standalone/ModulePluginProcessor.h"
#include "Core/ModulePresets.h"
#include "AllocationAudit.h"
#include "ZastavaPresetIdentity.h"
#include "ZastavaS1Identity.h"
#include <juce_cryptography/juce_cryptography.h>
#include <chrono>
#include <fstream>
#include <iostream>
#include <map>

namespace
{
using P=hpeq::standalone::ModulePluginProcessor;constexpr auto kind=hpeq::standalone::ModuleKind::Zastava;
juce::var object(){return new juce::DynamicObject;}
void put(juce::var& v,const char* k,const juce::var& value){v.getDynamicObject()->setProperty(k,value);}
juce::String sha(const void* data,size_t bytes){return juce::SHA256(data,bytes).toHexString();}
juce::String sha(const juce::MemoryBlock& b){return sha(b.getData(),b.getSize());}
std::map<juce::String,std::vector<float>> inputs;
const std::vector<float>& input(const juce::String& path)
{
    if(auto at=inputs.find(path);at!=inputs.end())return at->second;
    juce::File file(path);const auto size=file.getSize();if(size<=0||size%8)throw std::runtime_error("Expected stereo f32 input");
    std::vector<float> data(static_cast<size_t>(size/4));juce::FileInputStream stream(file);if(stream.read(data.data(),static_cast<int>(size))!=size)throw std::runtime_error("Short input read");
    return inputs.emplace(path,std::move(data)).first->second;
}
struct Head:juce::AudioPlayHead
{double bpm=120;int at=0;juce::Optional<PositionInfo> getPosition()const override{PositionInfo p;p.setBpm(bpm);p.setPpqPosition(at/48000.*bpm/60);p.setIsPlaying(true);return p;}};
struct Render{std::vector<float> data;size_t allocations=0;double seconds=0;bool finite=true;double peak=0;std::uint64_t hits=0;bool missing=false;};
Render render(P& p,const std::vector<float>& main,const std::vector<float>* key,double bpm,bool prepare,int limit=0)
{
    if(key&&key->size()!=main.size())throw std::runtime_error("Key/main lengths differ");
    const int frames=limit?std::min(limit,static_cast<int>(main.size()/2)):static_cast<int>(main.size()/2);
    if(prepare){auto layout=p.getBusesLayout();layout.inputBuses.set(0,juce::AudioChannelSet::stereo());layout.outputBuses.set(0,juce::AudioChannelSet::stereo());layout.inputBuses.set(1,key?juce::AudioChannelSet::stereo():juce::AudioChannelSet::disabled());if(!p.setBusesLayout(layout))throw std::runtime_error("Layout rejected");p.prepareToPlay(48000,256);}
    Render r;r.data.resize(static_cast<size_t>(frames)*2);juce::AudioBuffer<float> block(key?4:2,256);juce::MidiBuffer midi;Head head;head.bpm=bpm;p.setPlayHead(&head);
    for(int at=0;at<frames;at+=256)
    {
        const int count=std::min(256,frames-at);head.at=at;
        for(int c=0;c<2;++c)for(int n=0;n<count;++n){block.setSample(c,n,main[static_cast<size_t>((at+n)*2+c)]);if(key)block.setSample(c+2,n,(*key)[static_cast<size_t>((at+n)*2+c)]);}
        juce::AudioBuffer<float> view(block.getArrayOfWritePointers(),key?4:2,0,count);
        hamlo::test::allocation::begin();const auto start=std::chrono::steady_clock::now();p.processBlock(view,midi);r.seconds+=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();r.allocations+=hamlo::test::allocation::finish();
        for(int c=0;c<2;++c)for(int n=0;n<count;++n){const auto value=block.getSample(c,n);r.data[static_cast<size_t>((at+n)*2+c)]=value;r.finite=r.finite&&std::isfinite(value);r.peak=std::max(r.peak,std::abs(static_cast<double>(value)));}
    }
    p.setPlayHead(nullptr);r.hits=p.modernReading().hits;r.missing=p.modernReading().missingKey;return r;
}
juce::var values(P& p)
{auto v=object();for(auto* base:p.getParameters()){auto* q=dynamic_cast<juce::RangedAudioParameter*>(base);put(v,q->paramID.toRawUTF8(),p.parameterValue(q->paramID));}return v;}
juce::var identity()
{auto v=object();put(v,"dsp_id",ZASTAVA_PRESET_DSP_ID);put(v,"method_id",ZASTAVA_PRESET_METHOD_ID);put(v,"catalog_id",ZASTAVA_PRESET_CATALOG_ID);put(v,"production_id",hpeq::dsp::zastavaS1SourceIdentity());if(juce::String(hpeq::dsp::zastavaS1SourceIdentity())!=ZASTAVA_S1_SOURCE_ID)throw std::runtime_error("Stale shared library");return v;}
juce::var catalog()
{
    auto result=identity();auto p=std::make_unique<P>(kind);juce::Array<juce::var> rows;
    const auto& list=hpeq::presets::factoryModulePresets(p->getSpec().type);
    for(int i=0;i<p->getNumPrograms();++i)
    {
        p->setCurrentProgram(i);auto row=object();put(row,"program",i);put(row,"id",p->programIdentifier(i));put(row,"name",p->getProgramName(i));put(row,"values",values(*p));put(row,"modified",p->isProgramModified());
        if(i>0){const auto& s=list[static_cast<size_t>(i-1)];put(row,"genre",s.genre);put(row,"role",s.group);put(row,"note",s.note);put(row,"description",s.description);put(row,"material",s.material);put(row,"depth",s.depth);put(row,"bpm",s.bpm);}rows.add(row);
    }
    put(result,"programs",rows);return result;
}
juce::var job(const juce::var& request)
{
    auto p=std::make_unique<P>(kind);const int program=static_cast<int>(request["program"]);p->setCurrentProgram(program);
    if(auto* overrides=request["overrides"].getDynamicObject())for(const auto& pair:overrides->getProperties())p->setParameterValue(pair.name.toString(),static_cast<double>(pair.value));
    const auto& main=input(request["main"].toString());const auto keyPath=request["key"].toString();const auto* key=keyPath.isEmpty()?nullptr:&input(keyPath);const double bpm=static_cast<double>(request["bpm"]);
    juce::MemoryBlock saved;p->getStateInformation(saved);auto result=render(*p,main,key,bpm,true);bool stateExact=true,audioExact=true,parametersExact=true;
    if(static_cast<bool>(request["state"]))for(int edited=0;edited<2;++edited)
    {
        auto q=std::make_unique<P>(kind);
        if(edited){q->setCurrentProgram(program==7?206:7);q->setParameterValue("bypass",1);render(*q,main,key,bpm,true,1024);}
        q->setStateInformation(saved.getData(),static_cast<int>(saved.getSize()));juce::MemoryBlock restored;q->getStateInformation(restored);
        stateExact=stateExact&&saved==restored&&q->getCurrentProgram()==program&&q->programIdentifier(program)==p->programIdentifier(program);
        for(int n=16;n<p->getParameters().size();++n)parametersExact=parametersExact&&p->getParameters()[n]->getValue()==q->getParameters()[n]->getValue();
        const auto again=render(*q,main,key,bpm,!edited);audioExact=audioExact&&result.data.size()==again.data.size()&&std::memcmp(result.data.data(),again.data.data(),result.data.size()*4)==0;result.allocations+=again.allocations;result.finite=result.finite&&again.finite;
    }
    const juce::File output(request["output"].toString());if(output.exists())throw std::runtime_error("Refusing to overwrite render");output.getParentDirectory().createDirectory();if(!output.replaceWithData(result.data.data(),result.data.size()*4))throw std::runtime_error("Cannot write render");
    auto reply=identity();put(reply,"program",program);put(reply,"id",p->programIdentifier(program));put(reply,"values",values(*p));put(reply,"frames",static_cast<int>(result.data.size()/2));put(reply,"latency",p->getLatencySamples());put(reply,"finite",result.finite);put(reply,"allocations",static_cast<int>(result.allocations));put(reply,"peak",result.peak);put(reply,"hits",static_cast<juce::int64>(result.hits));put(reply,"missing",result.missing);put(reply,"mean_fraction",result.seconds/(result.data.size()/2/48000.));put(reply,"state_exact",stateExact);put(reply,"audio_exact",audioExact);put(reply,"parameters_exact",parametersExact);put(reply,"state_sha256",sha(saved));put(reply,"output_sha256",sha(result.data.data(),result.data.size()*4));return reply;
}
}
int main()
{
    juce::ScopedJuceInitialiser_GUI gui;std::string line;
    while(std::getline(std::cin,line))
    {
        try{const auto request=juce::JSON::parse(line);const auto command=request["cmd"].toString();const auto response=command=="catalog"?catalog():command=="hello"?identity():job(request);std::cout<<juce::JSON::toString(response,true).toStdString()<<std::endl;}
        catch(const std::exception& e){auto error=object();put(error,"error",e.what());std::cout<<juce::JSON::toString(error,true).toStdString()<<std::endl;}
    }
    return 0;
}

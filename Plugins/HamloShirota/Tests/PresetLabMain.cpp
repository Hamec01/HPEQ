#include "Standalone/ModulePluginProcessor.h"
#include "Standalone/V2Parameters.h"
#include "Standalone/GenrePresets.h"
#include "S5Identity.h"
#include "AllocationAudit.h"
#include <juce_cryptography/juce_cryptography.h>
#include <cstring>
#include <cmath>
#include <memory>
#include <iostream>
#include <stdexcept>

namespace
{
using hpeq::standalone::ModulePluginProcessor;
juce::var object() { return juce::var(new juce::DynamicObject()); }
void put(juce::var& value,const char* key,const juce::var& data) { value.getDynamicObject()->setProperty(key,data); }
struct Head final:juce::AudioPlayHead
{
    double bpm=120,rate=48000;int sample=0;
    juce::Optional<PositionInfo> getPosition() const override
    { PositionInfo p;p.setBpm(bpm);p.setTimeInSamples(sample);p.setPpqPosition(sample/rate*bpm/60);p.setIsPlaying(true);return p; }
};
juce::AudioBuffer<float> render(ModulePluginProcessor& p,const juce::AudioBuffer<float>& input,double rate,double bpm,int block,size_t& allocations)
{
    Head head;head.rate=rate;head.bpm=bpm;p.setPlayHead(&head);p.prepareToPlay(rate,256);
    auto result=input;juce::MidiBuffer midi;
    hamlo::test::allocation::begin();
    for(int at=0;at<result.getNumSamples();at+=block)
    {head.sample=at;juce::AudioBuffer<float> part(result.getArrayOfWritePointers(),2,at,juce::jmin(block,result.getNumSamples()-at));p.processBlock(part,midi);}
    allocations=hamlo::test::allocation::finish();p.setPlayHead(nullptr);return result;
}
juce::var settings(ModulePluginProcessor& p)
{
    auto values=object();
    for(auto* parameter:p.getParameters())
        if(auto* named=dynamic_cast<juce::RangedAudioParameter*>(parameter))
            values.getDynamicObject()->setProperty(named->paramID,p.parameterValue(named->paramID));
    return values;
}
}
int main(int argc,char** argv)
{
    juce::ScopedJuceInitialiser_GUI gui;
    try
    {
        if(argc!=3)throw std::runtime_error("PresetLab requests.json NEW_output_directory");
        const juce::File request=juce::File::getCurrentWorkingDirectory().getChildFile(argv[1]);
        const auto input=juce::JSON::parse(request);if(!input.isObject())throw std::runtime_error("Invalid request");
        const juce::File folder=juce::File::getCurrentWorkingDirectory().getChildFile(argv[2]);
        if(folder.exists())throw std::runtime_error("Refusing to overwrite native run");
        if(!folder.createDirectory())throw std::runtime_error("Cannot create run directory");
        auto report=object();put(report,"sound_id",SHIROTA_S5_SOUND_ID);put(report,"method_id",SHIROTA_S5_METHOD_ID);
        auto baseOwned=std::make_unique<ModulePluginProcessor>(hpeq::standalone::ModuleKind::Shirota);auto& base=*baseOwned;
        put(report,"program_count",base.getNumPrograms());put(report,"genre_count",static_cast<int>(hpeq::standalone::genrePresets().size()));
        if(static_cast<bool>(input["catalog"]))
        {
            juce::Array<juce::var> programs;
            const int firstGenre=base.getNumPrograms()-static_cast<int>(hpeq::standalone::genrePresets().size());
            for(int i=0;i<base.getNumPrograms();++i)
            {
                base.setCurrentProgram(i);auto row=object();put(row,"program",i);put(row,"name",base.getProgramName(i));put(row,"values",settings(base));
                if(i>=firstGenre)
                {
                    const auto& g=hpeq::standalone::genrePresets()[static_cast<size_t>(i-firstGenre)];
                    put(row,"id",g.id);put(row,"genre",g.genre);put(row,"group",g.group);put(row,"note",g.note);put(row,"material",g.material);put(row,"depth",g.depth);put(row,"bpm",g.bpm);
                }programs.add(row);
            }put(report,"programs",programs);
            juce::Array<juce::var> params;
            for(const auto& spec:hpeq::standalone::v2Parameters())
            {auto r=object();put(r,"id",spec.id);put(r,"minimum",spec.minimum);put(r,"maximum",spec.maximum);put(r,"interval",spec.interval);params.add(r);}put(report,"parameters",params);
        }
        juce::Array<juce::var> rows;
        if(const auto* jobs=input["jobs"].getArray())for(const auto& job:*jobs)
        {
            const auto id=job["id"].toString();if(id.isEmpty()||id.containsAnyOf("/\\.:"))throw std::runtime_error("Invalid job ID");
            juce::MemoryBlock bytes;const juce::File source(job["input"].toString());
            if(!source.loadFileAsData(bytes)||bytes.getSize()%8!=0)throw std::runtime_error("Invalid stereo float input");
            const int frames=static_cast<int>(bytes.getSize()/8);juce::AudioBuffer<float> audio(2,frames);
            const auto* raw=static_cast<const float*>(bytes.getData());for(int i=0;i<frames;++i)for(int ch=0;ch<2;++ch)audio.setSample(ch,i,raw[2*i+ch]);
            auto pOwned=std::make_unique<ModulePluginProcessor>(hpeq::standalone::ModuleKind::Shirota);auto& p=*pOwned;p.setCurrentProgram(static_cast<int>(job["program"]));
            if(auto* values=job["values"].getDynamicObject())for(const auto& entry:values->getProperties())
            {if(!p.parameter(entry.name.toString()))throw std::runtime_error("Unknown parameter");p.setParameterValue(entry.name.toString(),static_cast<double>(entry.value));}
            juce::MemoryBlock state;p.getStateInformation(state);const auto original=settings(p);
            const double rate=static_cast<double>(job["rate"]),bpm=static_cast<double>(job["bpm"]);const int block=job.hasProperty("block")?static_cast<int>(job["block"]):256;
            if(rate<8000||rate>192000||bpm<=0||block<1||block>4096)throw std::runtime_error("Invalid render configuration");
            size_t allocations=0;const auto y=render(p,audio,rate,bpm,block,allocations);bool roundtrip=true;
            if(static_cast<bool>(job["roundtrip"]))
            {
                auto qOwned=std::make_unique<ModulePluginProcessor>(hpeq::standalone::ModuleKind::Shirota);auto& q=*qOwned;q.setParameterValue("v2Width",3);q.setStateInformation(state.getData(),static_cast<int>(state.getSize()));
                roundtrip=q.getCurrentProgram()==p.getCurrentProgram()&&juce::JSON::toString(original)==juce::JSON::toString(settings(q));
                size_t second=0;const auto z=render(q,audio,rate,bpm,block,second);allocations+=second;
                for(int ch=0;ch<2;++ch)roundtrip=roundtrip&&std::memcmp(y.getReadPointer(ch),z.getReadPointer(ch),static_cast<size_t>(frames)*4)==0;
            }
            std::vector<float> interleaved(static_cast<size_t>(frames)*2);bool finite=true;
            for(int i=0;i<frames;++i)for(int ch=0;ch<2;++ch){const float v=y.getSample(ch,i);finite=finite&&std::isfinite(v);interleaved[static_cast<size_t>(2*i+ch)]=v;}
            const auto file=folder.getChildFile(id+".f32");if(!file.replaceWithData(interleaved.data(),interleaved.size()*4))throw std::runtime_error("Cannot write render");
            auto row=object();put(row,"id",id);put(row,"sha256",juce::SHA256(interleaved.data(),interleaved.size()*4).toHexString());put(row,"input_sha256",juce::SHA256(bytes).toHexString());
            put(row,"state_sha256",juce::SHA256(state).toHexString());put(row,"roundtrip",roundtrip);put(row,"finite",finite);put(row,"allocations",static_cast<juce::int64>(allocations));put(row,"latency",p.getLatencySamples());put(row,"values",original);rows.add(row);
            std::cout<<id<<" finite="<<finite<<" state="<<roundtrip<<" allocations="<<allocations<<std::endl;
        }
        put(report,"renders",rows);if(!folder.getChildFile("result.json").replaceWithText(juce::JSON::toString(report)))throw std::runtime_error("Cannot write result");return 0;
    }
    catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}
}

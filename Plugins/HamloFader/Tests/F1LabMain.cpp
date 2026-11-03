#include "Standalone/ModulePluginProcessor.h"
#include "FaderF1Identity.h"
#include "AllocationAudit.h"
#include <juce_cryptography/juce_cryptography.h>
#include <fstream>
#include <iostream>
#include <cmath>

namespace
{
using P=hpeq::standalone::ModulePluginProcessor;
constexpr auto kind=hpeq::standalone::ModuleKind::Fader;
struct Case{juce::String name;int program=0;double rate=48000;int channels=2,block=127,scenario=-1;std::vector<std::pair<juce::String,double>> values;};
juce::var object(){return juce::var(new juce::DynamicObject);}
void put(juce::var& v,const char* k,const juce::var& x){v.getDynamicObject()->setProperty(k,x);}
juce::String hash(const juce::MemoryBlock& b){return juce::SHA256(b).toHexString();}
void write(const juce::File& p,const juce::var& v){p.getParentDirectory().createDirectory();if(!p.replaceWithText(juce::JSON::toString(v)))throw std::runtime_error("Cannot write JSON");}
juce::MemoryBlock state(P& p){juce::MemoryBlock b;p.getStateInformation(b);return b;}
void load(P& p,const juce::MemoryBlock& b){p.setStateInformation(b.getData(),static_cast<int>(b.getSize()));}
juce::var values(P& p)
{auto v=object();for(auto* base:p.getParameters()){auto* q=dynamic_cast<juce::RangedAudioParameter*>(base);put(v,q->paramID.toRawUTF8(),q->getValue());}return v;}
juce::var contract(P& p)
{
    auto v=object();juce::Array<juce::var> params,programs;
    for(auto* base:p.getParameters())
    {auto* q=dynamic_cast<juce::RangedAudioParameter*>(base);auto r=object();put(r,"id",q->paramID);put(r,"name",q->getName(100));put(r,"default",q->getDefaultValue());const auto& range=q->getNormalisableRange();put(r,"min",range.start);put(r,"max",range.end);put(r,"interval",range.interval);put(r,"skew",range.skew);put(r,"choices",juce::var(q->getAllValueStrings()));params.add(r);}
    for(int i=0;i<p.getNumPrograms();++i){p.setCurrentProgram(i);auto r=object();put(r,"index",i);put(r,"name",p.getProgramName(i));put(r,"values",values(p));programs.add(r);}
    put(v,"parameters",params);put(v,"programs",programs);put(v,"state_tag",juce::AudioProcessor::getXmlFromBinary(state(p).getData(),static_cast<int>(state(p).getSize()))->getTagName());return v;
}
std::vector<Case> cases(P& p)
{
    std::vector<Case> all;
    for(int program=0;program<8;++program)for(double rate:{44100.,48000.,96000.})for(int channels:{1,2})for(int block:{64,127,512})
        all.push_back({"program_"+juce::String(program)+"_"+juce::String(rate,0)+"_ch"+juce::String(channels)+"_b"+juce::String(block),program,rate,channels,block});
    for(const auto& spec:p.getSpec().parameters)for(int edge=0;edge<2;++edge){Case c;c.name="edge_"+spec.id+"_"+juce::String(edge);c.values={{spec.id,edge?spec.maximum:spec.minimum}};all.push_back(c);}
    for(double gain:{-60.,-6.,24.})for(int polarity=0;polarity<4;++polarity){Case c;c.name="gain_pol_"+juce::String(gain)+"_"+juce::String(polarity);c.values={{"gainDb",gain},{"polarity",polarity}};all.push_back(c);}
    for(double delay:{.125,2.,20.})for(double mix:{0.,.3,1.}){Case c;c.name="delay_mix_"+juce::String(delay)+"_"+juce::String(mix);c.values={{"delayMs",delay},{"mix",mix}};all.push_back(c);}
    for(int mute:{0,1})for(double mix:{0.,.5,1.}){Case c;c.name="mute_mix_"+juce::String(mute)+"_"+juce::String(mix);c.values={{"mute",mute},{"mix",mix},{"gainDb",6}};all.push_back(c);}
    for(int bypass:{0,1}){Case c;c.name="bypass_"+juce::String(bypass);c.values={{"bypass",bypass},{"gainDb",24},{"delayMs",20},{"polarity",3}};all.push_back(c);}
    for(double rate:{8000.,192000.})for(int channels:{1,2}){Case c;c.name="rate_"+juce::String(rate,0)+"_ch"+juce::String(channels);c.rate=rate;c.channels=channels;c.values={{"delayMs",20},{"gainDb",-6},{"polarity",2}};all.push_back(c);}
    for(int scenario=0;scenario<7;++scenario)for(int block:{1,64,127,512,4096}){Case c;c.name="events_"+juce::String(scenario)+"_b"+juce::String(block);c.block=block;c.scenario=scenario;all.push_back(c);}
    return all;
}
void configure(P& p,const Case& c){p.setCurrentProgram(c.program);for(const auto& v:c.values)p.setParameterValue(v.first,v.second);}
void layout(P& p,int channels)
{auto l=p.getBusesLayout();l.inputBuses.set(0,juce::AudioChannelSet::canonicalChannelSet(channels));l.outputBuses.set(0,juce::AudioChannelSet::canonicalChannelSet(channels));if(!p.setBusesLayout(l))throw std::runtime_error("Unsupported layout");}
void event(P& p,int type,int n)
{
    const double gain[]{-60,24,-6,0,12,-12,0};const double delay[]{20,.125,0,7.4,2,0,19.9};
    if(type==0)p.setParameterValue("gainDb",gain[(n-1)%7]);
    if(type==1)p.setParameterValue("delayMs",delay[(n-1)%7]);
    if(type==2)p.setParameterValue("polarity",n%4);
    if(type==3){p.setParameterValue("gainDb",-12);p.setParameterValue("mix",(n%3)*.5);}
    if(type==4){p.setParameterValue("bypass",n%3==1);p.setParameterValue("mute",n%3==2);p.setParameterValue("delayMs",n%2?12:0);p.setParameterValue("gainDb",6);}
    if(type==5)p.setCurrentProgram(n%8);
}
struct Render{juce::MemoryBlock bytes;juce::var meters;size_t allocations=0;bool finite=true;};
Render render(P& p,const Case& c,bool prepare)
{
    if(prepare)p.prepareToPlay(c.rate,256);const int frames=8192;juce::AudioBuffer<float> buffer(c.channels,4096);juce::MidiBuffer midi;Render r;juce::Array<juce::var> observations;
    std::vector<float> interleaved(static_cast<size_t>(4096*c.channels));
    for(int at=0;at<frames;)
    {
        if(c.scenario>=0&&at>0&&at%1024==0){event(p,c.scenario,at/1024);if(c.scenario==6&&at==4096){p.releaseResources();p.prepareToPlay(c.rate,256);}}
        const int count=std::min({c.block,frames-at,1024-at%1024});juce::AudioBuffer<float> view(buffer.getArrayOfWritePointers(),c.channels,count);
        for(int ch=0;ch<c.channels;++ch)for(int i=0;i<count;++i){const int n=at+i;const double t=n/c.rate;view.setSample(ch,i,static_cast<float>(.11*std::sin(t*juce::MathConstants<double>::twoPi*(ch?997:701))+.07*std::cos(t*juce::MathConstants<double>::twoPi*83)+(n%2003==17?.2:0)));}
        hamlo::test::allocation::begin();p.processBlock(view,midi);r.allocations+=hamlo::test::allocation::finish();
        for(int i=0;i<count;++i)for(int ch=0;ch<c.channels;++ch){const float value=view.getSample(ch,i);r.finite=r.finite&&std::isfinite(value);interleaved[static_cast<size_t>(i*c.channels+ch)]=value;}
        r.bytes.append(interleaved.data(),static_cast<size_t>(count*c.channels)*sizeof(float));at+=count;
        if(at%1024==0){auto m=object();const auto& node=p.moduleNode();put(m,"sample",at);put(m,"input",node.getFaderInputDb(0));put(m,"long",node.getFaderInputLongDb(0));put(m,"output",node.getFaderOutputDb(0));put(m,"latency",p.getLatencySamples());observations.add(m);}
    }
    r.meters=observations;return r;
}
int run(bool capture,const juce::File& folder,const juce::File& report)
{
    if(report.exists())throw std::runtime_error("Refusing to overwrite report");if(capture&&folder.exists())throw std::runtime_error("Refusing to overwrite capture");if(capture)folder.createDirectory();
    const auto old=capture?juce::var():juce::JSON::parse(folder.getChildFile("capture.json"));if(!capture&&old["method_id"].toString()!=FADER_F1_METHOD_ID)throw std::runtime_error("Changed method; recapture both sides");
    auto probe=std::make_unique<P>(kind);const auto actualContract=contract(*probe);const auto matrix=cases(*probe);int failures=0;juce::Array<juce::var> rows;
    if(!capture&&juce::JSON::toString(actualContract)!=juce::JSON::toString(old["contract"]))throw std::runtime_error("Changed host contract");
    if(!capture&&old["cases"].size()!=static_cast<int>(matrix.size()))throw std::runtime_error("Changed cases");
    for(size_t i=0;i<matrix.size();++i)
    {
        const auto& c=matrix[i];auto row=object();put(row,"case",c.name);juce::Array<juce::var> observations;
        auto p=std::make_unique<P>(kind);layout(*p,c.channels);configure(*p,c);const auto saved=state(*p);const auto file=folder.getChildFile(c.name+".state");juce::MemoryBlock original=saved;
        if(capture){if(!file.replaceWithData(saved.getData(),saved.getSize()))throw std::runtime_error("Cannot write state");}
        else{original.reset();if(!file.loadFileAsData(original))throw std::runtime_error("Missing original state");}
        bool passed=saved==original;put(row,"initial_state_sha256",hash(saved));
        for(int restore=0;restore<3;++restore)
        {
            auto q=std::make_unique<P>(kind);layout(*q,c.channels);
            if(restore==0)configure(*q,c);
            else
            {
                if(restore==2){q->setCurrentProgram(7);q->setParameterValue("gainDb",24);q->setParameterValue("delayMs",20);q->setParameterValue("polarity",3);q->setParameterValue("mute",1);Case dirty=c;dirty.scenario=-1;render(*q,dirty,true);}
                load(*q,original);
            }
            const auto observedState=state(*q);auto observed=object();put(observed,"restore",restore);put(observed,"state_sha256",hash(observedState));put(observed,"parameters",values(*q));put(observed,"program",q->getCurrentProgram());
            const auto audio=render(*q,c,restore!=2);put(observed,"render_sha256",hash(audio.bytes));put(observed,"meters",audio.meters);put(observed,"finite",audio.finite);put(observed,"allocations",static_cast<int>(audio.allocations));
            const auto raw=folder.getChildFile(c.name+"_"+juce::String(restore)+".f32"),restored=folder.getChildFile(c.name+"_"+juce::String(restore)+".state");
            if(capture){if(!raw.replaceWithData(audio.bytes.getData(),audio.bytes.getSize())||!restored.replaceWithData(observedState.getData(),observedState.getSize()))throw std::runtime_error("Cannot write observation");}
            else
            {juce::MemoryBlock rawBefore,stateBefore;if(!raw.loadFileAsData(rawBefore)||!restored.loadFileAsData(stateBefore))throw std::runtime_error("Missing reference observation");passed=passed&&rawBefore==audio.bytes&&stateBefore==observedState&&juce::JSON::toString(observed)==juce::JSON::toString(old["cases"][static_cast<int>(i)]["observations"][restore]);}
            passed=passed&&audio.finite&&audio.allocations==0;observations.add(observed);
        }
        put(row,"observations",observations);put(row,"passed",passed);rows.add(row);if(!passed)++failures;
        std::cout<<(passed?"PASS ":"FAIL ")<<c.name<<std::endl;
    }
    auto result=object();put(result,"source_id",FADER_F1_SOURCE_ID);put(result,"method_id",FADER_F1_METHOD_ID);put(result,"contract",actualContract);put(result,"cases",rows);put(result,"failures",failures);put(result,"processor_bytes",static_cast<int>(sizeof(P)));put(result,"new_features","n/a: F1 Legacy isolation");write(report,result);if(capture)write(folder.getChildFile("capture.json"),result);std::cout<<matrix.size()<<" cases, "<<failures<<" failures\n";return failures?1:0;
}
}
int main(int argc,char** argv)
{
    juce::ScopedJuceInitialiser_GUI gui;
    try{if(argc!=4||(juce::String(argv[1])!="capture"&&juce::String(argv[1])!="verify"))throw std::runtime_error("capture|verify <reference-dir> <NEW_report.json>");return run(juce::String(argv[1])=="capture",juce::File(argv[2]),juce::File(argv[3]));}
    catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 2;}
}

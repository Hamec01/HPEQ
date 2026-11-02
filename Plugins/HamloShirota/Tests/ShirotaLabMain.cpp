// Immutable capture/verification method for isolating the existing M0 processor.
#include "Standalone/ModulePluginProcessor.h"
#include <juce_cryptography/juce_cryptography.h>
#include <iostream>
#include <memory>
#include <vector>

namespace
{
using hpeq::standalone::ModulePluginProcessor;
constexpr auto kind=hpeq::standalone::ModuleKind::Shirota;
struct Case
{
    juce::String name;double rate=48000;int channels=2,block=127,program=0,automation=0,signal=0;
    std::vector<std::pair<juce::String,double>> values;
};
juce::var object(){return new juce::DynamicObject;}
void set(juce::var& v,const juce::Identifier& key,const juce::var& value){v.getDynamicObject()->setProperty(key,value);}
juce::String hash(const juce::MemoryBlock& data){return juce::SHA256(data).toHexString();}
void save(const juce::File& file,const juce::var& value)
{if(!file.replaceWithText(juce::JSON::toString(value)))throw std::runtime_error("Cannot save report");}
juce::var contract(ModulePluginProcessor& p)
{
    auto result=object();juce::Array<juce::var> parameters,programs;
    for(auto* base:p.getParameters())
    {
        auto* q=dynamic_cast<juce::RangedAudioParameter*>(base);if(!q)throw std::runtime_error("Unknown parameter");
        auto v=object();const auto& r=q->getNormalisableRange();set(v,"id",q->paramID);set(v,"name",q->getName(128));
        set(v,"minimum",r.start);set(v,"maximum",r.end);set(v,"interval",r.interval);set(v,"skew",r.skew);
        set(v,"default_normalised",q->getDefaultValue());set(v,"steps",q->getNumSteps());set(v,"label",q->getLabel());
        juce::Array<juce::var> labels;for(const auto& text:q->getAllValueStrings())labels.add(text);set(v,"choices",labels);parameters.add(v);
    }
    for(int i=0;i<p.getNumPrograms();++i)programs.add(p.getProgramName(i));
    juce::MemoryBlock state;p.getStateInformation(state);const auto xml=juce::AudioProcessor::getXmlFromBinary(state.getData(),static_cast<int>(state.getSize()));
    set(result,"parameters",parameters);set(result,"programs",programs);set(result,"state_root",xml?xml->getTagName():juce::String{});
    return result;
}
std::vector<Case> cases(ModulePluginProcessor& p)
{
    std::vector<Case> out;
    for(double rate:{44100.,48000.,96000.})for(int channels:{1,2})for(int block:{127,512})for(int program=0;program<p.getNumPrograms();++program)
    {Case c;c.rate=rate;c.channels=channels;c.block=block;c.program=program;c.name="program_"+juce::String(program)+"_"+juce::String(rate,0)+"_ch"+juce::String(channels)+"_b"+juce::String(block);out.push_back(c);}
    for(const auto& spec:p.getSpec().parameters)for(bool high:{false,true})
    {Case c;c.name="boundary_"+spec.id+(high?"_max":"_min");c.values.push_back({spec.id,high?spec.maximum:spec.minimum});out.push_back(c);}
    for(int block:{1,64,127,512,4096})for(int automation:{0,1,2,3})
    {Case c;c.name="events_"+juce::String(automation)+"_b"+juce::String(block);c.block=block;c.automation=automation+1;out.push_back(c);}
    for(int block:{1,127,4096})
    {Case c;c.name="m6_b"+juce::String(block);c.block=block;c.values={{"style",0},{"stereoize",0},{"space",0},{"monoBassHz",10},{"rotation",0},{"safe",0},{"monoListen",0},{"outputDb",0},{"width",1.7},{"balance",-.3}};out.push_back(c);}
    for(int signal=1;signal<=5;++signal)for(int style=0;style<3;++style)for(int safe=0;safe<2;++safe)
    {Case c;c.name="signal_"+juce::String(signal)+"_style"+juce::String(style)+"_safe"+juce::String(safe);c.signal=signal;c.values={{"style",style},{"safe",safe},{"width",2}};out.push_back(c);}
    {Case c;c.name="bypass_at_prepare";c.values={{"bypass",1}};out.push_back(c);}
    return out;
}
juce::var parameterValues(ModulePluginProcessor& p)
{
    auto v=object();for(auto* base:p.getParameters()){auto* q=dynamic_cast<juce::RangedAudioParameter*>(base);set(v,q->paramID,q->getValue());}return v;
}
int run(bool capture,const juce::File& reference,const juce::File& report)
{
    if(capture&&reference.exists())throw std::runtime_error("Refusing to overwrite immutable M0");
    if(capture&&!reference.createDirectory())throw std::runtime_error("Cannot create M0 directory");
    report.getParentDirectory().createDirectory();
    auto p=std::make_unique<ModulePluginProcessor>(kind);const auto layout=contract(*p);const auto layoutFile=reference.getChildFile("contract.json");
    const auto serial=juce::JSON::toString(layout);if(capture)save(layoutFile,layout);else if(layoutFile.loadFileAsString()!=serial)throw std::runtime_error("Original contract changed");
    const auto all=cases(*p);juce::Array<juce::var> rows;int failures=0;
    const auto oldReport=capture?juce::var{}:juce::JSON::parse(reference.getChildFile("capture.json").loadFileAsString());
    for(size_t index=0;index<all.size();++index)
    {
        const auto& c=all[index];p=std::make_unique<ModulePluginProcessor>(kind);p->setCurrentProgram(c.program);
        for(const auto& value:c.values)p->setParameterValue(value.first,value.second);
        const auto expected=juce::JSON::toString(parameterValues(*p));juce::MemoryBlock state;p->getStateInformation(state);
        const auto stateFile=reference.getChildFile(c.name+".state"),rawFile=reference.getChildFile(c.name+".f32");
        bool stateExact=true;
        if(capture){if(!stateFile.replaceWithData(state.getData(),state.getSize()))throw std::runtime_error("Cannot save state");}
        else
        {
            juce::MemoryBlock old;if(!stateFile.loadFileAsData(old))throw std::runtime_error("Missing M0 state");
            stateExact=state==old;
            for(bool edited:{false,true})
            {
                auto q=std::make_unique<ModulePluginProcessor>(kind);
                if(edited)for(const auto& spec:q->getSpec().parameters)q->setParameterValue(spec.id,spec.maximum);
                q->setStateInformation(old.getData(),static_cast<int>(old.getSize()));juce::MemoryBlock saved;q->getStateInformation(saved);
                stateExact=stateExact&&saved==old&&juce::JSON::toString(parameterValues(*q))==expected;
            }
            p->setStateInformation(old.getData(),static_cast<int>(old.getSize()));
        }
        const auto setLayout=p->setBusesLayout({juce::AudioChannelSet::canonicalChannelSet(c.channels),juce::AudioChannelSet::canonicalChannelSet(c.channels)});
        if(!setLayout)throw std::runtime_error("Original bus layout unsupported");
        p->prepareToPlay(c.rate,256);const int samples=static_cast<int>(c.rate*.36);
        juce::AudioBuffer<float> audio(c.channels,samples);uint32_t noise=0x484d5368u;
        for(int i=0;i<samples;++i)
        {
            noise=noise*1664525u+1013904223u;const double n=(static_cast<double>(noise)/4294967296.-.5)*.035;
            for(int ch=0;ch<c.channels;++ch)
            {const double t=i/c.rate;const bool same=c.signal==2||c.signal==3;double x=.15*std::sin(6.283185307179586*(ch&&!same?1319:997)*t)+.09*std::sin(6.283185307179586*83*t)+n+(i==31?.3:0);
                if(c.signal==1||(c.signal==4&&ch==1))x=0;if(c.signal==3&&ch==1)x=-x;if(c.signal==5)x=i==31?.3:0;
                audio.setSample(ch,i,static_cast<float>(x));}
        }
        juce::Array<juce::var> latency;juce::MidiBuffer midi;
        for(int event=0;event<6;++event)
        {
            if(c.automation==1){p->setParameterValue("style",event%3);p->setParameterValue("safe",event%2);p->setParameterValue("width",event%2?2:0);p->setParameterValue("rotation",event%2?-45:45);}
            if(c.automation==2){p->setParameterValue("bypass",event%2);p->setParameterValue("mix",event%3*.5);p->setParameterValue("style",2);}
            if(c.automation==3){p->setCurrentProgram(event%p->getNumPrograms());p->setParameterValue("balance",event%2?1:-1);p->setParameterValue("monoListen",event%2);}
            if(c.automation==4){p->setParameterValue("stereoize",event%2);p->setParameterValue("space",event%2);p->setParameterValue("safe",0);p->setParameterValue("monoBassHz",event%2?500:10);p->setParameterValue("outputDb",event%2?12:-24);}
            const int first=samples*event/6,last=samples*(event+1)/6;
            for(int pos=first;pos<last;pos+=c.block)
            {juce::AudioBuffer<float> part(audio.getArrayOfWritePointers(),c.channels,pos,juce::jmin(c.block,last-pos));p->processBlock(part,midi);}
            auto point=object();set(point,"latency",p->getLatencySamples());set(point,"tail",p->getTailLengthSeconds());latency.add(point);
        }
        juce::MemoryBlock bytes;bool finite=true;double peak=0;
        for(int ch=0;ch<c.channels;++ch){bytes.append(audio.getReadPointer(ch),static_cast<size_t>(samples)*sizeof(float));for(int i=0;i<samples;++i){const auto v=audio.getSample(ch,i);finite=finite&&std::isfinite(v);peak=std::max(peak,std::abs(static_cast<double>(v)));}}
        bool bitExact=true,latencyExact=true;
        if(capture){if(!rawFile.replaceWithData(bytes.getData(),bytes.getSize()))throw std::runtime_error("Cannot save render");}
        else
        {
            juce::MemoryBlock old;if(!rawFile.loadFileAsData(old))throw std::runtime_error("Missing M0 render");bitExact=old==bytes;
            if(!oldReport["cases"].isArray()||index>=static_cast<size_t>(oldReport["cases"].size()))throw std::runtime_error("Missing capture metadata");
            const auto previous=oldReport["cases"][static_cast<int>(index)];if(previous["case"].toString()!=c.name)throw std::runtime_error("Capture schedule changed");
            latencyExact=juce::JSON::toString(previous["latency_events"])==juce::JSON::toString(juce::var(latency));
        }
        const bool passed=bitExact&&stateExact&&latencyExact&&finite;if(!passed)++failures;
        auto row=object();set(row,"case",c.name);set(row,"rate",c.rate);set(row,"channels",c.channels);set(row,"block",c.block);set(row,"samples",samples);
        set(row,"render_sha256",hash(bytes));set(row,"state_sha256",hash(state));set(row,"latency_events",latency);set(row,"finite",finite);set(row,"peak",peak);
        set(row,"bit_exact",capture?juce::var("n/a: capture"):juce::var(bitExact));set(row,"state_exact",stateExact);set(row,"latency_exact",latencyExact);rows.add(row);
        std::cout<<(capture?"CAPTURE ":passed?"PASS ":"FAIL ")<<c.name<<std::endl;
    }
    auto result=object();set(result,"method","Shirota S1 v1: immutable signals, schedules, contract, raw renders and states");set(result,"cases",rows);set(result,"failures",failures);set(result,"processor_bytes",static_cast<int>(sizeof(ModulePluginProcessor)));set(result,"new_features","n/a: STEREO/SPACE/MOTION/Binaural V2");
    save(report,result);if(capture)save(reference.getChildFile("capture.json"),result);
    std::cout<<all.size()<<" cases, "<<failures<<" failures\n";return failures==0?0:1;
}
}
int main(int argc,char** argv)
{
    juce::ScopedJuceInitialiser_GUI gui;
    try{if(argc!=4||(juce::String(argv[1])!="capture"&&juce::String(argv[1])!="verify"))throw std::runtime_error("capture|verify <reference-dir> <report.json>");return run(juce::String(argv[1])=="capture",juce::File(argv[2]),juce::File(argv[3]));}
    catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 2;}
}

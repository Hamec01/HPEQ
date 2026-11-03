// Immutable capture/verification method for isolating the existing M0 processor.
#include "Standalone/ModulePluginProcessor.h"
#include <juce_cryptography/juce_cryptography.h>
#include <iostream>
#include <memory>
#include <vector>
#include "ZastavaS1Identity.h"

namespace
{
using hpeq::standalone::ModulePluginProcessor;
constexpr auto kind=hpeq::standalone::ModuleKind::Zastava;
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
    for(auto* base:juce::Array<juce::AudioProcessorParameter*>(p.getParameters().begin(),16))
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
    for(int block:{1,64,127,512,4096})for(int automation=0;automation<6;++automation)
    {Case c;c.name="events_"+juce::String(automation)+"_b"+juce::String(block);c.block=block;c.automation=automation+1;out.push_back(c);}
    for(int block:{1,127,4096})
    {Case c;c.name="m6_b"+juce::String(block);c.block=block;c.values={{"style",0},{"lookahead",0},{"keyHpHz",20},{"keyLpHz",20000},{"keyListen",0},{"flip",0},{"outputDb",0},{"thresholdDb",-40},{"holdMs",0},{"releaseMs",10}};out.push_back(c);}
    for(int signal=1;signal<=5;++signal)for(int style=0;style<3;++style)for(int look=0;look<3;++look)
    {Case c;c.name="signal_"+juce::String(signal)+"_style"+juce::String(style)+"_look"+juce::String(look);c.signal=signal;c.values={{"style",style},{"lookahead",look},{"thresholdDb",-32},{"rangeDb",60},{"attackMs",.05},{"holdMs",4},{"releaseMs",12}};out.push_back(c);}
    for(int listen=0;listen<2;++listen)for(int flip=0;flip<2;++flip)for(int style=0;style<3;++style)for(int look=0;look<3;++look)
    {Case c;c.name="options_"+juce::String(listen)+"_"+juce::String(flip)+"_"+juce::String(style)+"_"+juce::String(look);c.values={{"keyListen",listen},{"flip",flip},{"style",style},{"lookahead",look},{"keyHpHz",80},{"keyLpHz",3500}};out.push_back(c);}
    {Case c;c.name="bypass_at_prepare";c.values={{"bypass",1}};out.push_back(c);}
    return out;
}
juce::var parameterValues(ModulePluginProcessor& p)
{
    auto v=object();for(auto* base:juce::Array<juce::AudioProcessorParameter*>(p.getParameters().begin(),16)){auto* q=dynamic_cast<juce::RangedAudioParameter*>(base);set(v,q->paramID,q->getValue());}return v;
}
struct Rendered {juce::MemoryBlock bytes;juce::Array<juce::var> latency;bool finite;double peak;int samples;};
Rendered renderCase(ModulePluginProcessor& processor,const Case& c)
{
    auto* p=&processor;
        const auto setLayout=p->setBusesLayout({juce::AudioChannelSet::canonicalChannelSet(c.channels),juce::AudioChannelSet::canonicalChannelSet(c.channels)});
        if(!setLayout)throw std::runtime_error("Original bus layout unsupported");
        p->prepareToPlay(c.rate,256);const int samples=static_cast<int>(c.rate*.36);
        juce::AudioBuffer<float> audio(c.channels,samples);uint32_t noise=0x484d5a61u;
        for(int i=0;i<samples;++i)
        {
            noise=noise*1664525u+1013904223u;const double n=(static_cast<double>(noise)/4294967296.-.5)*.035;
            for(int ch=0;ch<c.channels;++ch)
            {const double t=i/c.rate;const bool same=c.signal==2||c.signal==3;const double phase=std::fmod(t,.09);const double burst=phase<.045?std::exp(-phase*65):.00015;double x=burst*(.15*std::sin(6.283185307179586*(ch&&!same?1319:997)*t)+.09*std::sin(6.283185307179586*83*t))+n*.0005+(i==31?.3:0);
                if(c.signal==1||(c.signal==4&&ch==1))x=0;if(c.signal==3&&ch==1)x=-x;if(c.signal==5)x=i==31?.3:0;
                audio.setSample(ch,i,static_cast<float>(x));}
        }
        juce::Array<juce::var> latency;juce::MidiBuffer midi;
        for(int event=0;event<6;++event)
        {
            if(c.automation==1){p->setParameterValue("style",event%3);p->setParameterValue("lookahead",event%3);p->setParameterValue("outputDb",event%2?12:0);}
            if(c.automation==2){p->setParameterValue("bypass",event%2);p->setParameterValue("mix",event%3*.5);p->setParameterValue("style",2);}
            if(c.automation==3){p->setCurrentProgram(event%p->getNumPrograms());p->setParameterValue("holdMs",event%2?500:0);p->setParameterValue("thresholdDb",event%2?-8:-90);p->setParameterValue("keyListen",event%2);}
            if(c.automation==4){for(const char* id:{"style","lookahead","keyListen","flip","outputDb"})p->setParameterValue(id,0);p->setParameterValue("keyHpHz",20);p->setParameterValue("keyLpHz",20000);p->setParameterValue("thresholdDb",event%2?-24:-90);p->setParameterValue("attackMs",event%2?100:.05);p->setParameterValue("releaseMs",event%2?2000:1);p->setParameterValue("hysteresisDb",event%2?24:0);}
            if(c.automation==5){p->setParameterValue("style",0);p->setParameterValue("lookahead",0);p->setParameterValue("keyHpHz",event%2?80:20);p->setParameterValue("keyLpHz",event%2?2500:20000);p->setParameterValue("keyListen",event%2);p->setParameterValue("flip",event%2);p->setParameterValue("outputDb",event%2?-24:0);}
            if(c.automation==6){p->setParameterValue("ratio",event%2?100:1);p->setParameterValue("rangeDb",event%2?90:0);p->setParameterValue("mix",event%2);p->setParameterValue("outputDb",event%2?-12:0);}
            const int first=samples*event/6,last=samples*(event+1)/6;
            for(int pos=first;pos<last;pos+=c.block)
            {juce::AudioBuffer<float> part(audio.getArrayOfWritePointers(),c.channels,pos,juce::jmin(c.block,last-pos));p->processBlock(part,midi);}
            auto point=object();set(point,"latency",p->getLatencySamples());set(point,"tail",p->getTailLengthSeconds());latency.add(point);
        }
        juce::MemoryBlock bytes;bool finite=true;double peak=0;
        for(int ch=0;ch<c.channels;++ch){bytes.append(audio.getReadPointer(ch),static_cast<size_t>(samples)*sizeof(float));for(int i=0;i<samples;++i){const auto v=audio.getSample(ch,i);finite=finite&&std::isfinite(v);peak=std::max(peak,std::abs(static_cast<double>(v)));}}
    return {std::move(bytes),latency,finite,peak,samples};
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
        bool stateExact=true,idempotent=true;juce::Array<juce::var> restorations;
        juce::MemoryBlock loaded=state;
        if(capture){if(!stateFile.replaceWithData(state.getData(),state.getSize()))throw std::runtime_error("Cannot save state");}
        else {loaded.reset();if(!stateFile.loadFileAsData(loaded))throw std::runtime_error("Missing M0 state");stateExact=state==loaded;}
        for(int edited=0;edited<2;++edited)
        {
            auto q=std::make_unique<ModulePluginProcessor>(kind);
            if(edited)for(const auto& spec:q->getSpec().parameters)q->setParameterValue(spec.id,spec.maximum);
            q->setStateInformation(loaded.getData(),static_cast<int>(loaded.getSize()));juce::MemoryBlock saved;q->getStateInformation(saved);
            const auto values=parameterValues(*q);idempotent=idempotent&&saved==loaded&&juce::JSON::toString(values)==expected;
            const auto audio=renderCase(*q,c);const auto prefix=c.name+"_restore"+juce::String(edited);
            const auto savedState=reference.getChildFile(prefix+".state"),savedAudio=reference.getChildFile(prefix+".f32");
            auto observed=object();set(observed,"edited",edited);set(observed,"state_sha256",hash(saved));set(observed,"render_sha256",hash(audio.bytes));set(observed,"parameters",values);set(observed,"program",q->getCurrentProgram());set(observed,"latency_events",audio.latency);restorations.add(observed);
            if(capture)
            {if(!savedState.replaceWithData(saved.getData(),saved.getSize())||!savedAudio.replaceWithData(audio.bytes.getData(),audio.bytes.getSize()))throw std::runtime_error("Cannot save restored-state observation");}
            else
            {
                juce::MemoryBlock originalState,originalAudio;if(!savedState.loadFileAsData(originalState)||!savedAudio.loadFileAsData(originalAudio))throw std::runtime_error("Missing restored-state baseline");
                const auto old=oldReport["cases"][static_cast<int>(index)]["restorations"][edited];
                stateExact=stateExact&&saved==originalState&&audio.bytes==originalAudio&&audio.finite&&juce::JSON::toString(observed)==juce::JSON::toString(old);
            }
        }
        if(!capture)p->setStateInformation(loaded.getData(),static_cast<int>(loaded.getSize()));
        auto rendered=renderCase(*p,c);const auto& bytes=rendered.bytes;const auto& latency=rendered.latency;const bool finite=rendered.finite;const double peak=rendered.peak;const int samples=rendered.samples;
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
        set(row,"restorations",restorations);set(row,"historical_roundtrip_idempotent",idempotent);set(row,"bit_exact",capture?juce::var("n/a: capture"):juce::var(bitExact));set(row,"state_exact",stateExact);set(row,"latency_exact",latencyExact);rows.add(row);
        std::cout<<(capture?"CAPTURE ":passed?"PASS ":"FAIL ")<<c.name<<std::endl;
    }
    auto result=object();set(result,"method","Zastava S1 v3: actual M0 restored states/audio; immutable signals, schedules, contract, raw renders and states");set(result,"source_id",ZASTAVA_S1_SOURCE_ID);set(result,"method_id",ZASTAVA_S1_METHOD_ID);set(result,"cases",rows);set(result,"failures",failures);set(result,"processor_bytes",static_cast<int>(sizeof(ModulePluginProcessor)));set(result,"new_features","n/a: V2 cleanup/drum/duck/rhythm, soft/hard profiles");
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

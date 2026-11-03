#include "Standalone/ModulePluginProcessor.h"
#include "DSP/V2Gate.h"
#include "AllocationAudit.h"
#include "ZastavaS3Identity.h"
#include <juce_cryptography/juce_cryptography.h>
#include <iostream>
#include <memory>

namespace
{
using P=hpeq::standalone::ModulePluginProcessor;
using namespace hamlo::zastava;
constexpr auto kind=hpeq::standalone::ModuleKind::Zastava;
juce::Array<juce::var> checks;
void check(const char* goal,const juce::String& name,bool pass)
{
    auto* row=new juce::DynamicObject;row->setProperty("goal",goal);row->setProperty("name",name);row->setProperty("passed",pass);checks.add(row);
    if(!pass)std::cout<<"FAIL "<<goal<<" "<<name<<std::endl;
}
void unavailable(const char* goal)
{auto* row=new juce::DynamicObject;row->setProperty("goal",goal);row->setProperty("passed","n/a");checks.add(row);}
template<typename T>auto missing(const T& p,int)->decltype(p.modernReading().missingKey){return p.modernReading().missingKey;}
template<typename T>bool missing(const T&,long){return false;}
bool same(const juce::AudioBuffer<float>& a,const juce::AudioBuffer<float>& b)
{if(a.getNumChannels()!=b.getNumChannels()||a.getNumSamples()!=b.getNumSamples())return false;for(int c=0;c<a.getNumChannels();++c)if(std::memcmp(a.getReadPointer(c),b.getReadPointer(c),static_cast<size_t>(a.getNumSamples())*4)!=0)return false;return true;}
void set(P& p,const char* id,double value){p.setParameterValue(id,value);}
void configure(P& p,int mode,int profile)
{
    for(const auto& pair:std::initializer_list<std::pair<const char*,double>>{
        {"v2_engine",mode},{"v2_enabled",1},{"v2_profile",profile},{"v2_threshold",-36},{"v2_range",30},
        {"v2_attack",2},{"v2_hold",12},{"v2_release",40},{"v2_detectorMs",0},{"v2_lookahead",5},
        {"v2_hp",0},{"v2_lp",0},{"v2_ratio",3},{"v2_knee",6},{"v2_expand",1},{"v2_body",10},{"v2_tail",80}})set(p,pair.first,pair.second);
    for(int n=0;n<32;++n)p.setParameterValue("v2_step"+juce::String(n+1),(n%3)*.5);
}
Settings settings(P& p)
{
    const auto v=[&](const char* id){return p.parameterValue(id);};Settings s;
    s.mode=static_cast<Mode>(static_cast<int>(v("v2_engine"))-1);s.profile=static_cast<Profile>(static_cast<int>(v("v2_profile")));
    s.enabled=v("v2_enabled")>.5;s.bypass=v("bypass")>.5;s.external=v("v2_external")>.5;s.expand=v("v2_expand")>.5;s.rms=v("v2_rms")>.5;s.sync=v("v2_sync")>.5;
    s.thresholdDb=v("v2_threshold");s.rangeDb=v("v2_range");s.ratio=v("v2_ratio");s.kneeDb=v("v2_knee");s.hysteresisDb=v("v2_hysteresis");
    s.attackMs=v("v2_attack");s.releaseMs=v("v2_release");if(s.mode==Mode::Duck)std::swap(s.attackMs,s.releaseMs);
    s.holdMs=v("v2_hold");s.detectorMs=v("v2_detectorMs");s.lookaheadMs=v("v2_lookahead");s.keyHpHz=v("v2_hp");s.keyLpHz=v("v2_lp");s.stereoLink=v("v2_link");s.mix=v("v2_mix");s.outputDb=v("v2_output");
    s.bodyMs=v("v2_body");s.tailMs=v("v2_tail");s.onsetDb=v("v2_onset");s.retriggerMs=v("v2_retrigger");s.steps=static_cast<int>(v("v2_steps"));s.stepBeats=v("v2_beats");s.stepMs=v("v2_ms");
    for(int n=0;n<32;++n)s.levels[static_cast<size_t>(n)]=p.parameterValue("v2_step"+juce::String(n+1));return s;
}
struct Head:juce::AudioPlayHead
{
    double bpm=120,ppq=0;bool playing=true;
    juce::Optional<PositionInfo> getPosition() const override{PositionInfo p;p.setBpm(bpm);p.setPpqPosition(ppq);p.setIsPlaying(playing);return p;}
};
float input(int n,int c,double rate)
{const double t=n/rate;return static_cast<float>(.13*std::sin(t*(c?5123:6283))+.07*std::sin(t*217)+((n%4800)==0?.2:0));}
float key(int n,int c,double rate)
{return std::fmod(n/rate,.14)<.035?static_cast<float>(.18*std::cos(n*(c?.11:.09))):.000001f;}
struct Result{juce::AudioBuffer<float> audio,reference;size_t allocations=0;bool keyKept=true,finite=true;};
Result render(P& p,int frames,int block=256,double rate=48000,int channels=2,int keys=0,bool reference=false,bool events=false,bool prepare=true)
{
    auto layout=p.getBusesLayout();layout.inputBuses.set(0,juce::AudioChannelSet::canonicalChannelSet(channels));layout.outputBuses.set(0,juce::AudioChannelSet::canonicalChannelSet(channels));
    if(layout.inputBuses.size()>1)layout.inputBuses.set(1,keys?juce::AudioChannelSet::canonicalChannelSet(keys):juce::AudioChannelSet::disabled());
    if(!p.setBusesLayout(layout))throw std::runtime_error("Unsupported laboratory layout");if(prepare)p.prepareToPlay(rate,256);
    Result r;r.audio.setSize(channels,frames);r.reference.setSize(channels,frames);
    juce::AudioBuffer<float> data(channels+keys,4096),ref(channels,4096),savedKey(keys,4096);juce::MidiBuffer midi;
    Gate gate;if(reference){gate.prepare(rate,256);gate.set(settings(p));}Head host;p.setPlayHead(&host);
    for(int at=0;at<frames;)
    {
        const int interval=4800,event=at/interval;
        if(events&&at%interval==0)
        {
            configure(p,event%5,event%2);set(p,"v2_external",event%4==3);set(p,"v2_hp",event%2?200:0);set(p,"v2_lp",event%3?3500:0);
            set(p,"bypass",event%7==6);set(p,"style",event%3);set(p,"lookahead",event%3);set(p,"thresholdDb",event%2?-32:-60);
        }
        const int count=std::min({block,frames-at,interval-at%interval});
        for(int c=0;c<channels;++c)for(int i=0;i<count;++i){const auto value=input(at+i,c,rate);data.setSample(c,i,value);ref.setSample(c,i,value);}
        for(int c=0;c<keys;++c)for(int i=0;i<count;++i){const auto value=key(at+i,c,rate);data.setSample(channels+c,i,value);savedKey.setSample(c,i,value);}
        const double t=at/rate;host.bpm=events&&t>=.5?174:120;host.ppq=t<.5?t*2:1+(t-.5)*host.bpm/60;if(events&&t>=.7)host.ppq+=3;host.playing=!(events&&t>=.8&&t<.9);
        juce::AudioBuffer<float> view(data.getArrayOfWritePointers(),channels+keys,0,count);
        if(reference){Clock clock;clock.bpm=host.bpm;clock.ppq=host.ppq;clock.hasPpq=true;clock.playing=host.playing;const float* k[]{keys?savedKey.getReadPointer(0):nullptr,keys?savedKey.getReadPointer(keys-1):nullptr};gate.process(ref.getArrayOfWritePointers(),channels,count,keys?k:nullptr,keys,clock);}
        hamlo::test::allocation::begin();p.processBlock(view,midi);r.allocations+=hamlo::test::allocation::finish();
        for(int c=0;c<channels;++c){r.audio.copyFrom(c,at,data,c,0,count);r.reference.copyFrom(c,at,ref,c,0,count);for(int i=0;i<count;++i)r.finite=r.finite&&std::isfinite(data.getSample(c,i));}
        for(int c=0;c<keys;++c)r.keyKept=r.keyKept&&std::memcmp(data.getReadPointer(channels+c),savedKey.getReadPointer(c),static_cast<size_t>(count)*4)==0;
        at+=count;
    }
    p.setPlayHead(nullptr);return r;
}
void dirty(P& p)
{for(int i=16;i<p.getParameters().size();++i){auto* q=dynamic_cast<juce::RangedAudioParameter*>(p.getParameters()[i]);p.setParameterValue(q->paramID,q->getNormalisableRange().end);}}
juce::MemoryBlock state(P& p){juce::MemoryBlock b;p.getStateInformation(b);return b;}
void load(P& p,const juce::MemoryBlock& b){p.setStateInformation(b.getData(),static_cast<int>(b.getSize()));}
}
int main(int argc,char** argv)
{
    juce::ScopedJuceInitialiser_GUI gui;
    try
    {
        if(argc!=2)throw std::runtime_error("S3HostLab NEW_report.json");juce::File file=juce::File::getCurrentWorkingDirectory().getChildFile(argv[1]);if(file.exists())throw std::runtime_error("Refusing to overwrite report");
        auto p=std::make_unique<P>(kind);const bool modern=p->parameter("v2_engine")!=nullptr;
        check("H01","original_program_count",p->getNumPrograms()==7);check("H01","bypass_index15",p->getParameters()[15]==p->getBypassParameter());
        if(!modern){for(const char* goal:{"H02","H03","H04","H05","H06","H07","H08"})unavailable(goal);}
        else
        {
            check("H02","neutral_boot_selection",p->parameterValue("v2_engine")==1&&p->parameterValue("v2_enabled")==0);
            for(double rate:{8000.,44100.,48000.,96000.,192000.})for(int channels:{1,2})for(int type=0;type<4;++type)
            {
                p=std::make_unique<P>(kind);if(type){configure(*p,1,0);if(type==1)set(*p,"bypass",1);if(type==2)set(*p,"v2_mix",0);if(type==3){set(*p,"v2_external",1);set(*p,"v2_output",12);}}
                const int length=static_cast<int>(rate*.08);auto r=render(*p,length,4096,rate,channels);juce::AudioBuffer<float> expected(channels,length);expected.clear();const int delay=static_cast<int>(std::lround(.005*rate));
                for(int c=0;c<channels;++c)for(int i=delay;i<length;++i)expected.setSample(c,i,input(i-delay,c,rate));
                const auto name=juce::String(rate,0)+"_"+juce::String(channels)+"_"+juce::String(type);check("H02",name,same(r.audio,expected)&&p->getLatencySamples()==delay);check("H08",name,r.allocations==0&&r.finite);if(type==3)check("H06",name+"_missing",missing(*p,0));
            }
            for(int mode=1;mode<=4;++mode)for(int profile=0;profile<2;++profile)for(int channels:{1,2})for(int keys:{0,1,2})
            {
                p=std::make_unique<P>(kind);configure(*p,mode,profile);set(*p,"v2_external",keys>0);auto r=render(*p,16384,4096,48000,channels,keys,true);
                const auto name=juce::String(mode)+"_"+juce::String(profile)+"_"+juce::String(channels)+"_"+juce::String(keys);check("H05",name,same(r.audio,r.reference));check("H06",name+"_key",r.keyKept);check("H08",name,r.allocations==0&&r.finite);
            }
            for(int mode=1;mode<=4;++mode)for(int profile=0;profile<2;++profile)
            {
                p=std::make_unique<P>(kind);p->setCurrentProgram(3);configure(*p,mode,profile);set(*p,"v2_mix",.713);set(*p,"v2_output",-2.37);set(*p,"v2_detectorMs",3.17);
                const auto saved=state(*p);const auto expected=render(*p,16384).audio;
                for(int edited=0;edited<2;++edited)
                {
                    auto q=std::make_unique<P>(kind);if(edited){dirty(*q);render(*q,4096);}load(*q,saved);bool equal=true;
                    for(int i=16;i<p->getParameters().size();++i)equal=equal&&p->getParameters()[i]->getValue()==q->getParameters()[i]->getValue();
                    const auto got=render(*q,16384,256,48000,2,0,false,false,!edited);
                    const auto restored=state(*q);const bool audioEqual=same(expected,got.audio);
                    // First candidate combined these observations and failed17state checks.
                    // Diagnostics split the cause; acceptance criteria are unchanged.
                    if(!(equal&&restored==saved&&q->getCurrentProgram()==3&&audioEqual))
                    {
                        std::cout<<"state diagnostics: parameters="<<equal<<" bytes="<<(restored==saved)<<" program="<<q->getCurrentProgram()<<" audio="<<audioEqual<<std::endl;
                        for(int i=16;i<p->getParameters().size();++i)if(p->getParameters()[i]->getValue()!=q->getParameters()[i]->getValue())std::cout<<p->getParameters()[i]->getName(100)<<" "<<p->getParameters()[i]->getValue()<<" -> "<<q->getParameters()[i]->getValue()<<std::endl;
                        const auto folder=juce::File::getCurrentWorkingDirectory().getChildFile("out/s3/diagnostics");folder.createDirectory();
                        const auto prefix=juce::String(mode)+"_"+juce::String(profile)+"_"+juce::String(edited);
                        juce::AudioProcessor::getXmlFromBinary(saved.getData(),static_cast<int>(saved.getSize()))->writeTo(folder.getChildFile(prefix+"_before.xml"));
                        juce::AudioProcessor::getXmlFromBinary(restored.getData(),static_cast<int>(restored.getSize()))->writeTo(folder.getChildFile(prefix+"_after.xml"));
                    }
                    check("H03",juce::String(mode)+"_"+juce::String(profile)+"_"+juce::String(edited),equal&&restored==saved&&q->getCurrentProgram()==3&&audioEqual);check("H08","state_callback",got.allocations==0);
                }
            }
            p=std::make_unique<P>(kind);p->setCurrentProgram(2);auto legacy=state(*p);
            for(int edited=0;edited<2;++edited)
            {
                auto q=std::make_unique<P>(kind);if(edited){dirty(*q);render(*q,4096);}load(*q,legacy);bool reset=q->parameterValue("v2_engine")==0;
                for(int i=17;i<q->getParameters().size();++i)reset=reset&&q->getParameters()[i]->getValue()==q->getParameters()[i]->getDefaultValue();check("H04","old_reset_"+juce::String(edited),reset&&state(*q)==legacy);
            }
            p=std::make_unique<P>(kind);p->setCurrentProgram(0);set(*p,"v2_tail",345.67);const auto fullLegacy=state(*p);auto q=std::make_unique<P>(kind);load(*q,fullLegacy);check("H03","retained_legacy_controls",q->parameterValue("v2_engine")==0&&q->parameterValue("v2_tail")==p->parameterValue("v2_tail")&&state(*q)==fullLegacy);
            p=std::make_unique<P>(kind);configure(*p,1,0);auto xml=juce::AudioProcessor::getXmlFromBinary(state(*p).getData(),static_cast<int>(state(*p).getSize()));
            for(auto* child=xml->getFirstChildElement();child!=nullptr;){auto* next=child->getNextElement();if(child->getStringAttribute("id")=="v2_tail")xml->removeChildElement(child,true);child=next;}
            juce::MemoryBlock partial;juce::AudioProcessor::copyXmlToBinary(*xml,partial);dirty(*p);load(*p,partial);check("H04","partial_defaults",p->parameterValue("v2_tail")==100);
            auto valid=state(*p);xml->setAttribute("v2_schema",99);juce::MemoryBlock future;juce::AudioProcessor::copyXmlToBinary(*xml,future);load(*p,future);check("H04","future_ignored",state(*p)==valid);
            const char bad[]{'n','o'};p->setStateInformation(bad,2);check("H04","malformed_ignored",state(*p)==valid);
            p=std::make_unique<P>(kind);configure(*p,1,0);const auto expected=render(*p,48000,1,48000,2,1,false,true).audio;
            for(int block:{64,127,512,4096}){q=std::make_unique<P>(kind);configure(*q,1,0);auto got=render(*q,48000,block,48000,2,1,false,true);check("H07","events_"+juce::String(block),same(expected,got.audio));check("H08","events_alloc_"+juce::String(block),got.allocations==0&&got.finite&&got.keyKept);}
        }
        int passes=0,failures=0,na=0;for(const auto& c:checks){if(c["passed"].isBool()){if(static_cast<bool>(c["passed"]))++passes;else ++failures;}else ++na;}
        auto* result=new juce::DynamicObject;result->setProperty("source_id",ZASTAVA_S3_SOURCE_ID);result->setProperty("method_id",ZASTAVA_S3_METHOD_ID);result->setProperty("modern",modern);result->setProperty("checks",checks);result->setProperty("passed",passes);result->setProperty("failed",failures);result->setProperty("unavailable",na);
        file.getParentDirectory().createDirectory();file.replaceWithText(juce::JSON::toString(juce::var(result)));std::cout<<"S3Host: "<<passes<<" passed, "<<failures<<" failed, "<<na<<" n/a\n";return failures?1:0;
    }
    catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}
}

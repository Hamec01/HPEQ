#include "DSP/FaderKernel.h"
#include "FaderF2Identity.h"
#include "AllocationAudit.h"
#include <juce_core/juce_core.h>
#include <juce_cryptography/juce_cryptography.h>
#include <fstream>
#include <iostream>
#include <cmath>
#include <chrono>
#include <limits>
#if FADER_F2_REFERENCE
#include "DSP/LegacyFaderCore.h"
#endif

namespace
{
using namespace hamlo::fader;
constexpr double pi=3.14159265358979323846;
juce::var obj(){return juce::var(new juce::DynamicObject);}
void put(juce::var& o,const char* n,const juce::var& v){o.getDynamicObject()->setProperty(n,v);}
juce::var meter(const MeterReading& m)
{auto o=obj();put(o,"rmsM",m.rmsM);put(o,"rmsS",m.rmsS);put(o,"rmsI",m.rmsI);put(o,"lufsM",m.lufsM);put(o,"lufsS",m.lufsS);put(o,"lufsI",m.lufsI);put(o,"samplePeak",m.samplePeak);put(o,"truePeak",m.truePeak);put(o,"readyM",m.readyM);put(o,"readyS",m.readyS);put(o,"readyI",m.readyI);put(o,"full",m.full);put(o,"samples",static_cast<juce::int64>(m.samples));put(o,"blocks",static_cast<juce::int64>(m.blocks));return o;}
juce::var snapshot(const Reading& r)
{auto o=obj();put(o,"input",meter(r.input));put(o,"output",meter(r.output));put(o,"learn",meter(r.learn));put(o,"learning",r.learning);put(o,"active",r.active);put(o,"correctionDb",r.correctionDb);put(o,"phase",r.phase);put(o,"motionRunning",r.motionRunning);put(o,"badInput",static_cast<juce::int64>(r.badInput));return o;}
juce::var proposal(const MatchProposal& p)
{auto o=obj();put(o,"valid",p.valid);put(o,"limited",p.limited);put(o,"gainDb",p.gainDb);put(o,"measuredDb",p.measuredDb);put(o,"predictedPeak",p.predictedPeak);return o;}
#if FADER_F2_REFERENCE
struct Engine
{
    hpeq::dsp::LegacyFaderCore core;
    void prepare(double rate,int){core.prepare(rate,4096);hpeq::dsp::FxSlotParams s;s.active=true;core.set(s);}
    void set(const Settings&)noexcept{}void reset()noexcept{}void startLearn()noexcept{}
    MatchProposal stopLearn()noexcept{return{};}Reading reading()const noexcept{return{};}
    void process(float* const*a,int c,int n,const Clock&)noexcept{core.process(a,c,n);}
};
#else
using Engine=Kernel;
#endif
struct Case{std::string name;double rate=48000,seconds=4;int channels=2,block=256,signal=0,event=0;bool control=false;Settings s;};
std::vector<Case> cases()
{
    std::vector<Case> a;auto add=[&](std::string n,Settings s,int signal=0,int event=0,double seconds=4){Case c;c.name=n;c.s=s;c.signal=signal;c.event=event;c.seconds=seconds;a.push_back(c);};
    for(double rate:{8000.,44100.,48000.,96000.,192000.})for(int c:{1,2}){Case x;x.name="neutral_"+std::to_string(static_cast<int>(rate))+"_"+std::to_string(c);x.rate=rate;x.channels=c;x.seconds=.1;x.control=true;a.push_back(x);}
    Settings s;
    for(double db:{-60.,-12.,6.,24.}){s.gainDb=db;add("gain_"+std::to_string(static_cast<int>(db)),s);}s={};
    for(int mode=0;mode<4;++mode){s.routing=static_cast<Routing>(mode);s.polarity=mode;add("routing_"+std::to_string(mode),s,1);}s={};s.swap=true;add("swap",s,1);s={};
    s.leftDelayMs=2;s.rightDelayMs=.125;add("delay",s,2);s={};
    s.leftDelayMs=.03125;s.rightDelayMs=1000*2.5/48000;add("fractional",s,2);s={};s.leftDb=-6;s.rightDb=6;add("channel_gain",s,1);s={};
    s.gainDb=24;s.enabled=false;add("disabled",s);s.enabled=true;s.bypass=true;add("bypass",s);s.bypass=false;s.mix=0;add("mix_zero",s);s={};add("gain_ramp",s,10,6,1);
    for(int sig:{0,3,4,5,6}){s={};add("meter_"+std::to_string(sig),s,sig,0,6);}
    for(double rate:{8000.,44100.,96000.,192000.}){Case c;c.name="meter_rate_"+std::to_string(static_cast<int>(rate));c.rate=rate;c.seconds=4;a.push_back(c);}
    {Case c;c.name="meter_mono";c.channels=1;c.seconds=4;a.push_back(c);}
    for(int sig:{0,3,4})for(int measure:{0,1}){s={};s.mode=Mode::Match;s.measure=static_cast<Measure>(measure);s.peakGuard=false;add("match_"+std::to_string(sig)+"_"+std::to_string(measure),s,sig,1,8);}
    s={};s.mode=Mode::Match;s.targetDb=0;s.matchBoost=24;s.peakGuard=true;add("match_guard",s,0,1,8);
    add("learn_silence",s,7,1,8);add("learn_short",s,0,2,1);
    for(int profile:{0,1}){s={};s.mode=Mode::Ride;s.profile=static_cast<Profile>(profile);s.targetDb=-26;add("ride_"+std::to_string(profile),s,8,0,16);add("pause_"+std::to_string(profile),s,9,0,5);}
    s={};s.mode=Mode::Motion;s.depthDb=24;s.edgeMs=.1;s.sync=false;s.freeMs=500;for(int i=0;i<32;++i)s.levels[static_cast<size_t>(i)]=i%4==0?1:0;add("motion_custom",s,10,0,3);
    s.shape=Shape::Sine;add("motion_sine",s,10,0,3);s.shape=Shape::Triangle;add("motion_triangle",s,10,0,3);
    add("motion_free_stop",s,10,3,4);
    s.sync=true;s.beats=1;s.shape=Shape::Custom;add("motion_transport",s,10,3,4);
    for(int block:{1,64,127,512,4096}){s={};Case c;c.name="events_"+std::to_string(block);c.block=block;c.seconds=4;c.signal=1;c.event=4;a.push_back(c);}
    s={};s.inputDb=std::numeric_limits<double>::quiet_NaN();s.mix=std::numeric_limits<double>::infinity();s.points=1000;s.leftDelayMs=-10;s.mode=static_cast<Mode>(100);add("invalid",s,12);
    s={};s.mode=Mode::Motion;s.depthDb=90;s.rightPhase=.5;s.freeMs=10;s.sync=false;s.shape=Shape::Triangle;Case stress;stress.name="stress";stress.s=s;stress.rate=192000;stress.seconds=300;stress.signal=11;stress.event=5;a.push_back(stress);
    return a;
}
float signal(int kind,std::int64_t n,int c,double fs)
{
    const double t=n/fs;
    if(kind==7)return 0;
    if(kind==10)return .1f;
    if(kind==2)return n==100? .2f:0;
    if(kind==1)return static_cast<float>(.1*std::sin(2*pi*(c?1733:701)*t)+.03*std::cos(2*pi*83*t));
    if(kind==3)return static_cast<float>((t<2?.1:t<4?.01:.001)*std::sin(2*pi*1000*t));
    if(kind==4)return static_cast<float>((t<2||t>4?.1:0)*std::sin(2*pi*1000*t));
    if(kind==5)return static_cast<float>(.1*std::sin(2*pi*12000*t+pi/4));
    if(kind==6)return static_cast<float>(.1*std::sin(2*pi*50*t));
    if(kind==8)return static_cast<float>((static_cast<int>(t/2)%2?.04:.1)*std::sin(2*pi*1000*t));
    if(kind==9)return static_cast<float>((t<2?.04:1e-4)*std::sin(2*pi*1000*t));
    if(kind==11)return static_cast<float>(.2*std::sin(2*pi*(c?19997:131)*t));
    if(kind==12&&n%127==0)return n%254==0?std::numeric_limits<float>::infinity():std::numeric_limits<float>::quiet_NaN();
    return static_cast<float>(.1*std::sin(2*pi*1000*t));
}
void event(Engine& k,Settings& s,int n)
{
    s.mode=static_cast<Mode>(n%4);s.gainDb=n%3==0?-12:0;s.inputDb=n%3==2?6:0;s.leftDelayMs=n%2?2:0;s.rightDelayMs=n%3==1?.125:0;s.polarity=n%4;s.routing=static_cast<Routing>(n%4);s.swap=n%2!=0;s.bypass=n%7==3;s.enabled=n%7!=5;s.mix=n%3*.5;s.profile=static_cast<Profile>(n%2);s.depthDb=36;s.sync=true;s.beats=n%2?.5:1;s.phase=n%3*.2;s.rightPhase=.25;s.points=16;
    for(int i=0;i<32;++i)s.levels[static_cast<size_t>(i)]=(i+n)%3==0?1:0;
    k.set(s);if(n%5==1)k.startLearn();if(n%5==3){const auto p=k.stopLearn();if(p.valid){s.matchedDb=p.gainDb;k.set(s);}}if(n%11==10)k.reset();
}
int run(const juce::File& folder)
{
    if(folder.exists())throw std::runtime_error("Refusing to overwrite run");folder.createDirectory();juce::Array<juce::var> rows;
    for(const auto& c:cases())
    {
        auto row=obj();put(row,"name",juce::String(c.name));put(row,"rate",c.rate);put(row,"channels",c.channels);put(row,"block",c.block);put(row,"signal",c.signal);put(row,"event",c.event);
        if(FADER_F2_REFERENCE&&!c.control){put(row,"available",false);rows.add(row);continue;}put(row,"available",true);
        Engine k;Settings s=c.s;k.set(s);k.prepare(c.rate,c.channels);const int frames=static_cast<int>(c.seconds*c.rate);std::array<std::array<float,4096>,2> audio{};std::array<float,8192> interleaved{};std::ofstream file(folder.getChildFile(c.name+".f32").getFullPathName().toStdString(),std::ios::binary),input(folder.getChildFile(c.name+"_input.f32").getFullPathName().toStdString(),std::ios::binary);juce::Array<juce::var> readings,proposals;size_t allocations=0;bool finite=true;double seconds=0;
        if(c.event==1||c.event==2)k.startLearn();
        const int boundary=static_cast<int>(c.rate*.25);for(int at=0;at<frames;)
        {
            if(c.event==1&&at==static_cast<int>(4*c.rate)){auto p=k.stopLearn();proposals.add(proposal(p));if(p.valid){s.matchedDb=p.gainDb;k.set(s);}}
            if(c.event==2&&at==boundary)proposals.add(proposal(k.stopLearn()));
            if(c.event==6&&at==boundary){s.gainDb=6;k.set(s);}
            if(c.event==4&&at>0&&at%boundary==0){hamlo::test::allocation::begin();event(k,s,at/boundary);allocations+=hamlo::test::allocation::finish();}
            if(c.event==5&&at>0&&at%static_cast<int>(c.rate)==0){s.mode=static_cast<Mode>(static_cast<int>(at/c.rate)%4);s.gainDb=static_cast<int>(at/c.rate)%2?24:-60;s.maxBoost=24;s.maxCut=60;k.set(s);}
            int count=std::min({c.block,frames-at,boundary-at%boundary});
            for(int ch=0;ch<c.channels;++ch)for(int i=0;i<count;++i)audio[static_cast<size_t>(ch)][static_cast<size_t>(i)]=signal(c.signal,c.event==1?(at+i)%static_cast<int>(4*c.rate):at+i,ch,c.rate);
            for(int i=0;i<count;++i)for(int ch=0;ch<c.channels;++ch)interleaved[static_cast<size_t>(i*c.channels+ch)]=audio[static_cast<size_t>(ch)][static_cast<size_t>(i)];input.write(reinterpret_cast<const char*>(interleaved.data()),count*c.channels*4);
            float* pointers[]{audio[0].data(),audio[1].data()};Clock clock;clock.bpm=c.event>=3&&at>=2*c.rate?174:120;clock.hasPpq=true;clock.hasSamples=true;clock.samplePosition=at;clock.ppq=at<2*c.rate?at/c.rate*2:4+(at/c.rate-2)*174/60;
            if((c.event==3||c.event==4)&&at>=2.5*c.rate)clock.ppq+=3.25;clock.playing=!(c.event==3&&at>=3*c.rate&&at<3.5*c.rate);
            hamlo::test::allocation::begin();const auto start=std::chrono::steady_clock::now();k.process(pointers,c.channels,count,clock);seconds+=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();allocations+=hamlo::test::allocation::finish();
            for(int i=0;i<count;++i)for(int ch=0;ch<c.channels;++ch){const float v=pointers[ch][i];finite=finite&&std::isfinite(v);interleaved[static_cast<size_t>(i*c.channels+ch)]=v;}
            file.write(reinterpret_cast<const char*>(interleaved.data()),count*c.channels*4);at+=count;
            if(at%boundary==0){auto r=snapshot(k.reading());put(r,"sample",at);readings.add(r);}
        }
        put(row,"readings",readings);put(row,"proposal",proposals);put(row,"finite",finite);put(row,"allocations",static_cast<int>(allocations));put(row,"frames",frames);put(row,"mean_fraction",seconds/c.seconds);rows.add(row);std::cout<<c.name<<" finite="<<finite<<" allocations="<<allocations<<std::endl;
    }
    auto m=obj();put(m,"implemented",!FADER_F2_REFERENCE);put(m,"source_id",FADER_F2_SOURCE_ID);put(m,"method_id",FADER_F2_METHOD_ID);put(m,"cases",rows);folder.getChildFile("manifest.json").replaceWithText(juce::JSON::toString(m));return 0;
}
}
int main(int argc,char** argv)
{try{if(argc!=2)throw std::runtime_error("NEW_output_folder");return run(juce::File(argv[1]));}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 2;}}

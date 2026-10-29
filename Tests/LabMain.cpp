// Acoustic acceptance harness: this source is shared by the M0-before and V2-after runs.
// Methods/thresholds are fixed in docs/HAMLOVERB_V2.md. Missing V2 capabilities are n/a, never passes.
#include <chrono>
#include <iomanip>
#include <iostream>
#include <sstream>
#include "VerbLabHelpers.h"

using namespace hamlo::verblab;
namespace
{
constexpr double fs=48000;
const char* names[]{"Legacy","Clean","Warm","Velvet","Crisp","Deep","Bloom"};
juce::String report;
int passed=0,failed=0;
void row(const juce::String& id,const juce::String& name,const juce::String& value,int pass=-1)
{
    report+="| "+id+" | "+name+" | "+value+" | "+(pass<0?"INFO":pass?"PASS":"FAIL")+" |\n";
    if(pass>=0) pass?++passed:++failed;
    std::cout<<id<<" "<<name<<": "<<value<<" "<<(pass<0?"INFO":pass?"PASS":"FAIL")<<std::endl;
}
juce::String number(double v,int places=4){return juce::String(v,places);}
template<class T> auto tankEnergy(const T& p,int) -> decltype(p.getTankEnergy()) {return p.getTankEnergy();}
template<class T> double tankEnergy(const T&,long) {return std::numeric_limits<double>::quiet_NaN();}
void decayTests(bool modern)
{
    for(double desired : { .1,.25,1.,2.,10.,30. })
    {
        const auto ir=impulse(neutral(modern?1:0,desired),std::max(2.,desired*2.5),fs);
        const auto t30=decay(ir,fs),t20=decay(ir,fs,-25);
        const bool pass=t30.seconds>0 && std::abs(t30.seconds-desired)<=std::max(.05,.1*desired);
        row("V3",juce::String(modern?"Clean":"Legacy Hall")+" decay "+number(desired,2),
            "T30="+number(t30.seconds)+" s, T20="+number(t20.seconds)+" s, R2="+number(t30.r2),modern?(pass?1:0):-1);
    }
    if(modern) for(int engine=2;engine<=6;++engine)
    {
        const auto ir=impulse(neutral(engine),6,fs);const auto fit=decay(ir,fs);
        row("V3",juce::String(names[engine])+" artistic decay","T30="+number(fit.seconds)+" s, R2="+number(fit.r2));
    }
    if(modern)
    {
        for(int type=0;type<3;++type)
        {
            auto s=neutral(1);set(s,"decayEq1On",1);set(s,"decayEq1Type",type);set(s,"decayEq1Hz",1000);set(s,"decayEq1Value",1);set(s,"decayEq1Q",.707);
            const auto ir=impulse(s,12,fs);
            row("V3","Decay EQ "+juce::String(type),"Broadband T30="+number(decay(ir,fs).seconds)+" s",finite(ir)&&energy(ir)>0);
        }
    }
}
void densityTests(bool modern)
{
    for(int engine : {1,4,3})
    {
        if(!modern && engine!=1) continue;
        const auto ir=impulse(neutral(modern?engine:0),2,fs);
        const auto time=densityTime(ir,fs);
        row("V5",modern?names[engine]:"Legacy Hall","NED .9 at "+number(time*1000,2)+" ms after onset",modern?(time>=0&&time<=.15?1:0):-1);
    }
}
void predelayTests(bool modern)
{
    const auto baseline=impulse(neutral(modern?1:0),2,fs);const int start=onset(baseline);
    for(double ms : {20.,250.,500.})
    {
        const auto delayed=impulse(neutral(modern?1:0,2,ms),2,fs);
        const int observed=onset(delayed)-start,expected=static_cast<int>(std::round(ms*.001*fs));
        row("V4","Predelay "+number(ms,0)+" ms","shift="+juce::String(observed)+" samples; expected="+juce::String(expected),std::abs(observed-expected)<=1);
    }
}
void stereoTests(bool modern)
{
    const int last=modern?6:0;
    for(int engine=modern?1:0;engine<=last;++engine)
    {
        auto s=neutral(engine);const auto ir=impulse(s,4,fs);const auto metrics=stereo(ir);
        set(s,"width",0);const auto mono=impulse(s,2,fs);
        const bool equal=std::memcmp(mono.getReadPointer(0),mono.getReadPointer(1),static_cast<size_t>(mono.getNumSamples())*sizeof(float))==0;
        row("V7",names[engine],"correlation="+number(metrics.first)+", mono energy="+number(metrics.second,2)+" dB; width0 exact="+juce::String(equal?"yes":"no"),equal);
    }
}
void freezeTests(bool modern,bool quick)
{
    if(!modern){row("V6","New Freeze","n/a: V2 engine absent");return;}
    for(int engine=1;engine<=6;++engine)
    {
        auto p=processor(neutral(engine));
        const auto feed=excitation(2);render(*p,feed);
        p->setParameterValue("freeze",1);
        const int seconds=quick?8:300;
        juce::AudioBuffer<float> block(2,256);juce::MidiBuffer midi;
        double windowA=0,windowB=0,peak=0,internalA=0,internalB=0;int countA=0,countB=0;bool ok=true;
        for(int pos=0;pos<seconds*static_cast<int>(fs);pos+=256)
        {
            block.clear();p->processBlock(block,midi);ok=ok&&finite(block);peak=std::max(peak,static_cast<double>(block.getMagnitude(0,256)));
            const double sec=pos/fs;
            if(sec>=2&&sec<7){windowA+=energy(block);++countA;internalA+=tankEnergy(*p,0);}
            if(sec>=62&&sec<67){windowB+=energy(block);++countB;internalB+=tankEnergy(*p,0);}
        }
        const double drift=countB>0?10*std::log10((windowB/countB)/(windowA/std::max(1,countA)+1e-300)+1e-300):0;
        const double stateDrift=countB>0?10*std::log10((internalB/countB)/(internalA/std::max(1,countA)+1e-300)+1e-300):0;
        row("V6",names[engine],"duration="+juce::String(seconds)+" s, output drift="+number(drift,3)+" dB, state drift="+number(stateDrift,3)+" dB, peak="+number(peak),
            quick?-1:(ok&&windowA>1e-20&&std::abs(drift)<=1&&peak<16?1:0));
        p->setParameterValue("freeze",0);
        juce::AudioBuffer<float> release(2,static_cast<int>(fs*10));release.clear();const auto tail=render(*p,release);
        row("V6",juce::String(names[engine])+" release","final-second energy="+number(10*std::log10(energy(tail,static_cast<int>(fs*9))+1e-300),2)+" dB",finite(tail));
    }
}
void shapeTests(bool modern)
{
    if(!modern){row("V8/V9","Time shapes and Shimmer","n/a: V2 parameters absent");return;}
    auto source=excitation(.05);juce::AudioBuffer<float> x(2,static_cast<int>(fs*3));x.clear();
    for(int ch=0;ch<2;++ch)x.copyFrom(ch,0,source,ch,0,source.getNumSamples());
    auto base=neutral(4);set(base,"earlyDb",0);set(base,"gateThresholdDb",-40);set(base,"shapeTimeMs",250);set(base,"gateReleaseMs",30);
    for(int shape=0;shape<4;++shape)
    {
        auto s=base;set(s,"timeShape",shape);auto p=processor(s);settle(*p);const auto y=render(*p,x);
        const double early=energy(y,0,static_cast<int>(fs*.1)),middle=energy(y,static_cast<int>(fs*.15),static_cast<int>(fs*.1));
        const double late=energy(y,static_cast<int>(fs*.4));
        const double residual=10*std::log10(late/(energy(y)+1e-300)+1e-300);
        row("V8","Shape "+juce::String(shape),"early="+number(early)+", rising window="+number(middle)+", residual after400ms="+number(residual,2)+" dB",
            finite(y)&&energy(y)>1e-15&&(shape!=1||residual<-60));
    }
    auto s=neutral(1);set(s,"shimmerAmount",1);set(s,"shimmerFeedback",.85);set(s,"shimmerPitch",12);
    const auto y=impulse(s,20,fs);
    row("V9","Shimmer max-feedback stress","peak="+number(y.getMagnitude(0,y.getNumSamples()))+", tail energy="+number(energy(y,static_cast<int>(fs*15))),
        finite(y)&&y.getMagnitude(0,y.getNumSamples())<16);
}
void cpuTests(bool modern)
{
    for(int engine=modern?1:0;engine<=(modern?6:0);++engine)
    {
        auto p=processor(neutral(engine));auto input=excitation(256/fs);juce::AudioBuffer<float> block(input);juce::MidiBuffer midi;
        for(int i=0;i<200;++i){block.makeCopyOf(input);p->processBlock(block,midi);}
        double total=0,worst=0;
        for(int i=0;i<1000;++i)
        {
            block.makeCopyOf(input);const auto start=std::chrono::steady_clock::now();p->processBlock(block,midi);
            const auto elapsed=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();total+=elapsed;worst=std::max(worst,elapsed);
        }
        row("V10",names[engine],"mean="+number(100*total/1000/(256/fs),2)+"%, maximum="+number(100*worst/(256/fs),2)+"%",modern?(total/1000/(256/fs)<=.25?1:0):-1);
    }
    if(modern)
    {
        auto p=processor(neutral(1,10));auto block=excitation(256/fs);juce::MidiBuffer midi;
        for(int i=0;i<200;++i)p->processBlock(block,midi);
        p->setParameterValue("engine",3);double total=0;
        for(int i=0;i<1000;++i){block.clear();const auto start=std::chrono::steady_clock::now();p->processBlock(block,midi);total+=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();}
        row("V10","Two-path transition","mean="+number(100*total/1000/(256/fs),2)+"%",total/1000/(256/fs)<=.5);
    }
}
}
int main(int argc,char** argv)
{
    juce::ScopedJuceInitialiser_GUI gui;
    if(argc<3){std::cerr<<"Usage: HamloVerb_Lab report.md label [all|quick|rt60|density|predelay|stereo|freeze|shapes|cpu]\n";return 2;}
    auto p=processor({});const bool modern=p->parameter("engine")!=nullptr;p.reset();
    const juce::String section=argc>3?argv[3]:"all";
    const auto run=[&](const char* name){return section=="all"||section=="quick"||section==name;};
    report="# HamloVerb acoustic laboratory: "+juce::String(argv[2])+"\n\n"
        "Fixed protocol in docs/HAMLOVERB_V2.md. 48 kHz stereo, prepare(256), one second of silent settling before impulses. "
        "Legacy wet is recovered by subtracting the direct impulse; modern uses Send. Neutral decay disables modulation, "
        "uses bass/HF ratios 1, Early -60 dB, cuts 10/20000 Hz. CPU warm-up excludes 200 blocks. "
        "NED uses a normalized 25 ms Hann window, 1 ms sampling, threshold .9 relative to wet onset. "
        "These numerical checks do not replace listening or the separate reference, preset and validator reports.\n\n"
        "| Goal | Case | Measurement | Result |\n|---|---|---|---|\n";
    if(run("rt60"))decayTests(modern);
    if(run("density"))densityTests(modern);
    if(run("predelay"))predelayTests(modern);
    if(run("stereo"))stereoTests(modern);
    if(run("freeze"))freezeTests(modern,section=="quick");
    if(run("shapes"))shapeTests(modern);
    if(run("cpu"))cpuTests(modern);
    report+="\n**"+juce::String(passed)+" checks passed; "+juce::String(failed)+" failed.** "
        "INFO/n/a rows are not passes. Section: `"+section+"`.\n";
    const auto file=juce::File::getCurrentWorkingDirectory().getChildFile(argv[1]);file.getParentDirectory().createDirectory();
    if(!file.replaceWithText(report))return 2;
    return failed==0?0:1;
}

#include "DSP/TransitionRender.h"
#include <algorithm>
#include <cmath>
#include <numeric>

namespace hpeq::dsp::transition
{
namespace
{
constexpr double pi=3.14159265358979323846;
std::uint64_t hash(std::uint64_t z) noexcept
{ z+=0x9e3779b97f4a7c15ull;z=(z^(z>>30))*0xbf58476d1ce4e5b9ull;z=(z^(z>>27))*0x94d049bb133111ebull;return z^(z>>31); }
double random(std::uint64_t seed,std::uint64_t salt) noexcept
{return static_cast<double>(hash(seed^hash(salt))>>11)/9007199254740992.;}
double fade(double x) noexcept {return .5-.5*std::cos(pi*std::clamp(x,0.,1.));}
struct Attack {int sample=0;double energy=0;};
std::vector<Attack> attacks(const juce::AudioBuffer<float>& x,double fs)
{
    const int hop=std::max(1,static_cast<int>(fs*.002)),frames=(x.getNumSamples()+hop-1)/hop;
    std::vector<double> energy(static_cast<size_t>(frames));
    for(int f=0;f<frames;++f)for(int c=0;c<x.getNumChannels();++c)
        for(int n=f*hop;n<std::min(x.getNumSamples(),(f+1)*hop);++n)
            energy[static_cast<size_t>(f)]+=static_cast<double>(x.getSample(c,n))*x.getSample(c,n);
    std::vector<double> novelty(energy.size());double maximum=0;
    for(int f=0;f<frames;++f)
    {const double e=std::sqrt(energy[static_cast<size_t>(f)]/hop);const double prev=f?std::sqrt(energy[static_cast<size_t>(f-1)]/hop):0;
        novelty[static_cast<size_t>(f)]=std::max(0.,e-prev);maximum=std::max(maximum,novelty[static_cast<size_t>(f)]);}
    std::vector<Attack> out;
    for(int f=0;f<frames;++f)
    {
        const double v=novelty[static_cast<size_t>(f)];
        if(v<maximum*.055||v<=1e-10||(f>0&&v<novelty[static_cast<size_t>(f-1)])||(f+1<frames&&v<novelty[static_cast<size_t>(f+1)]))continue;
        const int sample=std::max(0,f*hop-static_cast<int>(fs*.001));
        if(!out.empty()&&sample-out.back().sample<fs*.025){if(v>out.back().energy)out.back()={sample,v};continue;}
        out.push_back({sample,v});
    }
    if(out.empty())for(int k=0;k<16;++k)out.push_back({x.getNumSamples()*k/16,0});
    if(out.size()>96)
    {std::stable_sort(out.begin(),out.end(),[](auto a,auto b){return a.energy>b.energy;});out.resize(96);std::sort(out.begin(),out.end(),[](auto a,auto b){return a.sample<b.sample;});}
    return out;
}
// A normalized polyphase windowed-sinc reader. Filter bandwidth follows each
// grain's resampling ratio; coefficients are prepared offline,not per sample.
struct Reader
{
    static constexpr int phases=128,taps=16;
    float weights[phases][taps] {};
    explicit Reader(double rate,double bandwidth=.94)
    {
        const double cutoff=bandwidth/std::max(1.,rate);
        for(int phase=0;phase<phases;++phase)
        {double sum=0;for(int k=0;k<taps;++k)
            {const double d=k-7.-static_cast<double>(phase)/phases;const double t=pi*d*cutoff;
             const double w=cutoff*(std::abs(t)<1e-12?1.:std::sin(t)/t)*(.5+.5*std::cos(pi*d/8));
             weights[phase][k]=static_cast<float>(w);sum+=w;}
         for(auto& w:weights[phase])w=static_cast<float>(w/sum);}
    }
    float get(const juce::AudioBuffer<float>& x,int ch,double pos)const noexcept
    {
        const int base=static_cast<int>(std::floor(pos));const int phase=std::clamp(static_cast<int>((pos-base)*phases),0,phases-1);
        float y=0;for(int k=0;k<taps;++k){const int n=base+k-7;if(n>=0&&n<x.getNumSamples())y+=weights[phase][k]*x.getSample(ch,n);}return y;
    }
};
void addEvent(const juce::AudioBuffer<float>& source,juce::AudioBuffer<float>& out,const Event& e,double fs)
{
    Reader reader(e.rate);
    // Fade at the actual available source boundary as well as the event boundary.
    // Otherwise a late selected attack can run into zero-padding mid-grain.
    const int available=std::max(0,static_cast<int>((source.getNumSamples()-e.source-8)/std::max(.01,e.rate)));
    const int count=std::min({e.length,out.getNumSamples()-e.at,available});if(count<=0)return;
    const double edge=std::min(.003*fs,count*.2);
    for(int i=0;i<count;++i)
    {
        const double envelope=fade((i+1)/(edge+1))*fade((count-1-i)/(edge+1));
        const double pos=e.source+(e.reverse?(count-1-i)*e.rate:i*e.rate);
        for(int c=0;c<2;++c)
        {const double pan=c==0?std::min(1.,1-e.pan):std::min(1.,1+e.pan);
         out.addSample(c,e.at+i,static_cast<float>(e.velocity*envelope*pan*reader.get(source,c,pos)));}
    }
}
}
const char* recipeName(int kind,int recipe) noexcept
{
    static constexpr const char* rise[]{"CLASSIC","AUTO","LIFT","PULSE","SWELL","RUSH","ARC","PRESSURE"};
    static constexpr const char* fill[]{"CLASSIC","AUTO","POCKET","ROLL","BREAK","TRIPLETS","SCATTER","TURNAROUND"};
    return (kind==Rise?rise:fill)[std::clamp(recipe,0,7)];
}
int resolvedRecipe(const Settings& s) noexcept
{return s.recipe==1?2+static_cast<int>(hash(s.seed^0x52495345ull)%6):std::clamp(s.recipe,0,7);}

Plan makePlan(const juce::AudioBuffer<float>& source,const Settings& s)
{
    Plan plan;plan.recipe=resolvedRecipe(s);if(source.getNumChannels()<1||source.getNumSamples()==0)return plan;
    const auto bank=attacks(source,s.sampleRate);plan.attacks=static_cast<int>(bank.size());
    const int length=lengthSamples(s);const double beat=s.sampleRate*60/s.bpm,bar=beat*4;
    const int profile=plan.recipe-2;
    const auto pick=[&](int index,bool strong)
    {if(strong)return std::max_element(bank.begin(),bank.end(),[](auto a,auto b){return a.energy<b.energy;})->sample;
     return bank[static_cast<size_t>(hash(s.seed+static_cast<std::uint64_t>(index)*71)%bank.size())].sample;};
    const auto event=[&](double at,double duration,int index,double velocity,double semitones,bool backwards=false)
    {
        Event e;e.at=static_cast<int>(std::lround(at));e.length=std::max(8,static_cast<int>(std::lround(duration)));
        e.source=pick(index,index==0);e.velocity=velocity;e.rate=std::pow(2.,semitones/12);e.reverse=backwards;
        e.pan=(random(s.seed,static_cast<std::uint64_t>(index)+501)-.5)*.7;
        plan.events.push_back(e);
    };
    if(s.kind==Rise)
    {
        double at=0;int index=0;
        while(at<length)
        {
            const double x=at/length;
            double divisor=1;
            switch(profile)
            {
                case 0:divisor=x<.35?1:x<.62?2:x<.8?4:x<.92?8:16;break;
                case 1:divisor=x<.5?2: x<.8?4:8;break;
                case 2:divisor=x<.6?1:4;break;
                case 3:divisor=std::pow(2.,1+4*x);break;
                case 4:divisor=x<.35?4:x<.6?1:x<.85?4:16;break;
                default:divisor=x<.5?3:x<.8?6:12;break;
            }
            const double jitter=1.+.12*(random(s.seed,static_cast<std::uint64_t>(index)+800)-.5);
            double step=beat/divisor*jitter;
            if(profile==1&&index%4==3)step*=1.5;
            const double pitch=12*s.knobs[0]*(profile==4?std::clamp(x+.13*std::sin(4*pi*x),0.,1.):std::pow(x,.65+random(s.seed,18)*.7));
            const double velocity=s.knobs[1]*(index%4==0?1.:.65+.25*random(s.seed,static_cast<std::uint64_t>(index)+17));
            event(at,std::min(step*1.18,beat*(profile==2?2.:1.)),index,velocity,pitch,profile==2||((profile==4)&&index%5==3));
            at+=std::max(.008*s.sampleRate,step);++index;
        }
    }
    else
    {
        if(std::all_of(s.knobs.begin(),s.knobs.end(),[](double x){return x<=0;}))return plan;
        const double last=length-bar;double tick=0;int index=0;
        while(tick<96)
        {
            double step=6;
            switch(profile)
            {
                case 0:step=index%3==0?12:6;break;
                case 1:step=tick<48?12:tick<72?6:3;break;
                case 2:{static constexpr int sizes[]{9,6,3,12,6,9,3,6};step=sizes[(index+static_cast<int>(s.seed%8))%8];break;}
                case 3:step=tick<48?8:tick<72?4:2;break;
                case 4:step=3*(1+static_cast<int>(random(s.seed,static_cast<std::uint64_t>(index)+37)*4));break;
                default:step=tick<48?6:tick<72?4:3;break;
            }
            if(tick>=72)
            {
                // The ROLL control overrides the recipe's final-beat density,
                // including at zero; its acceleration shape also varies by seed.
                static constexpr double grid[]{12,8,6,4,3,2,1.5};
                const double progress=(tick-72)/24;
                const double shape=.5+random(s.seed,811);
                const int rate=std::clamp(static_cast<int>(std::lround(s.knobs[1]*(3+3*std::pow(progress,shape)))),0,6);
                step=profile==3?12./std::pow(2.,rate*.5):grid[rate];
            }
            const bool accent=std::fmod(tick,24)<.01;
            const bool play=accent||random(s.seed,static_cast<std::uint64_t>(index)+12)<.4+.55*s.knobs[0]||tick>=72;
            if(play)
            {
                const double swing=(profile==0&&index%2)?1.2:0.;
                const double velocity=accent?1.:.35+.5*random(s.seed,static_cast<std::uint64_t>(index)+44);
                const double pitch=profile==5?-2*(index%4):profile==4?(index%3-1)*2:0.;
                const bool backwards=!accent&&random(s.seed,static_cast<std::uint64_t>(index)+59)<s.knobs[2]*.35;
                event(last+(tick+swing)*bar/96,std::min(beat*.65,step*bar/96*(accent?1.05:.82)),index,velocity,pitch,backwards);
            }
            tick+=step;++index;
        }
    }
    return plan;
}

juce::AudioBuffer<float> renderEvolve(const juce::AudioBuffer<float>& input,const Settings& original)
{
    Settings s=original;
    if(!std::isfinite(s.sampleRate)||!std::isfinite(s.bpm)||s.sampleRate<8000||s.sampleRate>192000)return {};
    s.bpm=std::clamp(s.bpm,20.,999.);s.bars=s.bars==1||s.bars==4?s.bars:2;
    for(auto& k:s.knobs)k=std::isfinite(k)?std::clamp(k,0.,1.):0.;
    const int length=lengthSamples(s);juce::AudioBuffer<float> source(2,length);source.clear();
    if(input.getNumChannels()>0)for(int c=0;c<2;++c)
        for(int n=0;n<std::min(length,input.getNumSamples());++n)
        {const float x=input.getSample(std::min(c,input.getNumChannels()-1),n);source.setSample(c,n,std::isfinite(x)?x:0.f);}
    const auto plan=makePlan(source,s);juce::AudioBuffer<float> out(2,length);out.clear();
    if(s.kind==Fill)
    {
        out.makeCopyOf(source);const int last=static_cast<int>(std::lround(length*(s.bars-1.)/s.bars));
        if(!plan.events.empty())for(int c=0;c<2;++c)out.clear(c,last,length-last);
    }
    for(const auto& e:plan.events)addEvent(source,out,e,s.sampleRate);
    if(s.kind==Rise)
    {
        if(s.knobs[2]>0)
        {
            Settings swell=s;swell.recipe=0;swell.knobs={0,0,1,.4};const auto field=render(source,swell);
            const float amount=static_cast<float>(s.knobs[2]*(plan.recipe==4?.8:.35));
            for(int c=0;c<2;++c)out.addFrom(c,0,field,c,0,length,amount);
        }
        double low[2]{},high[2]{};
        for(int n=0;n<length;++n)
        {
            const double x=static_cast<double>(n)/std::max(1,length-1);
            const double cutoff=500*std::pow((4500+13500*s.knobs[3])/500,x);
            const double lp=std::exp(-2*pi*std::min(cutoff,s.sampleRate*.45)/s.sampleRate);
            const double hp=std::exp(-2*pi*(20+160*s.knobs[3]*x*x)/s.sampleRate);
            const double envelope=std::pow(10.,-(15+9*s.knobs[3])*std::pow(1-x,1.25)/20);
            for(int c=0;c<2;++c)
            {const double v=out.getSample(c,n);low[c]=v+lp*(low[c]-v);high[c]=low[c]+hp*(high[c]-low[c]);
             out.setSample(c,n,static_cast<float>((low[c]-high[c])*envelope));}
        }
    }
    else if(s.knobs[3]>.05&&random(s.seed,991)<s.knobs[3])
    {
        const int start=length-static_cast<int>(s.sampleRate*60/s.bpm*.25);const auto held=out;Reader reader(1);
        for(int n=start;n<length;++n)
        {const double x=static_cast<double>(n-start)/std::max(1,length-start-1);const double pos=start+(length-start)*(x-.5*x*x);
         for(int c=0;c<2;++c)out.setSample(c,n,static_cast<float>(reader.get(held,c,pos)*(1-x)));}
    }
    const int fadeSamples=std::max(1,static_cast<int>(s.sampleRate*.005));
    for(int n=0;n<std::min(length,fadeSamples);++n)for(int c=0;c<2;++c)
        out.setSample(c,length-1-n,out.getSample(c,length-1-n)*static_cast<float>(fade(static_cast<double>(n)/fadeSamples)));
    // Eight-phase full-band intersample measurement plus .5dB margin for the
    // independent release meter. Include raw samples explicitly:the grain
    // reader's anti-alias cutoff must not hide near-Nyquist source peaks.
    Reader meter(1,1.);double peak=0;
    for(int c=0;c<2;++c)for(int n=0;n<length;++n)
    {peak=std::max(peak,std::abs(static_cast<double>(out.getSample(c,n))));
     for(int phase=1;phase<8;++phase)peak=std::max(peak,std::abs(static_cast<double>(meter.get(out,c,n+phase*.125))));}
    const double ceiling=std::pow(10.,-1.5/20);if(peak>ceiling)out.applyGain(static_cast<float>(ceiling/peak));
    return out;
}
}

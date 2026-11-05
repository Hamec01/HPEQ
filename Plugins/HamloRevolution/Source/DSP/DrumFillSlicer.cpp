#include "DSP/TransitionRender.h"
#include <algorithm>
#include <cmath>
#include <numeric>
#include <set>

namespace hpeq::dsp::transition
{
namespace
{
constexpr double pi=3.14159265358979323846;
std::uint64_t mix(std::uint64_t z) noexcept
{z+=0x9e3779b97f4a7c15ull;z=(z^(z>>30))*0xbf58476d1ce4e5b9ull;z=(z^(z>>27))*0x94d049bb133111ebull;return z^(z>>31);}
double unit(std::uint64_t seed,std::uint64_t salt)noexcept{return static_cast<double>(mix(seed^mix(salt))>>11)/9007199254740992.;}
double cosine(double x)noexcept{return .5-.5*std::cos(pi*std::clamp(x,0.,1.));}
struct Sinc
{
    static constexpr int taps=24,phases=128;
    float coefficients[phases][taps]{};
    explicit Sinc(double ratio,double bandwidth=.94)
    {
        const double fc=bandwidth/std::max(1.,ratio);
        for(int p=0;p<phases;++p){double sum=0;for(int k=0;k<taps;++k){const double d=k-11.-static_cast<double>(p)/phases,t=pi*d*fc;const double w=fc*(std::abs(t)<1e-12?1:std::sin(t)/t)*(.5+.5*std::cos(pi*d/12));coefficients[p][k]=static_cast<float>(w);sum+=w;}for(auto& c:coefficients[p])c=static_cast<float>(c/sum);}
    }
    double read(const juce::AudioBuffer<float>& x,int channel,double at)const noexcept
    {
        const int base=static_cast<int>(std::floor(at)),p=std::clamp(static_cast<int>((at-base)*phases),0,phases-1);double y=0;
        for(int k=0;k<taps;++k){const int n=base+k-11;if(n>=0&&n<x.getNumSamples())y+=coefficients[p][k]*static_cast<double>(x.getSample(channel,n));}return y;
    }
};
std::vector<Slice> sliceBank(const juce::AudioBuffer<float>& x,const Settings& s)
{
    const int hop=std::max(1,static_cast<int>(s.sampleRate*.002)),frames=(x.getNumSamples()+hop-1)/hop;
    std::vector<double> envelope(static_cast<size_t>(frames)),novelty(static_cast<size_t>(frames));
    for(int f=0;f<frames;++f){double e=0;for(int c=0;c<2;++c)for(int n=f*hop;n<std::min(x.getNumSamples(),(f+1)*hop);++n)e+=x.getSample(c,n)*static_cast<double>(x.getSample(c,n));envelope[static_cast<size_t>(f)]=std::sqrt(e/(2*hop));}
    double maximum=0;for(int f=0;f<frames;++f){double preceding=0;for(int j=1;j<=3;++j)if(f>=j)preceding+=envelope[static_cast<size_t>(f-j)]/3;novelty[static_cast<size_t>(f)]=std::max(0.,envelope[static_cast<size_t>(f)]-preceding);maximum=std::max(maximum,novelty[static_cast<size_t>(f)]);}
    std::vector<int> attacks;
    for(int f=0;f<frames;++f)
    {
        const double v=novelty[static_cast<size_t>(f)];if(v<maximum*.065||v<1e-9||(f>0&&v<novelty[static_cast<size_t>(f-1)])||(f+1<frames&&v<novelty[static_cast<size_t>(f+1)]))continue;
        const int at=std::max(0,f*hop-hop/2);if(!attacks.empty()&&at-attacks.back()<s.sampleRate*.018)continue;attacks.push_back(at);
    }
    // Grids are fallback slices, not fabricated percussion. Actual attacks are
    // preferred whenever present; no perfect kit/stem separation is claimed.
    if(attacks.empty()&&x.getMagnitude(0,x.getNumSamples())>1e-9)for(int i=0;i<16;++i)attacks.push_back(x.getNumSamples()*i/16);
    if(attacks.size()>192){std::vector<int> reduced;for(size_t i=0;i<192;++i)reduced.push_back(attacks[i*attacks.size()/192]);attacks=std::move(reduced);}
    std::vector<Slice> result;
    const double lowK=1-std::exp(-2*pi*220/s.sampleRate),bodyK=1-std::exp(-2*pi*3400/s.sampleRate);
    for(size_t i=0;i<attacks.size();++i)
    {
        Slice slice;slice.sample=attacks[i];const int next=i+1<attacks.size()?attacks[i+1]:x.getNumSamples();
        slice.end=std::min(x.getNumSamples(),slice.sample+std::max(16,std::min(next-slice.sample,static_cast<int>(s.sampleRate*.24))));
        double energy[3]{},lp[2]{},bp[2]{};
        const int end=std::min(slice.end,slice.sample+static_cast<int>(s.sampleRate*.10));
        for(int n=slice.sample;n<end;++n)for(int c=0;c<2;++c){const double v=x.getSample(c,n);lp[c]+=lowK*(v-lp[c]);bp[c]+=bodyK*(v-bp[c]);energy[0]+=lp[c]*lp[c];energy[1]+=(bp[c]-lp[c])*(bp[c]-lp[c]);energy[2]+=(v-bp[c])*(v-bp[c]);}
        const double total=energy[0]+energy[1]+energy[2]+1e-30;slice.low=energy[0]/total;slice.body=energy[1]/total;slice.air=energy[2]/total;slice.energy=total;
        slice.role=slice.low>.38?0:slice.air>.52?2:1;result.push_back(slice);
    }
    return result;
}
int family(const Settings& s)noexcept{return s.recipe==8?static_cast<int>((s.seed%12+mix(s.seed/12^0x46494c4c523133ull)%12)%12):std::clamp(s.recipe-9,0,11);}
void grain(const juce::AudioBuffer<float>& source,juce::AudioBuffer<float>& out,juce::AudioBuffer<float>& sends,const Event& e,double fs)
{
    const int end=std::min(source.getNumSamples(),e.sourceEnd),available=std::max(0,static_cast<int>((end-e.source-2)/std::max(.125,e.rate)));
    const int count=std::min({e.length,available,out.getNumSamples()-e.at});if(count<=2)return;
    Sinc reader(e.rate);const double attack=std::min(count*.15,fs*(e.reverse?.002:.0004)),release=std::min(count*.25,fs*.004);
    const double lowK=1-std::exp(-2*pi*(e.role==0?2400:12000)/fs),highK=1-std::exp(-2*pi*(e.role==2?1800:35)/fs);
    double low[2]{},high[2]{};
    for(int n=0;n<count;++n)
    {
        const double envelope=cosine((n+1)/(attack+1))*cosine((count-1-n)/(release+1));const double at=e.source+(e.reverse?count-1-n:n)*e.rate;
        for(int c=0;c<2;++c)
        {
            double sample=reader.read(source,c,at);low[c]+=lowK*(sample-low[c]);high[c]+=highK*(low[c]-high[c]);
            sample=.65*sample+.35*(low[c]-high[c]);const double pan=c==0?std::min(1.,1-e.pan):std::min(1.,1+e.pan);
            const float value=static_cast<float>(sample*envelope*e.velocity*pan);out.addSample(c,e.at+n,value);if(e.space>0)sends.addSample(c,e.at+n,static_cast<float>(value*e.space));
        }
    }
}
void effects(juce::AudioBuffer<float>& out,const juce::AudioBuffer<float>& sends,const Plan& plan,const Settings& s)
{
    const double amount=s.fillFx;if(amount<=0)return;
    std::array<std::array<std::vector<double>,4>,2> delays;std::array<int,4> lengths{};
    constexpr double times[]{.0197,.0271,.0343,.0419};for(int k=0;k<4;++k){lengths[static_cast<size_t>(k)]=std::max(8,static_cast<int>(s.sampleRate*times[k]));for(int c=0;c<2;++c)delays[static_cast<size_t>(c)][static_cast<size_t>(k)].assign(static_cast<size_t>(lengths[static_cast<size_t>(k)]),0);}
    int positions[4]{};double damping[2][4]{},hp[2]{};
    const double dampingK=1-std::exp(-2*pi*5200/s.sampleRate),feedback=.78+.13*amount;
    for(int n=plan.phraseStart;n<out.getNumSamples();++n)
    {
        const double progress=static_cast<double>(n-plan.phraseStart)/std::max(1,out.getNumSamples()-plan.phraseStart-1),cutoff=25+(250+220*amount)*progress*progress*amount;
        const double hpk=1-std::exp(-2*pi*cutoff/s.sampleRate);
        for(int c=0;c<2;++c)
        {
            double wet=0;for(int k=0;k<4;++k){auto& line=delays[static_cast<size_t>(c)][static_cast<size_t>(k)];const double z=line[static_cast<size_t>(positions[k])];damping[c][k]+=dampingK*(z-damping[c][k]);line[static_cast<size_t>(positions[k])]=sends.getSample(c,n)+feedback*damping[c][k];wet+=(k%2&&c==1?-.8:1.)*z*.25;}
            double value=out.getSample(c,n);hp[c]+=hpk*(value-hp[c]);value-=hp[c]*amount;out.setSample(c,n,static_cast<float>(value+wet*amount*.9));
        }
        for(int k=0;k<4;++k)if(++positions[k]>=lengths[static_cast<size_t>(k)])positions[k]=0;
    }
}
}

Plan makeSlicedFillPlan(const juce::AudioBuffer<float>& source,const Settings& settings)
{
    Plan plan;Settings s=settings;
    if(source.getNumChannels()<1||source.getNumSamples()==0||!std::isfinite(s.sampleRate)||!std::isfinite(s.bpm)||s.sampleRate<8000||s.sampleRate>192000)return plan;
    s.bpm=std::clamp(s.bpm,20.,999.);s.bars=s.bars==1||s.bars==4?s.bars:2;s.fillBeats=s.fillBeats==1||s.fillBeats==2?s.fillBeats:4;
    for(auto& value:s.knobs)value=std::isfinite(value)?std::clamp(value,0.,1.):0.;
    juce::AudioBuffer<float> stereo;
    const juce::AudioBuffer<float>* input=&source;if(source.getNumChannels()==1){stereo.setSize(2,source.getNumSamples());for(int c=0;c<2;++c)stereo.copyFrom(c,0,source,0,0,source.getNumSamples());input=&stereo;}
    plan.slices=sliceBank(*input,s);plan.attacks=static_cast<int>(plan.slices.size());const int style=family(s);plan.recipe=9+style;
    const int length=lengthSamples(s);const double beat=s.sampleRate*60/s.bpm;plan.phraseStart=std::max(0,length-static_cast<int>(std::lround(beat*s.fillBeats)));plan.phraseEnd=length;
    if(plan.slices.empty()||std::all_of(s.knobs.begin(),s.knobs.end(),[](double value){return value==0;}))return plan;
    std::array<std::vector<int>,3> pools;
    for(size_t i=0;i<plan.slices.size();++i)pools[static_cast<size_t>(plan.slices[i].role)].push_back(static_cast<int>(i));
    const auto pick=[&](int role,std::uint64_t salt)
    {
        const auto& pool=pools[static_cast<size_t>(role)];
        if(!pool.empty())
        {
            double total=0;for(int index:pool)total+=std::pow(plan.slices[static_cast<size_t>(index)].energy,.25);
            double target=unit(s.seed,salt)*total;
            for(int index:pool){target-=std::pow(plan.slices[static_cast<size_t>(index)].energy,.25);if(target<=0)return index;}
            return pool.back();
        }
        int best=0;double score=-1;for(size_t i=0;i<plan.slices.size();++i){const auto& slice=plan.slices[i];const double ratio=role==0?slice.low:role==1?slice.body:slice.air;const double value=ratio*std::sqrt(slice.energy);if(value>score){score=value;best=static_cast<int>(i);}}return best;
    };
    const int anchor=pick(1,0x424f4459);int serial=0;
    const auto add=[&](double tick,double duration,int role,double velocity,bool roll=false,bool reverse=false)
    {
        const int index=roll?anchor:pick(role,static_cast<std::uint64_t>(serial)*193+static_cast<std::uint64_t>(style)*971+77);
        const auto& slice=plan.slices[static_cast<size_t>(index)];Event e;e.at=plan.phraseStart+static_cast<int>(std::lround(tick*beat/24));e.length=std::max(8,static_cast<int>(std::lround(duration*beat/24)));
        e.source=slice.sample;e.sourceEnd=slice.end;e.role=slice.role;e.velocity=velocity;e.pan=(unit(s.seed,static_cast<std::uint64_t>(serial)+701)-.5)*.5;e.reverse=reverse;e.rate=1;e.roll=roll;
        if(e.at<length)plan.events.push_back(e);++serial;
    };
    static const std::vector<std::vector<int>> motifs{{0,12,18},{0,6,12,18},{0,9,15,21},{0,8,16},{0,3,12,18,21},{6,12,21},{0,12},{0,6,9,18},{3,9,12,21},{0,4,8,16,20},{0,6,15,18},{0,3,6,12,15,21}};
    for(int b=0;b<s.fillBeats;++b)
    {
        const bool final=b==s.fillBeats-1;const auto seed=mix(s.seed^static_cast<std::uint64_t>(b*397+style*197+113));
        int motif=static_cast<int>(seed%motifs.size());if(style==3)motif=(seed&1)?3:9;
        std::set<int> ticks(motifs[static_cast<size_t>(motif)].begin(),motifs[static_cast<size_t>(motif)].end());
        const int mutations=1+static_cast<int>(s.knobs[0]*3);
        for(int j=0;j<mutations;++j)
        {
            const int grid=style==3?4:3,slot=static_cast<int>(mix(seed+static_cast<std::uint64_t>(j)*127)%(24/grid))*grid;
            if(slot>0&&ticks.count(slot)&&ticks.size()>2)ticks.erase(slot);else ticks.insert(slot);
        }
        if(style==4||style==6){const int gap=static_cast<int>((seed>>12)%3)*6+3;ticks.erase(gap);ticks.erase(gap+3);}
        // Real negative space distinguishes phrase families on dense loops.
        // More nominal events alone does not make a more varied audible fill.
        if(style==6&&b%2==0){ticks.clear();ticks.insert(static_cast<int>((seed>>5)%3)*3);if(seed&8)ticks.insert(12);}
        if(style==9){for(int tick:{9,12,15})ticks.erase(tick);}
        if(style==11&&b>=s.fillBeats/2){ticks.clear();ticks.insert(static_cast<int>((seed>>9)%4)*3);if(!final&&(seed&16))ticks.insert(18);}
        if(style==8)for(int tick:{3,9,15,21})if(unit(s.seed,static_cast<std::uint64_t>(b*19+tick))<s.knobs[0])ticks.insert(tick);
        if(style==11&&final){ticks.clear();ticks.insert(static_cast<int>((seed>>7)%3)*3);}
        for(int tick:ticks)
        {
            if(final&&(style==1||style==3)&&tick>=12&&s.knobs[1]>.05)continue;
            const bool accent=tick==0||tick==12;int role=accent?((b+tick/12)%2?1:0):1;
            if(style==2)role=(tick==0||tick>=15)?0:1;
            if(style==6)role=b%2==0?0:1;
            if(style==11&&b>=s.fillBeats/2)role=1;
            if(style==10&&!accent)role=(tick%2)?2:1;
            const double velocity=accent?.95:style==10?.18+.23*unit(seed,static_cast<std::uint64_t>(tick)):.38+.4*unit(seed,static_cast<std::uint64_t>(tick));
            const auto next=ticks.upper_bound(tick);const double gap=next==ticks.end()?24-tick:*next-tick;
            const bool backwards=tick>=15&&!accent&&unit(seed,static_cast<std::uint64_t>(tick)+991)<s.knobs[2]*(style==7?.95:.3);
            add(b*24+tick,std::min(12.,gap*(accent?1.08:.86)),role,velocity,false,backwards);
            if(style==9&&tick>=6&&unit(seed,static_cast<std::uint64_t>(tick)+19)<s.knobs[1])
                for(int r=1;r<=2;++r)add(b*24+tick+r*1.5,1.5,1,velocity*(.55+.2*r),true,false);
        }
        if(final&&(style==1||style==3||s.knobs[1]>.35)&&style!=9&&style!=11)
        {
            const int start=style==1?static_cast<int>((seed>>4)%3)*3:style==8?0:style==0||style==10?18:style==4?6:12;
            const double end=style==4?18.:24.;
            const double step=style==3?(s.knobs[1]>.7?2:4):s.knobs[1]>.85?1.5:s.knobs[1]>.6?3:6;
            for(double t=start;t<end;t+=step)
            {
                if(style!=1&&style!=3&&unit(seed,static_cast<std::uint64_t>(t*2)+885)<.16)continue;
                const double progress=(t-start)/std::max(1.,end-start);add(b*24+t,step*.9,1,.18+.82*progress,true,false);
            }
        }
    }
    std::stable_sort(plan.events.begin(),plan.events.end(),[](const auto& a,const auto& b){return a.at<b.at;});
    std::vector<size_t> body;for(size_t i=0;i<plan.events.size();++i)if(plan.events[i].role!=2)body.push_back(i);
    if((style==5||style==8||style==11)&&body.size()>=2)
    {
        for(int i=0;i<2;++i)plan.events[body[body.size()-2+static_cast<size_t>(i)]].rate=std::pow(2.,-(2.+i)/12);
        if(style==5)for(size_t j=0;j<body.size();++j)plan.events[body[j]].rate=std::pow(2.,(-1.-6.*j/std::max<size_t>(1,body.size()-1))/12);
    }
    if(style==7&&s.knobs[2]>.05)
    {add(s.fillBeats*24-6,6,1,.8*s.knobs[2],true,true);std::stable_sort(plan.events.begin(),plan.events.end(),[](const auto& a,const auto& b){return a.at<b.at;});}
    // Selected-hit room send is independent of the dry slice path. Prefer an
    // earlier body accent so the final hit can leave a meaningful empty pickup.
    body.clear();for(size_t i=0;i<plan.events.size();++i)if(plan.events[i].role!=2)body.push_back(i);
    std::vector<size_t> accents;
    for(size_t i:body)if(!plan.events[i].roll&&plan.events[i].velocity>=.65&&plan.events[i].at<=length-beat*.5)accents.push_back(i);
    if(!accents.empty())plan.events[accents.back()].space=1;
    else if(!body.empty())plan.events[body.front()].space=1;
    const double stop=s.knobs[3];if(stop>.02)plan.stopSample=length-static_cast<int>(beat*(.04+.38*stop)*stop);
    return plan;
}

juce::AudioBuffer<float> renderSlicedFill(const juce::AudioBuffer<float>& input,const Settings& original)
{
    Settings s=original;if(!std::isfinite(s.sampleRate)||!std::isfinite(s.bpm)||s.sampleRate<8000||s.sampleRate>192000)return {};
    s.kind=Fill;s.bpm=std::clamp(s.bpm,20.,999.);s.bars=s.bars==1||s.bars==4?s.bars:2;s.fillBeats=s.fillBeats==1||s.fillBeats==2?s.fillBeats:4;s.fillFx=std::isfinite(s.fillFx)?std::clamp(s.fillFx,0.,1.):0.;for(auto& k:s.knobs)k=std::isfinite(k)?std::clamp(k,0.,1.):0.;
    const int length=lengthSamples(s);juce::AudioBuffer<float> source(2,length);source.clear();
    if(input.getNumChannels()>0)for(int c=0;c<2;++c)for(int n=0;n<std::min(length,input.getNumSamples());++n){const float v=input.getSample(std::min(c,input.getNumChannels()-1),n);source.setSample(c,n,std::isfinite(v)?v:0.f);}
    const auto plan=makeSlicedFillPlan(source,s);juce::AudioBuffer<float> out(source),sends(2,length);sends.clear();
    if(!plan.events.empty())
    {
        for(int c=0;c<2;++c)out.clear(c,plan.phraseStart,length-plan.phraseStart);
        for(const auto& event:plan.events)grain(source,out,sends,event,s.sampleRate);
        effects(out,sends,plan,s);
        if(plan.stopSample>=plan.phraseStart){const int fade=std::max(1,static_cast<int>(s.sampleRate*.004));for(int c=0;c<2;++c){for(int i=std::max(plan.phraseStart,plan.stopSample-fade);i<plan.stopSample;++i)out.setSample(c,i,out.getSample(c,i)*static_cast<float>(cosine(static_cast<double>(plan.stopSample-i)/fade)));out.clear(c,plan.stopSample,length-plan.stopSample);}}
    }
    const int ending=std::min(length,static_cast<int>(s.sampleRate*.005));for(int i=0;i<ending;++i)for(int c=0;c<2;++c)out.setSample(c,length-1-i,out.getSample(c,length-1-i)*static_cast<float>(cosine(static_cast<double>(i)/std::max(1,ending))));
    // A hot untouched input may require common protection. Normally prepared
    // sources retain their pre-phrase PCM; only the generated ending is scaled.
    const double ceiling=std::pow(10.,-1.8/20);Sinc meter(1,1);double before=0,tail=0;
    for(int c=0;c<2;++c)for(int n=0;n<length;++n)
    {double peak=std::abs(static_cast<double>(out.getSample(c,n)));for(int p=1;p<8;++p)peak=std::max(peak,std::abs(meter.read(out,c,n+p*.125)));if(n<plan.phraseStart)before=std::max(before,peak);else tail=std::max(tail,peak);}
    // The short source crossfade also needs protection when the whole capture
    // is replaced (phraseStart == 0). The first run measured only rendered audio
    // and let a hot original leak into those first 3 ms; keep that failed run.
    if(!plan.events.empty())for(int c=0;c<2;++c)for(int n=plan.phraseStart;n<std::min(length,plan.phraseStart+static_cast<int>(s.sampleRate*.003));++n)
    {before=std::max(before,std::abs(static_cast<double>(source.getSample(c,n))));for(int p=1;p<8;++p)before=std::max(before,std::abs(meter.read(source,c,n+p*.125)));}
    double common=1;
    if(before>ceiling){common=ceiling/std::max(before,tail);out.applyGain(static_cast<float>(common));}
    else if(tail>ceiling)out.applyGain(plan.phraseStart,length-plan.phraseStart,static_cast<float>(ceiling/tail));
    if(!plan.events.empty())
    {const int cross=std::min(length-plan.phraseStart,static_cast<int>(s.sampleRate*.003));for(int i=0;i<cross;++i){const double blend=cosine(static_cast<double>(i)/std::max(1,cross-1));for(int c=0;c<2;++c)out.setSample(c,plan.phraseStart+i,static_cast<float>((1-blend)*common*source.getSample(c,plan.phraseStart+i)+blend*out.getSample(c,plan.phraseStart+i)));}}
    return out;
}
}

#include "DSP/TransitionRender.h"
#include "DSP/DrumFillGrammarData.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <set>

namespace hpeq::dsp::transition
{
namespace
{
constexpr double pi=3.14159265358979323846;
std::uint64_t hash(std::uint64_t z)noexcept
{z+=0x9e3779b97f4a7c15ull;z=(z^(z>>30))*0xbf58476d1ce4e5b9ull;z=(z^(z>>27))*0x94d049bb133111ebull;return z^(z>>31);}
double random(std::uint64_t seed,std::uint64_t salt)noexcept{return static_cast<double>(hash(seed^hash(salt))>>11)/9007199254740992.;}
double fade(double x)noexcept{return .5-.5*std::cos(pi*std::clamp(x,0.,1.));}
bool sanitize(Settings& s)
{
    if(!std::isfinite(s.sampleRate)||s.sampleRate<8000||s.sampleRate>192000||!std::isfinite(s.bpm))return false;
    s.kind=Fill;s.bpm=std::clamp(s.bpm,20.,999.);s.bars=s.bars==1||s.bars==4?s.bars:2;s.fillBeats=s.fillBeats==1||s.fillBeats==2?s.fillBeats:4;
    s.fillFx=std::isfinite(s.fillFx)?std::clamp(s.fillFx,0.,1.):0.;for(auto& k:s.knobs)k=std::isfinite(k)?std::clamp(k,0.,1.):0.;return true;
}
juce::AudioBuffer<float> sourceBuffer(const juce::AudioBuffer<float>& input,const Settings& s)
{
    juce::AudioBuffer<float> source(2,lengthSamples(s));source.clear();
    if(input.getNumChannels()>0)for(int c=0;c<2;++c)for(int n=0;n<std::min(input.getNumSamples(),source.getNumSamples());++n)
    {const float v=input.getSample(std::min(c,input.getNumChannels()-1),n);source.setSample(c,n,std::isfinite(v)?v:0.f);}return source;
}
struct Biquad
{
    double a1=0,a2=0,b0=1,b1=0,b2=0,z1=0,z2=0;
    Biquad(double fs,double hz,double q,bool high)
    {
        const double w=2*pi*std::clamp(hz,10.,fs*.44)/fs,c=std::cos(w),alpha=std::sin(w)/(2*q),a0=1+alpha;
        b0=(high?1+c:1-c)*.5/a0;b1=(high?-(1+c):1-c)/a0;b2=b0;a1=-2*c/a0;a2=(1-alpha)/a0;
    }
    double tick(double x){const double y=b0*x+z1;z1=b1*x-a1*y+z2;z2=b2*x-a2*y;return y;}
};
struct Reader
{
    static constexpr int taps=24,phases=128;double table[phases][taps]{};
    explicit Reader(double rate=1,double bandwidth=.94)
    {
        const double fc=bandwidth/std::max(1.,rate);
        for(int p=0;p<phases;++p){double sum=0;for(int k=0;k<taps;++k){const double d=k-11.-static_cast<double>(p)/phases,t=pi*d*fc;const double v=fc*(std::abs(t)<1e-12?1:std::sin(t)/t)*(.5+.5*std::cos(pi*d/12));table[p][k]=v;sum+=v;}for(auto& v:table[p])v/=sum;}
    }
    double read(const juce::AudioBuffer<float>& x,int c,double position,int start=0,int end=std::numeric_limits<int>::max())const
    {
        const int n=static_cast<int>(std::floor(position)),p=std::clamp(static_cast<int>((position-n)*phases),0,phases-1);double y=0;
        for(int k=0;k<taps;++k){const int i=n+k-11;if(i>=start&&i<end&&i>=0&&i<x.getNumSamples())y+=table[p][k]*x.getSample(c,i);}return y;
    }
};
struct Kit
{
    std::array<juce::AudioBuffer<float>,3> bands;
    std::vector<Slice> hits;
    std::array<std::vector<int>,3> pools;
};
Kit makeKit(const juce::AudioBuffer<float>& source,const Settings& s)
{
    Kit kit;kit.bands=extractDrumBands(source,s.sampleRate);
    const int hop=std::max(1,static_cast<int>(s.sampleRate*.002));const double rawPeak=source.getMagnitude(0,source.getNumSamples());
    for(int role=0;role<3;++role)
    {
        const auto& band=kit.bands[static_cast<size_t>(role)];const int frames=(band.getNumSamples()+hop-1)/hop;
        std::vector<double> env(static_cast<size_t>(frames)),novelty(static_cast<size_t>(frames));double maximum=0;
        for(int f=0;f<frames;++f){double energy=0;for(int c=0;c<2;++c)for(int n=f*hop;n<std::min((f+1)*hop,band.getNumSamples());++n)energy+=band.getSample(c,n)*static_cast<double>(band.getSample(c,n));env[static_cast<size_t>(f)]=std::sqrt(energy/(2*hop));}
        for(int f=0;f<frames;++f){double previous=0;for(int j=1;j<=6;++j)if(f>=j)previous+=env[static_cast<size_t>(f-j)]/6;novelty[static_cast<size_t>(f)]=std::max(0.,env[static_cast<size_t>(f)]-previous);maximum=std::max(maximum,novelty[static_cast<size_t>(f)]);}
        std::vector<int> onsets;const double spacing=role==0?.055:role==1?.038:.021;
        for(int f=0;f<frames;++f)
        {
            const double value=novelty[static_cast<size_t>(f)];if(value<maximum*.16||value<rawPeak*.001||value<1e-10)continue;
            if((f&&value<novelty[static_cast<size_t>(f-1)])||(f+1<frames&&value<novelty[static_cast<size_t>(f+1)]))continue;
            const int at=std::max(0,f*hop-2*hop);if(!onsets.empty()&&at-onsets.back()<spacing*s.sampleRate)continue;onsets.push_back(at);
        }
        for(size_t j=0;j<onsets.size();++j)
        {
            const int at=onsets[j],next=j+1<onsets.size()?onsets[j+1]:band.getNumSamples();const double duration=role==0?.19:role==1?.17:.065;
            Slice hit;hit.sample=at;hit.end=std::min(next,std::min(band.getNumSamples(),at+static_cast<int>(s.sampleRate*duration)));hit.role=role;
            for(int c=0;c<2;++c)for(int n=at;n<hit.end;++n)hit.energy+=band.getSample(c,n)*static_cast<double>(band.getSample(c,n));
            const double peak=band.getMagnitude(at,hit.end-at);if(peak<rawPeak*.018||hit.energy<1e-12)continue;
            kit.pools[static_cast<size_t>(role)].push_back(static_cast<int>(kit.hits.size()));kit.hits.push_back(hit);
        }
    }
    return kit;
}
using Mask=std::array<std::array<double,96>,3>;
void onset(Mask& mask,int role,double position,double sigma)
{
    for(int n=std::max(0,static_cast<int>(position-4*sigma));n<std::min(96,static_cast<int>(position+4*sigma)+1);++n)
    {const double d=(n-position)/sigma;mask[role][n]+=std::exp(-.5*d*d);}
}
double similarity(const Mask& a,const Mask& b)
{double dot=0,aa=0,bb=0;for(int r=0;r<3;++r)for(int n=0;n<96;++n){dot+=a[r][n]*b[r][n];aa+=a[r][n]*a[r][n];bb+=b[r][n]*b[r][n];}return dot/std::sqrt(std::max(1e-30,aa*bb));}
double sharedSimilarity(const Mask& a,const Mask& b)
{double dot=0,aa=0,bb=0;for(int n=0;n<96;++n){const double av=a[0][n]+a[1][n]+a[2][n],bv=b[0][n]+b[1][n]+b[2][n];dot+=av*bv;aa+=av*av;bb+=bv*bv;}return dot/std::sqrt(std::max(1e-30,aa*bb));}
Plan compose(const Kit& kit,const Settings& s)
{
    const int length=lengthSamples(s),ticks=s.fillBeats*24;const double beat=s.sampleRate*60/s.bpm;
    Plan base;base.recipe=resolvedRecipe(s);base.slices=kit.hits;base.attacks=static_cast<int>(kit.hits.size());base.phraseStart=length-static_cast<int>(std::lround(beat*s.fillBeats));base.phraseEnd=length;
    for(int r=0;r<3;++r)base.kitCounts[static_cast<size_t>(r)]=static_cast<int>(kit.pools[static_cast<size_t>(r)].size());
    if(std::all_of(s.knobs.begin(),s.knobs.end(),[](double x){return x==0;}))return base;
    const int style=base.recipe-22;Mask original{};const double sigma=std::max(.4,.020*s.sampleRate*96/(length-base.phraseStart));
    // First candidate scoring used hard32cells. A source onset just before a
    // cell edge looked different from a grid event a few ms later,while actual
    // audio remained similar. Continuous20ms tolerance fixes that false novelty.
    for(const auto& hit:kit.hits)if(hit.sample>=base.phraseStart)onset(original,hit.role,static_cast<double>(hit.sample-base.phraseStart)*96/(length-base.phraseStart),sigma);
    static constexpr double weights[8][3]{{.7,1.35,.75},{1.2,.95,.65},{.65,.7,1.6},{.7,1.35,.7},{.85,1.15,1.0},{.85,.7,.65},{1.05,1.1,1.15},{.6,1.2,.8}};
    Plan best=base;double bestScore=-1e30;
    for(int candidate=0;candidate<32;++candidate)
    {
        Plan p=base;const auto seed=hash(s.seed+static_cast<std::uint64_t>(candidate)*0x9e3779b97f4a7c15ull);Mask mask{};
        const auto select=[&](int role,std::uint64_t salt,bool anchor)
        {
            const auto& pool=kit.pools[static_cast<size_t>(role)];double strongest=0,total=0;
            for(int i:pool)strongest=std::max(strongest,kit.hits[static_cast<size_t>(i)].energy);
            // A coherent snare roll must anchor a strong body transient,not a
            // quiet mid-band residue of a hat. Ghost one-shots can still vary.
            const double floor=anchor?strongest*.2:0.;int fallback=pool.front();
            for(int i:pool)if(kit.hits[static_cast<size_t>(i)].energy>=floor){total+=std::sqrt(kit.hits[static_cast<size_t>(i)].energy);fallback=i;}
            double target=random(seed,anchor?0x534e415245ull:salt)*total;
            for(int i:pool)if(kit.hits[static_cast<size_t>(i)].energy>=floor){target-=std::sqrt(kit.hits[static_cast<size_t>(i)].energy);if(target<=0)return i;}return fallback;
        };
        int serial=0;
        const auto add=[&](double tick,int role,double velocity,bool roll,bool reverse=false)
        {
            const auto& pool=kit.pools[static_cast<size_t>(role)];if(pool.empty()||tick>=ticks)return;
            const int index=select(role,static_cast<std::uint64_t>(serial)+93,roll);const auto& hit=kit.hits[static_cast<size_t>(index)];Event e;
            e.at=p.phraseStart+static_cast<int>(std::lround(tick*beat/24));e.source=hit.sample;e.sourceEnd=hit.end;e.role=role;e.roll=roll;e.reverse=reverse;e.velocity=velocity;
            e.length=static_cast<int>(std::min((role==0?.18:role==1?.14:.055)*s.sampleRate,(roll?beat*(s.knobs[1]>.85?.055:.11):beat*.42)));
            e.pan=role==0?0:(random(seed,static_cast<std::uint64_t>(serial)+733)-.5)*(role==2?.65:.22);e.rate=1;
            if(role==1&&(style==1||style==7)&&tick>ticks-12)e.rate=std::pow(2.,(-2-5*(tick-(ticks-12))/12)/12);
            p.events.push_back(e);++serial;
        };
        for(int role=0;role<3;++role)
        {
            if(kit.pools[static_cast<size_t>(role)].empty())continue;
            const double density=role==0?.25+.45*s.knobs[0]:role==1?.50+.95*s.knobs[1]:.5+.9*s.knobs[0];
            const double count=grammar::medianCounts[static_cast<size_t>(role)]*s.fillBeats/4.*density*weights[style][role]*(.85+.3*random(seed,static_cast<std::uint64_t>(role)+17));
            const int wanted=std::clamp(static_cast<int>(std::lround(count)),std::max(1,s.fillBeats/2),role==0?s.fillBeats*3:role==1?s.fillBeats*6:s.fillBeats*7);
            const int grid=style==4?4:role==0?6:3;std::set<int> chosen;
            for(int hit=0;hit<wanted;++hit)
            {
                std::vector<std::pair<int,double>> positions;double total=0;
                for(int tick=0;tick<ticks;tick+=grid)
                {
                    if(chosen.count(tick))continue;
                    const double x=static_cast<double>(tick)/ticks;const int phase=std::min(15,tick*16/ticks),cell=std::min(95,tick*96/ticks);
                    const double build=.55+2*(1-grammar::lateFraction[static_cast<size_t>(role)]);
                    double w=grammar::phase[static_cast<size_t>(role)][static_cast<size_t>(phase)]*(.35+1.8*std::pow(x,build));
                    w*=std::max(.025,1-.97*original[role][cell]);
                    if(role==0&&tick>=ticks-12)w*=.18;
                    if(style==1)w*=((tick/6)%2==(role==0?0:1))?2.4:.25;
                    if(style==2&&role==2)w*=tick%6?2.8:.4;
                    if(style==3)w*=((tick/24)%2==(role==0?0:1))?3.:.14;
                    if(style==5)w*=role==1?(tick>=ticks/2&&tick<ticks/2+6?5.:.35):(tick%12? .45:2.);
                    if(style==6)w*=((tick/3* (role==0?3:role==1?5:7)+static_cast<int>(seed%11))%16)<5?3.:.3;
                    if(style==7)w*=x<.5?.15:std::pow(2*x,3.);
                    for(int previous:chosen)if(std::abs(tick-previous)<(role==0?12:6))w*=.3;
                    total+=w;positions.emplace_back(tick,w);
                }
                if(positions.empty())break;double target=random(seed,static_cast<std::uint64_t>(role*91+hit)+1103)*total;int tick=positions.back().first;
                for(const auto& position:positions){target-=position.second;if(target<=0){tick=position.first;break;}}
                chosen.insert(tick);const bool accent=role==0||tick%12==0||random(seed,static_cast<std::uint64_t>(tick+role*97)+36)>.55;
                const double velocity=accent?.85+.35*random(seed,static_cast<std::uint64_t>(serial)+41):.35+.35*random(seed,static_cast<std::uint64_t>(serial)+83);
                const bool reverse=role!=0&&tick>ticks-12&&random(seed,static_cast<std::uint64_t>(serial)+907)<s.knobs[2]*.7;
                add(tick,role,velocity,false,reverse);
            }
        }
        if(s.knobs[1]>.05&&!kit.pools[1].empty())
        {
            const double duration=(style==5?6:style==0||style==3||style==7?24:12)*(.55+.45*s.knobs[1]);const double start=ticks-duration;
            const double step=style==4?(s.knobs[1]>.8?2.:4.):s.knobs[1]>.85?1.5:s.knobs[1]>.45?3.:6.;
            for(double tick=start;tick<ticks-1;tick+=step)
            {
                const int at=p.phraseStart+static_cast<int>(std::lround(tick*beat/24));
                p.events.erase(std::remove_if(p.events.begin(),p.events.end(),[&](const auto& e){return e.role==1&&std::abs(e.at-at)<beat*.04;}),p.events.end());
                add(tick,1,.35+.9*(tick-start)/std::max(1.,duration),true);
            }
        }
        std::stable_sort(p.events.begin(),p.events.end(),[](const auto& a,const auto& b){return a.at<b.at;});
        mask={};for(const auto& e:p.events)onset(mask,e.role,static_cast<double>(e.at-p.phraseStart)*96/(length-p.phraseStart),sigma);
        p.inputSimilarity=similarity(mask,original);double early=0,late=0,collisions=0;
        for(const auto& e:p.events)(e.at<p.phraseStart+(length-p.phraseStart)/2?early:late)+=e.velocity*e.velocity;
        for(int n=0;n<96;++n)if(mask[0][n]+mask[1][n]+mask[2][n]>=3)++collisions;
        const double score=(p.inputSimilarity<=.65?10.:0.)+3*(1-p.inputSimilarity)+1.5*(1-sharedSimilarity(mask,original))+.6*(late-early)/std::max(1.,late+early)-.012*collisions;
        if(score>bestScore){bestScore=score;best=std::move(p);}
    }
    // Kit switches mute parts of the same chosen composition;they never redraw
    // a different rhythm merely because the user is auditioning a solo role.
    best.events.erase(std::remove_if(best.events.begin(),best.events.end(),[&](const auto& e){return !s.kitEnabled[static_cast<size_t>(e.role)];}),best.events.end());
    if(s.knobs[3]>.02)best.stopSample=length-static_cast<int>(beat*s.knobs[3]*(.06+.32*s.knobs[3]));
    for(auto i=best.events.rbegin();i!=best.events.rend();++i)if(i->role==1&&!i->roll&&i->at<length-beat*.5){i->space=1;break;}
    return best;
}
void renderHit(const Kit& kit,juce::AudioBuffer<float>& out,juce::AudioBuffer<float>& sends,const Event& e,const Settings& s,double rawPeak)
{
    const auto& source=kit.bands[static_cast<size_t>(e.role)];const int count=std::min({e.length,out.getNumSamples()-e.at,static_cast<int>((e.sourceEnd-e.source-1)/e.rate)});if(count<3)return;
    const double peak=source.getMagnitude(e.source,e.sourceEnd-e.source),target=std::min(.7,rawPeak*(e.role==2?.48:e.role==1?1.0:.9));const double gain=std::clamp(target/std::max(1e-12,peak),.2,3.);
    Reader reader(e.rate);const double attack=s.sampleRate*(e.reverse?.0015:.00015),release=std::min(count*.2,s.sampleRate*.0025);
    for(int i=0;i<count;++i)
    {
        const double at=e.source+(e.reverse?count-1-i:i)*e.rate,envelope=fade((i+1)/(attack+1))*fade((count-i-1)/(release+1));
        for(int c=0;c<2;++c){const double pan=c==0?std::min(1.,1-e.pan):std::min(1.,1+e.pan);const float value=static_cast<float>(reader.read(source,c,at,e.source,e.sourceEnd)*envelope*gain*e.velocity*pan);out.addSample(c,e.at+i,value);if(e.space>0)sends.addSample(c,e.at+i,value);}
    }
}
void room(juce::AudioBuffer<float>& out,const juce::AudioBuffer<float>& send,const Plan& p,const Settings& s)
{
    if(s.fillFx<=0)return;constexpr double times[]{.0233,.0307,.0371,.0437};std::array<std::array<std::vector<double>,4>,2> lines;int index[4]{},length[4]{};double lp[2][4]{},hp[2]{};
    for(int k=0;k<4;++k){length[k]=static_cast<int>(s.sampleRate*times[k]);for(int c=0;c<2;++c)lines[c][k].assign(static_cast<size_t>(length[k]),0);}
    const double damping=1-std::exp(-2*pi*4800/s.sampleRate);
    for(int n=p.phraseStart;n<out.getNumSamples();++n)
    {
        const double x=static_cast<double>(n-p.phraseStart)/std::max(1,out.getNumSamples()-p.phraseStart),k=1-std::exp(-2*pi*(25+700*s.fillFx*x*x)/s.sampleRate);
        for(int c=0;c<2;++c){double wet=0;for(int j=0;j<4;++j){const double v=lines[c][j][static_cast<size_t>(index[j])];lp[c][j]+=damping*(v-lp[c][j]);lines[c][j][static_cast<size_t>(index[j])]=send.getSample(c,n)+(.8+.1*s.fillFx)*lp[c][j];wet+=(c==1&&j%2?-.7:1.)*v*.25;}const double dry=out.getSample(c,n);hp[c]+=k*(dry-hp[c]);out.setSample(c,n,static_cast<float>(dry-s.fillFx*hp[c]+wet*s.fillFx));}
        for(int j=0;j<4;++j)if(++index[j]>=length[j])index[j]=0;
    }
}
}

std::array<juce::AudioBuffer<float>,3> extractDrumBands(const juce::AudioBuffer<float>& input,double fs)
{
    std::array<juce::AudioBuffer<float>,3> result;if(!std::isfinite(fs)||fs<8000||fs>192000)return result;
    constexpr double lows[]{30,260,5500},highs[]{260,5000,20000},q[]{.541196100146197,1.306562964876377};
    for(int r=0;r<3;++r)
    {
        auto& band=result[static_cast<size_t>(r)];band.setSize(2,input.getNumSamples());band.clear();if(input.getNumChannels()==0)continue;
        for(int c=0;c<2;++c){Biquad h1(fs,lows[r],q[0],true),h2(fs,lows[r],q[1],true),l1(fs,highs[r],q[0],false),l2(fs,highs[r],q[1],false);
            for(int n=0;n<input.getNumSamples();++n){const double v=input.getSample(std::min(c,input.getNumChannels()-1),n);band.setSample(c,n,static_cast<float>(l2.tick(l1.tick(h2.tick(h1.tick(std::isfinite(v)?v:0.))))));}}
    }
    return result;
}
Plan makeComposedFillPlan(const juce::AudioBuffer<float>& input,const Settings& settings)
{Settings s=settings;if(!sanitize(s))return {};const auto source=sourceBuffer(input,s);return compose(makeKit(source,s),s);}
juce::AudioBuffer<float> renderComposedFill(const juce::AudioBuffer<float>& input,const Settings& settings,Plan* info)
{
    Settings s=settings;if(!sanitize(s))return {};const auto source=sourceBuffer(input,s);const auto kit=makeKit(source,s);const auto p=compose(kit,s);if(info)*info=p;
    const int length=source.getNumSamples();juce::AudioBuffer<float> out(source),send(2,length);send.clear();const bool neutral=std::all_of(s.knobs.begin(),s.knobs.end(),[](double v){return v==0;});
    if(!neutral){for(int c=0;c<2;++c)out.clear(c,p.phraseStart,length-p.phraseStart);const double rawPeak=source.getMagnitude(0,length);for(const auto& e:p.events)renderHit(kit,out,send,e,s,rawPeak);room(out,send,p,s);}
    if(!neutral&&p.stopSample>=p.phraseStart){const int count=std::max(1,static_cast<int>(s.sampleRate*.004));for(int c=0;c<2;++c){for(int n=std::max(p.phraseStart,p.stopSample-count);n<p.stopSample;++n)out.setSample(c,n,out.getSample(c,n)*static_cast<float>(fade(static_cast<double>(p.stopSample-n)/count)));out.clear(c,p.stopSample,length-p.stopSample);}}
    const int end=std::min(length,static_cast<int>(s.sampleRate*.005));for(int c=0;c<2;++c)for(int n=0;n<end;++n)out.setSample(c,length-1-n,out.getSample(c,length-1-n)*static_cast<float>(fade(static_cast<double>(n)/std::max(1,end))));
    // Continue the last prefix value through a2msboundary taper,not a copy of
    // the original full-band slice. This avoids a discontinuity at partial edits.
    if(!neutral)
    {
        const int count=std::min(length-p.phraseStart,static_cast<int>(s.sampleRate*.002));
        const bool active=!p.events.empty();
        for(int c=0;c<2;++c)for(int n=0;n<count;++n){const double blend=fade(static_cast<double>(n)/std::max(1,count-1)),previous=p.phraseStart>0&&active?source.getSample(c,p.phraseStart-1):0.;out.setSample(c,p.phraseStart+n,static_cast<float>((1-blend)*previous+blend*out.getSample(c,p.phraseStart+n)));}
    }
    // Peak protection is common only if an already hot original prefix needs it.
    const double ceiling=std::pow(10.,-1.8/20);Reader meter(1,1);double prefix=0,tail=0;
    for(int c=0;c<2;++c)for(int n=0;n<length;++n){double peak=std::abs(static_cast<double>(out.getSample(c,n)));for(int k=1;k<8;++k)peak=std::max(peak,std::abs(meter.read(out,c,n+k*.125)));if(n<p.phraseStart)prefix=std::max(prefix,peak);else tail=std::max(tail,peak);}
    if(prefix>ceiling)out.applyGain(static_cast<float>(ceiling/std::max(prefix,tail)));else if(tail>ceiling)out.applyGain(p.phraseStart,length-p.phraseStart,static_cast<float>(ceiling/tail));
    return out;
}
}

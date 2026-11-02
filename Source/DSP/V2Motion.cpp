#include "DSP/V2Motion.h"
#include "S4Identity.h"
#include <juce_cryptography/juce_cryptography.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <map>
#include <mutex>
#include "ShirotaHrtf.h"
#if defined(_M_X64) || defined(__SSE2__)
#include <emmintrin.h>
#endif
#include <stdexcept>
#include <vector>

namespace hamlo::shirota::v2
{
namespace
{
constexpr double pi=3.14159265358979323846,rootHalf=.7071067811865475244;
double valid(double x,double lo,double hi,double fallback) noexcept{return std::isfinite(x)?std::clamp(x,lo,hi):fallback;}
double wrap(double x) noexcept{x=std::fmod(x,360.);return x<0?x+360:x;}
double arc(double from,double to,MotionArc how) noexcept
{
    const double positive=wrap(to-from);
    if(how==MotionArc::Counterclockwise)return positive==0?360:positive;
    if(how==MotionArc::Clockwise)return positive==0?-360:positive-360;
    return positive>180?positive-360:positive;
}
struct Ramp
{
    double value=0,target=0,step=0;int remaining=0;
    void set(double x,int n) noexcept {if(x==target)return;target=x;step=(target-value)/n;remaining=n;}
    void snap() noexcept{value=target;remaining=0;}
    double next() noexcept{if(remaining>0)value=--remaining==0?target:value+step;return value;}
};
struct PreparedBank {int n=512,d=56;std::vector<float> samples;};
std::shared_ptr<const PreparedBank> preparedBank(double fs)
{
    static std::mutex mutex;static std::map<double,std::shared_ptr<const PreparedBank>> cache;
    const std::lock_guard<std::mutex> lock(mutex);if(auto found=cache.find(fs);found!=cache.end())return found->second;
    static const bool verified=[](){if(juce::SHA256(ShirotaHrtf::production_bin,static_cast<size_t>(ShirotaHrtf::production_binSize)).toHexString().toStdString()!=SHIROTA_S4_BANK_SHA256)throw std::runtime_error("Stale embedded HRIR bank");return true;}();(void)verified;
    constexpr int base=512,filters=3*72*2;std::vector<float> source(static_cast<size_t>(base*filters));
    if(ShirotaHrtf::production_binSize!=base*filters*4)throw std::runtime_error("Invalid embedded production HRIR");
    std::memcpy(source.data(),ShirotaHrtf::production_bin,source.size()*4);
    auto bank=std::make_shared<PreparedBank>();const double ratio=fs/48000.;
    const int spatial=2*static_cast<int>(std::lround(32*ratio));bank->d=static_cast<int>(std::lround(.0025*fs))-spatial;
    if(fs==48000){bank->samples=std::move(source);}
    else
    {
        const double shift=bank->d-56*ratio,cutoff=std::min(1.,ratio),radius=32/cutoff;
        bank->n=4*static_cast<int>(std::ceil((base*ratio+std::max(0.,shift)+32*ratio)/4));bank->samples.resize(static_cast<size_t>(filters*bank->n));
        for(int k=0;k<bank->n;++k)
        {
            const double at=(k-shift)/ratio;const int first=std::max(0,static_cast<int>(std::ceil(at-radius))),last=std::min(base-1,static_cast<int>(std::floor(at+radius)));
            for(int j=first;j<=last;++j)
            {
                const double x=at-j,u=x/radius,window=.42+.5*std::cos(pi*u)+.08*std::cos(2*pi*u);
                const double sinc=std::abs(x)<1e-12?cutoff:std::sin(pi*cutoff*x)/(pi*x);const double gain=sinc*window/ratio;
                for(int f=0;f<filters;++f)bank->samples[static_cast<size_t>(f*bank->n+k)]+=static_cast<float>(source[static_cast<size_t>(f*base+j)]*gain);
            }
        }
    }
    if(cache.size()>=12)cache.erase(cache.begin());cache.emplace(fs,bank);return bank;
}
std::array<double,2> pan(double angle) noexcept
{const double s=std::clamp(std::sin(angle*pi/180),-1.,1.);return {std::sqrt((1+s)*.5),std::sqrt((1-s)*.5)};}
}
void MotionClock::prepare(double rate) noexcept{fs=valid(rate,8000,384000,48000);reset();}
void MotionClock::setTempo(double tempo) noexcept{bpm=valid(tempo,1,999,120);}
void MotionClock::setRate(double rate) noexcept{speed=valid(rate,.25,4,1);}
void MotionClock::reset() noexcept{phase=0;node=0;travelling=false;finished=false;}
void MotionClock::setPath(const MotionPath& requested) noexcept
{
    path=requested;path.count=std::clamp(path.count,1,16);double duration=0;
    for(int i=0;i<path.count;++i)
    {
        auto& n=path.nodes[static_cast<size_t>(i)];n.azimuth=wrap(valid(n.azimuth,-1000000,1000000,0));
        const double maximum=path.unit==MotionUnit::Beats?4096:3600000;
        n.hold=valid(n.hold,0,maximum,0);n.travel=valid(n.travel,0,maximum,0);duration+=n.hold+n.travel;
    }stationary=duration==0;reset();
}
MotionPosition MotionClock::next() noexcept
{
    // Retain fractional phase through every segment instead of rounding durations.
    if(!stationary&&!finished)for(int skip=0;skip<34;++skip)
    {
        const auto& current=path.nodes[static_cast<size_t>(node)];const double length=travelling?current.travel:current.hold;
        if(phase+1e-12<length)break;
        phase=std::max(0.,phase-length);
        if(!travelling)
        {if(!path.loop&&node==path.count-1){finished=true;phase=0;break;}travelling=true;}
        else {travelling=false;node=(node+1)%path.count;}
    }
    const auto& current=path.nodes[static_cast<size_t>(node)];MotionPosition position;position.node=node;position.azimuth=current.azimuth;
    if(travelling&&!finished&&!stationary)
    {
        const double u=current.travel>0?std::clamp(phase/current.travel,0.,1.):0;
        const double eased=current.curve==MotionCurve::Smooth?u*u*(3-2*u):u;
        const double nextAngle=path.nodes[static_cast<size_t>((node+1)%path.count)].azimuth;
        position.azimuth=wrap(current.azimuth+arc(current.azimuth,nextAngle,current.arc)*eased);position.travelling=true;position.progress=u;
    }
    if(!finished&&!stationary)phase+=speed*(path.unit==MotionUnit::Beats?bpm/(60*fs):1000/fs);
    return position;
}
struct MotionProcessor::Impl
{
    double fs=48000;int n=256,d=128,pos=0,rampSamples=960,quantum=0;
    bool started=false,usePath=false;
    MotionClock clock;
    std::shared_ptr<const PreparedBank> bank;
    std::vector<float> historyLeft,historyRight;
    MotionPosition latest;double requestedCenter=0;
    std::vector<std::array<float,4>> coefficients,increments,targets;
    std::array<double,4> speaker{},speakerSteps{},speakerTargets{};
    Ramp enabled,mode,center,spread,balance,point;
    std::array<Ramp,3> profiles;
    void load(double rate)
    {
        fs=valid(rate,8000,384000,48000);rampSamples=static_cast<int>(std::ceil(.02*fs));clock.prepare(fs);
        bank=preparedBank(fs);n=bank->n;d=bank->d;
        historyLeft.assign(static_cast<size_t>(n*2),0);historyRight=historyLeft;
        coefficients.resize(static_cast<size_t>(n));increments=coefficients;targets=coefficients;
        profiles[0].value=profiles[0].target=1;spread.value=spread.target=180;
    }
    void settings(const MotionSettings& p) noexcept
    {
        enabled.set(p.enabled?1:0,rampSamples);mode.set(p.renderer==MotionRenderer::Binaural?1:0,rampSamples);
        const double wanted=wrap(valid(p.center,-1000000,1000000,0));if(wanted!=requestedCenter){requestedCenter=wanted;center.set(center.value+arc(wrap(center.value),wanted,MotionArc::Shortest),rampSamples);}
        spread.set(valid(p.spread,0,180,180),rampSamples);balance.set(valid(p.balance,-1,1,0),rampSamples);
        point.set(p.source==MotionSource::Point?1:0,rampSamples);usePath=p.usePath;
        for(int i=0;i<3;++i)profiles[static_cast<size_t>(i)].set(i==std::clamp(p.profile,0,2)?1:0,rampSamples);
        if(!started){for(auto* r:{&enabled,&mode,&center,&spread,&balance,&point})r->snap();for(auto& r:profiles)r.snap();}
    }
    struct Direction {int a,b;double f;};
    static Direction direction(double angle) noexcept {const double index=wrap(angle)/5;const int a=static_cast<int>(index);return {a,(a+1)%72,index-a};}
    double hrir(int profile,Direction dir,int ear,int tap) const noexcept
    {
        const int a=dir.a,b=dir.b;const double f=dir.f;
        const auto get=[&](int direction){return bank->samples[static_cast<size_t>(((profile*72+direction)*2+ear)*n+tap)];};
        return get(a)*(1-f)+get(b)*f;
    }
    void targetKernels(double angle,double extent,double pointAmount,const std::array<double,3>& weights,bool snap) noexcept
    {
        const auto a=pan(angle+extent*.5),b=pan(angle-extent*.5),p=pan(angle);
        const auto leftDir=direction(angle+extent*.5),rightDir=direction(angle-extent*.5),pointDir=direction(angle);
        for(int c=0;c<4;++c)
        {
            const int ear=c%2;
            speakerTargets[static_cast<size_t>(c)]=(1-pointAmount)*(c<2?a[static_cast<size_t>(ear)]:b[static_cast<size_t>(ear)])+pointAmount*rootHalf*p[static_cast<size_t>(ear)];
            if(snap)speaker[static_cast<size_t>(c)]=speakerTargets[static_cast<size_t>(c)];
            speakerSteps[static_cast<size_t>(c)]=(speakerTargets[static_cast<size_t>(c)]-speaker[static_cast<size_t>(c)])/16;
        }
        for(int k=0;k<n;++k)for(int c=0;c<4;++c)
        {
            double value=0;for(int pIndex=0;pIndex<3;++pIndex)if(weights[static_cast<size_t>(pIndex)]!=0)
            {
                const auto sourceDir=c<2?leftDir:rightDir;
                value+=weights[static_cast<size_t>(pIndex)]*((1-pointAmount)*rootHalf*hrir(pIndex,sourceDir,c%2,n-1-k)+pointAmount*.5*hrir(pIndex,pointDir,c%2,n-1-k));
            }
            auto& current=coefficients[static_cast<size_t>(k)][static_cast<size_t>(c)];const auto target=static_cast<float>(value);targets[static_cast<size_t>(k)][static_cast<size_t>(c)]=target;
            if(snap)current=target;increments[static_cast<size_t>(k)][static_cast<size_t>(c)]=(target-current)/16.f;
        }
    }
    void sample(float& left,float& right,MotionPosition* telemetry) noexcept
    {
        const bool first=!started;started=true;const float inL=std::isfinite(left)?left:0,inR=std::isfinite(right)?right:0;
        historyLeft[static_cast<size_t>(pos)]=historyLeft[static_cast<size_t>(pos+n)]=inL;
        historyRight[static_cast<size_t>(pos)]=historyRight[static_cast<size_t>(pos+n)]=inR;
        const int dryPos=(pos+n-d)%n;const float dryL=historyLeft[static_cast<size_t>(dryPos)],dryR=historyRight[static_cast<size_t>(dryPos)];
        const auto position=clock.next();latest=position;if(telemetry)*telemetry=position;
        const double e=enabled.next(),m=mode.next(),manual=center.next(),extent=spread.next(),bl=balance.next(),pt=point.next();
        latest.azimuth=usePath?position.azimuth:wrap(manual);
        std::array<double,3> weights{};for(int i=0;i<3;++i)weights[static_cast<size_t>(i)]=profiles[static_cast<size_t>(i)].next();
        if(quantum==0)targetKernels(usePath?position.azimuth:manual,extent,pt,weights,first);
        const float* hl=historyLeft.data()+pos+1;const float* hr=historyRight.data()+pos+1;
        double binauralL=0,binauralR=0;
       #if defined(_M_X64) || defined(__SSE2__)
        __m128 sums[4]{_mm_setzero_ps(),_mm_setzero_ps(),_mm_setzero_ps(),_mm_setzero_ps()};
        for(int k=0;k<n;k+=4)for(int lane=0;lane<4;++lane)
        {
            const auto index=static_cast<size_t>(k+lane);auto* c=coefficients[index].data();
            const auto updated=_mm_add_ps(_mm_loadu_ps(c),_mm_loadu_ps(increments[index].data()));_mm_storeu_ps(c,updated);
            sums[lane]=_mm_add_ps(sums[lane],_mm_mul_ps(updated,_mm_set_ps(hr[index],hr[index],hl[index],hl[index])));
        }
        std::array<float,4> sumsOut{};_mm_storeu_ps(sumsOut.data(),_mm_add_ps(_mm_add_ps(sums[0],sums[1]),_mm_add_ps(sums[2],sums[3])));
        binauralL=static_cast<double>(sumsOut[0])+sumsOut[2];binauralR=static_cast<double>(sumsOut[1])+sumsOut[3];
       #else
        for(int k=0;k<n;++k)
        {const auto index=static_cast<size_t>(k);auto& c=coefficients[index];for(int j=0;j<4;++j)c[static_cast<size_t>(j)]+=increments[index][static_cast<size_t>(j)];binauralL+=hl[index]*c[0]+hr[index]*c[2];binauralR+=hl[index]*c[1]+hr[index]*c[3];}
       #endif
        for(int c=0;c<4;++c)speaker[static_cast<size_t>(c)]+=speakerSteps[static_cast<size_t>(c)];
        const double stereoL=dryL*speaker[0]+dryR*speaker[2],stereoR=dryL*speaker[1]+dryR*speaker[3];
        const double processedL=(stereoL+m*(binauralL-stereoL))*std::min(1.,1-bl),processedR=(stereoR+m*(binauralR-stereoR))*std::min(1.,1+bl);
        if(e==0){left=dryL;right=dryR;}
        else {left=static_cast<float>(dryL+e*(processedL-dryL));right=static_cast<float>(dryR+e*(processedR-dryR));}
        if(++quantum==16)
        {quantum=0;for(int k=0;k<n;++k)coefficients[static_cast<size_t>(k)]=targets[static_cast<size_t>(k)];speaker=speakerTargets;}
        if(++pos==n)pos=0;
    }
};
MotionProcessor::MotionProcessor()=default;
MotionProcessor::~MotionProcessor()=default;
void MotionProcessor::prepare(double rate){impl=std::make_unique<Impl>();impl->load(rate);}
void MotionProcessor::setTargets(const MotionSettings& s) noexcept{impl->settings(s);}
void MotionProcessor::setPath(const MotionPath& p) noexcept{impl->clock.setPath(p);}
void MotionProcessor::setTempo(double bpm) noexcept{impl->clock.setTempo(bpm);}
void MotionProcessor::setRate(double rate) noexcept{impl->clock.setRate(rate);}
void MotionProcessor::resetPath() noexcept{impl->clock.reset();}
void MotionProcessor::process(float* l,float* r,int frames,MotionPosition* telemetry) noexcept{for(int i=0;i<frames;++i)impl->sample(l[i],r[i],telemetry?telemetry+i:nullptr);}
void MotionProcessor::clearHistory() noexcept {std::fill(impl->historyLeft.begin(),impl->historyLeft.end(),0);std::fill(impl->historyRight.begin(),impl->historyRight.end(),0);impl->pos=0;impl->quantum=0;impl->started=false;impl->clock.reset();for(auto* ramp:{&impl->enabled,&impl->mode,&impl->center,&impl->spread,&impl->balance,&impl->point})ramp->snap();for(auto& ramp:impl->profiles)ramp.snap();}
int MotionProcessor::latency() const noexcept{return impl->d;}
MotionPosition MotionProcessor::position() const noexcept{return impl->latest;}
bool MotionProcessor::implemented() noexcept{return true;}
const char* MotionProcessor::compiledIdentity() noexcept{return SHIROTA_S4_SOURCE_ID;}
}

#include "DSP/MotionPrototype.h"
#include "MotionIdentity.h"
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <bcrypt.h>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <stdexcept>
#include <vector>

namespace hamlo::shirota
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
std::string digest(const std::vector<unsigned char>& bytes)
{
    struct Algorithm {BCRYPT_ALG_HANDLE handle=nullptr;~Algorithm(){if(handle)BCryptCloseAlgorithmProvider(handle,0);}} algorithm;
    if(BCryptOpenAlgorithmProvider(&algorithm.handle,BCRYPT_SHA256_ALGORITHM,nullptr,0)<0)throw std::runtime_error("SHA256 provider failed");
    DWORD length=0,written=0;
    if(BCryptGetProperty(algorithm.handle,BCRYPT_OBJECT_LENGTH,reinterpret_cast<PUCHAR>(&length),sizeof(length),&written,0)<0)throw std::runtime_error("SHA256 object failed");
    std::vector<unsigned char> object(length);
    struct Hash {BCRYPT_HASH_HANDLE handle=nullptr;~Hash(){if(handle)BCryptDestroyHash(handle);}} hash;
    std::array<unsigned char,32> result{};
    if(BCryptCreateHash(algorithm.handle,&hash.handle,object.data(),length,nullptr,0,0)<0||
       BCryptHashData(hash.handle,const_cast<PUCHAR>(bytes.data()),static_cast<ULONG>(bytes.size()),0)<0||
       BCryptFinishHash(hash.handle,result.data(),static_cast<ULONG>(result.size()),0)<0)throw std::runtime_error("SHA256 bank verification failed");
    const char* hex="0123456789abcdef";std::string text;for(auto b:result){text+=hex[b>>4];text+=hex[b&15];}return text;
}
std::array<double,2> pan(double angle) noexcept
{const double s=std::clamp(std::sin(angle*pi/180),-1.,1.);return {std::sqrt((1+s)*.5),std::sqrt((1-s)*.5)};}
}
void MotionClock::prepare(double rate) noexcept{fs=valid(rate,8000,192000,48000);reset();}
void MotionClock::setTempo(double tempo) noexcept{bpm=valid(tempo,20,300,120);}
void MotionClock::reset() noexcept{phase=0;node=0;travelling=false;finished=false;}
void MotionClock::setPath(const MotionPath& requested) noexcept
{
    path=requested;path.count=std::clamp(path.count,1,16);double duration=0;
    for(int i=0;i<path.count;++i)
    {
        auto& n=path.nodes[static_cast<size_t>(i)];n.azimuth=wrap(valid(n.azimuth,-1000000,1000000,0));
        const double maximum=path.unit==MotionUnit::Beats?64:60000;
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
    if(!finished&&!stationary)phase+=path.unit==MotionUnit::Beats?bpm/(60*fs):1000/fs;
    return position;
}
struct MotionPrototype::Impl
{
    double fs=48000;int n=256,d=128,pos=0,rampSamples=960,quantum=0;
    bool started=false,usePath=false;
    MotionClock clock;
    std::vector<float> bank,historyLeft,historyRight;
    std::vector<std::array<float,4>> coefficients,increments,targets;
    std::array<double,4> speaker{},speakerSteps{},speakerTargets{};
    Ramp enabled,mode,center,spread,balance,point;
    std::array<Ramp,3> profiles;
    void load(double rate,const std::string& file)
    {
        if(rate!=44100&&rate!=48000&&rate!=96000)throw std::runtime_error("S3 prototype supports44.1/48/96kHz");
        fs=rate;rampSamples=static_cast<int>(std::ceil(.02*fs));clock.prepare(fs);
        std::ifstream source(file,std::ios::binary|std::ios::ate);if(!source)throw std::runtime_error("Cannot open HRTF bank");const auto bytes=source.tellg();
        if(bytes<20||bytes>4000000)throw std::runtime_error("Invalid HRTF bank size");
        std::vector<unsigned char> data(static_cast<size_t>(bytes));source.seekg(0);source.read(reinterpret_cast<char*>(data.data()),bytes);if(!source)throw std::runtime_error("Truncated HRTF bank");
        if(digest(data)!=SHIROTA_HRTF_SHA256)throw std::runtime_error("HRTF bank SHA256 mismatch");
        size_t offset=8;auto integer=[&](){uint32_t x=0;std::memcpy(&x,data.data()+offset,4);offset+=4;return x;};
        if(std::memcmp(data.data(),"HmHrS3A\0",8)!=0||integer()!=3||integer()!=3||integer()!=72)throw std::runtime_error("Invalid HRTF bank layout");
        for(int i=0;i<3;++i)
        {
            const auto hz=integer(),taps=integer();const size_t count=3*72*2*static_cast<size_t>(taps),length=count*4;
            if((taps!=256&&taps!=512)||offset+length>data.size())throw std::runtime_error("Invalid HRTF bank dimensions");
            if(hz==static_cast<uint32_t>(fs)){n=static_cast<int>(taps);bank.resize(count);std::memcpy(bank.data(),data.data()+offset,length);}
            offset+=length;
        }
        if(bank.empty()||offset!=data.size())throw std::runtime_error("HRTF rate missing");
        d=n/2;historyLeft.assign(static_cast<size_t>(n*2),0);historyRight=historyLeft;
        coefficients.resize(static_cast<size_t>(n));increments=coefficients;targets=coefficients;
        profiles[0].value=profiles[0].target=1;spread.value=spread.target=180;
    }
    void settings(const MotionSettings& p) noexcept
    {
        enabled.set(p.enabled?1:0,rampSamples);mode.set(p.renderer==MotionRenderer::Binaural?1:0,rampSamples);
        const double wanted=wrap(valid(p.center,-1000000,1000000,0));center.set(center.value+arc(wrap(center.value),wanted,MotionArc::Shortest),rampSamples);
        spread.set(valid(p.spread,0,180,180),rampSamples);balance.set(valid(p.balance,-1,1,0),rampSamples);
        point.set(p.source==MotionSource::Point?1:0,rampSamples);usePath=p.usePath;
        for(int i=0;i<3;++i)profiles[static_cast<size_t>(i)].set(i==std::clamp(p.profile,0,2)?1:0,rampSamples);
        if(!started){for(auto* r:{&enabled,&mode,&center,&spread,&balance,&point})r->snap();for(auto& r:profiles)r.snap();}
    }
    double hrir(int profile,double angle,int ear,int tap) const noexcept
    {
        const double index=wrap(angle)/5;const int a=static_cast<int>(index),b=(a+1)%72;const double f=index-a;
        const auto get=[&](int direction){return bank[static_cast<size_t>(((profile*72+direction)*2+ear)*n+tap)];};
        return get(a)*(1-f)+get(b)*f;
    }
    void targetKernels(double angle,double extent,double pointAmount,const std::array<double,3>& weights,bool snap) noexcept
    {
        const auto a=pan(angle+extent*.5),b=pan(angle-extent*.5),p=pan(angle);
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
                const double sourceAngle=angle+(c<2?extent*.5:-extent*.5);
                value+=weights[static_cast<size_t>(pIndex)]*((1-pointAmount)*rootHalf*hrir(pIndex,sourceAngle,c%2,n-1-k)+pointAmount*.5*hrir(pIndex,angle,c%2,n-1-k));
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
        const auto position=clock.next();if(telemetry)*telemetry=position;
        const double e=enabled.next(),m=mode.next(),manual=center.next(),extent=spread.next(),bl=balance.next(),pt=point.next();
        std::array<double,3> weights{};for(int i=0;i<3;++i)weights[static_cast<size_t>(i)]=profiles[static_cast<size_t>(i)].next();
        if(quantum==0)targetKernels(usePath?position.azimuth:manual,extent,pt,weights,first);
        std::array<double,4> sumL{},sumR{};
        const float* hl=historyLeft.data()+pos+1;const float* hr=historyRight.data()+pos+1;
        for(int k=0;k<n;k+=4)for(int lane=0;lane<4;++lane)
        {
            const auto index=static_cast<size_t>(k+lane);auto& c=coefficients[index];const auto& inc=increments[index];
            for(int j=0;j<4;++j)c[static_cast<size_t>(j)]+=inc[static_cast<size_t>(j)];
            sumL[static_cast<size_t>(lane)]+=static_cast<double>(hl[index])*c[0]+static_cast<double>(hr[index])*c[2];
            sumR[static_cast<size_t>(lane)]+=static_cast<double>(hl[index])*c[1]+static_cast<double>(hr[index])*c[3];
        }
        for(int c=0;c<4;++c)speaker[static_cast<size_t>(c)]+=speakerSteps[static_cast<size_t>(c)];
        const double binauralL=sumL[0]+sumL[1]+sumL[2]+sumL[3],binauralR=sumR[0]+sumR[1]+sumR[2]+sumR[3];
        const double stereoL=dryL*speaker[0]+dryR*speaker[2],stereoR=dryL*speaker[1]+dryR*speaker[3];
        const double processedL=(stereoL+m*(binauralL-stereoL))*std::min(1.,1-bl),processedR=(stereoR+m*(binauralR-stereoR))*std::min(1.,1+bl);
        if(e==0){left=dryL;right=dryR;}
        else {left=static_cast<float>(dryL+e*(processedL-dryL));right=static_cast<float>(dryR+e*(processedR-dryR));}
        if(++quantum==16)
        {quantum=0;for(int k=0;k<n;++k)coefficients[static_cast<size_t>(k)]=targets[static_cast<size_t>(k)];speaker=speakerTargets;}
        if(++pos==n)pos=0;
    }
};
MotionPrototype::MotionPrototype()=default;
MotionPrototype::~MotionPrototype()=default;
void MotionPrototype::prepare(double rate,const std::string& bank){impl=std::make_unique<Impl>();impl->load(rate,bank);}
void MotionPrototype::setTargets(const MotionSettings& s) noexcept{impl->settings(s);}
void MotionPrototype::setPath(const MotionPath& p) noexcept{impl->clock.setPath(p);}
void MotionPrototype::setTempo(double bpm) noexcept{impl->clock.setTempo(bpm);}
void MotionPrototype::resetPath() noexcept{impl->clock.reset();}
void MotionPrototype::process(float* l,float* r,int frames,MotionPosition* telemetry) noexcept{for(int i=0;i<frames;++i)impl->sample(l[i],r[i],telemetry?telemetry+i:nullptr);}
int MotionPrototype::latency() const noexcept{return impl->d;}
bool MotionPrototype::implemented() noexcept{return true;}
const char* MotionPrototype::compiledIdentity() noexcept{return SHIROTA_MOTION_SOURCE_ID;}
}

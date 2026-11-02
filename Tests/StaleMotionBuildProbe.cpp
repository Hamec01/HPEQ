// S3 aligned-dry before implementation. Spatial/motion metrics are n/a.
#include "DSP/MotionPrototype.h"
#include "MotionIdentity.h"
#include <cmath>
#include <vector>
namespace hamlo::shirota
{
void MotionClock::prepare(double) noexcept{}
void MotionClock::setPath(const MotionPath&) noexcept{}
void MotionClock::setTempo(double) noexcept{}
void MotionClock::reset() noexcept{}
MotionPosition MotionClock::next() noexcept{return {};}
struct MotionPrototype::Impl {int d=128,pos=0;std::vector<float> l,r;};
MotionPrototype::MotionPrototype()=default;
MotionPrototype::~MotionPrototype()=default;
void MotionPrototype::prepare(double fs,const std::string&)
{impl=std::make_unique<Impl>();impl->d=fs==96000?256:128;impl->l.assign(static_cast<size_t>(impl->d+1),0);impl->r=impl->l;}
void MotionPrototype::setTargets(const MotionSettings&) noexcept{}
void MotionPrototype::setPath(const MotionPath&) noexcept{}
void MotionPrototype::setTempo(double) noexcept{}
void MotionPrototype::resetPath() noexcept{}
void MotionPrototype::process(float* l,float* r,int n,MotionPosition* trace) noexcept
{for(int i=0;i<n;++i){const int read=(impl->pos+1)%(impl->d+1);impl->l[static_cast<size_t>(impl->pos)]=l[i];impl->r[static_cast<size_t>(impl->pos)]=r[i];l[i]=impl->l[static_cast<size_t>(read)];r[i]=impl->r[static_cast<size_t>(read)];impl->pos=read;if(trace)trace[i]={};}}
int MotionPrototype::latency() const noexcept{return impl->d;}
bool MotionPrototype::implemented() noexcept{return false;}
const char* MotionPrototype::compiledIdentity() noexcept{return "intentional-stale-motion-object";}
}

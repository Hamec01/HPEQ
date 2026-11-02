// S2 before implementation: common aligned-dry reference; features are n/a.
#include "DSP/SpatialPrototype.h"
#include "SpatialIdentity.h"
#include <cmath>
#include <vector>

namespace hamlo::shirota
{
struct SpatialPrototype::Impl
{
    int delay=64,pos=0;
    std::vector<float> left,right;
};
SpatialPrototype::SpatialPrototype()=default;
SpatialPrototype::~SpatialPrototype()=default;
void SpatialPrototype::prepare(double fs,int)
{
    impl=std::make_unique<Impl>();impl->delay=2*static_cast<int>(std::lround(32*fs/48000));
    impl->left.assign(static_cast<size_t>(impl->delay+1),0);impl->right=impl->left;
}
void SpatialPrototype::setTargets(const SpatialSettings&) noexcept{}
void SpatialPrototype::process(float* left,float* right,int samples) noexcept
{
    auto& p=*impl;const int size=p.delay+1;
    for(int i=0;i<samples;++i)
    {
        p.left[static_cast<size_t>(p.pos)]=left[i];p.right[static_cast<size_t>(p.pos)]=right[i];
        const int read=(p.pos+1)%size;left[i]=p.left[static_cast<size_t>(read)];right[i]=p.right[static_cast<size_t>(read)];p.pos=read;
    }
}
int SpatialPrototype::latency() const noexcept{return impl->delay;}
double SpatialPrototype::guardDb() const noexcept{return 0;}
bool SpatialPrototype::implemented() noexcept{return false;}
const char* SpatialPrototype::compiledIdentity() noexcept{return SHIROTA_SPATIAL_SOURCE_ID;}
}

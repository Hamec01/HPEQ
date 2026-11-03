#include "DSP/LegacyFaderCore.h"
#include <algorithm>

namespace hpeq::dsp
{
void LegacyFaderCore::prepare(double rate,int block)
{
    gain.prepare(rate);steps=stepsFor(.020,rate);
    for(auto* group:{&chain,&work})for(auto& c:*group)c.assign(static_cast<size_t>(block),0);
    nodeRunning=running=false;nodeMix.reset(0);slotMix.reset(0);
}
void LegacyFaderCore::set(const FxSlotParams& p) noexcept
{
    if(!nodeRunning){nodeRunning=true;nodeMix.reset(0);gain.reset();running=false;slotMix.reset(0);}
    nodeMix.setTarget(1,steps);
    if(p.active&&!running){gain.reset();slotMix.reset(0);running=true;}
    params=p;
    if(running)slotMix.setTarget(p.active?static_cast<double>(p.mix):0,steps);
}
void LegacyFaderCore::process(float* const* audio,int channels,int samples) noexcept
{
    if(!nodeRunning)return;
    float* chainPtr[]{chain[0].data(),chain[1].data()};float* workPtr[]{work[0].data(),work[1].data()};
    for(int c=0;c<channels;++c)std::copy(audio[c],audio[c]+samples,chainPtr[c]);
    if(running)
    {
        for(int c=0;c<channels;++c)std::copy(chainPtr[c],chainPtr[c]+samples,workPtr[c]);
        gain.process(workPtr,channels,samples,params);
        for(int i=0;i<samples;++i)
        {
            const auto m=static_cast<float>(slotMix.next());
            for(int c=0;c<channels;++c){const float d=audio[c][i];chainPtr[c][i]=m==1?workPtr[c][i]:(m==0?d:d+m*(workPtr[c][i]-d));}
        }
        if(!params.active&&!slotMix.isRamping()&&slotMix.value()==0){running=false;slotMix.reset(0);gain.reset();}
    }
    if(nodeMix.isRamping()||nodeMix.value()!=0)
    {
        for(int i=0;i<samples;++i)
        {
            const auto g=static_cast<float>(nodeMix.next())*1.0f;
            for(int c=0;c<channels;++c){float delta=0;delta+=g*(chainPtr[c][i]-audio[c][i]);audio[c][i]+=delta;}
        }
    }
}
}

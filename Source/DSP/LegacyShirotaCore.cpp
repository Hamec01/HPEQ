#include "DSP/LegacyShirotaCore.h"
#include <algorithm>

namespace hpeq::dsp
{
void LegacyShirotaCore::prepare(double sampleRate,int maximumBlockSize)
{
    fs=sampleRate;stereo.prepare(fs);
    for(auto* group:{&chain,&work,&aligned,&bandDelayed})for(auto& channel:*group)channel.assign(static_cast<size_t>(maximumBlockSize),0.f);
    const int capacity=ShirotaCore::safeLookaheadSamples(fs);
    slotAlign.prepare(capacity);bandAlign.prepare(capacity);layerAlign.prepare(capacity);
    mixSteps=stepsFor(kMixRampSeconds,fs);nodeMix.reset(0);mix.reset(0);running=false;latency=0;
    // LayerEngine::prepare reapplied its previously published snapshot too.
    if(hasTargets)setTargets(params);
}
void LegacyShirotaCore::setTargets(const FxSlotParams& p) noexcept
{
    params=p;hasTargets=true;latency=LegacyStereoProcessor::latencySamples(p,fs);
    nodeMix.setTarget(1.,mixSteps);
    if(p.active&&!running){running=true;mix.reset(0);}
    if(running)mix.setTarget(p.active?static_cast<double>(p.mix):0.,mixSteps);
}
void LegacyShirotaCore::process(float* const* channels,int numChannels,int samples,const Transport&) noexcept
{
    float* c[kMaxChannels]{};float* a[kMaxChannels]{};float* w[kMaxChannels]{};float* b[kMaxChannels]{};
    for(int ch=0;ch<numChannels;++ch)
    {
        c[ch]=chain[static_cast<size_t>(ch)].data();a[ch]=aligned[static_cast<size_t>(ch)].data();w[ch]=work[static_cast<size_t>(ch)].data();b[ch]=bandDelayed[static_cast<size_t>(ch)].data();
        std::copy(channels[ch],channels[ch]+samples,c[ch]);
    }
    const float* const* dry=c;
    if(latency>0){slotAlign.process(c,a,numChannels,samples,latency);dry=a;}
    if(running)
    {
        for(int ch=0;ch<numChannels;++ch)std::copy(c[ch],c[ch]+samples,w[ch]);
        stereo.process(w,numChannels,samples,params);
        for(int i=0;i<samples;++i)
        {
            const auto m=static_cast<float>(mix.next());
            for(int ch=0;ch<numChannels;++ch)
            {const float d=dry[ch][i];c[ch][i]=m==1.f?w[ch][i]:(m==0.f?d:d+m*(w[ch][i]-d));}
        }
        if(!params.active&&!mix.isRamping()&&mix.value()==0){running=false;mix.reset(0);}
    }
    else if(latency>0)for(int ch=0;ch<numChannels;++ch)std::copy(a[ch],a[ch]+samples,c[ch]);
    const float* const* reference=channels;
    if(latency>0){bandAlign.process(channels,b,numChannels,samples,latency);reference=b;}
    // Compute the delta before advancing the layer's independent dry aligner.
    // Reuse work after processing; the original delta buffer started at +0.
    for(int i=0;i<samples;++i)
    {
        const auto g=static_cast<float>(nodeMix.next())*1.f;
        for(int ch=0;ch<numChannels;++ch){float delta=0.f;delta+=g*(c[ch][i]-reference[ch][i]);w[ch][i]=delta;}
    }
    if(latency>0)layerAlign.process(channels,channels,numChannels,samples,latency);
    for(int ch=0;ch<numChannels;++ch)for(int i=0;i<samples;++i)channels[ch][i]+=w[ch][i];
}
ShirotaCore::Meters LegacyShirotaCore::getStereoMeters(int) const noexcept
{return running&&stereo.shirotaActive()?stereo.shirota().meters():ShirotaCore::Meters{};}
bool LegacyShirotaCore::getStereoScope(int,std::array<float,ShirotaCore::kScopePoints>& left,std::array<float,ShirotaCore::kScopePoints>& right) const noexcept
{if(!running||!stereo.shirotaActive())return false;stereo.shirota().scope(left,right);return true;}
}

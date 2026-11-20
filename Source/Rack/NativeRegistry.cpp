#include "Rack/NativeRegistry.h"
#include "DSP/FxNodeProcessor.h"
#include <algorithm>
#include <map>

namespace hpeq::native
{
void Registry::collect()
{
    const auto safe=acknowledged.load(std::memory_order_acquire);
    entries.erase(std::remove_if(entries.begin(),entries.end(),[&](const auto& e)
        {return e->retired>0 && safe>=e->retired && e->runtime->inUse()==0;}),entries.end());
}

void Registry::clear()
{
    for(const auto& e:entries)jassert(e->runtime->inUse()==0);
    entries.clear();published=0;acknowledged.store(0,std::memory_order_relaxed);
}

double Registry::tailSeconds() const noexcept
{
    double total=0;
    for(const auto& e:entries)if(e->seen==published || e->runtime->inUse()>0)total+=e->runtime->tailSeconds();
    return total; // conservative sum: serial native sends can feed each other's tails
}

bool Registry::stage(const FxProjectState& project,std::array<dsp::LayerSnapshot,kNumLayers>& images,
                     double rate,int maximumBlock,juce::String& error)
{
    collect();
    struct Request {int key;bool active;const FxSlotState* state;native::Values* image;};
    std::vector<Request> requests;
    bool supported=true;
    std::map<FxType,int> counts;
    for(size_t j=0;j<project.layers.size();++j)
    {
        const auto& l=project.layers[j];
        const auto add=[&](int node,bool active,const std::vector<FxSlotState>& chain,dsp::FxNodeParams& target)
        {
            if(chain.size()>target.slots.size()){supported=false;error="Native node chain exceeds capacity";return;}
            for(size_t k=0;k<chain.size();++k)
                if(chain[k].engineRevision!=0)
                {
                    if(target.channel==ChannelMode::DualMono)
                    {supported=false;error="Native wrapper has no Dual Mono mode; use independent Left/Right nodes";return;}
                    if(target.slots[k].nativeValues.latency>dsp::FxNodeProcessor::maxSlotLatency(rate))
                    {supported=false;error="Native fixed latency exceeds alignment capacity";return;}
                    requests.push_back({static_cast<int>((j*9+static_cast<size_t>(node))*6+k),active&&l.enabled,&chain[k],&target.slots[k].nativeValues});
                    ++counts[chain[k].type];
                }
        };
        add(8,true,l.mixChain,images[j].mix);
        for(const auto& n:l.fxNodes)if(n.engineSlot>=0)add(n.engineSlot,n.enabled,n.chain,images[j].fx[static_cast<size_t>(n.engineSlot)]);
        for(const auto& n:l.eqNodes)if(n.engineSlot>=0)add(n.engineSlot,n.enabled,n.chain,images[j].fx[static_cast<size_t>(n.engineSlot)]);
    }
    if(!supported)return false;
    if(requests.size()>kActiveCapacity || counts[FxType::Reverb]>16 || counts[FxType::Delay]>8 || counts[FxType::MonstaFx]>8)
    {error="Native processor capacity exceeded (128 total; 16 Verb / 8 Echo / 8 Revolution)";return false;}
    std::vector<std::unique_ptr<Entry>> created;
    std::vector<Entry*> chosen;
    const auto next=published+1;
    for(auto& request:requests)
    {
        const auto& state=*request.state;
        Entry* entry=nullptr;
        for(const auto& candidate:entries)
            if(candidate->retired==0 && candidate->key==request.key && candidate->type==state.type
                && candidate->active==request.active && candidate->structural==state.nativeState
                && (state.type!=FxType::MonstaFx || candidate->structuralParameters==state.parameters))
            {entry=candidate.get();break;}
        if(entry==nullptr)
        {
            if(entries.size()+created.size()>=kActiveCapacity+kRetiredCapacity)
            {error="Native handover capacity exhausted; audio has not retired the previous topology";return false;}
            auto fresh=std::make_unique<Entry>();fresh->key=request.key;fresh->type=state.type;
            fresh->active=request.active;fresh->structural=state.nativeState;fresh->structuralParameters=state.parameters;
            fresh->runtime=std::make_unique<Runtime>(state.type,state.nativeState,state.parameters,rate,maximumBlock,error);
            if(!fresh->runtime->valid())return false;
            entry=fresh.get();created.push_back(std::move(fresh));
        }
        request.image->runtime=entry->runtime.get();
        request.image->latency=entry->runtime->latency();chosen.push_back(entry);
    }
    for(auto* e:chosen)e->seen=next;
    for(auto& e:created)entries.push_back(std::move(e));
    for(auto& e:entries)if(e->seen!=next && e->retired==0)e->retired=next;
    published=next;
    return true;
}
} // namespace hpeq::native

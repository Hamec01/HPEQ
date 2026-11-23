#include "Rack/NativeRegistry.h"
#include "DSP/FxNodeProcessor.h"
#include <algorithm>
#include <map>

namespace hpeq::native
{
Runtime* Registry::find(int layer,int node,int slot) const noexcept
{
    const int key=(layer*9+node)*6+slot;
    for(const auto& e:entries)if(e->key==key&&e->retired==0&&e->seen==published)return e->runtime.get();
    return nullptr;
}
Runtime* Registry::findRight(int layer,int node,int slot) const noexcept
{
    const int key=(layer*9+node)*6+slot;
    for(const auto& e:entries)if(e->key==key&&e->retired==0&&e->seen==published)return e->right.get();
    return nullptr;
}
std::shared_ptr<Runtime> Registry::share(int layer,int node,int slot) const noexcept
{
    const int key=(layer*9+node)*6+slot;
    for(const auto& e:entries)if(e->key==key&&e->retired==0&&e->seen==published)return e->runtime;
    return {};
}
void Registry::collect()
{
    const auto safe=acknowledged.load(std::memory_order_acquire);
    entries.erase(std::remove_if(entries.begin(),entries.end(),[&](const auto& e)
        {return e->retired>0 && safe>=e->retired && e->runtime->inUse()==0&&(!e->right||e->right->inUse()==0);}),entries.end());
}

void Registry::clear()
{
    for(const auto& e:entries){jassert(e->runtime->inUse()==0);jassert(!e->right||e->right->inUse()==0);}
    entries.clear();published=0;acknowledged.store(0,std::memory_order_relaxed);
}
void Registry::reprepare(double rate,int maximumBlock)
{
    collect();
    for(auto& e:entries)
    {
        e->runtime->reprepare(rate,maximumBlock,e->channels);
        if(e->right)e->right->reprepare(rate,maximumBlock,1);
    }
    preservePendingAfterPrepare=true;
}

double Registry::tailSeconds() const noexcept
{
    double total=0;
    for(const auto& e:entries)if(e->seen==published || e->runtime->inUse()>0)total+=e->runtime->tailSeconds();
    return total; // conservative sum: serial native sends can feed each other's tails
}

bool Registry::stage(const FxProjectState& project,std::array<dsp::LayerSnapshot,kNumLayers>& images,
                     double rate,int maximumBlock,juce::String& error,int streamChannels)
{
    collect();
    struct Request {int key;bool active;const FxSlotState* state;native::Values* image;int channels;bool dual;};
    std::vector<Request> requests;
    bool supported=true;
    std::map<FxType,int> counts;
    for(size_t j=0;j<project.layers.size();++j)
    {
        const auto& l=project.layers[j];
        const auto add=[&](int node,const std::vector<FxSlotState>& chain,dsp::FxNodeParams& target)
        {
            if(chain.size()>target.slots.size()){supported=false;error="Native node chain exceeds capacity";return;}
            for(size_t k=0;k<chain.size();++k)
                if(chain[k].engineRevision!=0)
                {
                    const auto mono=[](ChannelMode m){return m==ChannelMode::Left||m==ChannelMode::Right||m==ChannelMode::Mid||m==ChannelMode::Side;};
                    const bool one=streamChannels==1||mono(target.channel)||mono(l.channel);
                    const bool dual=!one&&(target.channel==ChannelMode::DualMono||l.channel==ChannelMode::DualMono);
                    const int channels=dual||one?1:2;
                    if(target.slots[k].nativeValues.latency>dsp::FxNodeProcessor::maxSlotLatency(rate))
                    {supported=false;error="Native fixed latency exceeds alignment capacity";return;}
                    requests.push_back({static_cast<int>((j*9+static_cast<size_t>(node))*6+k),l.enabled,&chain[k],&target.slots[k].nativeValues,channels,dual});
                    counts[chain[k].type]+=dual?2:1;
                }
        };
        add(8,l.mixChain,images[j].mix);
        for(const auto& n:l.fxNodes)if(n.engineSlot>=0)add(n.engineSlot,n.chain,images[j].fx[static_cast<size_t>(n.engineSlot)]);
        for(const auto& n:l.eqNodes)if(n.engineSlot>=0)add(n.engineSlot,n.chain,images[j].fx[static_cast<size_t>(n.engineSlot)]);
    }
    if(!supported)return false;
    int total=0;for(const auto& count:counts)total+=count.second;
    if(total>kActiveCapacity || counts[FxType::Reverb]>16 || counts[FxType::Delay]>8 || counts[FxType::MonstaFx]>8)
    {error="Native processor capacity exceeded (128 total; 16 Verb / 8 Echo / 8 Revolution)";return false;}
    std::vector<std::unique_ptr<Entry>> created;
    std::vector<Entry*> chosen;
    struct SceneChange {Entry* entry;juce::MemoryBlock bytes;std::unique_ptr<SceneUpdate> update;};
    std::vector<SceneChange> sceneChanges;
    const auto next=published+1;
    for(auto& request:requests)
    {
        const auto& state=*request.state;
        Entry* entry=nullptr;
        for(const auto& candidate:entries)
            if(candidate->retired==0 && candidate->key==request.key && candidate->type==state.type
                && (candidate->active==request.active||!request.active)
                && candidate->channels==request.channels&&candidate->dual==request.dual
                && (candidate->structural==state.nativeState||state.type==FxType::MonstaFx))
            {entry=candidate.get();break;}
        if(entry==nullptr)
        {
            if(entries.size()+created.size()>=kActiveCapacity+kRetiredCapacity)
            {error="Native handover capacity exhausted; audio has not retired the previous topology";return false;}
            auto fresh=std::make_unique<Entry>();fresh->key=request.key;fresh->type=state.type;
            fresh->active=request.active;fresh->structural=state.nativeState;
            fresh->channels=request.channels;fresh->dual=request.dual;
            fresh->runtime=std::make_shared<Runtime>(state.type,state.nativeState,state.parameters,rate,maximumBlock,error,request.channels);
            if(!fresh->runtime->valid())return false;
            if(request.dual)
            {
                fresh->right=std::make_unique<Runtime>(state.type,state.nativeState,state.parameters,rate,maximumBlock,error,1,true);
                if(!fresh->right->valid())return false;
            }
            entry=fresh.get();created.push_back(std::move(fresh));
        }
        else if(state.type==FxType::MonstaFx&&entry->structural!=state.nativeState)
        {
            auto update=prepareRevolutionUpdate(state.nativeState,error);if(!update)return false;
            sceneChanges.push_back({entry,state.nativeState,std::move(update)});
        }
        request.image->runtime=entry->runtime.get();
        request.image->rightRuntime=entry->right.get();
        request.image->latency=entry->runtime->latency();chosen.push_back(entry);
    }
    for(auto* e:chosen)e->seen=next;
    for(size_t i=0;i<chosen.size();++i)chosen[i]->active=requests[i].active;
    for(auto& e:created)entries.push_back(std::move(e));
    for(auto& e:entries)if(e->seen!=next && e->retired==0)e->retired=next;
    for(auto& image:images)image.nativeGeneration=next;
    for(size_t i=0;i<chosen.size();++i)
    {
        chosen[i]->runtime->synchronizeEditor(*requests[i].image,preservePendingAfterPrepare);
        if(chosen[i]->right)chosen[i]->right->synchronizeEditor(*requests[i].image,preservePendingAfterPrepare);
    }
    preservePendingAfterPrepare=false;
    // Preparation above can fail without editing a running processor. Applying
    // an already decoded scene uses only the native control-thread scene API;
    // its atomic scene slots are read by audio without a lock or a DSP reset.
    for(auto& change:sceneChanges)
    {
        change.update->apply(change.entry->runtime->referenceProcessor());
        if(change.entry->right)change.update->apply(change.entry->right->referenceProcessor());
        change.entry->structural.swapWith(change.bytes);
    }
    published=next;
    return true;
}
} // namespace hpeq::native

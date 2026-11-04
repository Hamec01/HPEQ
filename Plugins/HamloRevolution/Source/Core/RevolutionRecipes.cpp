#include "Core/RevolutionRecipes.h"
#include <algorithm>
#include <cmath>
#include <cstring>
namespace hpeq::revolution::simple
{
namespace
{
constexpr const char* ids[4][8] {
    {"shapeAOn","shapeAMix","shapeADepth","shapeASmooth","shapeARes","shapeACurve",nullptr,nullptr},
    {"shapeBOn","shapeBMix","shapeBDepth","shapeBSmooth","shapeBRes","shapeBCurve",nullptr,nullptr},
    {"throwAOn","throwAMix","throwADepth","throwATail","throwATone","throwASource",nullptr,nullptr},
    {"throwBOn","throwBMix","throwBDepth","throwBTail","throwBTone","throwBSource",nullptr,nullptr}
};
void put(Result& r,const char* id,double value)
{
    for(auto& p:r.parameters)if(std::strcmp(p.first,id)==0){p.second=value;return;}
    r.parameters.emplace_back(id,value);
}
}
Result levelRecipe(int lane,int target,int variant,const Settings& settings,const Hits& hits)
{
    Result r;if(lane<kFirstShape||lane>=kLanes)return r;
    auto& out=r.lanes[static_cast<size_t>(lane)];out.length=16;out.rate=0;out.condition=Always;
    const int v=variant&7;const auto& id=ids[lane-kFirstShape];
    const double amount=std::clamp(settings.amount,0.0,1.0);
    if(lane<kFirstThrow)
    {
        // Envelopes describe a gesture. Variants change rhythm/direction, not just gain.
        static constexpr int pump[4][4] {{15,8,2,0},{15,3,0,5},{0,3,8,15},{13,0,8,0}};
        static constexpr int sweep[4][8] {{1,2,4,8,12,15,9,3},{15,10,6,2,0,2,6,10},{0,0,4,12,4,0,2,9},{2,11,5,0,13,7,1,4}};
        for(int s=0;s<16;++s)
        {
            int level=0;
            if(target==ShapeVolume)level=pump[v%4][(s+(v>=4?1:0))%4];
            else if(target==ShapePan||target==ShapeWidth)level=(s+v)%((v%3)+2)==0?14:3+((s+v)%3)*3;
            else if(target==ShapeDrive||target==ShapeCrush)level=((s+v)%4==0)?14:((s+v)%4==1?8:0);
            else level=sweep[v%4][(s/((v>=4)?1:2)+v)%8];
            out.first[static_cast<size_t>(s)]=static_cast<std::uint8_t>(level);
        }
        const bool curve=target!=ShapeVolume&&target!=ShapeCrush;
        const double params[6]{1,1,.65+.3*amount,0,.18,curve?1.0:0.0};
        for(int i=0;i<6;++i)put(r,id[i],params[i]);
    }
    else
    {
        // A send must capture source, not the silence just after a hit. Rank the strongest
        // attacks; templates are a fallback only when LISTEN/HPDG has no attacks at all.
        std::vector<std::pair<float,int>> attacks;
        for(int s=0;s<16;++s)
        {
            const auto i=static_cast<size_t>(s);
            const float score=std::max({hits.snare[i],.75f*hits.hats[i],.6f*hits.kick[i]});
            if(score>=.25f)attacks.emplace_back(score,s);
        }
        if(attacks.empty())for(int s:{0,4,8,12})attacks.emplace_back(1.f,s);
        std::stable_sort(attacks.begin(),attacks.end(),[](auto a,auto b){return a.first>b.first;});
        const bool send=throwIsSend(target);
        const int count=std::min(static_cast<int>(attacks.size()),send?2+v%3:1+v%3);
        const int width=settings.sustained?4+v%3:(send?2+v%2:1+v%2);
        for(int a=0;a<count;++a)
        {
            // Always include the strongest attack. Rotating the entire ranked list
            // could pick only quiet hats; the initial Techno delay measured -25.38dB.
            const auto index = a == 0 || attacks.size() == 1 ? size_t{0}
                : size_t{1} + static_cast<size_t>(a - 1 + v) % (attacks.size() - 1);
            const int start=attacks[index].second;
            for(int k=0;k<width;++k)
            {
                const auto s=static_cast<size_t>((start+k)%16);
                const int level=k==0?15:std::max(5,14-k*3);
                out.first[s]=std::max(out.first[s],static_cast<std::uint8_t>(level));
            }
        }
        // Keep returns distinct from dry. TONAL is a resonator, so it needs less gain.
        const double mix=target==ThrowTonal?.3:(send?.75:.8);
        const double tail=target==ThrowTonal?.6:target==ThrowReverb?.55:target==ThrowVinyl?.85:.6;
        const double params[6]{1,mix,.85+.15*amount,tail,.65,send?1.0:0.0};
        for(int i=0;i<6;++i)put(r,id[i],params[i]);
    }
    return r;
}
void addSpatialRecipes(Result& r,const Settings& settings,const Hits& hits)
{
    if(settings.amount<=0)return;
    // Both spatial lanes are available without waiting for the fill bar. Selected
    // frequency protection remains in force; full-band is always an explicit choice.
    for(int lane=kFirstThrow;lane<kLanes;++lane)
    {
        const int target=lane==kFirstThrow?ThrowDelay:ThrowReverb;
        auto generated=levelRecipe(lane,target,static_cast<int>(mix64(settings.seed+static_cast<std::uint64_t>(lane))&7),settings,hits);
        const int oldBand=r.lanes[static_cast<size_t>(lane)].band;
        r.lanes[static_cast<size_t>(lane)]=generated.lanes[static_cast<size_t>(lane)];
        r.lanes[static_cast<size_t>(lane)].band=oldBand;
        r.onHit[static_cast<size_t>(lane)]=0;r.fills[static_cast<size_t>(lane)]=0;
        for(const auto& p:generated.parameters)put(r,p.first,p.second);
        put(r,lane==kFirstThrow?"throwATarget":"throwBTarget",target);
        put(r,lane==kFirstThrow?"throwAMix":"throwBMix",(.15+.6*settings.amount)*(intentOf(settings.intent)==Subtle?.5:1.));
        // DELAY: a spaced repeat, rather than a short flam hidden inside the attack.
        if(lane==kFirstThrow)put(r,"throwATime",(settings.seed&1)?7:5);
    }
    r.events=0;
    for(int lane=0;lane<kLanes;++lane)
    {
        const auto& first=lane<kEngines?r.pattern.steps[static_cast<size_t>(lane)]:r.lanes[static_cast<size_t>(lane)].first;
        for(auto v:first)r.events+=v!=0;
        for(auto v:r.lanes[static_cast<size_t>(lane)].extra)r.events+=v!=0;
    }
}
}

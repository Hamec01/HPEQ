#include "DSP/V2Integrated.h"
#include "S4Identity.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <stdexcept>
namespace hpeq::dsp
{
const char* s4SourceIdentity() noexcept{return SHIROTA_S4_SOURCE_ID;}
void V2Integrated::prepare(double fs,int block)
{
    if(std::strcmp(spatial.compiledIdentity(),SHIROTA_S4_SOURCE_ID)!=0||std::strcmp(motion.compiledIdentity(),SHIROTA_S4_SOURCE_ID)!=0)throw std::runtime_error("Mixed S4 spatial/motion objects");
    spatial.prepare(fs,block);motion.prepare(fs);steps=std::max(1,static_cast<int>(std::ceil(.02*fs)));
    dryDelay.prepare(static_cast<int>(std::lround(.0025*fs)));for(auto& c:dry)c.assign(static_cast<size_t>(block),0);
    mix.reset(1);gain.reset(1);mono.reset(0);started=false;hasPath=false;
}
void V2Integrated::targets(const v2::Controls& c,bool on) noexcept
{
    namespace p=v2;namespace s=hamlo::shirota::v2;
    s::SpatialSettings a;a.enabled=true;a.width=c[p::Width];a.bandWidth={c[p::LowWidth],c[p::MidWidth],c[p::HighWidth]};a.splitLow=c[p::LowSplit];a.splitHigh=c[p::HighSplit];a.monoBassHz=c[p::MonoBass];
    a.generator=static_cast<s::Generator>(static_cast<int>(c[p::Generator]));a.generate=c[p::Generate];a.guard=c[p::Guard]>.5f;a.correlationFloor=c[p::Floor];a.character=static_cast<s::Space>(static_cast<int>(c[p::SpaceType]));a.space=c[p::Space];a.size=c[p::Size];a.depth=c[p::Depth];a.damping=c[p::Damping];a.predelayMs=c[p::Predelay];spatial.setTargets(a);
    s::MotionSettings b;b.enabled=c[p::PositionOn]>.5f;b.renderer=c[p::Mode]>1.5f?s::MotionRenderer::Binaural:s::MotionRenderer::Stereo;b.usePath=c[p::Motion]>.5f;b.center=c[p::Position];b.spread=c[p::Spread];b.balance=c[p::Balance];b.source=c[p::Source]>.5f?s::MotionSource::Point:s::MotionSource::Spread;b.profile=static_cast<int>(c[p::Profile]);motion.setTargets(b);
    bool changed=!hasPath;
    for(int i=p::Unit;i<=p::Nodes;++i)if(i!=p::Rate)changed=changed||c[static_cast<size_t>(i)]!=previous[static_cast<size_t>(i)];
    for(int i=p::NodeStart;i<p::PositionOn;++i)changed=changed||c[static_cast<size_t>(i)]!=previous[static_cast<size_t>(i)];
    changed=changed||c[p::Motion]!=previous[p::Motion];
    if(changed)
    {
        s::MotionPath path;path.count=std::clamp(static_cast<int>(c[p::Nodes]),1,16);path.loop=c[p::Loop]>.5f;const bool beats=c[p::Unit]>.5f;path.unit=beats?s::MotionUnit::Beats:s::MotionUnit::Milliseconds;

        for(int i=0;i<16;++i){const int n=p::NodeStart+i*7;auto& node=path.nodes[static_cast<size_t>(i)];node.azimuth=c[static_cast<size_t>(n)];node.hold=c[static_cast<size_t>(n+(beats?3:1))];node.travel=c[static_cast<size_t>(n+(beats?4:2))];node.arc=static_cast<s::MotionArc>(static_cast<int>(c[static_cast<size_t>(n+5)]));node.curve=static_cast<s::MotionCurve>(static_cast<int>(c[static_cast<size_t>(n+6)]));}
        motion.setPath(path);hasPath=true;previousPath=path;
    }
    motion.setRate(c[p::Rate]);
    if(c[p::Restart]!=previous[p::Restart])motion.resetPath();
    const double m=on?std::clamp(static_cast<double>(c[p::Mix]),0.,1.):0,g=std::pow(10.,std::clamp(static_cast<double>(c[p::Output]),-24.,12.)/20),mn=c[p::Mono]>.5f?1:0;
    if(!started){mix.reset(m);gain.reset(on?g:1);mono.reset(mn);}else{mix.setTarget(m,steps);gain.setTarget(on?g:1,steps);mono.setTarget(mn,steps);}previous=c;
}
void V2Integrated::process(float* l,float* r,int n,double bpm) noexcept
{
    started=true;float* ptrs[]{dry[0].data(),dry[1].data()};const float* input[]{l,r};
    dryDelay.process(input,ptrs,2,n,dryDelay.capacity());spatial.process(l,r,n);motion.setTempo(bpm);motion.process(l,r,n);
    for(int i=0;i<n;++i)
    {
        const float m=static_cast<float>(mix.next()),g=static_cast<float>(gain.next()),mn=static_cast<float>(mono.next());
        const float left=m==0?ptrs[0][i]:(m==1?l[i]:ptrs[0][i]+m*(l[i]-ptrs[0][i]));
        const float right=m==0?ptrs[1][i]:(m==1?r[i]:ptrs[1][i]+m*(r[i]-ptrs[1][i]));
        const float mid=.5f*(left+right);l[i]=(left+mn*(mid-left))*g;r[i]=(right+mn*(mid-right))*g;
    }
}
}

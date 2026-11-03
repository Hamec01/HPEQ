#include "DSP/FaderKernel.h"
#include <algorithm>
#include <cmath>
#include <vector>
#include <limits>

namespace hamlo::fader
{
namespace
{
constexpr double pi=3.14159265358979323846,ln10=2.302585092994046;
constexpr std::uint64_t cycle=1ULL<<48,mask=cycle-1;
double valid(double v,double lo,double hi,double fallback) noexcept{return std::isfinite(v)?std::clamp(v,lo,hi):fallback;}
double amp(double db) noexcept{return std::pow(10.,db/20);}
double fract(double x) noexcept{return x-std::floor(x);}
std::uint64_t fixed(double x) noexcept{return static_cast<std::uint64_t>(std::llround(fract(x)*cycle))&mask;}
struct Ramp
{
    double value=0,target=0,step=0;int remaining=0;
    void prime(double x) noexcept{value=target=x;remaining=0;}
    void set(double x,int n) noexcept{if(x==target)return;target=x;remaining=std::max(1,n);step=(target-value)/remaining;}
    double next() noexcept{if(remaining>0)value=--remaining==0?target:value+step;return value;}
};
Settings sanitize(Settings s) noexcept
{
    auto en=[](int x,int hi){return x>=0&&x<=hi?x:0;};s.mode=static_cast<Mode>(en(static_cast<int>(s.mode),3));s.profile=static_cast<Profile>(en(static_cast<int>(s.profile),1));s.routing=static_cast<Routing>(en(static_cast<int>(s.routing),3));s.measure=static_cast<Measure>(en(static_cast<int>(s.measure),1));s.shape=static_cast<Shape>(en(static_cast<int>(s.shape),2));
    s.inputDb=valid(s.inputDb,-60,24,0);s.gainDb=valid(s.gainDb,-60,24,0);s.outputDb=valid(s.outputDb,-60,24,0);s.mix=valid(s.mix,0,1,1);s.leftDb=valid(s.leftDb,-24,24,0);s.rightDb=valid(s.rightDb,-24,24,0);s.leftDelayMs=valid(s.leftDelayMs,0,20,0);s.rightDelayMs=valid(s.rightDelayMs,0,20,0);s.polarity=std::clamp(s.polarity,0,3);
    s.targetDb=valid(s.targetDb,-40,0,-18);s.matchBoost=valid(s.matchBoost,0,24,12);s.matchCut=valid(s.matchCut,0,60,24);s.matchCeiling=valid(s.matchCeiling,-24,0,-1);s.matchedDb=valid(s.matchedDb,-60,24,0);s.strength=valid(s.strength,0,1,1);s.maxBoost=valid(s.maxBoost,0,24,6);s.maxCut=valid(s.maxCut,0,60,12);s.floorDb=valid(s.floorDb,-90,-20,-55);s.holdMs=valid(s.holdMs,0,1000,80);s.idleMs=valid(s.idleMs,1,2000,100);s.riseMs=valid(s.riseMs,.1,5000,250);s.fallMs=valid(s.fallMs,.1,5000,120);
    s.points=std::clamp(s.points,2,32);for(auto& v:s.levels)v=valid(v,0,1,1);s.depthDb=valid(s.depthDb,0,90,0);s.beats=valid(s.beats,.125,32,1);s.freeMs=valid(s.freeMs,10,60000,500);s.phase=valid(s.phase,0,1,0);s.rightPhase=valid(s.rightPhase,0,1,0);s.edgeMs=valid(s.edgeMs,.1,50,1);return s;
}
}
struct Kernel::Impl
{
    Settings s;double fs=48000;int channels=2,pos=0;
    Meter input,output,learn;
    bool learning=false,processed=false,active=false,reanchor=true,clockValid=false,lastPlaying=false,wasRiding=false,wasSync=true;
    double detector=0,rideA=0,rideB=0,motionDb[2]{},roleFrom[2]{1,1},lastRole[2]{1,1},expectedPpq=0;
    std::int64_t expectedSamples=0,hold=0;
    std::uint64_t phase=0,badInput=0,freeStart=0,freePosition=0,freePeriod=0;
    std::array<std::vector<float>,2> delay;
    Ramp inputGain,gain,outputGain,mix,blend,matchGain,left,right,pol[2],delayTime[2],matrix[4],roleBlend;
    MatchProposal lastProposal;
    std::array<double,4> routing()const noexcept
    {
        if(channels==1)return{1,0,0,1};
        std::array<double,4> m{1,0,0,1};
        if(s.routing==Routing::MonoSum)m={.5,.5,.5,.5};
        if(s.routing==Routing::LeftBoth)m={1,0,1,0};if(s.routing==Routing::RightBoth)m={0,1,0,1};
        if(s.swap){std::swap(m[0],m[2]);std::swap(m[1],m[3]);}return m;
    }
    void targets(bool prime) noexcept
    {
        const int normal=static_cast<int>(std::ceil(.02*fs)),shortRamp=static_cast<int>(std::ceil(.01*fs));
        auto set=[&](Ramp& r,double v,int n){if(prime)r.prime(v);else r.set(v,n);};
        set(inputGain,amp(s.inputDb),normal);set(gain,amp(s.gainDb),normal);set(outputGain,amp(s.outputDb),normal);set(mix,s.mix,normal);set(blend,s.enabled&&!s.bypass?1:0,normal);set(matchGain,amp(s.matchedDb),normal);set(left,amp(s.leftDb),normal);set(right,amp(s.rightDb),normal);
        for(int c=0;c<2;++c){set(pol[c],s.polarity&(1<<c)?-1:1,shortRamp);set(delayTime[c],(c?s.rightDelayMs:s.leftDelayMs)*fs*.001,normal);}
        const auto m=routing();for(int i=0;i<4;++i)set(matrix[i],m[static_cast<size_t>(i)],shortRamp);
        if(prime){roleBlend.prime(1);for(int c=0;c<2;++c)roleFrom[c]=lastRole[c]=s.mode==Mode::Match?amp(s.matchedDb):1;}
    }
    void reset() noexcept
    {
        input.reset();output.reset();learn.reset();learning=processed=active=clockValid=lastPlaying=wasRiding=false;wasSync=s.sync;reanchor=true;detector=rideA=rideB=0;hold=0;motionDb[0]=motionDb[1]=0;phase=fixed(s.phase);freeStart=phase;freePosition=freePeriod=0;badInput=0;pos=0;lastProposal={};for(auto& c:delay)std::fill(c.begin(),c.end(),0);targets(true);
    }
    double level(std::uint64_t at)const noexcept
    {
        const double phaseValue=static_cast<double>(at&mask)/cycle;
        if(s.shape==Shape::Sine)return .5+.5*std::cos(2*pi*phaseValue);
        if(s.shape==Shape::Triangle)return 1-2*std::abs(phaseValue-.5);
        const double x=phaseValue*s.points;const int i=static_cast<int>(x);double f=x-i;if(s.smooth)f=f*f*(3-2*f);return s.levels[static_cast<size_t>(i)]+f*(s.levels[static_cast<size_t>((i+1)%s.points)]-s.levels[static_cast<size_t>(i)]);
    }
};
Kernel::Kernel():p(std::make_unique<Impl>()){}Kernel::~Kernel()=default;
void Kernel::prepare(double rate,int channels)
{
    p->fs=valid(rate,8000,192000,48000);p->channels=std::clamp(channels,1,2);for(auto& c:p->delay)c.assign(static_cast<size_t>(std::ceil(.02*p->fs))+4,0);p->input.prepare(p->fs,p->channels);p->output.prepare(p->fs,p->channels);p->learn.prepare(p->fs,p->channels);p->reset();
}
void Kernel::set(const Settings& settings) noexcept
{
    const auto old=p->s;p->s=sanitize(settings);
    if(p->s.mode!=old.mode&&p->processed){for(int c=0;c<2;++c)p->roleFrom[c]=p->lastRole[c];p->roleBlend.prime(0);p->roleBlend.set(1,static_cast<int>(std::ceil(.02*p->fs)));}
    if(p->s.profile!=old.profile)p->rideA=p->rideB;
    if(p->s.phase!=old.phase){const auto shift=fixed(p->s.phase)-fixed(old.phase);p->phase=(p->phase+shift)&mask;p->freeStart=(p->freeStart+shift)&mask;p->reanchor=true;}
    if(p->s.beats!=old.beats||p->s.sync!=old.sync)p->reanchor=true;
    if((!p->s.enabled||p->s.bypass)&&p->learning){p->learning=false;p->learn.reset();p->lastProposal={};}
    if(p->processed)p->targets(false);
    p->rideA=std::clamp(p->rideA,-p->s.maxCut,p->s.maxBoost);p->rideB=std::clamp(p->rideB,-p->s.maxCut,p->s.maxBoost);
}
Settings Kernel::settings()const noexcept{return p->s;}
void Kernel::reset()noexcept{p->reset();}
void Kernel::startLearn()noexcept{p->learn.reset();p->lastProposal={};p->learning=p->s.enabled&&!p->s.bypass;}
MatchProposal Kernel::stopLearn()noexcept
{
    p->learning=false;p->learn.finishPeak();const auto m=p->learn.reading();MatchProposal result;result.measuredDb=p->s.measure==Measure::Rms?m.rmsI:m.lufsI;
    if(m.samples<static_cast<std::uint64_t>(std::ceil(.4*p->fs))||result.measuredDb<=-60||(p->s.measure==Measure::Lufs&&!m.readyI)){p->lastProposal=result;return result;}
    const double wanted=p->s.targetDb-result.measuredDb;double db=std::clamp(wanted,-p->s.matchCut,p->s.matchBoost);
    if(p->s.peakGuard)
    {
        const double allowed=p->s.matchCeiling-m.truePeak;
        if(allowed< -p->s.matchCut){result.limited=true;result.gainDb=-p->s.matchCut;result.predictedPeak=m.truePeak+result.gainDb;p->lastProposal=result;return result;}
        db=std::min(db,allowed);
    }
    db=std::clamp(db,-p->s.matchCut,p->s.matchBoost);result.valid=true;result.limited=std::abs(db-wanted)>1e-9;result.gainDb=db;result.predictedPeak=m.truePeak+db;p->lastProposal=result;return result;
}
MatchProposal Kernel::proposal()const noexcept{return p->lastProposal;}
void Kernel::process(float* const* audio,int channels,int count,const Clock& clock)noexcept
{
    if(count<=0||channels<=0||p->delay[0].empty())return;channels=std::min(channels,p->channels);const auto& s=p->s;
    const bool first=!p->processed;if(first){p->targets(true);p->processed=true;}
    const double bpm=valid(clock.bpm,20,400,120);const bool ppq=clock.hasPpq&&std::isfinite(clock.ppq);
    if(s.sync&&ppq)
    {
        if(p->reanchor||!p->clockValid||(clock.playing&&!p->lastPlaying)||(clock.hasSamples&&clock.samplePosition!=p->expectedSamples)||std::abs(clock.ppq-p->expectedPpq)>1e-7)p->phase=fixed(clock.ppq/s.beats+s.phase);
        p->clockValid=true;
    }
    else p->clockValid=false;
    const auto freePeriod=static_cast<std::uint64_t>(std::llround(p->fs*s.freeMs*.001));
    if(!s.sync&&(freePeriod!=p->freePeriod||p->wasSync)){p->freeStart=p->phase;p->freePosition=0;}
    p->freePeriod=freePeriod;p->wasSync=s.sync;
    p->reanchor=false;p->lastPlaying=clock.playing;p->expectedSamples=clock.samplePosition+count;p->expectedPpq=clock.ppq+count*bpm/(60*p->fs);
    const auto increment=static_cast<std::uint64_t>(std::llround((s.sync?bpm/(60*p->fs*s.beats):1000/(p->fs*s.freeMs))*cycle));
    const double detectorCoefficient=std::exp(-1/((s.profile==Profile::Natural?.05:.01)*p->fs)),idle=std::exp(-ln10/(s.idleMs*.001*p->fs)),edge=std::exp(-ln10/(s.edgeMs*.001*p->fs));
    const double response=s.profile==Profile::Natural?3.889720169867429:ln10,rise=std::exp(-response/(s.riseMs*.001*p->fs)),fall=std::exp(-response/(s.fallMs*.001*p->fs));
    for(int i=0;i<count;++i)
    {
        double dry[2]{};for(int c=0;c<channels;++c){dry[c]=audio[c][i];if(!std::isfinite(dry[c])){dry[c]=0;++p->badInput;}}
        p->input.push(dry[0],dry[1]);const double inputGain=p->inputGain.next();double pre[]{dry[0]*inputGain,dry[1]*inputGain};if(p->learning)p->learn.push(pre[0],pre[1]);
        const double power=(pre[0]*pre[0]+(channels==2?pre[1]*pre[1]:0))/channels;p->detector=power+detectorCoefficient*(p->detector-power);if(p->detector<1e-30)p->detector=0;
        const double detected=10*std::log10(std::max(p->detector,1e-30));
        if(detected>=s.floorDb+(p->active?-3:3)){p->active=true;p->hold=static_cast<std::int64_t>(std::ceil(s.holdMs*.001*p->fs));}else if(p->hold>0)--p->hold;else p->active=false;
        const bool ride=s.mode==Mode::Ride&&s.enabled&&!s.bypass&&p->active;
        if(!ride&&p->wasRiding)p->rideA=p->rideB;p->wasRiding=ride;
        const double target=ride?std::clamp(s.strength*(s.targetDb-detected),-s.maxCut,s.maxBoost):0,coefficient=ride?(target>p->rideB?rise:fall):idle;
        p->rideA=target+coefficient*(p->rideA-target);p->rideB=s.profile==Profile::Natural&&ride?p->rideA+coefficient*(p->rideB-p->rideA):p->rideA;
        p->rideA=std::clamp(p->rideA,-s.maxCut,s.maxBoost);p->rideB=std::clamp(p->rideB,-s.maxCut,s.maxBoost);if(std::abs(p->rideB)<1e-12)p->rideA=p->rideB=0;
        const double held=p->matchGain.next(),transition=p->roleBlend.next(),manual=p->gain.next();double shaped[2]{};
        for(int c=0;c<channels;++c)
        {
            const bool motion=s.mode==Mode::Motion&&(!s.sync||clock.playing);const auto phase=(p->phase+(c?fixed(s.rightPhase):0))&mask;const double desired=motion?-s.depthDb*(1-p->level(phase)):0;
            p->motionDb[c]=first?desired:desired+edge*(p->motionDb[c]-desired);if(std::abs(p->motionDb[c]-desired)<1e-12)p->motionDb[c]=desired;
            double role=s.mode==Mode::Match?held:s.mode==Mode::Ride?amp(p->rideB):s.mode==Mode::Motion?amp(p->motionDb[c]):1;
            role=transition==1?role:p->roleFrom[c]+transition*(role-p->roleFrom[c]);p->lastRole[c]=role;shaped[c]=pre[c]*role*manual;
        }
        double m[4];for(int j=0;j<4;++j)m[j]=p->matrix[j].next();double routed[2]{};
        routed[0]=m[0]==1&&m[1]==0?shaped[0]:m[0]*shaped[0]+m[1]*shaped[1];routed[1]=m[2]==0&&m[3]==1?shaped[1]:m[2]*shaped[0]+m[3]*shaped[1];
        const double trims[]{p->left.next(),p->right.next()},mix=p->mix.next(),out=p->outputGain.next(),blend=p->blend.next();double result[2]{};
        for(int c=0;c<channels;++c)
        {
            const double polarity=p->pol[c].next(),delay=p->delayTime[c].next();auto& line=p->delay[static_cast<size_t>(c)];const auto limit=static_cast<double>(std::numeric_limits<float>::max());const double wet=std::clamp(routed[c]*trims[c]*polarity,-limit,limit);line[static_cast<size_t>(p->pos)]=static_cast<float>(wet);
            const int whole=static_cast<int>(delay),size=static_cast<int>(line.size());const double fraction=delay-whole;int a=p->pos-whole;if(a<0)a+=size;const int b=a==0?size-1:a-1;
            const double aligned=delay==0?wet:line[static_cast<size_t>(a)]+fraction*(static_cast<double>(line[static_cast<size_t>(b)])-line[static_cast<size_t>(a)]);
            const double mixed=mix==0?dry[c]:mix==1?aligned:dry[c]+mix*(aligned-dry[c]);const double processed=mixed*out;const double value=blend==0?dry[c]:blend==1?processed:dry[c]+blend*(processed-dry[c]);result[c]=std::clamp(value,-limit,limit);audio[c][i]=static_cast<float>(result[c]);result[c]=audio[c][i];
        }
        p->output.push(result[0],result[1]);if(++p->pos==static_cast<int>(p->delay[0].size()))p->pos=0;
        if(!s.sync){if(++p->freePosition==p->freePeriod)p->freePosition=0;p->phase=(p->freeStart+fixed(static_cast<double>(p->freePosition)/p->freePeriod))&mask;}
        else if(clock.playing)p->phase=(p->phase+increment)&mask;
    }
}
Reading Kernel::reading()const noexcept
{Reading r;r.input=p->input.reading();r.output=p->output.reading();r.learn=p->learn.reading();r.learning=p->learning;r.active=p->active;r.correctionDb=p->rideB;r.phase=static_cast<double>(p->phase)/cycle;r.motionRunning=p->s.mode==Mode::Motion&&(!p->s.sync||p->lastPlaying);r.badInput=p->badInput;return r;}
}

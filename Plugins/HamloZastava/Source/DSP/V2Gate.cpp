#include "DSP/V2Gate.h"
#include "ZastavaS2Identity.h"
#include <algorithm>
#include <cmath>
#include <vector>
namespace hamlo::zastava
{
namespace
{
constexpr double pi=3.14159265358979323846;
double valid(double x,double low,double high,double fallback) noexcept{return std::isfinite(x)?std::clamp(x,low,high):fallback;}
double amplitude(double db) noexcept{return std::pow(10.,db/20);}
double decibels(double a) noexcept{return 20*std::log10(std::max(a,1e-12));}
double shape(double u,Profile profile) noexcept
{u=std::clamp(u,0.,1.);return profile==Profile::Hard?u:std::clamp(u*u*u*(10+u*(-15+6*u)),0.,1.);}
struct Pole
{
    double b0=1,b1=0,b2=0,a1=0,a2=0;std::array<std::array<double,2>,2> z{};
    double process(int c,double x) noexcept{auto& s=z[static_cast<size_t>(c)];const double y=b0*x+s[0];s[0]=b1*x-a1*y+s[1];s[1]=b2*x-a2*y;return y;}
};
struct Filter
{
    Pole hp,lp;bool hpOn=false,lpOn=false;
    static Pole design(double hz,double fs,bool high) noexcept
    {
        Pole p;const double w=2*pi*std::clamp(hz,10.,.45*fs)/fs,c=std::cos(w),alpha=std::sin(w)/(2*.7071067811865476),a0=1+alpha;
        p.b0=(high?1+c:1-c)/2/a0;p.b1=(high?-(1+c):1-c)/a0;p.b2=p.b0;p.a1=-2*c/a0;p.a2=(1-alpha)/a0;return p;
    }
    void setup(double high,double low,double fs) noexcept{*this={};hpOn=high>0;lpOn=low>0;if(hpOn)hp=design(high,fs,true);if(lpOn)lp=design(low,fs,false);}
    double process(int c,double x) noexcept{if(hpOn)x=hp.process(c,x);if(lpOn)x=lp.process(c,x);return x;}
};
struct Ramp
{
    double value=1,target=1,step=0;int left=0;
    void set(double v,int n) noexcept{if(v==target)return;target=v;left=n;step=(target-value)/std::max(1,n);}
    void snap(double v) noexcept{value=target=v;left=0;}
    double next() noexcept{if(left>0)value=--left==0?target:value+step;return value;}
};
struct Channel
{
    double peak=0,energy=0,fast=0,slow=0,a=1,b=1,last=1,from=1;
    bool open=false,armed=true;int hold=0,quiet=0;std::int64_t hit=-1,lastHit=-1000000000;std::uint64_t hits=0;
};
struct Queue
{
    std::vector<double> values;std::vector<std::int64_t> times;int head=0,tail=0,capacity=0;
    void prepare(int cap){capacity=cap;values.assign(static_cast<size_t>(cap),0);times.assign(static_cast<size_t>(cap),0);head=tail=0;}
    void push(double v,std::int64_t time,bool minimum) noexcept
    {
        while(head!=tail){const int at=(tail-1+capacity)%capacity;if(minimum?values[static_cast<size_t>(at)]<v:values[static_cast<size_t>(at)]>v)break;tail=at;}
        values[static_cast<size_t>(tail)]=v;times[static_cast<size_t>(tail)]=time;tail=(tail+1)%capacity;
    }
    double get(std::int64_t oldest) noexcept{while(head!=tail&&times[static_cast<size_t>(head)]<oldest)head=(head+1)%capacity;return values[static_cast<size_t>(head)];}
};
}
struct Gate::State
{
    double fs=48000;int delay=240,position=0,look=0,steps=960,filterLeft=0;std::int64_t counter=0,freeSamples=0;
    bool prepared=false,started=false,clockStarted=false,minimum=false;
    Settings p;std::array<Channel,2> channel;std::array<Queue,2> queue;
    std::array<std::vector<float>,2> audio,audition;std::array<std::vector<double>,2> window,raw;
    Filter oldFilter,newFilter;double installedHp=0,installedLp=0;
    double attack=0,release=0,detector=0,fastCoef=0,slowCoef=0,floor=.01;
    Ramp mix,output,listen;long double beat=0;
    std::atomic<double> meterGain{1},meterKey{-120};std::atomic<bool> meterOpen{true},meterMissing{false};std::atomic<std::uint64_t> meterHits{0};
    void coefficients() noexcept
    {
        const double root=p.profile==Profile::Soft?3.889720169867429:2.302585092994046;
        attack=std::exp(-root/(p.attackMs*.001*fs));release=std::exp(-root/(p.releaseMs*.001*fs));
        detector=p.detectorMs==0?0:std::exp(-1/(p.detectorMs*.001*fs));fastCoef=std::exp(-1/(.002*fs));slowCoef=std::exp(-1/(.020*fs));floor=amplitude(-p.rangeDb);
    }
    void rebuild() noexcept
    {
        for(int c=0;c<2;++c){auto& q=queue[static_cast<size_t>(c)];q.head=q.tail=0;for(auto at=std::max<std::int64_t>(0,counter-look);at<counter;++at)q.push(raw[static_cast<size_t>(c)][static_cast<size_t>(at%(delay+1))],at,minimum);}
    }
};
Gate::Gate():state(std::make_unique<State>()){}
Gate::~Gate()=default;
void Gate::prepare(double rate,int)
{
    auto& s=*state;s.fs=valid(rate,8000,192000,48000);s.delay=static_cast<int>(std::lround(.005*s.fs));s.steps=std::max(1,static_cast<int>(std::ceil(.02*s.fs)));
    for(auto& a:s.audio)a.assign(static_cast<size_t>(s.delay+1),0);
    for(auto& a:s.audition)a.assign(static_cast<size_t>(s.delay+1),0.f);
    for(auto* bank:{&s.window,&s.raw})for(auto& a:*bank)a.assign(static_cast<size_t>(s.delay+1),1);
    // One empty ring slot and one pending insertion before expiry are required.
    for(auto& q:s.queue)q.prepare(s.delay+3);s.prepared=true;reset();set(s.p);
}
void Gate::reset() noexcept
{
    auto& s=*state;s.position=0;s.counter=s.freeSamples=0;s.started=s.clockStarted=false;s.beat=0;s.channel={};s.filterLeft=0;
    for(auto& a:s.audio)std::fill(a.begin(),a.end(),0);
    for(auto& a:s.audition)std::fill(a.begin(),a.end(),0.f);
    for(auto* bank:{&s.window,&s.raw})for(auto& a:*bank)std::fill(a.begin(),a.end(),1);
    for(auto& q:s.queue)q.head=q.tail=0;
    s.oldFilter.setup(s.p.keyHpHz,s.p.keyLpHz,s.fs);s.newFilter=s.oldFilter;s.installedHp=s.p.keyHpHz;s.installedLp=s.p.keyLpHz;
    s.mix.snap(1);s.output.snap(1);s.meterGain=1;s.meterKey=-120;s.meterOpen=true;s.meterMissing=false;s.meterHits=0;
    s.listen.snap(0);
}
void Gate::set(const Settings& requested) noexcept
{
    auto& s=*state;auto p=requested;
    p.mode=static_cast<Mode>(std::clamp(static_cast<int>(p.mode),0,3));p.profile=static_cast<Profile>(std::clamp(static_cast<int>(p.profile),0,1));
    p.thresholdDb=valid(p.thresholdDb,-90,0,-40);p.rangeDb=valid(p.rangeDb,0,90,40);p.ratio=valid(p.ratio,1,100,4);p.kneeDb=valid(p.kneeDb,0,24,6);p.hysteresisDb=valid(p.hysteresisDb,0,24,4);
    p.attackMs=valid(p.attackMs,.01,200,1);p.holdMs=valid(p.holdMs,0,2000,20);p.releaseMs=valid(p.releaseMs,.01,5000,100);p.detectorMs=valid(p.detectorMs,0,100,10);p.lookaheadMs=valid(p.lookaheadMs,0,5,2);
    p.keyHpHz=valid(p.keyHpHz,0,20000,0);p.keyLpHz=valid(p.keyLpHz,0,20000,0);p.stereoLink=valid(p.stereoLink,0,1,1);p.mix=valid(p.mix,0,1,1);p.outputDb=valid(p.outputDb,-24,12,0);
    p.bodyMs=valid(p.bodyMs,0,2000,20);p.tailMs=valid(p.tailMs,.01,5000,100);p.onsetDb=valid(p.onsetDb,0,24,3);p.retriggerMs=valid(p.retriggerMs,0,200,10);
    p.steps=std::clamp(p.steps,16,32);p.stepBeats=valid(p.stepBeats,1./64,16,.25);p.stepMs=valid(p.stepMs,1,10000,125);for(auto& v:p.levels)v=valid(v,0,1,1);
    if(p.profile!=s.p.profile)for(auto& c:s.channel)c.a=c.b;
    if(!s.started){s.oldFilter.setup(p.keyHpHz,p.keyLpHz,s.fs);s.newFilter=s.oldFilter;s.installedHp=p.keyHpHz;s.installedLp=p.keyLpHz;}
    if(p.sync!=s.p.sync)s.freeSamples=0;
    const int look=p.mode==Mode::Rhythm?0:std::clamp(static_cast<int>(std::lround(p.lookaheadMs*.001*s.fs)),0,s.delay);const bool minimum=p.mode==Mode::Duck;
    s.p=p;s.coefficients();if(s.prepared&&(look!=s.look||minimum!=s.minimum)){s.look=look;s.minimum=minimum;s.rebuild();}
}
void Gate::process(float* const* audio,int channels,int frames,const float* const* key,int keyChannels,const Clock& clock) noexcept
{
    auto& s=*state;if(!s.prepared||!audio||frames<=0||channels<=0)return;channels=std::min(channels,2);
    const auto& p=s.p;const bool missing=p.external&&(!key||keyChannels<=0),stopped=p.mode==Mode::Rhythm&&p.sync&&!clock.playing;
    const bool passthrough=!p.enabled||p.bypass||missing||stopped;
    const double mixTarget=passthrough?0:p.mix,gainTarget=passthrough?1:amplitude(p.outputDb);
    if(!s.started){s.mix.snap(mixTarget);s.output.snap(gainTarget);}else{s.mix.set(mixTarget,s.steps);s.output.set(gainTarget,s.steps);}
    const double listenTarget=p.listen&&!p.bypass&&!missing?1:0;
    if(!s.started)s.listen.snap(listenTarget);else s.listen.set(listenTarget,s.steps);
    if(clock.hasPpq&&std::isfinite(clock.ppq)&&(!s.clockStarted||std::abs(static_cast<double>(s.beat)-clock.ppq)>1e-7)){s.beat=clock.ppq;s.clockStarted=true;}
    const double bpm=valid(clock.bpm,1,999,120),delta=bpm/(60*s.fs);double keyMaximum=-240,gainLast=1;bool anyOpen=false;
    for(int i=0;i<frames;++i)
    {
        if(s.filterLeft==0&&(p.keyHpHz!=s.installedHp||p.keyLpHz!=s.installedLp)){s.newFilter.setup(p.keyHpHz,p.keyLpHz,s.fs);s.installedHp=p.keyHpHz;s.installedLp=p.keyLpHz;s.filterLeft=s.steps;}
        double input[2]{},levels[2]{},filteredKey[2]{};
        const double filterMix=s.filterLeft>0?1-static_cast<double>(s.filterLeft)/s.steps:0;
        for(int c=0;c<channels;++c)
        {
            input[c]=std::isfinite(audio[c][i])?audio[c][i]:0;
            const double selected=p.external?(missing?0:key[std::min(c,keyChannels-1)][i]):input[c];const double x=std::isfinite(selected)?selected:0;
            const double old=s.oldFilter.process(c,x);double filtered=old;if(s.filterLeft>0)filtered+=filterMix*(s.newFilter.process(c,x)-old);levels[c]=std::abs(filtered);filteredKey[c]=filtered;
        }
        const double linked=std::max(levels[0],levels[channels-1]);const int audioRead=(s.position+1)%(s.delay+1);
        const double mix=s.mix.next(),output=s.output.next();
        const double listen=s.listen.next();
        for(int c=0;c<channels;++c)
        {
            auto& d=s.channel[static_cast<size_t>(c)];const double level=levels[c]*(1-p.stereoLink)+linked*p.stereoLink;
            d.peak=std::max(level,d.peak*s.detector);d.energy=s.detector*d.energy+(1-s.detector)*level*level;
            const double db=decibels(p.rms?std::sqrt(d.energy):d.peak);keyMaximum=std::max(keyMaximum,db);
            if(db>p.thresholdDb){d.open=true;d.hold=static_cast<int>(std::ceil(p.holdMs*.001*s.fs));}
            else{if(d.hold>0)--d.hold;if(db<p.thresholdDb-p.hysteresisDb&&d.hold==0)d.open=false;}
            double target=d.open?1:s.floor;
            if(p.mode==Mode::Clean&&p.expand)
            {
                const double difference=p.thresholdDb-db;double reduction=std::max(0.,difference)*(p.ratio-1);
                if(p.kneeDb>0&&std::abs(difference)<p.kneeDb*.5)reduction=(p.ratio-1)*std::pow(difference+p.kneeDb*.5,2)/(2*p.kneeDb);
                target=amplitude(-std::clamp(reduction,0.,p.rangeDb));
            }
            else if(p.mode==Mode::Duck)target=d.open?s.floor:1;
            else if(p.mode==Mode::Rhythm)
            {
                const long double at=p.sync?s.beat/p.stepBeats:static_cast<long double>(s.freeSamples)/(p.stepMs*.001*s.fs);
                const auto index=static_cast<std::int64_t>(std::floor(at+1e-12L));const int step=static_cast<int>((index%p.steps+p.steps)%p.steps);target=s.floor+(1-s.floor)*p.levels[static_cast<size_t>(step)];
            }
            double raw=target;
            if(p.mode==Mode::Drum)
            {
                d.fast=std::max(level,d.fast*s.fastCoef);d.slow=s.slowCoef*d.slow+(1-s.slowCoef)*level;
                if(d.fast<d.slow*1.1){if(++d.quiet>=static_cast<int>(std::ceil(.002*s.fs)))d.armed=true;}else d.quiet=0;
                if(d.armed&&decibels(d.fast)>p.thresholdDb&&d.fast>d.slow*amplitude(p.onsetDb)&&s.counter-d.lastHit>=static_cast<std::int64_t>(std::ceil(p.retriggerMs*.001*s.fs)))
                {d.hit=d.lastHit=s.counter;d.from=d.last;d.armed=false;d.quiet=0;++d.hits;}
                if(d.hit<0)raw=s.floor;
                else
                {
                    const auto age=s.counter-d.hit;const auto attack=std::max<std::int64_t>(1,static_cast<std::int64_t>(std::ceil(p.attackMs*.001*s.fs))),body=static_cast<std::int64_t>(std::ceil(p.bodyMs*.001*s.fs)),tail=std::max<std::int64_t>(1,static_cast<std::int64_t>(std::ceil(p.tailMs*.001*s.fs)));
                    if(age<attack)raw=d.from+(1-d.from)*shape(static_cast<double>(age)/attack,p.profile);
                    else if(age<attack+body)raw=1;
                    else raw=s.floor+(1-s.floor)*(1-shape(static_cast<double>(age-attack-body)/tail,p.profile));
                }
                d.a=d.b=raw;
            }
            else
            {
                if(!s.started)d.a=d.b=target;
                const double coefficient=target>d.b?s.attack:s.release;d.a=target+coefficient*(d.a-target);d.b=p.profile==Profile::Soft?d.a+coefficient*(d.b-d.a):d.a;raw=d.b;
            }
            if(missing)raw=1;raw=std::clamp(raw,0.,1.);d.last=raw;anyOpen=anyOpen||d.open;
            s.raw[static_cast<size_t>(c)][static_cast<size_t>(s.position)]=raw;auto& q=s.queue[static_cast<size_t>(c)];q.push(raw,s.counter,s.minimum);const double window=q.get(s.counter-s.look);
            s.window[static_cast<size_t>(c)][static_cast<size_t>(s.position)]=window;const int gainRead=(s.position-(s.delay-s.look)+s.delay+1)%(s.delay+1);const double applied=s.window[static_cast<size_t>(c)][static_cast<size_t>(gainRead)];gainLast=applied;
            auto& line=s.audio[static_cast<size_t>(c)];line[static_cast<size_t>(s.position)]=static_cast<float>(input[c]);const float dry=line[static_cast<size_t>(audioRead)];
            audio[c][i]=mix==0&&output==1?dry:static_cast<float>((dry+mix*(dry*applied-dry))*output);
            auto& monitor=s.audition[static_cast<size_t>(c)];monitor[static_cast<size_t>(s.position)]=static_cast<float>(filteredKey[c]);
            if(listen==1)audio[c][i]=monitor[static_cast<size_t>(audioRead)];
            else if(listen!=0)audio[c][i]=static_cast<float>(audio[c][i]+listen*(monitor[static_cast<size_t>(audioRead)]-audio[c][i]));
        }
        if(s.filterLeft>0&&--s.filterLeft==0)s.oldFilter=s.newFilter;
        s.position=audioRead;++s.counter;++s.freeSamples;if(clock.playing)s.beat+=delta;s.started=true;
    }
    s.meterGain=gainLast;s.meterKey=keyMaximum;s.meterOpen=anyOpen;s.meterMissing=missing;s.meterHits=std::max(s.channel[0].hits,s.channel[1].hits);
}
int Gate::latency() const noexcept{return state->delay;}
Reading Gate::reading() const noexcept{return {state->meterGain.load(),state->meterKey.load(),state->meterOpen.load(),state->meterMissing.load(),state->meterHits.load()};}
bool Gate::implemented() noexcept{return true;}
const char* Gate::sourceIdentity() noexcept{return ZASTAVA_S2_SOURCE_ID;}
}

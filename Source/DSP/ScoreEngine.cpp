#include "DSP/ScoreEngine.h"
#include <algorithm>
#include <cmath>
#include <limits>

namespace hamlo::echo
{
namespace {constexpr double pi=3.14159265358979323846;
double gain(double db){return std::pow(10.,db/20.);}
double coefficient(double hz,double rate){return 1-std::exp(-2*pi*std::clamp(hz,1.,.45*rate)/rate);}
}
void ScoreEngine::History::prepare(int n)
{
    capacity=n;for(auto& ch:data)ch.assign(static_cast<size_t>(n),0);pos=0;clock=epoch=0;
}
void ScoreEngine::History::put(std::array<double,2> x) noexcept
{
    for(int ch=0;ch<2;++ch)data[static_cast<size_t>(ch)][static_cast<size_t>(pos)]=static_cast<float>(x[static_cast<size_t>(ch)]);
    if(++pos==capacity)pos=0;++clock;
}
double ScoreEngine::History::at(int ch,int64_t t) const noexcept
{
    if(t<epoch||t>=clock||t<=clock-capacity)return 0;
    int index=pos-static_cast<int>(clock-t);if(index<0)index+=capacity;
    return data[static_cast<size_t>(ch)][static_cast<size_t>(index)];
}
double ScoreEngine::History::ago(int ch,double delay) const noexcept
{
    const double position=static_cast<double>(clock-1)-delay;
    const auto base=static_cast<int64_t>(std::floor(position)); const double f=position-base;
    const double first=at(ch,base);return f==0?first:first+f*(at(ch,base+1)-first);
}
bool ScoreEngine::structuralChange(const ScoreSettings& a,const ScoreSettings& b) noexcept
{
    if(a.sourceNote!=b.sourceNote||a.bands!=b.bands||a.spaceRoute!=b.spaceRoute||a.spaceType!=b.spaceType||a.spaceSize!=b.spaceSize
       ||a.feedbackPitch!=b.feedbackPitch||a.feedbackMs!=b.feedbackMs||a.feedbackSync!=b.feedbackSync||a.bpm!=b.bpm||a.swing!=b.swing)return true;
    for(size_t band=0;band<3;++band)if(a.band[band].timeScale!=b.band[band].timeScale)return true;
    for(size_t t=0;t<8;++t)if(a.taps[t].enabled!=b.taps[t].enabled||a.taps[t].note!=b.taps[t].note||a.taps[t].action!=b.taps[t].action
        ||a.taps[t].timeMs!=b.taps[t].timeMs||a.taps[t].sync!=b.taps[t].sync||a.taps[t].fragmentMs!=b.taps[t].fragmentMs||a.taps[t].repeats!=b.taps[t].repeats)return true;
    return false;
}
void ScoreEngine::prepare(double fs,int n)
{
    rate=fs;blockSize=std::max(1,n);steps=hpeq::dsp::stepsFor(.02,rate);
    pitch.prepare(rate,blockSize);feedbackPitch.prepare(rate,1);
    const int capacity=static_cast<int>(std::ceil(6.2*rate))+16;
    for(auto& group:groups)group.prepare(capacity);
    for(auto& line:feedback)line.prepare(static_cast<int>(std::ceil(4.1*rate))+16);
    hold.prepare(static_cast<int>(std::ceil(4.1*rate))+16);
    for(auto& group:groupScratch)for(auto& ch:group)ch.assign(static_cast<size_t>(blockSize),0);
    for(auto& ch:sourceScratch)ch.assign(static_cast<size_t>(blockSize),0);
    space.prepare(rate);echoGlitch.prepare(rate);tailGlitch.prepare(rate);
    clock=epoch=0;start(settings);
}
void ScoreEngine::start(const ScoreSettings& s) noexcept
{
    clock=epoch=0;pitch.resetEpoch();feedbackPitch.resetEpoch();
    for(auto& h:groups)h.reset();for(auto& h:feedback)h.reset();hold.reset();
    space.reset();echoGlitch.reset();tailGlitch.reset();tapState={};feedbackLow={};feedbackHigh={};outputLow={};outputHigh={};
    colorState={};duckEnvelope=0;duckGain=1;groupCount=0;mapping.fill(-1);
    for(size_t t=0;t<8;++t)
    {
        const auto& tap=s.taps[t];const int interval=tap.note-s.sourceNote;
        if(!tap.enabled||tap.action==Action::Skip||std::abs(interval)>24)continue;
        int group=0;while(group<groupCount&&intervals[static_cast<size_t>(group)]!=interval)++group;
        if(group==groupCount)intervals[static_cast<size_t>(groupCount++)]=interval;
        mapping[t]=group;
    }
    dryWeight.reset(s.routing==1?0:1-s.mix);wetWeight.reset(s.routing==1?1:s.mix);
    outputGain.reset(gain(s.outputDb));inputWeight.reset(1);freezeWeight.reset(s.freeze?1:0);
    settings=s;releaseRemaining=0;feedbackShift=999;configure(s);
    for(size_t b=0;b<3;++b)bandWeight[b].reset(s.band[b].enabled?1:0);
}
void ScoreEngine::configure(const ScoreSettings& s) noexcept
{
    if(settings.freeze&&!s.freeze)releaseRemaining=freezeLength;
    if(s.freeze)releaseRemaining=0;
    settings=s;
    dryWeight.setTarget(s.routing==1?0:1-s.mix,steps);wetWeight.setTarget(s.routing==1?1:s.mix,steps);
    outputGain.setTarget(gain(s.outputDb),steps);freezeWeight.setTarget(s.freeze?1:0,steps);
    space.configure(s);
    splitLowCoefficient=coefficient(s.splitLow,rate);splitHighCoefficient=coefficient(std::max(s.splitHigh,s.splitLow*1.2),rate);
    postLowCoefficient=coefficient(s.lowCut,rate);postHighCoefficient=coefficient(s.highCut,rate);
    colorCoefficient=coefficient(s.color==2?7500:4000,rate);driveGain=gain(s.driveDb);
    for(int b=0;b<3;++b)
    {
        bandWeight[static_cast<size_t>(b)].setTarget(s.band[static_cast<size_t>(b)].enabled?1:0,steps);
        const auto& band=s.band[static_cast<size_t>(b)];const double level=gain(band.gainDb),pan=band.pan;
        bandReturnGain[static_cast<size_t>(b)]={level*(pan>0?1-pan:1),level*(pan<0?1+pan:1)};
        bandReturnBound[static_cast<size_t>(b)]=level*band.feedback;
        const double loopMs=std::clamp(timeMs(s.feedbackMs,s.feedbackSync,s.bpm)*(s.bands?s.band[static_cast<size_t>(b)].timeScale:1),20.,4000.);
        loopDelays[static_cast<size_t>(b)]=std::max(0.,loopMs*.001*rate-(s.feedbackPitch!=0?.08*rate:0)-1);
    }
    for(size_t t=0;t<8;++t)
    {
        auto& target=tapTargets[t];const auto& p=s.taps[t];target.low=coefficient(p.lowCut,rate);target.high=coefficient(p.highCut,rate);
        target.length=std::max(4,static_cast<int>(std::lround(p.fragmentMs*.001*rate))/2*2);
        target.repeats=std::clamp(static_cast<int>(p.repeats),2,8);
        for(size_t b=0;b<3;++b)
        {const double swing=p.sync>0&&t%2==1?15000*s.swing/s.bpm:0;
            const double requested=std::clamp((timeMs(p.timeMs,p.sync,s.bpm)+swing)*(s.bands?s.band[b].timeScale:1),1.,4000.);
            const int group=mapping[t],capture=group<0||intervals[static_cast<size_t>(group)]==0?0:static_cast<int>(std::ceil(.08*rate));
            target.delay[b]=std::max(0,static_cast<int>(std::lround(requested*.001*rate))-capture);
            target.frameDelay[b]=target.delay[b]+capture;
            const double pan=std::clamp(p.pan+(s.bands?s.band[b].pan:0),-1.,1.),level=gain(p.gainDb+(s.bands?s.band[b].gainDb:0));
            target.gain[b]={level*(pan>0?1-pan:1),level*(pan<0?1+pan:1)};}
    }
    Score plan;plan.sourceNote=60;
    for(int g=0;g<groupCount;++g)
    {auto& t=plan.taps[static_cast<size_t>(g)];t.enabled=intervals[static_cast<size_t>(g)]!=0;t.targetNote=60+intervals[static_cast<size_t>(g)];t.timeMs=80;t.gain=1;}
    pitch.setScore(plan);
    if(feedbackShift!=s.feedbackPitch) { feedbackPitch.resetEpoch();feedbackShift=s.feedbackPitch; }
    Score feedbackPlan;feedbackPlan.sourceNote=60;auto& f=feedbackPlan.taps[0];
    f.enabled=feedbackShift!=0;f.targetNote=60+feedbackShift;f.timeMs=80;f.gain=1;feedbackPitch.setScore(feedbackPlan);
    if(!s.freeze)freezeLength=std::clamp(static_cast<int>(std::lround(timeMs(s.feedbackMs,s.feedbackSync,s.bpm)*.001*rate)),1,hold.capacity-2);
}
std::array<double,2> ScoreEngine::reflection(int tap,int band) const noexcept
{
    const int group=mapping[static_cast<size_t>(tap)];if(group<0)return {};
    const auto& p=settings.taps[static_cast<size_t>(tap)];const auto& h=groups[static_cast<size_t>(group)];
    const auto& target=tapTargets[static_cast<size_t>(tap)];const int delay=target.delay[static_cast<size_t>(band)];
    if(p.action==Action::Forward)return {h.at(0,h.clock-1-delay),h.at(1,h.clock-1-delay)};
    const int length=target.length;
    const int frameDelay=target.frameDelay[static_cast<size_t>(band)];
    const int64_t elapsed=clock-epoch-frameDelay;
    if(p.action==Action::Reverse)
    {
        const int64_t t=elapsed-length;if(t<0)return {};const int64_t frame=t/(length/2);std::array<double,2> out{};
        for(int ago=0;ago<2;++ago)
        {const auto f=frame-ago;if(f<0)continue;const int64_t start=epoch+frameDelay+length+f*(length/2);const int age=static_cast<int>(clock-start);
            if(age>=length)continue;const double w=.5-.5*std::cos(2*pi*age/length);for(int ch=0;ch<2;++ch)out[static_cast<size_t>(ch)]+=w*h.at(ch,start-delay-1-age);}
        return out;
    }
    if(elapsed<length)return {};
    const int64_t age=elapsed%length,frame=clock-age-delay;
    double position=frame-length+age;
    if(p.action==Action::Stutter)position=static_cast<double>(frame-length+subdivisionAge(age,length,target.repeats));
    if(p.action==Action::TapeStop){const double u=static_cast<double>(age)/length;position=frame-length+length*(u-.5*u*u);}
    const auto base=static_cast<int64_t>(std::floor(position));const double fraction=position-base;
    const double fade=std::clamp(std::min(static_cast<double>(age),static_cast<double>(length-1-age))/32.,0.,1.);
    std::array<double,2> out{};for(int ch=0;ch<2;++ch)out[static_cast<size_t>(ch)]=fade*(h.at(ch,base)+fraction*(h.at(ch,base+1)-h.at(ch,base)));return out;
}
std::array<double,2> ScoreEngine::color(std::array<double,2> x) noexcept
{
    const double drive=driveGain;
    for(size_t ch=0;ch<2;++ch)
    {
        double y=x[ch];
        if(settings.color!=0||settings.driveDb>0)y=std::tanh(y*drive)/drive;
        if(settings.color==2||settings.color==3)
        {colorState[ch]+=colorCoefficient*(y-colorState[ch]);y=colorState[ch];}
        if(settings.color==1)y=.9*y+.1*std::tanh(2*y)*.5;
        if(settings.color==4)y=std::round(y*512)/512;
        x[ch]=4*std::tanh(y/4);
    }
    return x;
}
double ScoreEngine::tailSeconds() const noexcept
{
    return estimateTail(settings);
}
double ScoreEngine::estimateTail(const ScoreSettings& s) noexcept
{
    if(s.freeze)return std::numeric_limits<double>::infinity();
    double loop=timeMs(s.feedbackMs,s.feedbackSync,s.bpm)*.001,fb=s.feedback;
    if(s.bands){fb=0;double scale=0;for(const auto& b:s.band)if(b.enabled){fb=std::max(fb,b.feedback);scale=std::max(scale,b.timeScale);}loop*=scale;}
    loop=std::clamp(loop,s.feedbackPitch==0?.02:.081,4.);
    return std::min(120.,6+s.spaceDecay+(fb>.001?loop*std::log(1e-5)/std::log(std::min(fb,.9)):0));
}
void ScoreEngine::process(float* const* audio,int channels,int samples,bool acceptInput,bool wetOnly) noexcept
{
    inputWeight.setTarget(acceptInput&&!settings.freeze?1:0,steps);
    const double splitLow=splitLowCoefficient,splitHigh=splitHighCoefficient;
    const double postLow=postLowCoefficient,postHigh=postHighCoefficient;
    const double duckAttack=1-std::exp(-1/(.004*rate)),duckRelease=1-std::exp(-1/(.15*rate));
    double energy=0;
    for(int start=0;start<samples;start+=blockSize)
    {
        const int count=std::min(blockSize,samples-start);
        for(int i=0;i<count;++i)
        {
            const double gate=inputWeight.next();std::array<double,2> x{audio[0][start+i]*gate,audio[channels>1?1:0][start+i]*gate};
            if(settings.spaceRoute==2)
            {auto tail=space.process(x);tail=tailGlitch.process(tail,settings.glitchTarget!=0?settings.glitch:0,settings.glitchMs,settings.glitchAmount);
                for(size_t ch=0;ch<2;++ch)x[ch]=(1-settings.spaceMix)*x[ch]+settings.spaceMix*tail[ch];}
            for(int ch=0;ch<2;++ch)sourceScratch[static_cast<size_t>(ch)][static_cast<size_t>(i)]=static_cast<float>(x[static_cast<size_t>(ch)]);
        }
        const float* in[]{sourceScratch[0].data(),sourceScratch[1].data()};
        float* const* separated[8];float* groupPointers[8][2];
        for(int g=0;g<8;++g){for(int ch=0;ch<2;++ch)groupPointers[g][ch]=groupScratch[static_cast<size_t>(g)][static_cast<size_t>(ch)].data();separated[g]=groupPointers[g];}
        pitch.processVoices(in,separated,2,count);
        for(int i=0;i<count;++i,++clock)
        {
            for(int g=0;g<groupCount;++g)
            {std::array<double,2> x{};for(size_t ch=0;ch<2;++ch)x[ch]=intervals[static_cast<size_t>(g)]==0?sourceScratch[ch][static_cast<size_t>(i)]:groupScratch[static_cast<size_t>(g)][ch][static_cast<size_t>(i)];groups[static_cast<size_t>(g)].put(x);}
            std::array<std::array<double,2>,3> bandSum{};
            std::array<std::array<double,2>,3> bandSpaceDelta{};
            std::array<double,3> bandAmount{};for(size_t b=0;b<3;++b)bandAmount[b]=settings.bands?bandWeight[b].next():1;
            for(int t=0;t<8;++t)
            {
                const auto& p=settings.taps[static_cast<size_t>(t)];if(mapping[static_cast<size_t>(t)]<0)continue;
                auto& state=tapState[static_cast<size_t>(t)];
                const int bands=settings.bands?3:1;
                for(int b=0;b<bands;++b)
                {
                    const auto reflectionSample=reflection(t,b);
                    for(int ch=0;ch<2;++ch)
                    {
                        double y=reflectionSample[static_cast<size_t>(ch)];
                        if(settings.bands)
                        {const double low=state.low[static_cast<size_t>(b)][static_cast<size_t>(ch)].single(y,splitLow);
                            const double high=state.high[static_cast<size_t>(b)][static_cast<size_t>(ch)].single(y,splitHigh);y=b==0?low:b==1?high-low:y-high;}
                        if(p.lowCut>10)y-=state.hp[static_cast<size_t>(b)][static_cast<size_t>(ch)].low(y,tapTargets[static_cast<size_t>(t)].low);
                        if(p.highCut<20000)y=state.lp[static_cast<size_t>(b)][static_cast<size_t>(ch)].low(y,tapTargets[static_cast<size_t>(t)].high);
                        const double contribution=y*tapTargets[static_cast<size_t>(t)].gain[static_cast<size_t>(b)][static_cast<size_t>(ch)];
                        bandSum[static_cast<size_t>(b)][static_cast<size_t>(ch)]+=contribution;
                        bandSpaceDelta[static_cast<size_t>(b)][static_cast<size_t>(ch)]+=contribution*(p.spaceSend-1);
                    }
                }
            }
            std::array<double,2> echo{};
            const int bands=settings.bands?3:1;
            // Ordered one-pole complementary bands obey sum |H_b(w)|^2 <= 1.
            // Cauchy-Schwarz then bounds arbitrary independent delay phases by
            // sqrt(sum effectiveFeedback_b^2). Keep that norm <= .9, including
            // band level/pan and the per-sample enabled crossfades.
            double feedbackNormSquared=0;
            if(settings.bands)for(size_t b=0;b<3;++b){const double v=bandAmount[b]*bandReturnBound[b];feedbackNormSquared+=v*v;}
            const double feedbackSafety=feedbackNormSquared>.81?.9/std::sqrt(feedbackNormSquared):1;
            for(int b=0;b<bands;++b)
                for(int ch=0;ch<2;++ch)
                {
                    double y=feedback[static_cast<size_t>(b)].ago(ch,loopDelays[static_cast<size_t>(b)]);
                    if(settings.bands)
                    {const double low=feedbackLow[static_cast<size_t>(b)][static_cast<size_t>(ch)].single(y,splitLow);
                        const double high=feedbackHigh[static_cast<size_t>(b)][static_cast<size_t>(ch)].single(y,splitHigh);y=b==0?low:b==1?high-low:y-high;}
                    const double feedbackGain=settings.bands?feedbackSafety*settings.band[static_cast<size_t>(b)].feedback*bandReturnGain[static_cast<size_t>(b)][static_cast<size_t>(ch)]:settings.feedback;
                    echo[static_cast<size_t>(ch)]+=bandAmount[static_cast<size_t>(b)]*(bandSum[static_cast<size_t>(b)][static_cast<size_t>(ch)]+feedbackGain*y);
                }
            auto spatialInput=echo;
            for(int b=0;b<bands;++b)for(size_t ch=0;ch<2;++ch)spatialInput[ch]+=bandAmount[static_cast<size_t>(b)]*bandSpaceDelta[static_cast<size_t>(b)][ch];
            echo=echoGlitch.process(echo,settings.glitchTarget!=1?settings.glitch:0,settings.glitchMs,settings.glitchAmount);
            std::array<double,2> loopInput=echo,wet=echo;
            if(settings.spaceRoute!=2)
            {
                auto tail=space.process(spatialInput);tail=tailGlitch.process(tail,settings.glitchTarget!=0?settings.glitch:0,settings.glitchMs,settings.glitchAmount);
                for(size_t ch=0;ch<2;++ch) {if(settings.spaceRoute==0)wet[ch]+=settings.spaceMix*tail[ch];else loopInput[ch]=(1-settings.spaceMix)*echo[ch]+settings.spaceMix*tail[ch];}
            }
            const double frozen=freezeWeight.next();
            std::array<double,2> held{hold.ago(0,freezeLength-1),hold.ago(1,freezeLength-1)};
            if(releaseRemaining>0)
            {
                for(size_t ch=0;ch<2;++ch)
                {const double released=(1-frozen)*settings.feedback*held[ch];wet[ch]+=released;loopInput[ch]+=released;}
                --releaseRemaining;
            }
            loopInput=color(loopInput);
            for(auto& x:loopInput)x*=1-frozen;
            if(feedbackShift!=0)
            {float a=static_cast<float>(loopInput[0]),b=static_cast<float>(loopInput[1]),oa=0,ob=0;
                const float* fi[]{&a,&b};float* fo[]{&oa,&ob};feedbackPitch.process(fi,fo,2,1);loopInput={oa,ob};}
            for(int b=0;b<bands;++b)feedback[static_cast<size_t>(b)].put(loopInput);
            for(int ch=0;ch<2;++ch)wet[static_cast<size_t>(ch)]=(1-frozen)*wet[static_cast<size_t>(ch)]+frozen*held[static_cast<size_t>(ch)];
            hold.put(wet);
            const double peak=std::max(std::abs(static_cast<double>(audio[0][start+i])),std::abs(static_cast<double>(audio[channels>1?1:0][start+i])));
            duckEnvelope+=(peak>duckEnvelope?duckAttack:duckRelease)*(peak-duckEnvelope);
            const double duckTarget=1/(1+12*settings.duck*duckEnvelope);duckGain+=(duckTarget>duckGain?duckRelease:duckAttack)*(duckTarget-duckGain);
            for(int ch=0;ch<2;++ch)
            {if(settings.lowCut>10)wet[static_cast<size_t>(ch)]-=outputLow[static_cast<size_t>(ch)].low(wet[static_cast<size_t>(ch)],postLow);
                if(settings.highCut<20000)wet[static_cast<size_t>(ch)]=outputHigh[static_cast<size_t>(ch)].low(wet[static_cast<size_t>(ch)],postHigh);}
            const double mid=.5*(wet[0]+wet[1]),side=.5*(wet[0]-wet[1])*settings.width;
            wet={(mid+side)*duckGain,(mid-side)*duckGain};
            const double d=dryWeight.next(),w=wetWeight.next()*outputGain.next();
            for(int ch=0;ch<channels;++ch)
            {const double y=(wetOnly?0:d*audio[ch][start+i])+w*(channels==1?.5*(wet[0]+wet[1]):wet[static_cast<size_t>(ch)]);
                audio[ch][start+i]=static_cast<float>(std::isfinite(y)?y:0);energy+=y*y;}
        }
    }
    if(samples>0)meter.store(static_cast<float>(10*std::log10(energy/(samples*channels)+1e-12)),std::memory_order_relaxed);
}
}

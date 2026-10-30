#include "Standalone/QuickControls.h"
#include <algorithm>
#include <cmath>

namespace hpeq::standalone
{
namespace
{
juce::NormalisableRange<float> range(int i) noexcept
{
    const auto& p=scoreParameterSpecs()[static_cast<size_t>(i)];
    return {p.minimum,p.maximum,p.interval,p.skew};
}
float bounded(float v) noexcept {return std::isfinite(v)?juce::jlimit(0.f,1.f,v):0.f;}
// Legacy choice indices stay intact; the beginner knob follows duration order.
constexpr int divisions[]{1,2,4,3,5,7,6,8,9,10,11};
}
float QuickRecipe::actual(int i) const noexcept
{
    const auto r=range(i);
    return r.snapToLegalValue(r.convertFrom0to1(normalised[static_cast<size_t>(i)]));
}
double quickMilliseconds(float time) noexcept {return std::pow(2000.,bounded(time));}
int quickDivision(float time) noexcept {return divisions[juce::roundToInt(10*bounded(time))];}
float quickTimeForDivision(int division) noexcept
{
    for(int i=0;i<11;++i)if(divisions[i]==division)return static_cast<float>(i)/10;
    return .5f;
}
const char* quickName(int style) noexcept
{
    constexpr const char* names[]{"ECHO","TAPE","BOUNCE","NOTE FALL","NOTE RISE","REVERSE","TEXTURE","CLOUD"};
    return names[juce::jlimit(0,7,style)];
}
const char* quickDescription(int style) noexcept
{
    constexpr const char* descriptions[]{
        "Clear repeats. MOTION adds reflections and feedback.",
        "Dark, driven repeats with a loose rhythmic feel.",
        "Alternating left / right reflections and a wider field.",
        "Descending note intervals, then a pitch step on every loop.",
        "Ascending note intervals, then a pitch step on every loop.",
        "Captured backwards echoes. Fragment capture adds to the delay.",
        "Chopped reflections, spectral timing and repeated space fragments.",
        "Soft, widely spaced echoes with a cloud of reversed space."};
    return descriptions[juce::jlimit(0,7,style)];
}
QuickRecipe quickRecipe(int style,float time,float motion,float space,bool sync) noexcept
{
    using namespace scoreparam;
    QuickRecipe r;style=juce::jlimit(0,7,style);time=bounded(time);motion=bounded(motion);space=bounded(space);
    const auto set=[&](int i,float value)
    {
        const auto rg=range(i);
        r.normalised[static_cast<size_t>(i)]=rg.convertTo0to1(rg.snapToLegalValue(value));
        r.applies[static_cast<size_t>(i)]=true;
    };
    for(int i=0;i<BaseCount;++i)set(i,scoreParameterSpecs()[static_cast<size_t>(i)].defaultValue);
    for(int i:{Mix,Output,Routing,MixLock,Freeze,Octaves})r.applies[static_cast<size_t>(i)]=false;
    const auto ms=static_cast<float>(quickMilliseconds(time));const int division=quickDivision(time);
    set(Mode,1);set(Source,60);set(Width,style==2?1.4f:1);set(Duck,.18f*motion);
    set(Feedback,.68f*motion);set(FeedbackMs,juce::jlimit(20.f,4000.f,ms));set(FeedbackSync,sync?static_cast<float>(division):0);
    set(FeedbackPitch,(style==3?-1:style==4?1:0)*static_cast<float>(juce::roundToInt(12*motion)));
    set(Color,style==1?2.f:0.f);set(Drive,style==1?9*motion:0);set(LowCut,style==1?90.f:10.f);
    set(HighCut,style==1?12000-8500*motion:20000);set(Swing,style==1?.22f*motion:0);
    set(Bands,style==6?1.f:0.f);set(SplitLow,250);set(SplitHigh,3000);
    for(int b=0;b<3;++b)
    {
        set(bandIndex(b,BandTime),style==6?static_cast<float>(b==0?1:b==1?1.5:2):1);
        set(bandIndex(b,BandFeedback),style==6?.25f*motion:0);
        set(bandIndex(b,BandGain),0);set(bandIndex(b,BandPan),style==6?static_cast<float>(b-1)*.5f*motion:0);
        set(bandIndex(b,BandEnabled),1);
    }
    set(SpaceMix,.8f*space);set(SpaceDecay,.1f+space*(style==7?11.9f:5.9f));
    set(SpaceSize,style==7?1.6f:1);set(SpaceDamping,style==1?4000.f:style==7?6500.f:12000.f);
    set(SpaceType,style==7?2:style==6?1.f:0.f);set(SpaceRoute,0);
    set(GlitchTarget,1);set(Glitch,style==6?1.f:style==7?2.f:0.f);set(GlitchMs,100+300*time);
    set(GlitchAmount,(style==6||style==7)?.55f*motion:0);
    const int count=1+juce::jmin(3,static_cast<int>(motion*4));
    for(int t=0;t<8;++t)
    {
        set(tapIndex(t,On),t<count?1.f:0.f);
        const int intervals[]{0,7,12,24};const int interval=t<4?intervals[t]:0;
        set(tapIndex(t,Note),static_cast<float>(60+(style==3?-interval:style==4?interval:0)));
        set(tapIndex(t,Time),juce::jlimit(1.f,4000.f,ms*static_cast<float>(1+t)));
        set(tapIndex(t,Sync),sync?static_cast<float>(quickDivision(juce::jmin(1.f,time+.2f*static_cast<float>(t)))):0);
        set(tapIndex(t,Gain),static_cast<float>(-6-4*t));set(tapIndex(t,Pan),t<count&&count>1?(t%2==0?-.75f:.75f)*motion:0);
        set(tapIndex(t,Shape),style==5?1.f:style==6?2.f:0.f);set(tapIndex(t,Fragment),50+350*time);
        set(tapIndex(t,Repeats),static_cast<float>(2+juce::roundToInt(6*motion)));
        set(tapIndex(t,Low),10);set(tapIndex(t,High),20000);set(SpaceSend+t,1);
    }
    return r;
}
}

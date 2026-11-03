#include "Standalone/ModernParameters.h"
#include <algorithm>
#include <cmath>
namespace hpeq::standalone::modern
{
namespace
{
ParameterSpec number(const char* id,const char* name,float lo,float hi,float value,float interval=.01f,float skew=1,const char* unit="")
{ParameterSpec p;p.id=id;p.name=name;p.minimum=lo;p.maximum=hi;p.defaultValue=value;p.interval=interval;p.skew=skew;p.unit=unit;return p;}
ParameterSpec choice(const char* id,const char* name,int value,std::initializer_list<const char*> labels)
{auto p=number(id,name,0,static_cast<float>(labels.size()-1),static_cast<float>(value),1);for(const auto* s:labels)p.choices.add(s);return p;}
std::vector<ParameterSpec> make()
{
    std::vector<ParameterSpec> p{
        choice("v2_engine","Engine",1,{"Legacy","Clean","Drum","Duck","Rhythm"}),
        choice("v2_enabled","Modern Processing",0,{"Off","On"}),
        choice("v2_profile","Profile",0,{"Soft","Hard"}),
        choice("v2_external","Key Source",0,{"Main","External"}),
        choice("v2_expand","Clean Shape",0,{"Gate","Expand"}),
        choice("v2_rms","Detector",0,{"Peak","RMS"}),
        choice("v2_sync","Rhythm Clock",1,{"Free","Tempo"}),
        number("v2_threshold","Modern Threshold",-90,0,-40,.01f,1,"dB"),
        number("v2_range","Modern Range",0,90,40,.01f,1,"dB"),
        number("v2_ratio","Modern Ratio",1,100,4,.01f,.5f,":1"),
        number("v2_knee","Knee",0,24,6,.01f,1,"dB"),
        number("v2_hysteresis","Modern Hysteresis",0,24,4,.01f,1,"dB"),
        number("v2_attack","Modern Attack",.01f,200,1,.01f,.35f,"ms"),
        number("v2_hold","Modern Hold",0,2000,20,.01f,.5f,"ms"),
        number("v2_release","Modern Release",.01f,5000,100,.01f,.35f,"ms"),
        number("v2_detectorMs","Detector Time",0,100,10,.01f,.5f,"ms"),
        number("v2_lookahead","Modern Lookahead",0,5,2,.01f,1,"ms"),
        number("v2_hp","Modern Key HP",0,20000,0,1,.4f,"Hz"),
        number("v2_lp","Modern Key LP",0,20000,0,1,.4f,"Hz"),
        number("v2_link","Stereo Link",0,1,1,.001f),
        number("v2_mix","Modern Mix",0,1,1,.001f),
        number("v2_output","Modern Output",-24,12,0,.01f,1,"dB"),
        number("v2_body","Drum Body",0,2000,20,.01f,.5f,"ms"),
        number("v2_tail","Drum Tail",.01f,5000,100,.01f,.35f,"ms"),
        number("v2_onset","Onset Contrast",0,24,3,.01f,1,"dB"),
        number("v2_retrigger","Retrigger Gap",0,200,10,.01f,.5f,"ms"),
        number("v2_steps","Pattern Steps",16,32,16,1),
        number("v2_beats","Step Beats",1.f/64,16,.25f,1.f/64,.5f),
        number("v2_ms","Step Time",1,10000,125,.01f,.35f,"ms")};
    for(int n=0;n<32;++n){auto step=number("","",0,1,1,.001f);step.id="v2_step"+juce::String(n+1);step.name="Step "+juce::String(n+1);p.push_back(step);}
    p.push_back(choice("v2_listen","Listen Key",0,{"Off","On"}));
    jassert(p.size()==Count);return p;
}
}
const std::vector<ParameterSpec>& specs(){static const auto p=make();return p;}
void append(juce::AudioProcessorValueTreeState::ParameterLayout& layout)
{
    for(const auto& p:specs())
    {
        if(!p.choices.isEmpty())layout.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{p.id,1},p.name,p.choices,juce::roundToInt(p.defaultValue)));
        else layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{p.id,1},p.name,juce::NormalisableRange<float>(p.minimum,p.maximum,p.interval,p.skew),p.defaultValue,juce::AudioParameterFloatAttributes{}.withLabel(p.unit)));
    }
}
int engine(const Raw& raw) noexcept{return std::clamp(static_cast<int>(raw[Engine]->load(std::memory_order_relaxed)),0,4);}
hamlo::zastava::Settings settings(const Raw& raw,bool bypass,int modeOverride) noexcept
{
    // APVTS constructs its raw defaults without snapping. Canonicalize only the
    // new controls to their published grid, identically for DSP and state.
    using S=hamlo::zastava::Settings;const auto v=[&](size_t n)
    {
        const auto& p=specs()[n];float value=raw[n]->load(std::memory_order_relaxed);
        value=p.minimum+p.interval*std::floor((value-p.minimum)/p.interval+.5f);
        return static_cast<double>(std::clamp(value,p.minimum,p.maximum));
    };S s;
    s.mode=static_cast<hamlo::zastava::Mode>(modeOverride>=0?modeOverride:std::max(0,engine(raw)-1));s.profile=static_cast<hamlo::zastava::Profile>(static_cast<int>(v(Profile)));
    s.enabled=v(Enabled)>.5;s.bypass=bypass;s.external=v(External)>.5;s.expand=v(Expand)>.5;s.rms=v(Rms)>.5;s.sync=v(Sync)>.5;
    s.thresholdDb=v(Threshold);s.rangeDb=v(Range);s.ratio=v(Ratio);s.kneeDb=v(Knee);s.hysteresisDb=v(Hysteresis);
    s.attackMs=v(Attack);s.holdMs=v(Hold);s.releaseMs=v(Release);if(s.mode==hamlo::zastava::Mode::Duck)std::swap(s.attackMs,s.releaseMs);
    s.detectorMs=v(Detector);s.lookaheadMs=v(Lookahead);s.keyHpHz=v(Hp);s.keyLpHz=v(Lp);s.stereoLink=v(Link);s.mix=v(Mix);s.outputDb=v(Output);
    s.bodyMs=v(Body);s.tailMs=v(Tail);s.onsetDb=v(Onset);s.retriggerMs=v(Retrigger);s.steps=static_cast<int>(v(Steps));s.stepBeats=v(Beats);s.stepMs=v(Ms);
    for(size_t n=0;n<32;++n)s.levels[n]=v(Step0+n);s.listen=v(Listen)>.5;return s;
}
}

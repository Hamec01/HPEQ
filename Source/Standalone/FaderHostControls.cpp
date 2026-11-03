#include "Standalone/FaderHostControls.h"
namespace hpeq::standalone
{
const std::array<FaderControlSpec,ControlCount>& faderControls()
{
    static const auto result=[]
    {
        std::array<FaderControlSpec,ControlCount> a{};
        auto number=[&](FaderControl i,const char* id,const char* name,float lo,float hi,float initial,const char* unit="",float step=0)
        {a[i]={id,name,unit,{},lo,hi,initial,step};};
        auto choice=[&](FaderControl i,const char* id,const char* name,const char* options,float initial=0)
        {a[i]={id,name,{},options,0,0,initial,1};};
        choice(Engine,"engine","Engine","Legacy|Modern",1);
        choice(Mode,"mode","Mode","Trim|Match|Ride|Motion");
        choice(Enabled,"enabled","Enabled","Off|On",1);
        number(Input,"inputDb","Input Trim",-60,24,0,"dB");
        number(Output,"outputDb","Output Trim",-60,24,0,"dB");
        number(Left,"leftDb","Left Trim",-24,24,0,"dB");
        number(Right,"rightDb","Right Trim",-24,24,0,"dB");
        number(LeftDelay,"leftDelayMs","Left Alignment",0,20,0,"ms");
        number(RightDelay,"rightDelayMs","Right Alignment",0,20,0,"ms");
        choice(Routing,"routing","Routing","Stereo|Mono Sum|Left to Both|Right to Both");
        choice(Swap,"swap","Swap Channels","Off|On");
        choice(Measure,"measure","Measurement","RMS|LUFS");
        number(MatchBoost,"matchBoost","Match Max Boost",0,24,12,"dB");
        number(MatchCut,"matchCut","Match Max Cut",0,60,24,"dB");
        number(Ceiling,"matchCeiling","Match Peak Ceiling",-24,0,-1,"dBTP");
        number(Matched,"matchedDb","Applied Match Gain",-60,24,0,"dB");
        choice(Guard,"peakGuard","Match Peak Guard","Off|On",1);
        choice(Profile,"profile","Ride Profile","Natural|Fast");
        number(Strength,"strength","Ride Strength",0,1,1);
        number(Boost,"maxBoost","Ride Max Boost",0,24,6,"dB");
        number(Cut,"maxCut","Ride Max Cut",0,60,12,"dB");
        number(Floor,"floorDb","Ride Activity Floor",-90,-20,-55,"dBFS");
        number(Hold,"holdMs","Ride Hold",0,1000,80,"ms");
        number(Idle,"idleMs","Ride Idle Return",1,2000,100,"ms");
        number(Rise,"riseMs","Ride Rise",.1f,5000,250,"ms");
        number(Fall,"fallMs","Ride Fall",.1f,5000,120,"ms");
        choice(Shape,"shape","Motion Shape","Custom|Sine|Triangle");
        number(Points,"points","Motion Points",2,32,16,"",1);
        choice(Smooth,"smooth","Motion Curve","Linear|Smooth",1);
        choice(Sync,"sync","Motion Clock","Free|Sync",1);
        number(Depth,"depthDb","Motion Depth",0,90,0,"dB");
        number(Beats,"beats","Motion Length",.125f,32,1,"beats");
        number(Free,"freeMs","Motion Free Period",10,60000,500,"ms");
        number(Phase,"phase","Motion Phase",0,1,0,"cycles");
        number(RightPhase,"rightPhase","Motion Right Offset",0,1,0,"cycles");
        number(Edge,"edgeMs","Motion Edge",.1f,50,1,"ms");
        for(size_t n=0;n<32;++n)
            a[Level0+n]={"level"+juce::String(static_cast<int>(n)).paddedLeft('0',2),"Motion Point "+juce::String(static_cast<int>(n+1)),{},{},0,1,1,0};
        return a;
    }();
    return result;
}
void appendFaderControls(juce::AudioProcessorValueTreeState::ParameterLayout& layout)
{
    for(const auto& p:faderControls())
    {
        const juce::ParameterID id{p.id,1};
        if(p.choices.isNotEmpty())
            layout.add(std::make_unique<juce::AudioParameterChoice>(id,p.name,juce::StringArray::fromTokens(p.choices,"|",""),static_cast<int>(p.initial)));
        else
            layout.add(std::make_unique<juce::AudioParameterFloat>(id,p.name,juce::NormalisableRange<float>{p.minimum,p.maximum,p.interval},p.initial,juce::AudioParameterFloatAttributes{}.withLabel(p.unit)));
    }
}
void FaderHostControls::bind(juce::AudioProcessorValueTreeState& state)
{
    const auto& specs=faderControls();for(size_t i=0;i<raw.size();++i){raw[i]=state.getRawParameterValue(specs[i].id);jassert(raw[i]);}
}
hamlo::fader::Settings FaderHostControls::read(double gain,double mix,double target,int polarity,bool mute,bool bypass) const noexcept
{
    using namespace hamlo::fader;Settings s;
    s.mode=static_cast<hamlo::fader::Mode>(static_cast<int>(value(Mode)));
    s.enabled=value(Enabled)>.5f&&!mute;s.bypass=bypass;
    s.gainDb=gain;s.mix=mix;s.targetDb=target;s.polarity=polarity;
    s.inputDb=value(Input);s.outputDb=value(Output);s.leftDb=value(Left);s.rightDb=value(Right);
    s.leftDelayMs=value(LeftDelay);s.rightDelayMs=value(RightDelay);
    s.routing=static_cast<hamlo::fader::Routing>(static_cast<int>(value(Routing)));s.swap=value(Swap)>.5f;
    s.measure=static_cast<hamlo::fader::Measure>(static_cast<int>(value(Measure)));
    s.matchBoost=value(MatchBoost);s.matchCut=value(MatchCut);s.matchCeiling=value(Ceiling);s.matchedDb=value(Matched);s.peakGuard=value(Guard)>.5f;
    s.profile=static_cast<hamlo::fader::Profile>(static_cast<int>(value(Profile)));s.strength=value(Strength);
    s.maxBoost=value(Boost);s.maxCut=value(Cut);s.floorDb=value(Floor);s.holdMs=value(Hold);s.idleMs=value(Idle);s.riseMs=value(Rise);s.fallMs=value(Fall);
    s.shape=static_cast<hamlo::fader::Shape>(static_cast<int>(value(Shape)));s.points=static_cast<int>(value(Points));
    s.smooth=value(Smooth)>.5f;s.sync=value(Sync)>.5f;s.depthDb=value(Depth);s.beats=value(Beats);s.freeMs=value(Free);
    s.phase=value(Phase);s.rightPhase=value(RightPhase);s.edgeMs=value(Edge);
    for(size_t n=0;n<32;++n)s.levels[n]=value(static_cast<FaderControl>(Level0+n));
    return s;
}
}

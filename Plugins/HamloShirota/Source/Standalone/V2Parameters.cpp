#include "Standalone/V2Parameters.h"
namespace hpeq::standalone
{
const std::vector<ParameterSpec>& v2Parameters()
{
    static const auto specs=[]
    {
        std::vector<ParameterSpec> v;
        auto knob=[&](const char* id,const char* name,float lo,float hi,float value,const char* unit="",float skew=1.f)
        {ParameterSpec p;p.id=id;p.name=name;p.minimum=lo;p.maximum=hi;p.defaultValue=value;p.unit=unit;p.skew=skew;v.push_back(p);};
        auto choice=[&](const char* id,const char* name,int value,std::initializer_list<const char*> labels)
        {ParameterSpec p;p.id=id;p.name=name;p.defaultValue=static_cast<float>(value);p.minimum=0;p.maximum=static_cast<float>(labels.size()-1);p.interval=1;for(auto l:labels)p.choices.add(l);v.push_back(p);};
        choice("v2Mode","Engine",0,{"Legacy","Stereo / Space","Binaural"});
        choice("v2Page","Editor Page",4,{"QUICK","STEREO","SPACE","MOTION","LEGACY"});
        knob("v2Width","Width",0,4,1,"x");knob("v2LowWidth","Low Width",0,4,1,"x");knob("v2MidWidth","Mid Width",0,4,1,"x");knob("v2HighWidth","High Width",0,4,1,"x");
        knob("v2LowSplit","Low Crossover",60,1500,300,"Hz",.5f);knob("v2HighSplit","High Crossover",1500,12000,3000,"Hz",.5f);
        knob("v2MonoBass","Mono Bass",0,500,0,"Hz",.5f);choice("v2Generator","Mono Generator",0,{"Phase","Micro","Diffuse"});knob("v2Generate","Generate",0,1.5f,0);
        choice("v2Guard","Guard",0,{"Off","On"});knob("v2Floor","Guard Budget",0,.95f,0);
        choice("v2SpaceType","Space Character",0,{"Tight","Open","Scatter"});knob("v2Space","Space Amount",0,1,0);
        knob("v2Size","Size",.5f,2,1,"x");knob("v2Depth","Depth",0,1,.5f);knob("v2Damping","Damping",500,18000,9000,"Hz",.5f);knob("v2Predelay","Predelay",0,60,0,"ms");
        knob("v2Position","Position",-180,180,0,"deg");knob("v2Spread","Source Spread",0,180,180,"deg");knob("v2Balance","Channel Balance",-1,1,0);
        choice("v2Source","Source Model",0,{"Stereo Spread","Point / Mid"});choice("v2Profile","Headphone Profile",0,{"Profile 1 / D1","Profile 2 / H3","Profile 3 / H4"});
        choice("v2Motion","Motion",0,{"Off","On"});choice("v2Unit","Time Unit",0,{"Milliseconds","Quarter-note beats"});choice("v2Loop","Loop",1,{"One shot","Loop"});
        knob("v2Rate","Travel Rate",.25f,4,1,"x");
        knob("v2Nodes","Path Nodes",1,16,2);v.back().interval=1;
        knob("v2Mix","Spatial Mix",0,1,1);knob("v2Output","Spatial Output",-24,12,0,"dB");choice("v2Mono","Mono Audition",0,{"Stereo","Mono"});
        for(int i=0;i<16;++i)
        {
            const auto prefix="v2Node"+juce::String(i);const auto title="Node "+juce::String(i+1)+" ";
            knob((prefix+"Angle").toRawUTF8(),(title+"Position").toRawUTF8(),-180,180,i==0?-90.f:90.f,"deg");
            knob((prefix+"Hold").toRawUTF8(),(title+"Hold").toRawUTF8(),0,60000,250,"ms");
            knob((prefix+"Travel").toRawUTF8(),(title+"Travel").toRawUTF8(),0,60000,750,"ms");
            knob((prefix+"HoldBeat").toRawUTF8(),(title+"Hold Beats").toRawUTF8(),0,64,.5f,"beat");
            knob((prefix+"TravelBeat").toRawUTF8(),(title+"Travel Beats").toRawUTF8(),0,64,1.5f,"beat");
            choice((prefix+"Arc").toRawUTF8(),(title+"Arc").toRawUTF8(),0,{"Shortest","Clockwise","Counterclockwise"});
            choice((prefix+"Curve").toRawUTF8(),(title+"Curve").toRawUTF8(),1,{"Linear","Smooth"});
        }choice("v2PositionOn","Position / Headphones",0,{"Off","On"});choice("v2Restart","Restart Path",0,{"A","B"});knob("v2EditNode","Editor Node",1,16,1);v.back().interval=1;return v;
    }();return specs;
}
void addV2Parameters(juce::AudioProcessorValueTreeState::ParameterLayout& layout)
{
    for(const auto& p:v2Parameters())
    {
        const juce::ParameterID id{p.id,1};
        if(p.choices.isEmpty())layout.add(std::make_unique<juce::AudioParameterFloat>(id,p.name,juce::NormalisableRange<float>(p.minimum,p.maximum,p.interval,p.skew),p.defaultValue,juce::AudioParameterFloatAttributes{}.withLabel(p.unit)));
        else layout.add(std::make_unique<juce::AudioParameterChoice>(id,p.name,p.choices,juce::roundToInt(p.defaultValue)));
    }
}
}

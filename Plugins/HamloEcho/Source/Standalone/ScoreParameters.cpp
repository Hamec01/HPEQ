#include "Standalone/ScoreParameters.h"
#include "Standalone/QuickControls.h"

namespace hpeq::standalone
{
const std::vector<ParameterSpec>& scoreParameterSpecs()
{
    static const auto specs = []
    {
        std::vector<ParameterSpec> out;
        const auto add = [&](juce::String id, juce::String name, float lo, float hi, float value, float step = 0, const char* unit = "")
        {
            ParameterSpec p; p.id = id; p.name = name; p.minimum = lo; p.maximum = hi;
            p.defaultValue = value; p.interval = step; p.unit = unit; p.slotValueIndex = -2; out.push_back(p);
        };
        const auto choice = [&](juce::String id, juce::String name, int value, std::initializer_list<const char*> labels)
        {
            add(id, name, 0, static_cast<float>(labels.size()-1), static_cast<float>(value), 1);
            for (auto* text : labels) out.back().choices.add(text);
        };
        const auto sync = [&](juce::String id, juce::String name)
        { choice(id, name, 0, {"Free","1/32","1/16","1/16 D","1/8 T","1/8","1/8 D","1/4 T","1/4","1/4 D","1/2","1/1"}); };
        choice("echoMode","Engine",0,{"Legacy","Score"});
        add("scoreSource","Source Note",0,127,60,1);
        choice("scoreOctaves","Note Labels",0,{"FL Studio: C5=60","Standard: C4=60"});
        add("scoreMix","Score Mix",0,1,.35f); add("scoreOutput","Wet Output",-24,12,0,0,"dB");
        choice("scoreRouting","Routing",0,{"Insert","Send"}); choice("scoreMixLock","Mix Lock",0,{"Off","On"});
        add("scoreWidth","Score Width",0,1.5f,1); add("scoreDuck","Score Duck",0,1,0);
        choice("scoreFreeze","Score Freeze",0,{"Off","On"});
        add("scoreFeedback","Loop Feedback",0,.9f,.3f); add("scoreFeedbackMs","Loop Time",20,4000,500,0,"ms");
        sync("scoreFeedbackSync","Loop Sync"); add("scoreFeedbackPitch","Loop Pitch Step",-12,12,0,1,"st");
        choice("scoreColor","Loop Character",0,{"Clean","Warm","Tape","BBD","Dust"});
        add("scoreDrive","Loop Drive",0,18,0,0,"dB"); add("scoreLowCut","Score Low Cut",10,2000,10,0,"Hz");
        add("scoreHighCut","Score High Cut",500,20000,20000,0,"Hz");
        choice("scoreBands","Spectral Bands",0,{"Off","On"}); add("scoreSplitLow","Low Split",60,1500,250,0,"Hz");
        add("scoreSplitHigh","High Split",1500,12000,3000,0,"Hz");
        add("scoreSpaceMix","Space Amount",0,1,.25f); add("scoreSpaceDecay","Space Decay",.1f,20,2,0,"s");
        add("scoreSpaceSize","Space Size",.25f,2,1); add("scoreSpaceDamping","Space Damping",500,18000,6000,0,"Hz");
        choice("scoreSpaceType","Space Character",0,{"Room","Plate","Cloud"});
        choice("scoreSpaceRoute","Space Routing",0,{"After Echo","In Feedback","Before Echo"});
        choice("scoreGlitchTarget","Glitch Target",1,{"Echoes","Space Tail","Echoes + Tail"});
        choice("scoreGlitch","Glitch",0,{"Off","Repeat","Reverse","Stutter","Tape Stop","Omit"});
        add("scoreGlitchMs","Glitch Fragment",25,2000,125,0,"ms"); add("scoreGlitchAmount","Glitch Amount",0,1,0);
        for (int b = 0; b < 3; ++b)
        {
            const auto id = "band" + juce::String(b+1);
            const auto name = juce::String(b == 0 ? "Low " : b == 1 ? "Mid " : "High ");
            add(id+"Time",name+"Time Scale",.25f,4,1); add(id+"Feedback",name+"Feedback",0,.9f,.25f);
            add(id+"Gain",name+"Level",-24,6,0,0,"dB"); add(id+"Pan",name+"Pan",-1,1,0);
            choice(id+"On",name+"Enabled",1,{"Off","On"});
        }
        const int notes[] {48,55,60,36,60,60,60,60};
        for (int t = 0; t < 8; ++t)
        {
            const auto id = "tap" + juce::String(t+1), name = "Reflection " + juce::String(t+1) + " ";
            choice(id+"On",name+"Enabled",t<4?1:0,{"Off","On"}); add(id+"Note",name+"Note",0,127,static_cast<float>(notes[t]),1);
            add(id+"Time",name+"Time",1,4000,static_cast<float>(250*(t+1)),0,"ms"); sync(id+"Sync",name+"Sync");
            add(id+"Gain",name+"Level",-48,0,-12,0,"dB"); add(id+"Pan",name+"Pan",-1,1,0);
            choice(id+"Shape",name+"Action",0,{"Forward","Reverse","Stutter","Tape Stop","Skip"});
            add(id+"Fragment",name+"Fragment",50,2000,125,0,"ms"); add(id+"Repeats",name+"Repeats",2,8,4,1);
            add(id+"Low",name+"Low Cut",10,2000,10,0,"Hz"); add(id+"High",name+"High Cut",500,20000,20000,0,"Hz");
        }
        add("scoreSwing","Score Swing",0,.75f,0);
        for(int t=0;t<8;++t)add("tap"+juce::String(t+1)+"SpaceSend","Reflection "+juce::String(t+1)+" Space Send",0,1,1);
        choice("quickEnabled","Quick Enabled",0,{"Off","On"});
        choice("quickStyle","Quick Move",0,{"Echo","Tape","Bounce","Note Fall","Note Rise","Reverse","Texture","Cloud"});
        add("quickTime","Time",0,1,.5f);add("quickMotion","Motion",0,1,0);add("quickSpace","Space",0,1,0);
        choice("quickSync","Quick Timing",1,{"Milliseconds","Host Sync"});
        jassert(out.size() == scoreparam::Count);
        return out;
    }();
    return specs;
}

hamlo::echo::ScoreSettings readScoreSettings(const std::vector<std::atomic<float>*>& values, double bpm) noexcept
{
    using namespace scoreparam;
    const auto raw = [&](int i) { const float value=values[static_cast<size_t>(i)]->load(std::memory_order_relaxed);return std::isfinite(value)?value:scoreParameterSpecs()[static_cast<size_t>(i)].defaultValue; };
    const bool quick=raw(QuickEnabled)>.5f;
    const auto recipe=quick?quickRecipe(juce::roundToInt(raw(QuickStyle)),raw(QuickTime),raw(QuickMotion),raw(QuickSpace),raw(QuickSync)>.5f):QuickRecipe{};
    const auto v = [&](int i)->double {return quick&&recipe.applies[static_cast<size_t>(i)]?recipe.actual(i):raw(i);};
    hamlo::echo::ScoreSettings s;
    s.sourceNote=static_cast<int>(v(Source)); s.bpm=bpm; s.mix=v(Mix); s.outputDb=v(Output); s.routing=static_cast<int>(v(Routing));
    s.width=v(Width); s.duck=v(Duck); s.freeze=v(Freeze)>.5; s.feedback=v(Feedback); s.feedbackMs=v(FeedbackMs);
    s.feedbackSync=static_cast<int>(v(FeedbackSync)); s.feedbackPitch=static_cast<int>(v(FeedbackPitch)); s.color=static_cast<int>(v(Color));
    s.driveDb=v(Drive); s.lowCut=v(LowCut); s.highCut=v(HighCut); s.bands=v(Bands)>.5; s.splitLow=v(SplitLow); s.splitHigh=v(SplitHigh);
    s.spaceMix=v(SpaceMix); s.spaceDecay=v(SpaceDecay); s.spaceSize=v(SpaceSize); s.spaceDamping=v(SpaceDamping);
    s.spaceType=static_cast<int>(v(SpaceType)); s.spaceRoute=static_cast<int>(v(SpaceRoute));
    s.glitch=static_cast<int>(v(Glitch)); s.glitchTarget=static_cast<int>(v(GlitchTarget)); s.glitchMs=v(GlitchMs); s.glitchAmount=v(GlitchAmount);s.swing=v(Swing);
    for(int b=0;b<3;++b) { auto& p=s.band[static_cast<size_t>(b)]; p.timeScale=v(bandIndex(b,BandTime)); p.feedback=v(bandIndex(b,BandFeedback)); p.gainDb=v(bandIndex(b,BandGain)); p.pan=v(bandIndex(b,BandPan));p.enabled=v(bandIndex(b,BandEnabled))>.5; }
    for(int t=0;t<8;++t) { auto& p=s.taps[static_cast<size_t>(t)]; const auto a=[&](Tap f){return v(tapIndex(t,f));};
        p.enabled=a(On)>.5; p.note=static_cast<int>(a(Note)); p.timeMs=a(Time); p.sync=static_cast<int>(a(Sync)); p.gainDb=a(Gain); p.pan=a(Pan);
        p.action=static_cast<hamlo::echo::Action>(static_cast<int>(a(Shape))); p.fragmentMs=a(Fragment); p.repeats=a(Repeats); p.lowCut=a(Low); p.highCut=a(High);p.spaceSend=v(SpaceSend+t); }
    return s;
}

juce::String noteName(int midi, bool fl)
{
    constexpr const char* names[]{"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};
    midi=juce::jlimit(0,127,midi); return juce::String(names[midi%12])+juce::String(midi/12-(fl?0:1));
}
int parseNote(const juce::String& text, bool fl)
{
    auto s=text.trim().toUpperCase();
    if(s.isEmpty()) return -1;
    if(s.containsOnly("0123456789")) { const int n=s.getIntValue(); return n>=0&&n<=127?n:-1; }
    const juce::String letters="C D EF G A B";
    const int base=letters.indexOfChar(s[0]); if(base<0||s[0]==' ') return -1;
    int offset=1, pitch=base;
    if(s.length()>offset && (s[offset]=='#'||s[offset]=='B')) { pitch+=s[offset]=='#'?1:-1; ++offset; }
    auto oct=s.substring(offset).trim(); if(fl&&oct.startsWithChar('-')) oct=oct.substring(1);
    if(oct.isEmpty()||!oct.containsOnly("-0123456789")) return -1;
    const int midi=(oct.getIntValue()+(fl?0:1))*12+pitch;
    return midi>=0&&midi<=127?midi:-1;
}
}

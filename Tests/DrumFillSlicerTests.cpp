#include "DSP/TransitionRender.h"
#include "Standalone/ModulePluginEditor.h"
#include "Standalone/ModulePluginProcessor.h"
#include "UI/HamloRevolutionTransitionPage.h"
#include "TestSignals.h"
#include <set>
#include <map>
#include <limits>
namespace hamlo::test
{
namespace tr=hpeq::dsp::transition;
namespace
{
bool same(const juce::AudioBuffer<float>& a,const juce::AudioBuffer<float>& b)
{if(a.getNumSamples()!=b.getNumSamples()||a.getNumChannels()!=b.getNumChannels())return false;for(int c=0;c<a.getNumChannels();++c)for(int n=0;n<a.getNumSamples();++n)if(a.getSample(c,n)!=b.getSample(c,n))return false;return true;}
double difference(const juce::AudioBuffer<float>& a,const juce::AudioBuffer<float>& b)
{double result=0;for(int c=0;c<a.getNumChannels();++c)for(int n=0;n<a.getNumSamples();++n){const double d=a.getSample(c,n)-b.getSample(c,n);result+=d*d;}return result;}
}
class DrumFillSlicerTests final:public juce::UnitTest
{
public:
    DrumFillSlicerTests():juce::UnitTest("Drum fill slicer R13","HamloRevolution"){}
    void runTest()override
    {
        auto source=drums(8);source.applyGain(.35f);
        tr::Settings s;s.kind=tr::Fill;s.bars=1;s.recipe=8;s.knobs=tr::defaultKnobs(tr::Fill);
        beginTest("Rhythm diversity excludes pitch, pan, reverse, effects and gain");
        for(int recipe=8;recipe<=20;++recipe)
        {
            std::set<std::uint32_t> masks;std::set<juce::String> structures;
            for(int seed=1;seed<=(recipe==8?64:32);++seed)
            {
                s.recipe=recipe;s.seed=static_cast<std::uint64_t>(seed);const auto p=tr::makePlan(source,s);std::uint32_t mask=0;juce::String signature;
                for(const auto& e:p.events){mask|=1u<<std::clamp(static_cast<int>(e.at*32LL/tr::lengthSamples(s)),0,31);signature<<e.at<<":"<<e.length<<":"<<e.source<<";";}
                masks.insert(mask);structures.insert(signature);
            }
            logMessage("recipe="+juce::String(recipe)+" masks="+juce::String(masks.size())+" plans="+juce::String(structures.size()));
            expect(masks.size()>=static_cast<size_t>(recipe==8?24:16));if(recipe==8)expect(structures.size()>=48);
        }
        beginTest("Only selected ending changes; exact length, deterministic, neutral and soft endpoint");
        s.recipe=8;s.seed=17;s.bars=2;
        for(int beats:{1,2,4})
        {
            s.fillBeats=beats;const auto p=tr::makePlan(source,s);const auto y=tr::render(source,s);expect(same(y,tr::render(source,s)));expectEquals(y.getNumSamples(),tr::lengthSamples(s));
            bool prefix=true;for(int c=0;c<2;++c)for(int n=0;n<p.phraseStart;++n)prefix=prefix&&y.getSample(c,n)==source.getSample(c,n);expect(prefix,"prefix PCM");expectEquals(y.getSample(0,y.getNumSamples()-1),0.f);
        }
        {auto neutral=s;neutral.knobs.fill(0);const auto y=tr::render(source,neutral);bool identical=true;for(int c=0;c<2;++c)for(int n=0;n<y.getNumSamples()-240;++n)identical=identical&&y.getSample(c,n)==source.getSample(c,n);expect(identical);}
        beginTest("Role analysis distinguishes separated low, body and air attacks");
        {
            juce::AudioBuffer<float> kit(2,96000);kit.clear();juce::Random random(731);
            for(int role=0;role<3;++role)for(int i=0;i<4800;++i)
            {const double t=i/48000.,tone=role==0?std::sin(2*juce::MathConstants<double>::pi*65*t):role==1?std::sin(2*juce::MathConstants<double>::pi*1200*t):std::sin(2*juce::MathConstants<double>::pi*10500*t);const float v=static_cast<float>(.3*std::exp(-t/.022)*tone);for(int c=0;c<2;++c)kit.setSample(c,role*24000+i,v);}
            const auto p=tr::makePlan(kit,s);std::set<int> roles;for(const auto& slice:p.slices)roles.insert(slice.role);expectEquals(static_cast<int>(roles.size()),3);
        }
        beginTest("Controls produce actual audio; coherent roll ramps, pitch falls, reverse and final rest");
        s.bars=1;s.fillBeats=4;s.recipe=10;s.seed=31;
        {
            const auto p=tr::makePlan(source,s);std::map<int,std::vector<double>> repeats;
            // First fixture included accented non-roll hits sharing that source,
            // so its first .95 accent incorrectly masked the actual rising roll.
            for(const auto& e:p.events)if(e.at>=72000&&e.roll)repeats[e.source].push_back(e.velocity);
            bool ramp=false;for(const auto& group:repeats)if(group.second.size()>=3&&group.second.back()>group.second.front()+.2)ramp=true;expect(ramp,"same body slice grows");
        }
        for(int knob=0;knob<4;++knob)
        {auto on=s;on.recipe=knob==2?16:13;on.knobs[static_cast<size_t>(knob)]=1;auto off=on;off.knobs[static_cast<size_t>(knob)]=0;expect(difference(tr::render(source,on),tr::render(source,off))>.001,"knob "+juce::String(knob));}
        {auto off=s;off.fillFx=0;auto on=off;on.fillFx=1;expect(difference(tr::render(source,on),tr::render(source,off))>.01);}
        {const auto p=tr::makePlan(source,s);bool room=false;for(const auto& e:p.events)if(e.space>0)room=room||(!e.roll&&e.at<=p.phraseEnd-12000);expect(room,"room accent has at least half a beat to develop");}
        {auto pitched=s;pitched.recipe=14;const auto p=tr::makePlan(source,pitched);bool falls=false;for(const auto& e:p.events)falls=falls||e.rate<.9;expect(falls);}
        {auto reverse=s;reverse.recipe=16;reverse.knobs[2]=1;const auto p=tr::makePlan(source,reverse);bool backwards=false;for(const auto& e:p.events)backwards=backwards||e.reverse;expect(backwards);}
        {auto stopped=s;stopped.knobs[3]=1;const auto p=tr::makePlan(source,stopped);const auto y=tr::render(source,stopped);expect(p.stopSample<y.getNumSamples()-4000);expectEquals(y.getMagnitude(p.stopSample,y.getNumSamples()-p.stopSample),0.f);}
        beginTest("Rates, tempi, durations, silence, mono and hostile numbers stay bounded");
        for(double fs:{44100.,48000.,96000.})for(double bpm:{70.,175.})for(int bars:{1,2,4})
        {
            auto t=s;t.sampleRate=fs;t.bpm=bpm;t.bars=bars;const auto y=tr::render(source,t);expectEquals(y.getNumSamples(),tr::lengthSamples(t));bool finite=true;for(int c=0;c<2;++c)for(int n=0;n<y.getNumSamples();++n)finite=finite&&std::isfinite(y.getSample(c,n));expect(finite);expect(y.getMagnitude(0,y.getNumSamples())<=.814f);expectEquals(y.getSample(0,y.getNumSamples()-1),0.f);
        }
        {juce::AudioBuffer<float> silent(1,1000);silent.clear();expectEquals(tr::render(silent,s).getMagnitude(0,96000),0.f);const auto mono=sine(2,440,-24,1);expectEquals(tr::render(mono,s).getNumChannels(),2);}
        {auto hostile=s;hostile.bpm=std::numeric_limits<double>::quiet_NaN();expectEquals(tr::render(source,hostile).getNumSamples(),0);hostile=s;hostile.fillFx=std::numeric_limits<double>::infinity();hostile.knobs[0]=std::numeric_limits<double>::quiet_NaN();auto bad=source;bad.setSample(0,1,std::numeric_limits<float>::infinity());expect(std::isfinite(tr::render(bad,hostile).getMagnitude(0,96000)));}
        {auto hot=source;hot.applyGain(50);const auto y=tr::render(hot,s);expect(y.getMagnitude(0,y.getNumSamples())<=.814f,"hot-source crossfade peak="+juce::String(y.getMagnitude(0,y.getNumSamples())));}
        beginTest("UI variation reuses input, keeps files immutable and restores scratch controls");
        {
            using P=hpeq::standalone::ModulePluginProcessor;auto p=std::make_unique<P>(hpeq::standalone::ModuleKind::Revolution);p->prepareToPlay(48000,256);
            const auto folder=juce::File::getSpecialLocation(juce::File::tempDirectory).getChildFile("HamloR13Tests");folder.createDirectory();p->setRendersFolder(folder);
            juce::MidiBuffer midi;
            for(int at=0;at<192000;at+=256){auto block=segment(source,at,256);p->processBlock(block,midi);}
            std::unique_ptr<juce::AudioProcessorEditor> owner(p->createEditor());auto* editor=dynamic_cast<hpeq::standalone::ModulePluginEditor*>(owner.get());auto* page=editor?editor->getTransitionPage():nullptr;expect(page!=nullptr);
            if(page)
            {
                page->chooseKind(tr::Fill);page->chooseFillBeats(2);page->chooseFillFx(.65);const auto before=p->lastTransition();juce::MemoryBlock original;before.file.loadFileAsData(original);const auto captured=p->transitionClip();
                // Replace the live capture history with a different source.
                // NEW VARIATION must still use the earlier drum capture.
                const auto other=sine(4,333,-18,2);for(int at=0;at<192000;at+=256){auto block=segment(other,at,256);p->processBlock(block,midi);}
                page->pressVariation();const auto after=p->lastTransition();expect(before.settings.seed!=after.settings.seed);expect(before.file!=after.file);expectEquals(after.settings.fillBeats,2);expectWithinAbsoluteError(after.settings.fillFx,.65,1e-12);juce::MemoryBlock unchanged;before.file.loadFileAsData(unchanged);expect(original==unchanged);
                const auto varied=p->transitionClip();bool prefix=true;for(int c=0;c<2;++c)for(int n=0;n<96000;++n)prefix=prefix&&captured.getSample(c,n)==varied.getSample(c,n);expect(prefix,"same captured prefix despite changed live history");expect(!same(captured,varied));expect(tr::resolvedRecipe(before.settings)!=tr::resolvedRecipe(after.settings));
                owner.reset();owner.reset(p->createEditor());editor=dynamic_cast<hpeq::standalone::ModulePluginEditor*>(owner.get());expectEquals(editor->getTransitionPage()->getValues().fillBeats,2);expectWithinAbsoluteError(editor->getTransitionPage()->getValues().fillFx,.65,1e-12);expectEquals(editor->getTransitionPage()->getValues().recipe,8);
            }
        }
    }
};static DrumFillSlicerTests drumFillSlicerTests;
}

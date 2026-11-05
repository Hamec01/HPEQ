#include "DSP/TransitionRender.h"
#include "Standalone/ModulePluginProcessor.h"
#include "Standalone/ModulePluginEditor.h"
#include "UI/HamloRevolutionTransitionPage.h"
#include "TestSignals.h"
#include <set>
#include <limits>
namespace hamlo::test
{
namespace tr=hpeq::dsp::transition;
namespace
{
bool identical(const juce::AudioBuffer<float>& a,const juce::AudioBuffer<float>& b)
{if(a.getNumSamples()!=b.getNumSamples()||a.getNumChannels()!=b.getNumChannels())return false;for(int c=0;c<a.getNumChannels();++c)for(int n=0;n<a.getNumSamples();++n)if(a.getSample(c,n)!=b.getSample(c,n))return false;return true;}
double amplitude(const juce::AudioBuffer<float>& x,double hz)
{double re=0,im=0;for(int n=24000;n<48000;++n){const double phase=2*juce::MathConstants<double>::pi*hz*n/48000;re+=x.getSample(0,n)*std::cos(phase);im+=x.getSample(0,n)*std::sin(phase);}return std::sqrt(re*re+im*im)/12000;}
}
class DrumComposeTests final:public juce::UnitTest
{
public:
    DrumComposeTests():juce::UnitTest("Drum composer R14","HamloRevolution"){}
    void runTest()override
    {
        auto source=drums(8);source.applyGain(.35f);tr::Settings s;s.kind=tr::Fill;s.recipe=21;s.bars=1;s.knobs=tr::defaultKnobs(tr::Fill);
        beginTest("Kick/snare/hat streams reject other representative bands by18dB");
        {
            juce::AudioBuffer<float> tones(2,48000);const double frequencies[]{65,1200,10500};
            for(int n=0;n<48000;++n){double value=0;for(double f:frequencies)value+=.1*std::sin(2*juce::MathConstants<double>::pi*f*n/48000);for(int c=0;c<2;++c)tones.setSample(c,n,static_cast<float>(value));}
            const auto bands=tr::extractDrumBands(tones,48000);
            for(int r=0;r<3;++r){const double own=amplitude(bands[static_cast<size_t>(r)],frequencies[r]);expect(own>.04);for(int other=0;other<3;++other)if(other!=r){const double db=20*std::log10(std::max(1e-20,amplitude(bands[static_cast<size_t>(r)],frequencies[other]))/own);expect(db<=-18,juce::String(r)+" / "+juce::String(other)+" = "+juce::String(db));}}
        }
        beginTest("New rhythmic plans,distinct role masks and reduced original-pattern copying");
        for(int recipe=21;recipe<=29;++recipe)
        {
            s.recipe=recipe;std::set<juce::String> plans,masks;int novel=0;
            for(int seed=1;seed<=(recipe==21?64:32);++seed)
            {
                s.seed=static_cast<std::uint64_t>(seed);const auto p=tr::makePlan(source,s);juce::String signature;std::array<std::uint32_t,3> mask{};std::set<int> roles;
                for(const auto& e:p.events){signature<<e.at<<":"<<e.source<<":"<<e.role<<";";mask[static_cast<size_t>(e.role)]|=1u<<std::clamp(static_cast<int>(e.at*32LL/tr::lengthSamples(s)),0,31);roles.insert(e.role);}
                plans.insert(signature);masks.insert(juce::String(mask[0])+":"+juce::String(mask[1])+":"+juce::String(mask[2]));if(p.inputSimilarity<=.65)++novel;expectEquals(static_cast<int>(roles.size()),3);
            }
            logMessage("recipe="+juce::String(recipe)+" plans="+juce::String(plans.size())+" masks="+juce::String(masks.size())+" dissimilar="+juce::String(novel));
            expect(plans.size()>=static_cast<size_t>(recipe==21?64:24));expect(masks.size()>=static_cast<size_t>(recipe==21?48:24));if(recipe==21)expect(novel>=58);
        }
        beginTest("Selected phrase,neutral,kit switches,controls and finite edge cases");
        {auto roll=s;roll.recipe=22;roll.knobs[1]=1;int strong=0,total=0;for(int seed=1;seed<=8;++seed){roll.seed=static_cast<std::uint64_t>(seed);const auto p=tr::makePlan(source,roll);for(const auto& e:p.events)if(e.roll){const int at=e.source%96000;const int distance=std::min(std::abs(at-24000),std::abs(at-72000));++total;if(distance<9600)++strong;}}expect(total>0&&strong>=.9*total,"roll source belongs to a strong synthetic backbeat,not weak hat leakage");}
        s.recipe=21;s.seed=37;s.bars=2;
        for(int beats:{1,2,4}){s.fillBeats=beats;const auto p=tr::makePlan(source,s);const auto y=tr::render(source,s);expect(identical(y,tr::render(source,s)));bool prefix=true;for(int c=0;c<2;++c)for(int n=0;n<p.phraseStart;++n)prefix=prefix&&source.getSample(c,n)==y.getSample(c,n);expect(prefix);expectEquals(y.getSample(0,y.getNumSamples()-1),0.f);expectEquals(y.getSample(0,p.phraseStart),source.getSample(0,p.phraseStart-1));}
        s.bars=1;s.fillBeats=4;
        for(int role=0;role<3;++role){auto solo=s;solo.kitEnabled={false,false,false};solo.kitEnabled[static_cast<size_t>(role)]=true;const auto p=tr::makePlan(source,solo);bool only=true;for(const auto& e:p.events)only=only&&e.role==role;expect(only&&!p.events.empty());expect(tr::render(source,solo).getMagnitude(0,96000)>0);
            const auto full=tr::makePlan(source,s);juce::String selected,all;for(const auto& e:p.events)selected<<e.at<<":"<<e.source<<";";for(const auto& e:full.events)if(e.role==role)all<<e.at<<":"<<e.source<<";";expect(selected==all,"solo preserves the composed beat");}
        {auto muted=s;muted.kitEnabled={false,false,false};expectEquals(tr::render(source,muted).getMagnitude(0,96000),0.f);}
        {auto neutral=s;neutral.knobs.fill(0);const auto y=tr::render(source,neutral);bool exact=true;for(int c=0;c<2;++c)for(int n=0;n<95760;++n)exact=exact&&y.getSample(c,n)==source.getSample(c,n);expect(exact);}
        for(int k=0;k<4;++k){auto a=s;a.knobs[static_cast<size_t>(k)]=0;auto b=a;b.knobs[static_cast<size_t>(k)]=1;expect(!identical(tr::render(source,a),tr::render(source,b)),"macro "+juce::String(k));}
        for(double fs:{44100.,48000.,96000.})for(double bpm:{70.,175.})for(int bars:{1,2,4})
        {auto t=s;t.sampleRate=fs;t.bpm=bpm;t.bars=bars;const auto y=tr::render(source,t);expectEquals(y.getNumSamples(),tr::lengthSamples(t));bool finite=true;for(int c=0;c<2;++c)for(int n=0;n<y.getNumSamples();++n)finite=finite&&std::isfinite(y.getSample(c,n));expect(finite);expect(y.getMagnitude(0,y.getNumSamples())<=.814f);}
        {auto hot=source;hot.applyGain(50);expect(tr::render(hot,s).getMagnitude(0,96000)<=.814f);juce::AudioBuffer<float> silent(1,1200);silent.clear();expectEquals(tr::render(silent,s).getMagnitude(0,96000),0.f);auto t=s;t.bpm=std::numeric_limits<double>::quiet_NaN();expectEquals(tr::render(source,t).getNumSamples(),0);}
        beginTest("Composer is default;kit switches reach renderer and survive editor reopen");
        {
            using P=hpeq::standalone::ModulePluginProcessor;auto p=std::make_unique<P>(hpeq::standalone::ModuleKind::Revolution);p->prepareToPlay(48000,256);const auto folder=juce::File::getSpecialLocation(juce::File::tempDirectory).getChildFile("HamloR14Tests");folder.createDirectory();p->setRendersFolder(folder);
            juce::MidiBuffer midi;for(int n=0;n<192000;n+=256){auto block=segment(source,n,256);p->processBlock(block,midi);}
            std::unique_ptr<juce::AudioProcessorEditor> owner(p->createEditor());auto* editor=dynamic_cast<hpeq::standalone::ModulePluginEditor*>(owner.get());auto* page=editor->getTransitionPage();page->chooseKind(tr::Fill);expectEquals(page->getValues().recipe,21);expect(p->lastTransition().kitCounts[0]>0&&p->lastTransition().kitCounts[1]>0&&p->lastTransition().kitCounts[2]>0);
            page->toggleKit(0);page->toggleKit(2);expect(!p->lastTransition().settings.kitEnabled[0]&&p->lastTransition().settings.kitEnabled[1]&&!p->lastTransition().settings.kitEnabled[2]);const auto old=p->lastTransition();juce::MemoryBlock data;old.file.loadFileAsData(data);page->pressVariation();expect(p->lastTransition().settings.seed==old.settings.seed+1);juce::MemoryBlock after;old.file.loadFileAsData(after);expect(data==after);
            owner.reset();owner.reset(p->createEditor());editor=dynamic_cast<hpeq::standalone::ModulePluginEditor*>(owner.get());expect(editor->getTransitionPage()->getValues().kitEnabled==p->lastTransition().settings.kitEnabled);
        }
    }
};static DrumComposeTests drumComposeTests;
}

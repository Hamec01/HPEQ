#include "DSP/TransitionRender.h"
#include "Standalone/ModulePluginEditor.h"
#include "Standalone/ModulePluginProcessor.h"
#include "UI/HamloRevolutionTransitionPage.h"
#include "TestSignals.h"
#include <set>
#include <tuple>
namespace hamlo::test
{
namespace tr=hpeq::dsp::transition;
namespace
{
std::vector<std::tuple<int,int,int,bool>> signature(const tr::Plan& p)
{std::vector<std::tuple<int,int,int,bool>> out;for(const auto& e:p.events)out.emplace_back(e.at,e.length,e.source,e.reverse);return out;}
bool exact(const juce::AudioBuffer<float>& a,const juce::AudioBuffer<float>& b)
{if(a.getNumSamples()!=b.getNumSamples()||a.getNumChannels()!=b.getNumChannels())return false;
 for(int c=0;c<a.getNumChannels();++c)for(int n=0;n<a.getNumSamples();++n)if(a.getSample(c,n)!=b.getSample(c,n))return false;return true;}
}
class TransitionRecipeTests final:public juce::UnitTest
{
public:
    TransitionRecipeTests():juce::UnitTest("Transition recipes","HamloRevolution"){}
    void runTest()override
    {
        const auto source=drums(8);
        beginTest("Auto has12+ structural plans per16 seeds; every fixed recipe4+ per8,not gain-only");
        for(int kind:{tr::Rise,tr::Fill})for(int recipe=1;recipe<=7;++recipe)
        {
            std::set<std::vector<std::tuple<int,int,int,bool>>> signatures;
            const int seeds=recipe==1?16:8;
            for(int seed=1;seed<=seeds;++seed)
            {tr::Settings s;s.kind=kind;s.recipe=recipe;s.seed=static_cast<std::uint64_t>(seed);signatures.insert(signature(tr::makePlan(source,s)));}
            logMessage("kind="+juce::String(kind)+" recipe="+juce::String(recipe)+" structures="+juce::String(signatures.size()));
            expect(signatures.size()>=static_cast<size_t>(recipe==1?12:4));
        }
        beginTest("Exact bar lengths and finite soft endings across rates,tempos,bars and both kinds");
        for(double fs:{44100.,48000.,96000.})for(double bpm:{70.,95.,120.,140.,175.})for(int bars:{1,2,4})for(int kind:{tr::Rise,tr::Fill})
        {
            tr::Settings s;s.sampleRate=fs;s.bpm=bpm;s.bars=bars;s.kind=kind;s.recipe=1;s.knobs=tr::defaultKnobs(kind);
            // A short captured source plus silent remainder also exercises bounds.
            const auto y=tr::render(source,s);expectEquals(y.getNumSamples(),static_cast<int>(std::lround(fs*bars*240/bpm)));
            bool finite=true;for(int c=0;c<2;++c)for(int n=0;n<y.getNumSamples();++n)finite=finite&&std::isfinite(y.getSample(c,n));
            expect(finite);expectEquals(y.getSample(0,y.getNumSamples()-1),0.f);expectEquals(y.getSample(1,y.getNumSamples()-1),0.f);
        }
        beginTest("Determinism,silence,mono source and earlier Fill bars preserve the source up to master gain");
        for(int kind:{tr::Rise,tr::Fill})
        {
            tr::Settings s;s.kind=kind;s.recipe=1;s.knobs=tr::defaultKnobs(kind);
            expect(exact(tr::render(source,s),tr::render(source,s)));
            juce::AudioBuffer<float> zero(1,10000);zero.clear();const auto silent=tr::render(zero,s);expectEquals(silent.getMagnitude(0,silent.getNumSamples()),0.f);
            const auto mono=sine(4,440,-18,1);const auto y=tr::render(mono,s);expectEquals(y.getNumChannels(),2);expect(y.getMagnitude(0,y.getNumSamples())>0);
            if(kind==tr::Fill)
            {
                const auto fill=tr::render(source,s);double xy=0,xx=0;for(int n=0;n<96000;++n){xy+=fill.getSample(0,n)*source.getSample(0,n);xx+=source.getSample(0,n)*source.getSample(0,n);}
                const double gain=xy/std::max(1e-30,xx);bool same=true;
                for(int c=0;c<2;++c)for(int n=0;n<96000;++n)same=same&&std::abs(fill.getSample(c,n)-gain*source.getSample(c,n))<1e-6;
                expect(same);
            }
        }
        beginTest("All controls affect new audio; zero Fill controls retain source and fade only");
        for(int kind:{tr::Rise,tr::Fill})
        {
            tr::Settings s;s.kind=kind;s.recipe=kind==tr::Rise?3:6;s.seed=37;s.bars=1;s.knobs=tr::defaultKnobs(kind);
            // Initial fixture used default STOP.3 and failed because that seed
            // legitimately chose no stop. Test the meaningful full0..1 range.
            for(int knob=0;knob<4;++knob){auto full=s;full.knobs[static_cast<size_t>(knob)]=1;auto off=full;off.knobs[static_cast<size_t>(knob)]=0;expect(!exact(tr::render(source,full),tr::render(source,off)),"kind "+juce::String(kind)+" knob "+juce::String(knob));}
        }
        {
            tr::Settings s;s.kind=tr::Fill;s.recipe=1;s.bars=1;s.knobs.fill(0);
            const auto y=tr::render(source,s);const double gain=y.getSample(0,1200)/source.getSample(0,1200);bool same=true;
            for(int c=0;c<2;++c)for(int n=0;n<y.getNumSamples()-240;++n)
                same=same&&std::abs(y.getSample(c,n)-gain*source.getSample(c,n))<1e-6;
            expect(same);
        }
        beginTest("Hot near-Nyquist input and impulses obey the sample ceiling as well as intersample metering");
        for(int kind:{tr::Rise,tr::Fill})
        {
            juce::AudioBuffer<float> hot(2,192000);
            for(int c=0;c<2;++c)for(int n=0;n<hot.getNumSamples();++n)hot.setSample(c,n,n%2?2.f:-2.f);
            hot.setSample(0,1000,4.f);tr::Settings s;s.kind=kind;s.recipe=1;s.knobs=tr::defaultKnobs(kind);
            const auto y=tr::render(hot,s);expect(y.getMagnitude(0,y.getNumSamples())<=std::pow(10.,-1.5/20)+1e-5);
        }
        beginTest("Modern recipe UI reaches capture/export; existing clips immutable and original API remains Classic");
        {
            using P=hpeq::standalone::ModulePluginProcessor;
            auto p=std::make_unique<P>(hpeq::standalone::ModuleKind::Revolution);p->prepareToPlay(48000,256);
            const auto folder=juce::File::getSpecialLocation(juce::File::tempDirectory).getChildFile("HamloR12Tests");folder.createDirectory();p->setRendersFolder(folder);
            std::unique_ptr<juce::AudioProcessorEditor> owned(p->createEditor());auto* editor=dynamic_cast<hpeq::standalone::ModulePluginEditor*>(owned.get());expect(editor!=nullptr);
            auto* page=editor?editor->getTransitionPage():nullptr;expect(page!=nullptr);
            if(page)
            {page->chooseRecipe(5);expectEquals(p->lastTransition().settings.recipe,5);expect(p->lastTransition().valid);
             const auto first=p->lastTransition().file;juce::MemoryBlock contents;first.loadFileAsData(contents);
             page->chooseRecipe(6);expectEquals(p->lastTransition().settings.recipe,6);juce::MemoryBlock after;first.loadFileAsData(after);expect(contents==after);
             expectEquals(page->getValues().recipe,6);page->chooseKind(tr::Fill);
             owned.reset();owned.reset(p->createEditor());editor=dynamic_cast<hpeq::standalone::ModulePluginEditor*>(owned.get());
             expectEquals(editor->getTransitionPage()->getValues().kind,static_cast<int>(tr::Fill));
             expectEquals(editor->getTransitionPage()->getValues().recipe,8); // Selecting Fill starts the R13 slicer.
            }
            const auto legacy=p->makeTransition(0,1,tr::defaultKnobs(tr::Rise),true,false);expectEquals(legacy.settings.recipe,0);
        }
    }
};static TransitionRecipeTests transitionRecipeTests;
}

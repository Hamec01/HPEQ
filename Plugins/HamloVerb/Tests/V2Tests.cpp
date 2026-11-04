#include <atomic>
#include <cstdlib>
#include <new>
#include <malloc.h>
#include "VerbLabHelpers.h"
#include "Standalone/ModulePluginEditor.h"
#include "UI/VerbV2Panel.h"
#include "Core/ModulePresets.h"

// Count C++ allocations only while this thread is inside processBlock. JUCE GUI/background allocations
// cannot produce false positives. malloc/realloc in DSP are additionally prohibited by source review.
namespace hamlo::verblab { thread_local bool countAllocations=false; thread_local size_t allocations=0; }
void* operator new(std::size_t n)
{
    if(hamlo::verblab::countAllocations) ++hamlo::verblab::allocations;
    if(auto* p=std::malloc(n?n:1))return p;
    throw std::bad_alloc();
}
void* operator new[](std::size_t n){return ::operator new(n);}
void operator delete(void* p) noexcept{std::free(p);}
void operator delete[](void* p) noexcept{std::free(p);}
void operator delete(void* p,std::size_t) noexcept{std::free(p);}
void operator delete[](void* p,std::size_t) noexcept{std::free(p);}
void* operator new(std::size_t n,std::align_val_t alignment)
{
    if(hamlo::verblab::countAllocations) ++hamlo::verblab::allocations;
    if(auto* p=_aligned_malloc(n?n:1,static_cast<size_t>(alignment)))return p;
    throw std::bad_alloc();
}
void* operator new[](std::size_t n,std::align_val_t a){return ::operator new(n,a);}
void operator delete(void* p,std::align_val_t) noexcept{_aligned_free(p);}
void operator delete[](void* p,std::align_val_t) noexcept{_aligned_free(p);}
void operator delete(void* p,std::size_t,std::align_val_t) noexcept{_aligned_free(p);}
void operator delete[](void* p,std::size_t,std::align_val_t) noexcept{_aligned_free(p);}

namespace hamlo::verblab
{
class V2Tests final:public juce::UnitTest
{
public:
    V2Tests():juce::UnitTest("V2 acceptance","HamloVerb"){}
    void runTest() override
    {
        beginTest("Additive parameter defaults and preserved old program indices");
        {
            auto p=processor({});
            expectEquals(p->getNumPrograms(),208);
            expectEquals(p->getProgramName(0),juce::String("Default"));
            expectWithinAbsoluteError(p->parameterValue("engine"),0.,0.);
            for(const auto* id:{"shimmerAmount","timeShape","outputDb","routing","earlyDb","lateDb"})
                expectWithinAbsoluteError(p->parameterValue(id),0.,0.,id);
            for(int i=1;i<=6;++i)for(const auto* family:{"decayEq","postEq"})
                expectWithinAbsoluteError(p->parameterValue(juce::String(family)+juce::String(i)+"On"),0.,0.);
            for(int program=1;program<=7;++program){p->setCurrentProgram(program);expectWithinAbsoluteError(p->parameterValue("engine"),0.,0.);}
        }
        beginTest("M0 state restores Legacy over modified V2 and resets every new parameter");
        {
            juce::MemoryBlock state;
            const auto fixture=juce::File::getCurrentWorkingDirectory().getChildFile("build/reference/m0/program5.state");
            expect(fixture.loadFileAsData(state),"M0 state fixture must exist");
            if(state.getSize()>0)
            {
                auto original=processor({});original->setStateInformation(state.getData(),static_cast<int>(state.getSize()));
                auto changed=processor({{"engine",6},{"routing",1},{"shimmerAmount",1},{"timeShape",3},{"decayEq1On",1},{"outputDb",-9}});
                changed->setMixLock(true);
                changed->setStateInformation(state.getData(),static_cast<int>(state.getSize()));
                for(const auto& spec:original->getSpec().parameters)
                    expectWithinAbsoluteError(changed->parameterValue(spec.id),original->parameterValue(spec.id),1e-6,spec.id);
                expect(!changed->getMixLock());
                const auto x=excitation(.5);
                original->prepareToPlay(48000,256);changed->prepareToPlay(48000,256);
                expect(exact(render(*original,x),render(*changed,x)));
            }
        }
        beginTest("Routing and Mix Lock survive presets; project-state load restores workflow and sound");
        {
            auto p=processor({{"engine",1},{"routing",1},{"mix",.37}});
            p->setMixLock(true);p->setCurrentProgram(8);
            expectWithinAbsoluteError(p->parameterValue("mix"),.37,1e-6);
            expectWithinAbsoluteError(p->parameterValue("routing"),1.,0.);
            juce::MemoryBlock state;p->getStateInformation(state);
            auto q=processor({});q->setStateInformation(state.getData(),static_cast<int>(state.getSize()));
            expect(q->getMixLock());expectWithinAbsoluteError(q->parameterValue("routing"),1.,0.);
            expectWithinAbsoluteError(q->parameterValue("mix"),.37,1e-6);
            expectEquals(q->getCurrentProgram(),p->getCurrentProgram());
            q->setParameterValue("decaySeconds",7);expect(q->isProgramModified());
            p->setMixLock(false);p->setCurrentProgram(9);expect(!p->getMixLock());
        }
        beginTest("New Send contains no dry; Insert MIX0 and bypass return dry exactly");
        {
            auto s=neutral(1);set(s,"predelayMs",500);set(s,"earlyDb",0);
            juce::AudioBuffer<float> input(2,4096);input.clear();input.setSample(0,0,.5f);input.setSample(1,0,-.25f);
            auto send=processor(s);settle(*send);const auto wet=render(*send,input);expectEquals(wet.getMagnitude(0,4096),0.0f);
            set(s,"routing",0);set(s,"mix",0);auto dry=processor(s);settle(*dry);expect(exact(render(*dry,input),input));
            set(s,"mix",1);set(s,"bypass",1);auto bypass=processor(s);settle(*bypass);expect(exact(render(*bypass,input),input));
            expectEquals(send->getLatencySamples(),0);expectEquals(dry->getLatencySamples(),0);
        }
        beginTest("Six engines are finite and invariant to host blocks, including sample-position automation");
        {
            const auto input=excitation(.8);
            const std::vector<Event> events{{4093,"mix",.21},{7001,"freeze",1},{9997,"freeze",0},
                {13001,"timeShape",2},{17003,"engine",3},{19001,"engine",5},{25003,"timeShape",3},{30001,"predelayMs",200}};
            for(int engine=1;engine<=6;++engine)
            {
                auto s=neutral(engine);set(s,"routing",0);set(s,"mix",.6);set(s,"modDepth",.3);
                set(s,"decayEq1On",1);set(s,"decayEq1Value",.5);set(s,"postEq1On",1);set(s,"postEq1Value",-3);
                set(s,"shimmerAmount",.2);set(s,"shimmerPitch",7);
                for(bool automated:{false,true})
                {
                    auto ref=processor(s);const auto expected=render(*ref,input,256,automated?events:std::vector<Event>{});
                    expect(finite(expected)&&expected.getMagnitude(0,expected.getNumSamples())<16,"engine "+juce::String(engine));
                    for(int block:{1,64,127,512,4096})
                    {
                        auto p=processor(s);const auto actual=render(*p,input,block,automated?events:std::vector<Event>{});
                        expect(exact(actual,expected),"engine "+juce::String(engine)+" block "+juce::String(block)+(automated?" automated":" static"));
                    }
                }
            }
        }
        beginTest("No processBlock allocations, including first block and engine/effect switches");
        {
            auto p=processor({{"engine",1}});auto input=excitation(4096./48000);juce::MidiBuffer midi;
            for(int engine=1;engine<=6;++engine)
            {
                p->setParameterValue("engine",engine);p->setParameterValue("timeShape",engine%4);
                p->setParameterValue("shimmerAmount",.7);p->setParameterValue("decayEq1On",1);
                p->setParameterValue("keepDry",1); // Also audit the new additive Insert path.
                allocations=0;countAllocations=true;p->processBlock(input,midi);countAllocations=false;
                expectEquals(static_cast<int>(allocations),0,"engine "+juce::String(engine));
            }
        }
        beginTest("Swell rises relative to the same tank; Gate Hold changes the cutoff time");
        {
            auto settings=neutral(4,2);set(settings,"earlyDb",0);set(settings,"gateThresholdDb",-40);
            set(settings,"shapeTimeMs",250);set(settings,"gateReleaseMs",30);
            const auto plain=impulse(settings,2);
            set(settings,"timeShape",2);const auto swell=impulse(settings,2);
            const double early=energy(swell,960,1920)/energy(plain,960,1920);
            const double later=energy(swell,7200,2400)/energy(plain,7200,2400);
            expect(later>early*10&&later>.05,"Relative envelope must rise, independent of tank decay");
            expect(energy(swell,19200)/energy(swell)<1e-6,"Swell must end after its time and release");
            set(settings,"timeShape",1);const auto shortGate=impulse(settings,2);
            set(settings,"shapeTimeMs",500);const auto longGate=impulse(settings,2);
            const double at350=energy(shortGate,16800,2400)/energy(longGate,16800,2400);
            expect(at350<.01,"250 ms gate closes while 500 ms gate remains open");
        }
        beginTest("Freeze preserves the captured tank under size and diffusion automation");
        {
            for(int engine=1;engine<=6;++engine)
            {
                auto a=processor(neutral(engine)),b=processor(neutral(engine));
                const auto feed=excitation(.7);render(*a,feed);render(*b,feed);
                a->setParameterValue("freeze",1);b->setParameterValue("freeze",1);
                settle(*a,48000,2);settle(*b,48000,2);
                b->setParameterValue("size",1.5);b->setParameterValue("lateDiffusion",.1);b->setParameterValue("modDepth",1);
                juce::AudioBuffer<float> silence(2,48000);silence.clear();
                expect(exact(render(*a,silence),render(*b,silence)),"frozen engine "+juce::String(engine));
            }
        }
        beginTest("Material buttons filter only; Load Start and modified state are truthful");
        {
            auto p=processor({});
            std::unique_ptr<juce::AudioProcessorEditor> owned(p->createEditor());
            auto* editor=dynamic_cast<hpeq::standalone::ModulePluginEditor*>(owned.get());
            auto* panel=editor?dynamic_cast<hpeq::ui::VerbV2Panel*>(editor->getPanel()):nullptr;
            expect(panel!=nullptr);
            if(panel)
            {
                juce::MemoryBlock before,after;p->getStateInformation(before);
                panel->onMaterial("Drums");p->getStateInformation(after);
                expect(before==after,"Filtering must not edit project state");
                expectEquals(editor->getPresetScreen().group(),juce::String("Drums"));
                panel->onLoadStart("Drums");
                const auto& bank=hpeq::presets::factoryModulePresets(p->getSpec().type);
                expectEquals(juce::String(bank[static_cast<size_t>(p->getCurrentProgram()-1)].group),juce::String("Drums"));
                expect(!p->isProgramModified(),"Loaded recipe has no modified marker");
                p->setParameterValue("decaySeconds",9);expect(p->isProgramModified());
            }
            auto& host=p->getParameters();
            auto* bypass=dynamic_cast<juce::AudioProcessorParameterWithID*>(host[22]);
            auto* engine=dynamic_cast<juce::AudioProcessorParameterWithID*>(host[23]);
            expect(bypass&&bypass->paramID=="bypass");expect(engine&&engine->paramID=="engine");
        }
        beginTest("Sample rates, mono/stereo, full predelay/decay range and finite tail contract");
        {
            for(double rate:{44100.,48000.,96000.,192000.})for(int channels:{1,2})for(int engine=1;engine<=6;++engine)
            {
                auto s=neutral(engine,30,500);set(s,"earlyDb",0);
                auto p=processor(s,rate,channels);const auto x=excitation(.02,rate,channels);const auto y=render(*p,x,4096);
                expect(finite(y));expectEquals(p->getLatencySamples(),0);
                p->setParameterValue("freeze",1);expect(std::isinf(p->getTailLengthSeconds()));
                p->setParameterValue("freeze",0);expect(std::isfinite(p->getTailLengthSeconds())&&p->getTailLengthSeconds()>=30.5);
            }
        }
    }
};
static V2Tests tests;
}

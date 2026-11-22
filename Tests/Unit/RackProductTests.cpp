#include "Rack/RackPresets.h"
#include "Plugin/FxPluginProcessor.h"
#include "Plugin/FxPluginEditor.h"
#include "UI/NativeModulePanel.h"
#include "UI/NodeInspector.h"
#include "DSP/FxNodeProcessor.h"
#include "Harness/RtAllocationProbe.h"
#include <thread>
#include <chrono>
#include <cmath>

namespace hpeq::test
{
namespace
{
class RackProductTests final:public juce::UnitTest
{
public:
    RackProductTests():UnitTest("HamloRack product","HPEQ"){}
    void runTest() override
    {
        beginTest("250 executable recipes preserve all native controls and exact state");
        const auto& bank=rack::recipes();expectEquals(static_cast<int>(bank.size()),250);
        for(const auto& recipe:bank)
        {
            FxProjectState project;juce::String error;
            expect(rack::makeProject(recipe,project,error),recipe.id+": "+error);
            expect(project.rackPresetId==recipe.id);
            const auto tree=toValueTree(project);auto restored=fromValueTree(tree,&error);
            expect(restored.has_value(),recipe.id+": "+error);
            if(restored)expect(tree.isEquivalentTo(toValueTree(*restored)),recipe.id+" state changes on reload");
            if(recipe.category=="Master")
            {
                expect(project.masterRoute&&project.routing==LayerRouting::Serial);
                expect(project.layers.back().mixChain.back().type==FxType::Limiter);
            }
        }

        beginTest("native panels edit pinned physical parameters and survive all module types");
        for(auto type:{FxType::Compressor,FxType::Limiter,FxType::Reverb,FxType::Delay,FxType::Gain,
            FxType::Saturator,FxType::TransientShaper,FxType::StereoProcessor,FxType::GateExpander,FxType::MonstaFx})
        {
            auto slot=makeNativeModule(type);auto panel=std::make_unique<ui::NativeModulePanel>(type);
            panel->readSlot=[&]{return slot;};panel->onChange=[&](const FxSlotState& changed){slot=changed;};
            panel->setBounds(0,0,1100,700);panel->refresh();
            const auto id=type==FxType::Compressor?"thresholdDb":type==FxType::Gain?"gainDb":"";
            if(juce::String(id).isNotEmpty())
            {
                auto* control=dynamic_cast<juce::Slider*>(panel->control(id));expect(control!=nullptr);
                if(control){control->setValue(-12.0,juce::sendNotificationSync);expectWithinAbsoluteError(static_cast<double>(slot.parameters[id]),-12.0,1.e-4);}
            }
            if(type==FxType::MonstaFx)expect(panel->control("scene_6_15")!=nullptr);
        }

        beginTest("native insert/send composition, dry alignment and slot bypass");
        for(auto type:{FxType::Compressor,FxType::Limiter,FxType::Reverb,FxType::Delay,FxType::Gain,
            FxType::Saturator,FxType::TransientShaper,FxType::StereoProcessor,FxType::GateExpander,FxType::MonstaFx})
            for(float mix:{0.f,.5f,1.f})composition(type,mix);

        beginTest("node bypass preserves native send tails, then retires them");
        tails(FxType::Reverb);tails(FxType::Delay);

        beginTest("Dual Mono uses independent detectors and retains Fader right-channel controls");
        {
            const auto process=[&](FxType type,ChannelMode mode,float rightLevel)
            {
                FxProjectState project;project.layers[0].channel=mode;
                auto slot=makeNativeModule(type);
                if(type==FxType::Compressor)
                {slot.parameters.set("thresholdDb",-24.0);slot.parameters.set("ratio",8.0);slot.parameters.set("attackMs",1.0);}
                else
                {slot.parameters.set("leftDb",-6.0);slot.parameters.set("rightDb",-18.0);slot.parameters.set("polarity",2);}
                project.layers[0].mixChain={slot};
                auto engine=std::make_unique<dsp::ProjectEngine>();engine->prepare(48000,256);
                expect(engine->publish(project),engine->publicationError());
                expect((engine->nativeRightRuntime(0,8,0)!=nullptr)==(mode==ChannelMode::DualMono));
                juce::AudioBuffer<float> audio(2,256);double l=0,r=0;
                for(int b=0;b<128;++b)
                {
                    for(int i=0;i<256;++i){audio.setSample(0,i,.04f);audio.setSample(1,i,rightLevel);}
                    engine->process(audio.getArrayOfWritePointers(),2,256);
                    if(b==127){l=audio.getSample(0,255);r=audio.getSample(1,255);}
                }
                return std::pair<double,double>{l,r};
            };
            const auto quiet=process(FxType::Compressor,ChannelMode::DualMono,.01f);
            const auto loud=process(FxType::Compressor,ChannelMode::DualMono,.8f);
            expectWithinAbsoluteError(quiet.first,loud.first,1.e-7);
            const auto linked=process(FxType::Compressor,ChannelMode::LinkedStereo,.8f);
            expect(linked.first<loud.first*.8,"Linked detector should react to the loud right input");
            const auto gain=process(FxType::Gain,ChannelMode::DualMono,.2f);
            expectWithinAbsoluteError(gain.first,.04*std::pow(10.,-6./20),2.e-6);
            expectWithinAbsoluteError(gain.second,-.2*std::pow(10.,-18./20),2.e-6);
        }

        beginTest("a self-noising native insert cannot keep a bypassed node running forever");
        {
            auto slot=makeNativeModule(FxType::Saturator);slot.parameters.set("hiss",.5);slot.parameters.set("noiseFollow",0);
            juce::String error;auto runtime=std::make_unique<native::Runtime>(slot.type,slot.nativeState,slot.parameters,48000,256,error);
            expect(runtime->valid(),error);
            auto node=std::make_unique<dsp::FxNodeProcessor>();node->prepare(48000,256,nullptr);
            dsp::FxNodeParams p;p.active=true;p.numSlots=1;auto& s=p.slots[0];s.active=true;s.type=slot.type;s.engineRevision=native::kRevision;
            expect(native::describe(slot.type,slot.parameters,48000,s.nativeValues,error));s.nativeValues.runtime=runtime.get();
            node->setTargets(p);juce::AudioBuffer<float> input(2,256),delta(2,256);input.clear();
            std::array<float,256> presence;presence.fill(1);
            for(int i=0;i<16;++i){delta.clear();node->process(input.getArrayOfReadPointers(),presence.data(),delta.getArrayOfWritePointers(),2,256,{},0);}
            p.active=false;node->setTargets(p);
            for(int i=0;i<40;++i){delta.clear();node->process(input.getArrayOfReadPointers(),presence.data(),delta.getArrayOfWritePointers(),2,256,{},0);}
            expect(!node->isRunning());expectEquals(runtime->inUse(),0);expectEquals(delta.getMagnitude(0,256),0.f);
        }

        beginTest("wet-only routing removes the dry impulse and schema-2 controls survive state");
        FxProjectState wet;wet.wetOnly=true;wet.referenceBpm=90;wet.outputTrimDb=-3;wet.inputTrimDb=2;
        auto wetEngine=std::make_unique<dsp::ProjectEngine>();wetEngine->prepare(48000,256);expect(wetEngine->publish(wet));
        juce::AudioBuffer<float> block(2,256);block.clear();block.setSample(0,0,.25f);
        wetEngine->process(block.getArrayOfWritePointers(),2,256);expectEquals(block.getMagnitude(0,256),0.f);
        auto roundTrip=fromValueTree(toValueTree(wet));expect(roundTrip.has_value());
        if(roundTrip)expect(roundTrip->wetOnly&&roundTrip->outputTrimDb==-3&&roundTrip->inputTrimDb==2&&roundTrip->referenceBpm==90);

        beginTest("Revolution knob and scene edits keep the live DSP history and all eight scenes");
        auto revolution=std::make_unique<dsp::ProjectEngine>();revolution->prepare(48000,256);
        FxProjectState rev;rev.layers[0].mixChain={makeNativeModule(FxType::MonstaFx)};
        expect(revolution->publish(rev));block.clear();revolution->process(block.getArrayOfWritePointers(),2,256);
        auto* original=revolution->nativeRuntime(0,8,0);
        for(int i=0;i<320;++i)
        {
            rev.layers[0].mixChain[0].parameters.set("timeDepth",.2+(i%10)*.05);
            expect(revolution->publish(rev),revolution->publicationError());
            expect(revolution->nativeRuntime(0,8,0)==original,"Knob edit reset the native DSP");
        }
        auto& revSlot=rev.layers[0].mixChain[0];revSlot.parameters.set("scene",2);
        native::Scene event;event.lanes[0][3]=2;juce::String sceneError;
        revSlot.nativeState=native::editRevolutionScene(revSlot.nativeState,revSlot.parameters,event,sceneError);
        expect(sceneError.isEmpty(),sceneError);expect(revolution->publish(rev),revolution->publicationError());
        expect(revolution->nativeRuntime(0,8,0)==original,"Scene edit reset the native DSP");
        native::Scene one,three;
        expect(native::readRevolutionSceneAt(revSlot.nativeState,0,one,sceneError));
        expect(native::readRevolutionSceneAt(revSlot.nativeState,2,three,sceneError));
        expect(one.lanes[0][3]==0&&three.lanes[0][3]==2);
        block.clear();{ScopedAllocationProbe probe;revolution->process(block.getArrayOfWritePointers(),2,256);
            expectEquals(static_cast<juce::int64>(probe.counts().allocations),juce::int64(0));}

        beginTest("publication and runtime retirement under an active audio reader");
        auto engine=std::make_unique<dsp::ProjectEngine>();engine->prepare(48000,256);
        std::atomic<bool> stop{false},finite{true};std::atomic<std::uint64_t> allocations{0},frees{0},frames{0};
        std::thread reader([&] {
            juce::AudioBuffer<float> audio(2,256);
            while(!stop.load())
            {
                audio.clear();audio.setSample(0,0,.1f);audio.setSample(1,0,.05f);
                {ScopedAllocationProbe probe;engine->process(audio.getArrayOfWritePointers(),2,256);
                    allocations.fetch_add(probe.counts().allocations);frees.fetch_add(probe.counts().frees);}
                for(int c=0;c<2;++c)for(int i=0;i<256;++i)if(!std::isfinite(audio.getSample(c,i)))finite.store(false);
                frames.fetch_add(256);
            }
        });
        int rejected=0;
        for(int i=0;i<96;++i)
        {
            FxProjectState p;
            if(i%3!=0)
            {
                auto slot=makeNativeModule(i%2?FxType::Gain:FxType::TransientShaper);
                p.layers[0].mixChain={slot};
                if(i%4==0)p.layers[1].mixChain={makeNativeModule(FxType::Compressor)};
            }
            if(!engine->publish(p))++rejected;
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        stop.store(true);reader.join();
        expect(finite.load());expect(frames.load()>0);expectEquals(rejected,0);
        expectEquals(static_cast<juce::int64>(allocations.load()),juce::int64(0));
        expectEquals(static_cast<juce::int64>(frees.load()),juce::int64(0));

        beginTest("rack factory load, native editor, undo and explicit Fader Match actions");
        auto processor=std::make_unique<FxPluginProcessor>();processor->prepareToPlay(48000,256);
        expect(processor->loadRackPreset(0));
        auto editor=std::make_unique<FxPluginEditor>(*processor);editor->selectMix(0);
        expect(editor->getInspector().wantsLargeView());
        expect(processor->getUndoManager().undo());expect(processor->getProject().rackPresetId.isEmpty());
        expect(processor->getUndoManager().redo());expect(processor->getProject().rackPresetId=="HR001");

        beginTest("Fader Match captures real stereo and Dual Mono audio, applies once and supports undo");
        double stereoProposal=0;
        for(auto channel:{ChannelMode::Stereo,ChannelMode::DualMono})
        {
            auto p=std::make_unique<FxPluginProcessor>();FxProjectState project;project.layers[0].channel=channel;
            auto slot=makeNativeModule(FxType::Gain);slot.parameters.set("mode",1);slot.parameters.set("targetDb",-30.0);slot.parameters.set("measure",0);
            project.layers[0].mixChain={slot};expect(p->setProject(project));p->prepareToPlay(48000,256);
            expect(p->nativeFaderAction(0,8,0,0));juce::AudioBuffer<float> audio(2,256);juce::MidiBuffer midi;
            for(int b=0;b<384;++b)
            {
                for(int c=0;c<2;++c)for(int i=0;i<256;++i)audio.setSample(c,i,.02f*std::sin(static_cast<float>(b*256+i)*.03f));
                p->processBlock(audio,midi);
            }
            expect(p->nativeFaderAction(0,8,0,1));audio.clear();p->processBlock(audio,midi);
            const auto learned=p->nativeFaderView(0,8,0);expect(learned.canApply&&!learned.learning);
            if(channel==ChannelMode::Stereo)stereoProposal=learned.proposal;
            else expectWithinAbsoluteError(learned.proposal,stereoProposal,1.e-4);
            expect(p->nativeFaderAction(0,8,0,3));
            expectWithinAbsoluteError(static_cast<double>(p->getProject().layers[0].mixChain[0].parameters["matchedDb"]),learned.proposal,1.e-4);
            expect(p->getUndoManager().undo());expectWithinAbsoluteError(static_cast<double>(p->getProject().layers[0].mixChain[0].parameters["matchedDb"]),0.,1.e-6);
        }
    }
private:
    void tails(FxType type)
    {
        auto engine=std::make_unique<dsp::ProjectEngine>();engine->prepare(48000,256);
        FxProjectState p;FxNodeState band;band.id=p.nextNodeId++;band.lowHz=20;band.highHz=20000;
        auto slot=makeNativeModule(type);slot.mix=1;band.chain={slot};p.layers[0].fxNodes={band};assignEngineSlots(p.layers[0]);
        const int index=p.layers[0].fxNodes[0].engineSlot;
        expect(engine->publish(p),engine->publicationError());auto* original=engine->nativeRuntime(0,index,0);
        juce::AudioBuffer<float> audio(2,256);double tailPeak=0;
        for(int b=0;b<3750;++b)
        {
            audio.clear();if(b==8)audio.setSample(0,0,.4f);
            if(b==12){p.layers[0].fxNodes[0].enabled=false;expect(engine->publish(p));expect(engine->nativeRuntime(0,index,0)==original,"Node bypass reset the wet processor");}
            dsp::Transport t;t.playing=true;t.bpm=120;t.hasPosition=true;t.ppqAtBlockStart=b*256./24000;
            engine->process(audio.getArrayOfWritePointers(),2,256,t);
            if(b>64)tailPeak=std::max(tailPeak,static_cast<double>(audio.getMagnitude(0,256)));
        }
        expect(tailPeak>1.e-5,native::productName(type)+" bypass cut the tail");
        expect(!engine->getLayer(0).getNode(index).isRunning(),native::productName(type)+" tail did not retire after 20 seconds");
    }
    void composition(FxType type,float amount)
    {
        auto slot=makeNativeModule(type);
        if(type==FxType::Gain)slot.parameters.set("gainDb",-6.0);
        if(type==FxType::Compressor){slot.parameters.set("thresholdDb",-24.0);slot.parameters.set("ratio",4.0);}
        if(type==FxType::Limiter)slot.parameters.set("thresholdDb",-18.0);
        juce::String error;
        auto runtime=std::make_unique<native::Runtime>(type,slot.nativeState,slot.parameters,48000,256,error);
        auto reference=std::make_unique<native::Runtime>(type,slot.nativeState,slot.parameters,48000,256,error);
        expect(runtime->valid()&&reference->valid(),native::productName(type)+": "+error);if(!runtime->valid()||!reference->valid())return;
        dsp::FxNodeParams params;params.active=true;params.numSlots=1;params.slots[0].active=true;
        auto& sp=params.slots[0];sp.type=type;sp.engineRevision=native::kRevision;sp.mix=amount;
        expect(native::describe(type,slot.parameters,48000,sp.nativeValues,error),error);sp.nativeValues.runtime=runtime.get();
        auto node=std::make_unique<dsp::FxNodeProcessor>();node->prepare(48000,256,nullptr);node->setTargets(params);
        dsp::AlignmentDelay align;align.prepare(dsp::FxNodeProcessor::maxSlotLatency(48000));
        juce::AudioBuffer<float> input(2,256),processed(2,256),dry(2,256),delta(2,256);
        std::array<float,256> presence;presence.fill(1);
        double maximum=0;AllocationCounts counts;
        const bool send=type==FxType::Reverb||type==FxType::Delay;
        for(int b=0;b<56;++b)
        {
            for(int c=0;c<2;++c)for(int i=0;i<256;++i)
                input.setSample(c,i,b<12?0.f:static_cast<float>(.17*std::sin((b*256+i)*.019+c*.3)));
            processed.makeCopyOf(input,true);delta.clear();
            reference->process(processed.getArrayOfWritePointers(),2,256,sp.nativeValues,120,true,true,b*256.0/24000);
            align.process(input.getArrayOfReadPointers(),dry.getArrayOfWritePointers(),2,256,runtime->latency());
            dsp::Transport t;t.playing=true;t.hasPosition=true;t.ppqAtBlockStart=b*256.0/24000;
            {ScopedAllocationProbe probe;node->process(input.getArrayOfReadPointers(),presence.data(),delta.getArrayOfWritePointers(),2,256,t,0);
                counts.allocations+=probe.counts().allocations;counts.frees+=probe.counts().frees;}
            if(b>12)for(int c=0;c<2;++c)for(int i=0;i<256;++i)
            {
                const double expected=amount*(processed.getSample(c,i)-(send?0.f:dry.getSample(c,i)));
                maximum=std::max(maximum,std::abs(delta.getSample(c,i)-expected));
            }
        }
        expect(maximum<=2.e-6,native::productName(type)+" mix "+juce::String(amount)+" error "+juce::String(maximum,9));
        expectEquals(static_cast<juce::int64>(counts.allocations),juce::int64(0));
        expectEquals(static_cast<juce::int64>(counts.frees),juce::int64(0));
    }
} rackProductTests;
class RackHostTailTests final:public juce::UnitTest
{
public:
    RackHostTailTests():UnitTest("HamloRack host tails","HPEQ"){}
    void runTest() override
    {
        beginTest("native freeze/unfreeze updates the host tail after audio consumes the snapshot");
        auto processor=std::make_unique<FxPluginProcessor>();FxProjectState p;
        p.layers[0].mixChain={makeNativeModule(FxType::Delay)};expect(processor->setProject(p));processor->prepareToPlay(48000,256);
        juce::AudioBuffer<float> audio(2,256);audio.clear();juce::MidiBuffer midi;
        processor->processBlock(audio,midi);expect(std::isfinite(processor->getTailLengthSeconds()));
        expect(processor->editProject("Freeze",[](FxProjectState& project){project.layers[0].mixChain[0].parameters.set("scoreFreeze",1);}));
        processor->processBlock(audio,midi);expect(std::isinf(processor->getTailLengthSeconds()));
        expect(processor->editProject("Unfreeze",[](FxProjectState& project){project.layers[0].mixChain[0].parameters.set("scoreFreeze",0);}));
        processor->processBlock(audio,midi);expect(std::isfinite(processor->getTailLengthSeconds()));
    }
} rackHostTailTests;
}
}

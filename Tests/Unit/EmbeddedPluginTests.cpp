#include "Plugin/FxPluginProcessor.h"
#include "UI/EmbeddedPluginPanel.h"
#include "UI/RackPresetBar.h"
#include "Rack/RackPresets.h"

namespace hpeq::test
{
class EmbeddedPluginTests final:public juce::UnitTest
{
public:
    EmbeddedPluginTests():UnitTest("HamloRack original editors","HPEQ"){}
    void runTest() override
    {
        beginTest("All ten original editors operate their actual audio processor, retain edits, save and undo");
        for(auto type:{FxType::Compressor,FxType::Limiter,FxType::Reverb,FxType::Delay,FxType::Gain,
            FxType::Saturator,FxType::TransientShaper,FxType::StereoProcessor,FxType::GateExpander,FxType::MonstaFx})
        {
            const auto name=native::productName(type);
            auto host=std::make_unique<FxPluginProcessor>();FxProjectState project;
            project.layers[0].mixChain={makeNativeModule(type)};
            expect(host->setProject(project),name);host->prepareToPlay(48000,256);
            auto panel=std::make_unique<ui::EmbeddedPluginPanel>();
            panel->readRuntime=[&]{return host->nativeEditorRuntime(0,8,0);};
            panel->readSlot=[&]{return host->getProject().layers[0].mixChain[0];};
            panel->onGesture=[&]{host->getUndoManager().beginNewTransaction("Native gesture");};
            panel->onChange=[&](const FxSlotState& slot){host->editProject("Native editor",[&](FxProjectState& p){p.layers[0].mixChain[0]=slot;});};
            panel->setSize(1100,800);panel->refresh();
            auto* editor=panel->originalEditor();expect(editor!=nullptr,name);
            expect(dynamic_cast<juce::GenericAudioProcessorEditor*>(editor)==nullptr,name+" must use its own editor");
            auto runtime=host->nativeEditorRuntime(0,8,0);
            expect(editor&&editor->getAudioProcessor()==&runtime->referenceProcessor(),name+" editor must control the actual audio instance");
            juce::RangedAudioParameter* mix=nullptr;
            for(auto* p:runtime->referenceProcessor().getParameters())
                if(auto* ranged=dynamic_cast<juce::RangedAudioParameter*>(p);ranged&&ranged->paramID=="mix")mix=ranged;
            expect(mix!=nullptr,name);
            if(!mix)continue;
            host->getUndoManager().beginNewTransaction("Editor mix");
            mix->setValueNotifyingHost(mix->convertTo0to1(.37f));
            juce::AudioBuffer<float> block(2,127);block.clear();juce::MidiBuffer midi;
            host->processBlock(block,midi); // audio must not overwrite an edit awaiting its message callback
            juce::MemoryBlock immediate;host->getStateInformation(immediate);
            auto restored=std::make_unique<FxPluginProcessor>();restored->setStateInformation(immediate.getData(),static_cast<int>(immediate.getSize()));
            expectWithinAbsoluteError(static_cast<double>(restored->getProject().layers[0].mixChain[0].parameters["mix"]),.37,1.e-5,name+" immediate host save");
            panel->flushPendingEdits();
            expect(host->nativeEditorRuntime(0,8,0)==runtime,name+" parameter edit recreated the native processor");
            expectWithinAbsoluteError(static_cast<double>(host->getProject().layers[0].mixChain[0].parameters["mix"]),.37,1.e-5,name);
            expect(host->getUndoManager().undo(),name+" native control must be undoable");
            expectWithinAbsoluteError(static_cast<double>(host->getProject().layers[0].mixChain[0].parameters["mix"]),1.,1.e-5,name);
            expectWithinAbsoluteError(static_cast<double>(mix->convertFrom0to1(mix->getValue())),1.,1.e-5,name+" undo must reach the native editor without waiting for audio");
            // The original editor/workbench stays on the same processor while
            // prepareToPlay reallocates sample-rate-dependent DSP memory.
            host->prepareToPlay(44100,256);panel->refresh();
            expect(host->nativeEditorRuntime(0,8,0)==runtime,name+" host prepare replaced the native processor");
            expect(panel->originalEditor()!=nullptr,name+" reprepare");
            mix->setValueNotifyingHost(mix->convertTo0to1(.61f));
            host->prepareToPlay(96000,256);
            juce::MemoryBlock pendingState;host->getStateInformation(pendingState);
            restored->setStateInformation(pendingState.getData(),static_cast<int>(pendingState.getSize()));
            expectWithinAbsoluteError(static_cast<double>(restored->getProject().layers[0].mixChain[0].parameters["mix"]),.61,1.e-5,name+" pending edit across host prepare");
            panel->flushPendingEdits();
            panel.reset();runtime.reset();
        }
        beginTest("Native send BYPASS keeps exactly one dry reference throughout both transitions");
        for(auto type:{FxType::Reverb,FxType::Delay})
        {
            auto host=std::make_unique<FxPluginProcessor>();FxProjectState p;
            auto slot=makeNativeModule(type);slot.mix=1;
            if(type==FxType::Reverb){slot.parameters.set("predelayMs",500.);slot.parameters.set("predelaySync",0);}
            else for(int tap=1;tap<=8;++tap)slot.parameters.set("tap"+juce::String(tap)+"On",0);
            p.layers[0].mixChain={slot};expect(host->setProject(p));host->prepareToPlay(48000,256);
            auto panel=std::make_unique<ui::EmbeddedPluginPanel>();
            panel->readRuntime=[&]{return host->nativeEditorRuntime(0,8,0);};
            panel->readSlot=[&]{return host->getProject().layers[0].mixChain[0];};
            panel->onChange=[&](const FxSlotState& changed){host->editProject("Native bypass",[&](FxProjectState& state){state.layers[0].mixChain[0]=changed;});};
            panel->setSize(1100,800);panel->refresh();
            auto runtime=host->nativeEditorRuntime(0,8,0);juce::RangedAudioParameter* bypass=nullptr;
            for(auto* param:runtime->referenceProcessor().getParameters())
                if(auto* ranged=dynamic_cast<juce::RangedAudioParameter*>(param);ranged&&ranged->paramID=="bypass")bypass=ranged;
            expect(bypass!=nullptr);if(!bypass)continue;
            juce::AudioBuffer<float> block(2,127);juce::MidiBuffer midi;double worst=0;
            for(int frame=0;frame<150;++frame)
            {
                if(frame==50||frame==100)bypass->setValueNotifyingHost(frame==50?1.f:0.f);
                for(int c=0;c<2;++c)for(int i=0;i<127;++i)block.setSample(c,i,.1f);
                host->processBlock(block,midi);
                if(frame>=32)for(int c=0;c<2;++c)for(int i=0;i<127;++i)worst=std::max(worst,std::abs(static_cast<double>(block.getSample(c,i))-.1));
                panel->flushPendingEdits();
            }
            expect(worst<=2.e-6,native::productName(type)+" dry-reference error "+juce::String(worst,9));
        }
        beginTest("Imported HPEQ modules migrate to current code without a default reset");
        FxProjectState old;FxSlotState comp;comp.type=FxType::Compressor;comp.mix=.43f;
        comp.parameters.set("thresholdDb",-31.0);comp.parameters.set("ratio",7.0);
        old.layers[0].mixChain={comp};useCurrentModules(old);
        expectEquals(old.layers[0].mixChain[0].engineRevision,native::kRevision);
        expectEquals(static_cast<double>(old.layers[0].mixChain[0].parameters["thresholdDb"]),-31.0);
        expectEquals(static_cast<double>(old.layers[0].mixChain[0].parameters["ratio"]),7.0);
        expectWithinAbsoluteError(old.layers[0].mixChain[0].mix,.43f,1.e-6f);
        for(auto type:{FxType::Compressor,FxType::Limiter,FxType::Reverb,FxType::Delay,FxType::Gain,
            FxType::Saturator,FxType::TransientShaper,FxType::StereoProcessor,FxType::GateExpander,FxType::MonstaFx})
        {
            FxSlotState legacy;legacy.type=type;const auto current=useCurrentModule(legacy);
            juce::String error;expect(isModuleSupported(current,&error),native::productName(type)+": "+error);
            if(type==FxType::Saturator)
            {
                expectEquals(static_cast<double>(current.parameters["model"]),0.);
                // First assertion assumed zero; LayerSnapshot's actual implicit
                // legacy value is .15, with Classic model 0. Keep that value.
                expectWithinAbsoluteError(static_cast<double>(current.parameters["hiss"]),.15,1.e-6);
                expectEquals(static_cast<double>(current.parameters["driveDb"]),12.);
            }
        }
        beginTest("Rack Power uses real layers, multiple modules and all ten current products");
        juce::StringArray names;int count=0;
        for(const auto& recipe:rack::recipes())if(recipe.id.startsWith("PW"))
        {
            ++count;FxProjectState p;juce::String error;expect(rack::makeProject(recipe,p,error),error);
            int layers=0,modules=0;
            const auto chain=[&](const std::vector<FxSlotState>& slots){for(const auto& s:slots){++modules;names.addIfNotAlreadyThere(native::productName(s.type));expectEquals(s.engineRevision,native::kRevision);}};
            for(const auto& l:p.layers)if(l.enabled){++layers;chain(l.mixChain);for(const auto& n:l.fxNodes)chain(n.chain);}
            expect(layers>=2&&modules>=3,recipe.name);
        }
        expectEquals(count,20);expectEquals(names.size(),10);
    }
};
static EmbeddedPluginTests embeddedPluginTests;
}

#include "Plugin/FxPluginProcessor.h"
#include "Plugin/FxParameterContract.h"
#include "Plugin/FxPluginEditor.h"
#include "UI/EmbeddedPluginPanel.h"
#include "Harness/RtAllocationProbe.h"
#include <cmath>
namespace hpeq::test {
namespace {
void set(FxPluginProcessor& p,const juce::String& id,float physical){auto* v=p.getParameters().getParameter(id);v->setValueNotifyingHost(v->convertTo0to1(physical));}
float norm(FxType kind,const juce::String& id,float physical){for(const auto& s:native::layout(kind))if(s.id==id)return s.range.convertTo0to1(physical);return 0;}
FxProjectState compModel(int model=0,bool external=false){FxProjectState p;auto s=makeNativeModule(FxType::Compressor);
    s.parameters.set("model",model);s.parameters.set("thresholdDb",-24.);s.parameters.set("ratio",8.);s.parameters.set("kneeDb",0.);
    // First fixture used legacy .05 ms, rejected by the pinned native range.
    // Use the same valid 1 ms as the unchanged black-box baseline audit.
    s.parameters.set("attackMs",1.);s.parameters.set("releaseMs",40.);s.parameters.set("makeupDb",0.);s.parameters.set("autoMakeup",0);
    s.externalSidechain=external;p.layers[0].mixChain={s};return p;
}
juce::AudioBuffer<float> render(FxPluginProcessor& p,int block,int length=24000,bool key=false,bool silence=false,bool events=false,float amplitude=.01f,bool side=false,bool leftKey=false){
    const int main=p.getMainBusNumInputChannels(),channels=juce::jmax(main,p.getTotalNumInputChannels());
    juce::AudioBuffer<float> output(main,length),scratch(channels,juce::jmax(block,1));juce::MidiBuffer midi;
    for(int at=0;at<length;){int n=juce::jmin(block,length-at);
        if(events){for(int event:{2051,8195})if(at<event)n=juce::jmin(n,event-at);
            if(at==2051)set(p,params::macroId(0),norm(FxType::Compressor,"makeupDb",6));
            if(at==8195)set(p,params::macroId(0),norm(FxType::Compressor,"makeupDb",3));}
        scratch.clear();for(int i=0;i<n;++i){const float x=silence?0.f:amplitude*std::sin(float(2*juce::MathConstants<double>::pi*997*(at+i)/48000));
            for(int ch=0;ch<main;++ch)scratch.setSample(ch,i,side&&ch==1?-x:x);
            if(key)for(int ch=main;ch<channels;++ch)if(!leftKey||ch==main)scratch.setSample(ch,i,.5f*std::sin(float(2*juce::MathConstants<double>::pi*83*(at+i)/48000)));}
        juce::AudioBuffer<float> view(scratch.getArrayOfWritePointers(),channels,n);p.processBlock(view,midi);
        for(int ch=0;ch<main;++ch)output.copyFrom(ch,at,view,ch,0,n);at+=n;
    }return output;
}
double energy(const juce::AudioBuffer<float>& b){double e=0;for(int ch=0;ch<b.getNumChannels();++ch)for(int i=b.getNumSamples()/2;i<b.getNumSamples();++i)e+=double(b.getSample(ch,i))*b.getSample(ch,i);return e;}
bool exact(const juce::AudioBuffer<float>& a,const juce::AudioBuffer<float>& b){if(a.getNumSamples()!=b.getNumSamples())return false;for(int ch=0;ch<a.getNumChannels();++ch)for(int i=0;i<a.getNumSamples();++i)if(a.getSample(ch,i)!=b.getSample(ch,i))return false;return true;}
bool keyLayout(FxPluginProcessor& p,int main=2,int key=2){auto l=p.getBusesLayout();l.inputBuses.set(0,main==1?juce::AudioChannelSet::mono():juce::AudioChannelSet::stereo());l.outputBuses.set(0,l.inputBuses[0]);l.inputBuses.set(1,key==1?juce::AudioChannelSet::mono():juce::AudioChannelSet::stereo());return p.setBusesLayout(l);}
}
class ControlStageTests final:public juce::UnitTest {
public:ControlStageTests():UnitTest("Rack controls stage","HPEQ"){}
void runTest()override{
    beginTest("Permanent old host IDs and sixteen appended macros plus neutral final trim");{
        auto p=std::make_unique<FxPluginProcessor>();expectEquals(static_cast<juce::AudioProcessor&>(*p).getParameters().size(),19);
        expect(p->getParameters().getParameter("outputGain")!=nullptr);expect(p->getParameters().getParameter("bypass")!=nullptr);
        for(int i=0;i<control::kMacros;++i)expect(p->getParameters().getParameter(params::macroId(i))!=nullptr);
        expectEquals(p->getParameters().getRawParameterValue("finalTrim")->load(),0.f);expect(!p->externalKeyEnabled());
    }
    beginTest("Module-owned assignments persist, move/copy explicitly, undo, and reject malformed destinations");{
        auto p=std::make_unique<FxPluginProcessor>();auto model=compModel();expect(p->setProject(model));auto targets=p->macroDestinations();
        const auto found=std::find_if(targets.begin(),targets.end(),[](const auto& t){return t.parameter=="makeupDb";});expect(found!=targets.end());
        if(found!=targets.end()){
            expect(p->assignMacro(0,*found,1,0));expectEquals(int(p->macroDestinations(0).size()),1);
            const auto tree=toValueTree(p->getProject());expectEquals(int(tree["schemaVersion"]),3);juce::String error;auto restored=fromValueTree(tree,&error);expect(restored.has_value(),error);
            if(restored){expectEquals(int(restored->layers[0].mixChain[0].macros.size()),1);expectEquals(restored->layers[0].mixChain[0].macros[0].minimum,1.f);}
            expect(p->getUndoManager().undo());expect(p->macroDestinations(0).empty());expect(p->getUndoManager().redo());
            auto moved=p->getProject();moved.layers[0].mixChain.insert(moved.layers[0].mixChain.begin(),makeNativeModule(FxType::Gain));expect(p->setProject(moved));expectEquals(p->macroDestinations(0)[0].slot,1);
            moved=p->getProject();moved.layers[0].mixChain.erase(moved.layers[0].mixChain.begin()+1);expect(p->setProject(moved));expect(p->macroDestinations(0).empty());
            auto invalid=compModel();invalid.layers[0].mixChain[0].macros.push_back({99,"makeupDb",0,1});expect(!p->setProject(invalid));
        }
    }
    beginTest("Fanout, copied assignments, stale targets, native gestures and saved audio survive reopen");{
        auto p=std::make_unique<FxPluginProcessor>();auto model=compModel();model.layers[0].mixChain[0].parameters.set("ratio",1.);expect(p->setProject(model));
        auto targets=p->macroDestinations();const auto makeup=std::find_if(targets.begin(),targets.end(),[](const auto& t){return t.parameter=="makeupDb";});
        expect(makeup!=targets.end());if(makeup!=targets.end()){
            const auto stale=*makeup;expect(p->assignMacro(0,*makeup));expect(!p->assignMacro(1,stale));expect(!p->removeMacroDestination(0,stale));
            targets=p->macroDestinations();for(const auto& t:targets)if(t.parameter=="thresholdDb"){expect(p->assignMacro(0,t,1,0));break;}
            expectEquals(int(p->macroDestinations(0).size()),2);
            auto copied=p->getProject();copied.layers[0].mixChain.push_back(copied.layers[0].mixChain[0]);expect(p->setProject(copied));expectEquals(int(p->macroDestinations(0).size()),4);
            auto assigned=p->macroDestinations(0);expect(p->removeMacroDestination(0,assigned[0]));expectEquals(int(p->macroDestinations(0).size()),3);
            struct Listener final:juce::AudioProcessorParameter::Listener{int begins=0,ends=0,values=0;void parameterValueChanged(int,float)override{++values;}void parameterGestureChanged(int,bool b)override{b?++begins:++ends;}} listener;
            auto* host=p->getParameters().getParameter(params::macroId(0));host->addListener(&listener);const auto slot=p->getProject().layers[0].mixChain[1];
            p->nativeMacroGesture(slot,"makeupDb",true);p->nativeMacroTouch(slot,"makeupDb",.65f);p->nativeMacroGesture(slot,"makeupDb",false);
            expectEquals(listener.begins,1);expectEquals(listener.ends,1);expect(listener.values>0);expectWithinAbsoluteError(host->getValue(),.65f,1.e-6f);host->removeListener(&listener);
            set(*p,"finalTrim",-3);juce::MemoryBlock state;p->getStateInformation(state);auto reopened=std::make_unique<FxPluginProcessor>();reopened->setStateInformation(state.getData(),int(state.getSize()));
            expectEquals(int(reopened->macroDestinations(0).size()),3);expectEquals(reopened->getParameters().getRawParameterValue("finalTrim")->load(),-3.f);
            auto savedTree=juce::ValueTree::fromXml(*juce::AudioProcessor::getXmlFromBinary(state.getData(),int(state.getSize())));
            auto link=savedTree.getChildWithName("HpeqProject").getChild(0).getChildWithName("MixChain").getChild(0).getChildWithName("MacroBinding");
            // Walk by type: project layer ordering and MIX names are part of the persisted contract.
            if(!link.isValid()){
                std::function<juce::ValueTree(juce::ValueTree)> findLink=[&](auto tree){if(tree.hasType("MacroBinding"))return tree;for(auto child:tree){auto found=findLink(child);if(found.isValid())return found;}return juce::ValueTree();};link=findLink(savedTree);
            }
            expect(link.isValid());for(const auto bad:{"0.5","abc","99"}){link.setProperty("macro",bad,nullptr);juce::String error;expect(!fromValueTree(savedTree.getChildWithName("HpeqProject"),&error).has_value(),"bad XML macro "+juce::String(bad));}
            p->prepareToPlay(48000,256);reopened->prepareToPlay(48000,256);expect(exact(render(*p,127),render(*reopened,127)),"saved macro/range/trim must reproduce audio");
            expect(p->clearMacro(0));expect(p->macroDestinations(0).empty());expect(p->getUndoManager().undo());expectEquals(int(p->macroDestinations(0).size()),3);
        }
    }
    beginTest("Unassign and Clear keep current physical values, including reversed ranges, and undo restores mappings");{
        auto p=std::make_unique<FxPluginProcessor>();expect(p->setProject(compModel()));
        for(const auto& t:p->macroDestinations())if(t.parameter=="makeupDb"){expect(p->assignMacro(0,t));break;}
        set(*p,params::macroId(0),norm(FxType::Compressor,"makeupDb",6));auto target=p->macroDestinations(0)[0];expect(p->removeMacroDestination(0,target));
        expectWithinAbsoluteError(float(p->getProject().layers[0].mixChain[0].parameters["makeupDb"]),6.f,.001f);expect(p->getUndoManager().undo());expectEquals(int(p->macroDestinations(0).size()),1);
        for(const auto& t:p->macroDestinations())if(t.parameter=="thresholdDb"){expect(p->assignMacro(0,t,1,0));break;}
        set(*p,params::macroId(0),.25f);float expectedMakeup=0,expectedThreshold=0;
        for(const auto& s:native::layout(FxType::Compressor)){if(s.id=="makeupDb")expectedMakeup=s.range.snapToLegalValue(s.range.convertFrom0to1(.25f));if(s.id=="thresholdDb")expectedThreshold=s.range.snapToLegalValue(s.range.convertFrom0to1(.75f));}
        expect(p->clearMacro(0));const auto slot=p->getProject().layers[0].mixChain[0];expect(slot.macros.empty());
        expectWithinAbsoluteError(float(slot.parameters["makeupDb"]),expectedMakeup,.001f);expectWithinAbsoluteError(float(slot.parameters["thresholdDb"]),expectedThreshold,.001f);
        expect(p->getUndoManager().undo());expectEquals(int(p->macroDestinations(0).size()),2);
    }
    beginTest("Actual embedded-editor gestures end on the original host macro after Clear or module deletion");{
        for(bool removeModule:{false,true}){
            auto p=std::make_unique<FxPluginProcessor>();expect(p->setProject(compModel()));
            for(const auto& t:p->macroDestinations())if(t.parameter=="makeupDb"){expect(p->assignMacro(0,t));break;}
            auto editor=std::make_unique<FxPluginEditor>(*p);editor->selectMix(0);
            std::function<ui::EmbeddedPluginPanel*(juce::Component&)> findPanel=[&](auto& c)->ui::EmbeddedPluginPanel*{if(auto* panel=dynamic_cast<ui::EmbeddedPluginPanel*>(&c))return panel;for(auto* child:c.getChildren())if(auto* panel=findPanel(*child))return panel;return nullptr;};
            auto* panel=findPanel(*editor);expect(panel&&panel->originalEditor());
            juce::RangedAudioParameter* nativeParameter=nullptr;
            if(panel&&panel->originalEditor())for(auto* param:panel->originalEditor()->getAudioProcessor()->getParameters())if(auto* ranged=dynamic_cast<juce::RangedAudioParameter*>(param);ranged&&ranged->paramID=="makeupDb")nativeParameter=ranged;
            expect(nativeParameter!=nullptr);
            if(nativeParameter){
                struct Listener final:juce::AudioProcessorParameter::Listener{int begins=0,ends=0;void parameterValueChanged(int,float)override{}void parameterGestureChanged(int,bool b)override{b?++begins:++ends;}} listener;
                auto* host=p->getParameters().getParameter(params::macroId(0));host->addListener(&listener);
                nativeParameter->beginChangeGesture();expectEquals(listener.begins,1);
                if(removeModule)expect(p->setProject(FxProjectState{}));else expect(p->clearMacro(0));
                nativeParameter->endChangeGesture();expectEquals(listener.ends,1);editor.reset();host->removeListener(&listener);
            }
        }
    }
    beginTest("Native host automation with editor closed: fixed-clock output matches blocks 1/64/127/512/4096 after prepare256");{
        juce::AudioBuffer<float> reference;
        for(int block:{1,64,127,512,4096}){auto p=std::make_unique<FxPluginProcessor>();auto model=compModel();model.layers[0].mixChain[0].parameters.set("ratio",1.);expect(p->setProject(model));
            for(const auto& t:p->macroDestinations())if(t.parameter=="makeupDb"){expect(p->assignMacro(0,t));break;}
            p->prepareToPlay(48000,256);auto y=render(*p,block,24000,false,false,true);
            if(block==1)reference=y;else expect(exact(reference,y),"automation block mismatch "+juce::String(block));
            expect(y.getMagnitude(0,y.getNumSamples())>.01f,"macro must change audio");
        }
    }
    beginTest("External detector: all five models duck quiet program; key never leaks; mono and stereo buses");{
        for(int model=0;model<5;++model){auto a=std::make_unique<FxPluginProcessor>(),b=std::make_unique<FxPluginProcessor>(),silent=std::make_unique<FxPluginProcessor>();
            expect(keyLayout(*a,model%2+1,model%2+1));expect(keyLayout(*b,model%2+1,model%2+1));expect(keyLayout(*silent,model%2+1,model%2+1));
            expect(a->setProject(compModel(model,true)));expect(b->setProject(compModel(model,true)));expect(silent->setProject(compModel(model,true)));
            a->prepareToPlay(48000,256);b->prepareToPlay(48000,256);silent->prepareToPlay(48000,256);
            const auto loud=render(*a,127,48000,true),quiet=render(*b,127,48000,false),zero=render(*silent,127,24000,true,true);
            const double difference=10*std::log10(energy(loud)/energy(quiet));logMessage("model "+juce::String(model)+" external reduction "+juce::String(difference,3)+" dB");
            expect(std::isfinite(difference)&&difference<-6);expectEquals(zero.getMagnitude(0,zero.getNumSamples()),0.f);
        }
    }
    beginTest("Macro + external key callback is allocation-free (Debug covers malloc and new)");{
        auto p=std::make_unique<FxPluginProcessor>();expect(keyLayout(*p));expect(p->setProject(compModel(0,true)));
        for(const auto& t:p->macroDestinations())if(t.parameter=="makeupDb"){expect(p->assignMacro(0,t));break;}
        p->prepareToPlay(48000,256);juce::AudioBuffer<float> b(4,127);b.clear();juce::MidiBuffer midi;
        p->processBlock(b,midi);std::uint64_t allocations=0;
        for(int i=0;i<100;++i){
            set(*p,params::macroId(0),i%2?1.f:0.f);set(*p,"finalTrim",i%2?-6.f:0.f);
            for(int ch=0;ch<4;++ch)juce::FloatVectorOperations::fill(b.getWritePointer(ch),ch<2?.01f:.5f,127);
            ScopedAllocationProbe probe;p->processBlock(b,midi);allocations+=probe.counts().allocations;
        }
        expectEquals(juce::int64(allocations),juce::int64(0));
    }
    beginTest("External key aligns to preceding slot, serial layer and pre-MIX region latency");{
        // First used Fader, whose reported latency is zero. Exercise a real
        // fixed 5 ms delay with neutral Udarnik and read detector GR per sample.
        for(int route=0;route<3;++route){auto p=std::make_unique<FxPluginProcessor>();expect(keyLayout(*p));auto model=compModel(0,true);auto gain=makeNativeModule(FxType::TransientShaper);
            if(route==0)model.layers[0].mixChain.insert(model.layers[0].mixChain.begin(),gain);
            if(route==1){model.routing=LayerRouting::Serial;model.layers[1].mixChain=model.layers[0].mixChain;model.layers[0].mixChain={gain};}
            if(route==2){FxNodeState node;node.id=model.nextNodeId++;node.chain={gain};model.layers[0].fxNodes={node};}
            expect(p->setProject(model));p->prepareToPlay(48000,256);const int delay=p->getLatencySamples();expect(delay>0);
            constexpr int trigger=8192;const int length=trigger+delay+256;juce::AudioBuffer<float> b(4,1);juce::MidiBuffer midi;int onset=-1;
            for(int at=0;at<length;++at){b.clear();
                for(int ch=0;ch<2;++ch)b.setSample(ch,0,.01f);
                if(at==trigger){b.setSample(2,0,.5f);b.setSample(3,0,.5f);}p->processBlock(b,midi);
                if(onset<0&&p->nativeMeters(route==1?1:0,8,route==0?1:0).reduction>.01f)onset=at;
            }
            logMessage("key route "+juce::String(route)+": latency "+juce::String(delay)+", onset "+juce::String(onset));expectEquals(onset,trigger+delay);
        }
    }
    beginTest("Missing key falls back; centered key ducks Side; Dual Mono keeps independent key channels");{
        auto fallback=std::make_unique<FxPluginProcessor>(),internal=std::make_unique<FxPluginProcessor>();expect(fallback->setProject(compModel(0,true)));expect(internal->setProject(compModel()));
        fallback->prepareToPlay(48000,256);internal->prepareToPlay(48000,256);expect(exact(render(*fallback,127),render(*internal,127)));
        auto side=std::make_unique<FxPluginProcessor>(),sideDry=std::make_unique<FxPluginProcessor>();expect(keyLayout(*side));expect(keyLayout(*sideDry));auto s=compModel(0,true);s.layers[0].channel=ChannelMode::Side;
        expect(side->setProject(s));expect(sideDry->setProject(s));side->prepareToPlay(48000,256);sideDry->prepareToPlay(48000,256);
        const auto wet=render(*side,127,48000,true,false,false,.01f,true),dry=render(*sideDry,127,48000,false,false,false,.01f,true);
        expect(10*std::log10(energy(wet)/energy(dry))<-6);
        auto dual=std::make_unique<FxPluginProcessor>();expect(keyLayout(*dual));s=compModel(0,true);s.layers[0].channel=ChannelMode::DualMono;expect(dual->setProject(s));dual->prepareToPlay(48000,256);
        const auto y=render(*dual,127,48000,true,false,false,.01f,false,true);double left=0,right=0;
        for(int i=24000;i<48000;++i){left+=double(y.getSample(0,i))*y.getSample(0,i);right+=double(y.getSample(1,i))*y.getSample(1,i);}expect(10*std::log10(left/right)<-6);
    }
    beginTest("Final Trim follows the Master limiter without changing its GR; peak-only reset retains integrated loudness");{
        auto a=std::make_unique<FxPluginProcessor>(),b=std::make_unique<FxPluginProcessor>();FxProjectState model;model.routing=LayerRouting::Serial;model.masterRoute=true;
        auto limiter=makeNativeModule(FxType::Limiter);limiter.parameters.set("ceilingDb",-12.);model.layers[9].mixChain={limiter};expect(a->setProject(model));expect(b->setProject(model));set(*b,"finalTrim",-6);
        a->prepareToPlay(48000,256);b->prepareToPlay(48000,256);const auto x=render(*a,127,48000,false,false,false,.9f),y=render(*b,127,48000,false,false,false,.9f);
        expectWithinAbsoluteError(10*std::log10(energy(y)/energy(x)),-6.,.02);
        const auto ga=a->nativeMeters(9,8,0).reduction,gb=b->nativeMeters(9,8,0).reduction;expect(ga>1);expectWithinAbsoluteError(ga,gb,1.e-5f);
        auto peak=std::make_unique<FxPluginProcessor>();set(*peak,"finalTrim",12);peak->prepareToPlay(48000,256);render(*peak,127,48000,false,false,false,.5f);
        expect(peak->getOutputLoudness().truePeakDbtp()>0);peak->resetOutputPeak();render(*peak,127,48000);
        expect(peak->getOutputLoudness().truePeakDbtp()<0);expect(peak->getOutputLoudness().integratedLufs()>-15);
        set(*peak,"bypass",1);render(*peak,127,2400);const auto bypass=render(*peak,127);auto neutral=std::make_unique<FxPluginProcessor>();neutral->prepareToPlay(48000,256);expect(exact(bypass,render(*neutral,127)));
    }
    beginTest("Legacy state clears new controls; peak reset retains loudness; final trim is post-effect");{
        auto p=std::make_unique<FxPluginProcessor>();juce::MemoryBlock state;p->getStateInformation(state);
        auto root=juce::ValueTree::fromXml(*juce::AudioProcessor::getXmlFromBinary(state.getData(),int(state.getSize())));auto paramsTree=root.getChildWithName("Parameters");
        for(int i=paramsTree.getNumChildren()-1;i>=0;--i)if(paramsTree.getChild(i)["id"].toString()!="outputGain"&&paramsTree.getChild(i)["id"].toString()!="bypass")paramsTree.removeChild(i,nullptr);
        juce::AudioProcessor::copyXmlToBinary(*root.createXml(),state);set(*p,params::macroId(0),1);set(*p,"finalTrim",6);p->setStateInformation(state.getData(),int(state.getSize()));
        expectEquals(p->getParameters().getRawParameterValue(params::macroId(0))->load(),0.f);expectEquals(p->getParameters().getRawParameterValue("finalTrim")->load(),0.f);
        p->prepareToPlay(48000,256);auto normal=render(*p,127);set(*p,"finalTrim",-6);auto trimmed=render(*p,127);expectWithinAbsoluteError(10*std::log10(energy(trimmed)/energy(normal)),-6.,.02);
    }
    beginTest("Real editor contains usable final controls and sixteen macro sliders at minimum size");{
        auto p=std::make_unique<FxPluginProcessor>();auto model=compModel();model.layers[0].mixChain.push_back(makeNativeModule(FxType::Gain));expect(p->setProject(model));auto destinations=p->macroDestinations();
        for(int i=0;i<control::kMacros&&i<int(destinations.size());++i){destinations=p->macroDestinations();expect(p->assignMacro(i,destinations[static_cast<size_t>(i)]));}
        auto editor=std::make_unique<FxPluginEditor>(*p);editor->setSize(1280,840);editor->selectMix(0);
        const auto folder=juce::File::getCurrentWorkingDirectory().getChildFile("out/controls-20261124/ui");folder.createDirectory();
        {auto stream=folder.getChildFile("compressor-sidechain.png").createOutputStream();expect(stream!=nullptr);if(stream){stream->setPosition(0);stream->truncate();expect(juce::PNGImageFormat().writeImageToStream(editor->createComponentSnapshot(editor->getLocalBounds()),*stream));}}
        editor->setSize(1000,700);editor->showMacroControls();
        auto* panel=editor->findChildWithID("macroRoutingPanel");expect(panel&&panel->isVisible());
        std::function<int(juce::Component&)> count=[&](auto& c){int n=dynamic_cast<juce::Slider*>(&c)&&c.getComponentID().startsWith("macro")?1:0;for(auto* child:c.getChildren())n+=count(*child);return n;};
        if(panel)expectEquals(count(*panel),16);expect(editor->findChildWithID("finalTrim")!=nullptr);
        for(auto size:{juce::Point<int>{1000,700},juce::Point<int>{1280,840}}){editor->setSize(size.x,size.y);auto image=editor->createComponentSnapshot(editor->getLocalBounds());
            auto stream=folder.getChildFile("macros-"+juce::String(size.x)+".png").createOutputStream();expect(stream!=nullptr);if(stream){stream->setPosition(0);stream->truncate();expect(juce::PNGImageFormat().writeImageToStream(image,*stream));}}
    }
}
};static ControlStageTests controls;
}

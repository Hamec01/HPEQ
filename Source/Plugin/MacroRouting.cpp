#include "Plugin/FxPluginProcessor.h"
#include "Plugin/FxParameterContract.h"
#include <cmath>

namespace hpeq {
namespace {
template<class P,class F> void visit(P& p,F fn){
    for(int layer=0;layer<kNumLayers;++layer){auto& l=p.layers[static_cast<size_t>(layer)];
        for(int slot=0;slot<int(l.mixChain.size());++slot)fn(l.mixChain[static_cast<size_t>(slot)],layer,0,0u,slot);
        for(auto& n:l.fxNodes)for(int slot=0;slot<int(n.chain.size());++slot)fn(n.chain[static_cast<size_t>(slot)],layer,1,n.id,slot);
        for(auto& n:l.eqNodes)for(int slot=0;slot<int(n.chain.size());++slot)fn(n.chain[static_cast<size_t>(slot)],layer,2,n.id,slot);
    }
}
FxSlotState* find(FxProjectState& p,const FxPluginProcessor::MacroDestination& t){
    FxSlotState* result=nullptr;visit(p,[&](auto& s,int layer,int chain,std::uint32_t node,int slot){
        if(layer==t.layer&&chain==t.chain&&node==t.node&&slot==t.slot)result=&s;});return result;
}
void retainValue(FxSlotState& slot,const MacroAssignment& binding,float macro){
    for(const auto& spec:native::layout(slot.type))if(spec.id==binding.parameter){
        const auto n=juce::jlimit(0.f,1.f,binding.minimum+(binding.maximum-binding.minimum)*macro);
        auto value=spec.range.snapToLegalValue(spec.range.convertFrom0to1(n));
        for(int pass=0;pass<2;++pass)value=spec.range.snapToLegalValue(spec.range.convertFrom0to1(spec.range.convertTo0to1(value)));
        slot.parameters.set(spec.id,static_cast<double>(value));break;
    }
}
}
std::vector<FxPluginProcessor::MacroDestination> FxPluginProcessor::macroDestinations(int macro) const {
    const juce::ScopedLock lock(projectLock);auto p=project;const auto revision=projectRevision.load(std::memory_order_relaxed);std::vector<MacroDestination> result;
    visit(p,[&](auto& s,int layer,int chain,std::uint32_t node,int slot){
        if(s.engineRevision!=native::kRevision)return;
        for(const auto& spec:native::layout(s.type)){
            MacroDestination t;t.layer=layer;t.chain=chain;t.node=node;t.slot=slot;t.parameter=spec.id;t.revision=revision;
            juce::String region="Mix";
            if(chain==1)for(const auto& n:p.layers[static_cast<size_t>(layer)].fxNodes)if(n.id==node)region="FX "+juce::String(n.lowHz,0)+"-"+juce::String(n.highHz,0)+" Hz";
            if(chain==2)for(const auto& n:p.layers[static_cast<size_t>(layer)].eqNodes)if(n.id==node)region="EQ "+juce::String(n.frequencyHz,0)+" Hz";
            t.label="L"+juce::String(layer+1)+" / "+region
                +" / "+juce::String(slot+1)+" "+native::productName(s.type)+" / "+spec.name;
            if(macro<0)result.push_back(t);
            else for(const auto& m:s.macros)if(m.macro==macro&&m.parameter==spec.id){t.minimum=m.minimum;t.maximum=m.maximum;result.push_back(t);}
        }
    });return result;
}
bool FxPluginProcessor::assignMacro(int macro,const MacroDestination& target,float minimum,float maximum){
    if(macro<0||macro>=control::kMacros||!std::isfinite(minimum)||!std::isfinite(maximum)||minimum<0||minimum>1||maximum<0||maximum>1)return false;
    if(target.revision!=projectRevision.load(std::memory_order_acquire))return false;
    auto p=getProject();auto* s=find(p,target);if(!s||s->engineRevision!=native::kRevision)return false;
    const native::Parameter* spec=nullptr;for(const auto& candidate:native::layout(s->type))if(candidate.id==target.parameter)spec=&candidate;
    if(!spec)return false;
    if(s->macros.size()>=control::kMaxBindings&&std::none_of(s->macros.begin(),s->macros.end(),[&](const auto& m){return m.parameter==target.parameter;}))return false;
    const bool first=macroDestinations(macro).empty();auto* host=parameters.getParameter(params::macroId(macro));const auto previous=host->getValue();
    if(first&&maximum!=minimum){
        const auto value=s->parameters.contains(spec->id)?float(s->parameters[spec->id]):spec->initial;
        float current=spec->range.convertTo0to1(value);
        for(const auto& assigned:s->macros)if(assigned.parameter==target.parameter)
            current=assigned.minimum+(assigned.maximum-assigned.minimum)*parameters.getParameter(params::macroId(assigned.macro))->getValue();
        const auto norm=juce::jlimit(0.f,1.f,(current-minimum)/(maximum-minimum));
        host->beginChangeGesture();host->setValueNotifyingHost(norm);host->endChangeGesture();
        macroResetSerial[static_cast<size_t>(macro)].fetch_add(1,std::memory_order_release);
    }
    undoManager.beginNewTransaction("Assign Macro "+juce::String(macro+1));
    const bool ok=editProject("Assign Macro "+juce::String(macro+1),[&](auto& project){if(auto* slot=find(project,target)){
        auto& bindings=slot->macros;bindings.erase(std::remove_if(bindings.begin(),bindings.end(),[&](const auto& m){return m.parameter==target.parameter;}),bindings.end());
        if(bindings.size()<control::kMaxBindings)bindings.push_back({macro,target.parameter,minimum,maximum});
    }});
    if(!ok&&first){host->setValueNotifyingHost(previous);macroResetSerial[static_cast<size_t>(macro)].fetch_add(1,std::memory_order_release);}return ok;
}
bool FxPluginProcessor::clearMacro(int macro){
    if(macro<0||macro>=control::kMacros)return false;
    const auto value=parameters.getParameter(params::macroId(macro))->getValue();
    undoManager.beginNewTransaction("Clear Macro "+juce::String(macro+1));
    return editProject("Clear Macro "+juce::String(macro+1),[&](auto& p){visit(p,[&](auto& s,int,int,std::uint32_t,int){
        for(const auto& m:s.macros)if(m.macro==macro)retainValue(s,m,value);
        s.macros.erase(std::remove_if(s.macros.begin(),s.macros.end(),[&](const auto& m){return m.macro==macro;}),s.macros.end());});});
}
bool FxPluginProcessor::removeMacroDestination(int macro,const MacroDestination& target){
    if(macro<0||macro>=control::kMacros||target.revision!=projectRevision.load(std::memory_order_acquire))return false;
    const auto value=parameters.getParameter(params::macroId(macro))->getValue();
    undoManager.beginNewTransaction("Remove macro destination");
    return editProject("Remove macro destination",[&](auto& p){if(auto* s=find(p,target)){
        for(const auto& m:s->macros)if(m.macro==macro&&m.parameter==target.parameter)retainValue(*s,m,value);
        s->macros.erase(std::remove_if(s->macros.begin(),s->macros.end(),[&](const auto& m){return m.macro==macro&&m.parameter==target.parameter;}),s->macros.end());}});
}
void FxPluginProcessor::nativeMacroTouch(const FxSlotState& s,const juce::String& id,float value){
    for(const auto& m:s.macros)if(m.parameter==id&&m.maximum!=m.minimum)
        if(auto* p=parameters.getParameter(params::macroId(m.macro))){const auto n=juce::jlimit(0.f,1.f,(value-m.minimum)/(m.maximum-m.minimum));if(p->getValue()!=n)p->setValueNotifyingHost(n);}
}
void FxPluginProcessor::nativeMacroGesture(const FxSlotState& s,const juce::String& id,bool begin){
    for(const auto& m:s.macros)if(m.parameter==id)if(auto* p=parameters.getParameter(params::macroId(m.macro))){if(begin)p->beginChangeGesture();else p->endChangeGesture();}
}
}

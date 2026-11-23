#include "UI/EmbeddedPluginPanel.h"
#include <cmath>

namespace hpeq::ui
{
EmbeddedPluginPanel::EmbeddedPluginPanel()
{
    addAndMakeVisible(viewport);viewport.setViewedComponent(&content,false);
    viewport.setScrollBarsShown(true,false);startTimerHz(10);
}
EmbeddedPluginPanel::~EmbeddedPluginPanel()
{
    stopTimer();cancelPendingUpdate();handleAsyncUpdate();detach();
}
void EmbeddedPluginPanel::detach()
{
    synchronizing=true;
    if(runtime)
    {
        if(runtime->kind()==FxType::Gain)native::installFaderActionBridge(runtime->referenceProcessor(),{});
        runtime->referenceProcessor().removeListener(this);
    }
    if(editor)editor->removeComponentListener(this);
    editor.reset();runtime.reset();
    for(auto& flag:changed)flag.store(false);
    synchronizing=false;
}
void EmbeddedPluginPanel::refresh()
{
    const auto next=readRuntime?readRuntime():nullptr;
    if(next==runtime)return;
    detach();runtime=next;
    if(!runtime){repaint();return;}
    auto& processor=runtime->referenceProcessor();
    editor.reset(processor.createEditorIfNeeded());
    if(editor)
    {
        editor->setResizable(false,false);
        designWidth=editor->getWidth();designHeight=editor->getHeight();
        content.addAndMakeVisible(*editor);editor->addComponentListener(this);
    }
    structural=runtime->captureEditorState();
    if(runtime->kind()==FxType::Gain&&onFaderAction)
        native::installFaderActionBridge(processor,[this](int action){return onFaderAction(action);});
    processor.addListener(this);
    viewport.setViewPosition(0,0);resized();repaint();
}
void EmbeddedPluginPanel::resized()
{
    if(!editor||getWidth()<=0||getHeight()<=0)return;
    if(layingOut)return;const juce::ScopedValueSetter<bool> guard(layingOut,true);
    viewport.setBounds(getLocalBounds());
    const int width=juce::jmax(1,getWidth()-viewport.getScrollBarThickness());
    const float fit=std::min(static_cast<float>(width)/designWidth,static_cast<float>(getHeight())/designHeight);
    // Keep ordinary pages fitted. Tall original workbenches scroll instead of
    // shrinking all labels and controls below a useful reading size.
    const float scale=fit>=.65f?fit:std::min(1.f,static_cast<float>(width)/designWidth);
    content.setSize(width,juce::jmax(getHeight(),static_cast<int>(std::ceil(designHeight*scale))));
    editor->setTransform(juce::AffineTransform::scale(scale).translated(
        .5f*(width-designWidth*scale),.5f*juce::jmax(0.f,getHeight()-designHeight*scale)));
}
void EmbeddedPluginPanel::componentMovedOrResized(juce::Component& child,bool,bool wasResized)
{
    if(wasResized && &child==editor.get()&&(child.getWidth()!=designWidth||child.getHeight()!=designHeight))
    {designWidth=child.getWidth();designHeight=child.getHeight();resized();}
}
void EmbeddedPluginPanel::paint(juce::Graphics& g)
{
    if(editor)return;
    g.setColour(juce::Colour(0xffd5bf7c));
    g.drawFittedText("Preparing the original plug-in editor...",getLocalBounds(),juce::Justification::centred,2);
}
void EmbeddedPluginPanel::audioProcessorParameterChanged(juce::AudioProcessor* processor,int index,float normalized)
{
    if(synchronizing||!runtime||runtime->isSynchronizingEditor()||index<0||index>=native::kMaxParameters)return;
    auto* parameter=dynamic_cast<juce::RangedAudioParameter*>(processor->getParameters()[index]);
    if(!parameter)return;
    const float physical=parameter->convertFrom0to1(normalized);
    pending[static_cast<size_t>(index)].store(physical,std::memory_order_relaxed);
    changed[static_cast<size_t>(index)].store(true,std::memory_order_release);
    runtime->requestEditorValue(index,physical);
    triggerAsyncUpdate();
}
void EmbeddedPluginPanel::audioProcessorChanged(juce::AudioProcessor*,const ChangeDetails&)
{
    if(!synchronizing&&runtime&&!runtime->isSynchronizingEditor())triggerAsyncUpdate();
}
void EmbeddedPluginPanel::audioProcessorParameterChangeGestureBegin(juce::AudioProcessor*,int)
{
    if(onGesture&&!synchronizing)onGesture();
}
void EmbeddedPluginPanel::handleAsyncUpdate()
{
    if(flushing||!runtime||!readSlot||!onChange||(readRuntime&&readRuntime()!=runtime))return;
    const juce::ScopedValueSetter<bool> guard(flushing,true);
    auto slot=readSlot();if(slot.type!=runtime->kind()||slot.engineRevision!=native::kRevision)return;
    bool edited=false;
    const auto& parameters=runtime->referenceProcessor().getParameters();
    for(int index=0;index<parameters.size()&&index<native::kMaxParameters;++index)
        if(changed[static_cast<size_t>(index)].exchange(false,std::memory_order_acq_rel))
            if(auto* parameter=dynamic_cast<juce::RangedAudioParameter*>(parameters[index]))
            {
                slot.parameters.set(parameter->paramID,static_cast<double>(pending[static_cast<size_t>(index)].load(std::memory_order_relaxed)));
                edited=true;
            }
    if(slot.type==FxType::MonstaFx)
    {
        const auto bytes=runtime->captureEditorState();
        if(bytes!=structural){slot.nativeState=bytes;structural=bytes;edited=true;}
    }
    if(edited)onChange(slot);
}
void EmbeddedPluginPanel::timerCallback()
{
    refresh();
    // Scene and workbench buttons also edit native non-parameter state.
    if(runtime&&runtime->kind()==FxType::MonstaFx)handleAsyncUpdate();
}
}

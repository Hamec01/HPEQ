#pragma once
#include "Rack/NativeApi.h"
#include "Core/FxProjectState.h"
#include <functional>

namespace hpeq::ui
{
// The original editor operates the same prepared processor that renders audio.
class EmbeddedPluginPanel final : public juce::Component, private juce::AudioProcessorListener,
                                  private juce::AsyncUpdater, private juce::Timer, private juce::ComponentListener
{
public:
    EmbeddedPluginPanel();
    ~EmbeddedPluginPanel() override;
    std::function<std::shared_ptr<native::Runtime>()> readRuntime;
    std::function<FxSlotState()> readSlot;
    std::function<void(const FxSlotState&)> onChange;
    std::function<void()> onGesture;
    std::function<void(const juce::String&,float)> onParameterTouch;
    std::function<void(const juce::String&,bool)> onParameterGesture;
    std::function<bool(int)> onFaderAction;
    void refresh();
    void flushPendingEdits() {cancelPendingUpdate();handleAsyncUpdate();}
    void resized() override;
    void paint(juce::Graphics&) override;
    juce::AudioProcessorEditor* originalEditor() const noexcept {return editor.get();}

private:
    void detach();
    void handleAsyncUpdate() override;
    void timerCallback() override;
    void componentMovedOrResized(juce::Component&,bool,bool) override;
    void audioProcessorParameterChanged(juce::AudioProcessor*,int,float) override;
    void audioProcessorChanged(juce::AudioProcessor*,const ChangeDetails&) override;
    void audioProcessorParameterChangeGestureBegin(juce::AudioProcessor*,int) override;
    void audioProcessorParameterChangeGestureEnd(juce::AudioProcessor*,int) override;
    std::shared_ptr<native::Runtime> runtime;
    std::unique_ptr<juce::AudioProcessorEditor> editor;
    juce::Component content;
    juce::Viewport viewport;
    std::array<std::atomic<float>,native::kMaxParameters> pending {};
    std::array<std::atomic<bool>,native::kMaxParameters> changed {};
    std::array<std::atomic<bool>,native::kMaxParameters> touchForwarded {};
    std::array<bool,native::kMaxParameters> gestures {};
    juce::MemoryBlock structural;
    bool synchronizing=false,flushing=false,layingOut=false;
    int designWidth=1180,designHeight=940;
};
}

#include "UI/RevolutionSampleBar.h"
namespace hpeq::ui
{
RevolutionSampleBar::RevolutionSampleBar(standalone::ModulePluginProcessor& p):processor(p)
{
 setOpaque(true);
 for(auto* button:{&load,&play,&loop,&input,&render,&save,static_cast<juce::TextButton*>(&drag)})
 {
  addAndMakeVisible(button);button->setColour(juce::TextButton::buttonColourId,juce::Colour(0xff242922));button->setColour(juce::TextButton::textColourOffId,juce::Colour(0xffd8c788));button->setColour(juce::TextButton::buttonOnColourId,juce::Colour(0xff195c35));
 }
 loop.setClickingTogglesState(true);loop.setToggleState(true,juce::dontSendNotification);
 load.onClick=[this]{openLoad();};play.onClick=[this]{auto* w=processor.sampleWorkspace();if(isTransition())processor.setPreview(!processor.isPreviewing());else w->play(!w->isPlaying());};
 loop.onClick=[this]{processor.sampleWorkspace()->setLoop(loop.getToggleState());};input.onClick=[this]{fileMessage.clear();if(isTransition()){processor.useInputForTransition();renderTransition(true);}else processor.sampleWorkspace()->useInput();};
 render.onClick=[this]{fileMessage.clear();if(isTransition()){renderTransition(true);return;}const auto file=standalone::ModulePluginProcessor::rendersFolder().getChildFile("Sample_"+juce::Uuid().toString()+".wav");processor.renderSample(file,tail.getSelectedId()==1?0:tail.getSelectedId()==2?2:tail.getSelectedId()==3?4:8);};
 save.onClick=[this]{openSave();};drag.dragFile=[this]{if(values.ready&&values.rendered.existsAsFile())juce::DragAndDropContainer::performExternalDragDropOfFiles({values.rendered.getFullPathName()},false,this);};
 tail.addItem("Tail: 0 s",1);tail.addItem("Tail: 2 s",2);tail.addItem("Tail: 4 s",3);tail.addItem("Tail: 8 s",4);tail.setSelectedId(2);addAndMakeVisible(tail);
 load.setTooltip("Load mono/stereo WAV, AIFF, FLAC or supported MP3, up to 60 seconds. Drop audio anywhere on SIMPLE or ADVANCED. Temporary audition workspace; audio is not embedded in projects.");
 play.setTooltip("Play the source through the current effect using a local musical clock, including with host transport stopped.");
 loop.setTooltip("Loop the source without restarting the sequencer. Export is always one source pass.");
 input.setTooltip("Stop auditioning and return to the track input.");render.setTooltip("Render one source pass with a snapshot of current settings. Wait for WAV READY, then drag it to the DAW.");
 save.setTooltip("Save a fresh render to a chosen WAV file. 32-bit floating point preserves output headroom; no automatic normalization.");
 drag.setTooltip("Render first, then drag the finished WAV into your DAW or Explorer. Changing settings requires another render.");
 tail.setTooltip("Extra silence processed after the source. Long delay/reverb settings can require a longer tail.");refresh();
}
void RevolutionSampleBar::refresh()
{
 if(lastTransition!=isTransition()){fileMessage.clear();lastTransition=isTransition();}
 values=processor.sampleWorkspace()->status(processor.sampleRevision());
 if(isTransition())
 {
    const auto t=processor.lastTransition();values={};values.loaded=t.valid;values.ready=t.valid&&t.file.existsAsFile();values.rendered=t.file;values.source=t.sourceFile;values.playing=processor.isPreviewing();values.seconds=t.settings.sampleRate>0?dsp::transition::lengthSamples(t.settings)/t.settings.sampleRate:0;
    values.message=fileMessage.isEmpty()?juce::String("RISE / FILL: load/drop a source, preview, export or drag the transition."):fileMessage;
 }
 if(!isTransition())values.ready=values.ready&&values.tailSeconds==(tail.getSelectedId()==1?0:tail.getSelectedId()==2?2:tail.getSelectedId()==3?4:8);
 save.setTooltip(isTransition()?"Save the generated 24-bit transition WAV.":"Save a fresh 32-bit float WAV of the current effect, one source pass plus chosen tail. No normalization.");
 loop.setEnabled(!isTransition());tail.setEnabled(!isTransition());render.setButtonText(isTransition()?"GENERATE":"RENDER WAV");
 play.setButtonText(values.playing?"STOP":"PLAY");play.setEnabled(values.playing||(values.loaded&&!values.busy));render.setEnabled(values.loaded&&!values.busy);save.setEnabled(values.loaded&&!values.busy);drag.setEnabled(values.ready&&!values.busy);loop.setToggleState(values.loop,juce::dontSendNotification);repaint();
}
void RevolutionSampleBar::openLoad()
{
 fileMessage.clear();
 chooser=std::make_unique<juce::FileChooser>("Load sample",juce::File(),"*.wav;*.aif;*.aiff;*.flac;*.mp3;*.ogg");
 const juce::Component::SafePointer<RevolutionSampleBar> safe(this);
 chooser->launchAsync(juce::FileBrowserComponent::openMode|juce::FileBrowserComponent::canSelectFiles,[safe](const juce::FileChooser& c){if(safe&&c.getResult()!=juce::File())safe->loadFile(c.getResult());});
}
void RevolutionSampleBar::openSave()
{
 fileMessage.clear();
 const auto initial=standalone::ModulePluginProcessor::rendersFolder().getChildFile(values.source.getFileNameWithoutExtension()+"_Revolution.wav");
 chooser=std::make_unique<juce::FileChooser>(isTransition()?"Export generated transition - 24-bit WAV":"Export current settings - 32-bit float WAV",initial,"*.wav");
 const juce::Component::SafePointer<RevolutionSampleBar> safe(this);
 chooser->launchAsync(juce::FileBrowserComponent::saveMode|juce::FileBrowserComponent::canSelectFiles|juce::FileBrowserComponent::warnAboutOverwriting,[safe](const juce::FileChooser& c)
 {
    if(safe&&c.getResult()!=juce::File())
    {
        const auto destination=c.getResult();
        if(!destination.hasFileExtension("wav")){safe->fileMessage="Choose a filename ending in .wav.";return;}
        if(safe->isTransition())
        {
            const auto source=safe->processor.lastTransition().file;
            if(destination==source||destination==safe->processor.lastTransition().sourceFile){safe->fileMessage="Choose another filename; source and dragged files stay unchanged.";return;}
            juce::TemporaryFile temp(destination);
            const bool ok=source.copyFileTo(temp.getFile())&&temp.overwriteTargetFileWithTemporary();
            safe->fileMessage=ok?"Saved transition: "+destination.getFileName():"Export failed. Check the destination folder and try again.";
        }
        else safe->processor.renderSample(destination,safe->tail.getSelectedId()==1?0:safe->tail.getSelectedId()==2?2:safe->tail.getSelectedId()==3?4:8);
    }
 });
}
bool RevolutionSampleBar::isTransition()const noexcept{return processor.getPage()==standalone::ModulePluginProcessor::Page::Transition;}
void RevolutionSampleBar::renderTransition(bool capture)
{
 fileMessage.clear();
 const auto t=processor.lastTransition();processor.makeTransition(t.settings.kind,t.settings.bars,t.settings.knobs,capture,false,t.settings.recipe,t.settings.fillBeats,t.settings.fillFx,t.settings.kitEnabled);
 if(onTransitionRendered)onTransitionRendered();
 refresh();
}
void RevolutionSampleBar::loadFile(const juce::File& f)
{
 fileMessage.clear();
 if(isTransition()){if(processor.useSampleForTransition(f))renderTransition(true);else fileMessage="Cannot read the selected source. Previous transition retained.";}
 else processor.sampleWorkspace()->load(f);
 refresh();
}
bool RevolutionSampleBar::isInterestedInFileDrag(const juce::StringArray& files)
{for(const auto& f:files)if(juce::File(f).hasFileExtension("wav;aif;aiff;flac;mp3;ogg"))return true;return false;}
void RevolutionSampleBar::filesDropped(const juce::StringArray& files,int,int)
{for(const auto& f:files)if(isInterestedInFileDrag({f})){loadFile(juce::File(f));break;}}
juce::Rectangle<int> RevolutionSampleBar::controlBounds(int i)const
{
 const float scale=getWidth()/1180.f;const int starts[]{12,180,286,388,540,710,880,1040},widths[]{156,94,90,140,158,158,148,126};
 return juce::Rectangle<float>(static_cast<float>(starts[i]),10.f,static_cast<float>(widths[i]),38.f).operator*(scale).toNearestInt();
}
void RevolutionSampleBar::resized()
{
 juce::Component* controls[]{&load,&play,&loop,&input,&render,&save,&drag,&tail};for(int i=0;i<8;++i)controls[i]->setBounds(controlBounds(i));
}
void RevolutionSampleBar::paint(juce::Graphics& g)
{
 g.fillAll(juce::Colour(0xff161a17));const float scale=getWidth()/1180.f;g.addTransform(juce::AffineTransform::scale(scale));
 g.setColour(juce::Colour(0xff69705a));g.drawLine(0,0,1180,0,2);
 const juce::Rectangle<float> wave(12,60,1156,42);g.setColour(juce::Colour(0xff080d09));g.fillRoundedRectangle(wave,4);
 g.setColour(juce::Colour(0xff6dff9a));float peak=1e-6f;for(float v:values.wave)peak=std::max(peak,v);
 for(size_t i=0;i<values.wave.size();++i){const float x=16+static_cast<float>(i)*1148/128,y=19*values.wave[i]/peak;g.drawLine(x,81-y,x,81+y,3);}
 if(values.playing){const float x=12+static_cast<float>(values.progress)*1156;g.setColour(juce::Colour(0xffffdb72));g.drawLine(x,60,x,102,2);}
 g.setColour(juce::Colour(0xffd8c788));g.setFont(14.f);
 const auto title=values.loaded?(values.source==juce::File()?juce::String("TRACK CAPTURE"):values.source.getFileName())+"  |  "+juce::String(values.seconds,2)+" s  |  "+juce::String(processor.getHostBpm(),1)+" BPM":"SAMPLE WORKSPACE - DROP AUDIO HERE OR LOAD SAMPLE";
 g.drawText(title,12,106,1156,20,juce::Justification::centredLeft);
 g.setColour(juce::Colour(0xff9fd4ac));g.setFont(12.f);
 auto message=fileMessage.isNotEmpty()?fileMessage:values.message;if(!values.ready&&!values.busy&&values.rendered!=juce::File()&&message.startsWith("WAV ready"))message="Settings/source/tail changed - RENDER WAV again before dragging.";
 if(values.ready&&!isTransition())message+="  Peak: "+juce::String(values.peakDb,1)+" dBFS";
 g.drawText(message,12,130,1156,20,juce::Justification::centredLeft);
}
}

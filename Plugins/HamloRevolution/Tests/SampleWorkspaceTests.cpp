#include "Standalone/ModulePluginEditor.h"
#include "AllocationAudit.h"
#include "TestSignals.h"
#include <thread>
namespace hamlo::test
{
namespace
{
using P=hpeq::standalone::ModulePluginProcessor;using W=hpeq::standalone::RevolutionSample;
constexpr auto kind=hpeq::standalone::ModuleKind::Revolution;
bool wait(W& w,std::uint64_t revision,bool loaded=true)
{
 const auto start=juce::Time::getMillisecondCounter();
 while(juce::Time::getMillisecondCounter()-start<15000)
 {const auto s=w.status(revision);if(!s.busy&&(!loaded||s.loaded))return true;juce::Thread::sleep(2);}return false;
}
bool write(const juce::File& file,const juce::AudioBuffer<float>& x,double fs)
{
 juce::WavAudioFormat wav;auto stream=file.createOutputStream();std::unique_ptr<juce::OutputStream> owned(stream.release());const auto options=juce::AudioFormatWriterOptions{}.withSampleRate(fs).withNumChannels(x.getNumChannels()).withBitsPerSample(32).withSampleFormat(juce::AudioFormatWriterOptions::SampleFormat::floatingPoint);auto writer=wav.createWriterFor(owned,options);return writer&&writer->writeFromAudioSampleBuffer(x,0,x.getNumSamples());
}
juce::AudioBuffer<float> read(const juce::File& file,double& fs)
{
 juce::WavAudioFormat wav;auto stream=file.createInputStream();std::unique_ptr<juce::AudioFormatReader> r(wav.createReaderFor(stream.release(),true));if(!r)return {};fs=r->sampleRate;juce::AudioBuffer<float> b(static_cast<int>(r->numChannels),static_cast<int>(r->lengthInSamples));r->read(&b,0,b.getNumSamples(),0,true,true);return b;
}
struct Head:juce::AudioPlayHead{double ppq=0;juce::Optional<PositionInfo> getPosition()const override{PositionInfo p;p.setBpm(120);p.setIsPlaying(true);p.setPpqPosition(ppq);return p;}};
}
class SampleWorkspaceTests final:public juce::UnitTest
{
public:
 SampleWorkspaceTests():juce::UnitTest("Sample workspace R9","HamloRevolution"){}
 void runTest() override
 {
  beginTest("Sample fixture is written successfully");
  const auto root=juce::File::getCurrentWorkingDirectory().getChildFile("build/r9_tests_"+juce::Uuid().toString());root.createDirectory();
  const auto source=root.getChildFile("source.wav");const auto x=drums(2);expect(write(source,x,48000));
  beginTest("Matching-rate load is exact; input mode does not touch host audio");
  auto p=std::make_unique<P>(kind);p->prepareToPlay(48000,256);auto& w=*p->sampleWorkspace();expect(w.load(source));expect(wait(w,p->sampleRevision()));
  w.play(true);juce::AudioBuffer<float> sample(2,4096);allocation::begin();auto clock=w.fill(sample,2);const auto allocated=allocation::finish();expect(allocated==0&&clock.active&&clock.frame==0);
  expect(bitExact(sample,segment(x,0,4096)));w.useInput();sample.makeCopyOf(x);const auto original=sample;clock=w.fill(sample,2);expect(!clock.active&&bitExact(sample,original));
  beginTest("Invalid, nonfinite and too-long files preserve the previous source");
  expect(w.load(source));expect(wait(w,p->sampleRevision()));expect(w.load(root.getChildFile("missing.wav")));expect(wait(w,p->sampleRevision(),false));expect(w.status(p->sampleRevision()).source==source);
  auto invalid=sine(.1,440,-12);invalid.setSample(0,3,std::numeric_limits<float>::quiet_NaN());auto bad=root.getChildFile("nan.wav");expect(write(bad,invalid,48000));w.load(bad);expect(wait(w,p->sampleRevision(),false));expect(w.status(p->sampleRevision()).source==source);
  juce::AudioBuffer<float> longClip(1,61*8000);longClip.clear();auto tooLong=root.getChildFile("long.wav");expect(write(tooLong,longClip,8000));w.load(tooLong);expect(wait(w,p->sampleRevision(),false));expect(w.status(p->sampleRevision()).source==source);
  beginTest("Mono resampling preserves pitch, duration, channel identity and finite output");
  juce::AudioBuffer<float> mono(1,44100);for(int i=0;i<44100;++i)mono.setSample(0,i,static_cast<float>(.2*std::sin(2*juce::MathConstants<double>::pi*440*i/44100)));const auto monoFile=root.getChildFile("mono44100.wav");expect(write(monoFile,mono,44100));w.load(monoFile);expect(wait(w,p->sampleRevision()));w.setLoop(false);w.play(true);sample.setSize(2,48000);clock=w.fill(sample,2);
  expect(clock.active&&sample.getNumSamples()==48000);bool finite=true,equal=true;double e=0,cosine=0,sinus=0;
  for(int i=1000;i<47000;++i){const auto v=sample.getSample(0,i);finite=finite&&std::isfinite(v);equal=equal&&v==sample.getSample(1,i);const double q=2*juce::MathConstants<double>::pi*440*i/48000;cosine+=v*std::cos(q);sinus+=v*std::sin(q);e+=v*v;}
  expect(finite&&equal);const double amplitude=2*std::hypot(cosine,sinus)/46000;expectWithinAbsoluteError(amplitude,.2,.001);expect(e>0&&!w.isPlaying());
  beginTest("Downsampling removes out-of-band input, rather than folding into16kHz");
  juce::AudioBuffer<float> high(1,48000);for(int i=0;i<48000;++i)high.setSample(0,i,static_cast<float>(.2*std::sin(2*juce::MathConstants<double>::pi*32000*i/96000)));
  const auto highFile=root.getChildFile("high96.wav");expect(write(highFile,high,96000));w.load(highFile);expect(wait(w,p->sampleRevision()));w.play(true);sample.setSize(2,24000);w.fill(sample,2);
  double aliased=0;for(int i=1000;i<23000;++i)aliased+=sample.getSample(0,i)*sample.getSample(0,i);
  const double aliasDb=10*std::log10(aliased/22000+1e-30);logMessage("Downsampling residual="+juce::String(aliasDb,3)+" dBFS");expect(aliasDb<-70);
  w.load(monoFile);expect(wait(w,p->sampleRevision()));
  beginTest("Rate changes use cached source on the worker and stop playback");
  p->prepareToPlay(96000,256);expect(!w.isPlaying());expect(wait(w,p->sampleRevision()));sample.setSize(2,96000);w.play(true);clock=w.fill(sample,2);expect(clock.active);expectWithinAbsoluteError(w.status(p->sampleRevision()).seconds,1.,.0001);
  beginTest("Sample playback through actual sequencer is exact across callback schedules");
  juce::MemoryBlock reference;
  for(int block:{1,64,127,512,4096})
  {
   auto q=std::make_unique<P>(kind);q->makeBold();q->prepareToPlay(48000,256);auto& workspace=*q->sampleWorkspace();workspace.load(source);expect(wait(workspace,q->sampleRevision()));workspace.setLoop(true);workspace.play(true);
   juce::MemoryBlock raw;juce::MidiBuffer midi;bool allocations=false;
   for(int at=0;at<144000;)
   {
    const int count=std::min(block,144000-at);juce::AudioBuffer<float> b(2,count);b.clear();allocation::begin();q->processBlock(b,midi);allocations=allocation::finish()!=0||allocations;
    for(int i=0;i<count;++i)for(int c=0;c<2;++c){const auto v=b.getSample(c,i);raw.append(&v,sizeof(v));}at+=count;
   }
   expect(!allocations,"no callback allocation, block"+juce::String(block));if(block==1)reference=raw;else expect(raw==reference,"PCM block"+juce::String(block));
  }
  beginTest("32-bit float export is the actual offline sequencer output, with selected tail");
  p=std::make_unique<P>(kind);p->makeBold();p->prepareToPlay(48000,256);auto& exportW=*p->sampleWorkspace();exportW.load(source);expect(wait(exportW,p->sampleRevision()));const auto destination=root.getChildFile("result.wav");expect(p->renderSample(destination,2));expect(wait(exportW,p->sampleRevision()));expect(exportW.status(p->sampleRevision()).ready);
  double fs=0;auto exported=read(destination,fs);expect(fs==48000&&exported.getNumSamples()==192000&&exported.getNumChannels()==2);
  juce::MemoryBlock state;p->getStateInformation(state);auto offline=std::make_unique<P>(kind,true);offline->setStateInformation(state.getData(),static_cast<int>(state.getSize()));offline->prepareToPlay(48000,256);Head head;juce::MidiBuffer midi;juce::AudioBuffer<float> zeros(2,512);zeros.clear();for(int i=0;i<8;++i){zeros.clear();offline->processBlock(zeros,midi);}offline->setPlayHead(&head);
  juce::AudioBuffer<float> expected(2,192000);expected.clear();for(int c=0;c<2;++c)expected.copyFrom(c,0,x,c,0,x.getNumSamples());
  for(int at=0;at<expected.getNumSamples();at+=512){const int count=std::min(512,expected.getNumSamples()-at);head.ppq=at/24000.;juce::AudioBuffer<float> view(expected.getArrayOfWritePointers(),2,at,count);offline->processBlock(view,midi);}offline->setPlayHead(nullptr);
  expect(bitExact(expected,exported),"decoded 32f PCM exact, including2s effect tail");
  p->setParameterValue("mix",.37);expect(!exportW.status(p->sampleRevision()).ready,"stale render never marked current");
  beginTest("Float WAV metadata and headroom are preserved at Mix0");
  juce::WavAudioFormat format;auto stream=destination.createInputStream();std::unique_ptr<juce::AudioFormatReader> metadata(format.createReaderFor(stream.release(),true));expect(metadata&&metadata->bitsPerSample==32&&metadata->usesFloatingPointData);metadata.reset();
  auto hot=x;hot.applyGain(8.f);const auto hotFile=root.getChildFile("hot.wav");expect(write(hotFile,hot,48000));exportW.load(hotFile);expect(wait(exportW,p->sampleRevision()));p->setParameterValue("mix",0);const auto hotOut=root.getChildFile("hot_out.wav");p->renderSample(hotOut,0);expect(wait(exportW,p->sampleRevision()));auto hotRead=read(hotOut,fs);expect(bitExact(hot,hotRead)&&hotRead.getMagnitude(0,hotRead.getNumSamples())>1);
  exportW.load(source);expect(wait(exportW,p->sampleRevision()));
  beginTest("Mono host export keeps mono layout and actual left-input PCM");
  auto monoP=std::make_unique<P>(kind);auto layout=monoP->getBusesLayout();layout.inputBuses.set(0,juce::AudioChannelSet::mono());layout.outputBuses.set(0,juce::AudioChannelSet::mono());expect(monoP->setBusesLayout(layout));monoP->setParameterValue("mix",0);monoP->prepareToPlay(48000,256);monoP->sampleWorkspace()->load(source);expect(wait(*monoP->sampleWorkspace(),monoP->sampleRevision()));
  const auto monoOut=root.getChildFile("mono_host.wav");expect(monoP->renderSample(monoOut,0));expect(wait(*monoP->sampleWorkspace(),monoP->sampleRevision()));const auto monoResult=read(monoOut,fs);
  juce::AudioBuffer<float> monoExpected(1,x.getNumSamples());monoExpected.copyFrom(0,0,x,0,0,x.getNumSamples());expect(bitExact(monoResult,monoExpected));
  beginTest("Export failure preserves files; source cannot be overwritten; state load stops audition");
  juce::MemoryBlock bytes;source.loadFileAsData(bytes);p->renderSample(source,0);expect(wait(exportW,p->sampleRevision()));juce::MemoryBlock after;source.loadFileAsData(after);expect(after==bytes);
  const auto blocker=root.getChildFile("blocker");blocker.replaceWithText("keep");p->renderSample(blocker.getChildFile("result.wav"),0);expect(wait(exportW,p->sampleRevision()));expect(blocker.loadFileAsString()=="keep");
  exportW.play(true);p->setStateInformation(state.getData(),static_cast<int>(state.getSize()));expect(!exportW.isPlaying());
  auto restored=std::make_unique<P>(kind);restored->setStateInformation(state.getData(),static_cast<int>(state.getSize()));expect(!restored->sampleWorkspace()->status(restored->sampleRevision()).loaded,"temporary workspace is not embedded in project");
  beginTest("Bounded audio handoff remains finite under repeated loads and cancellation");
  std::atomic<bool> good{true};std::thread audio([&]{juce::AudioBuffer<float> b(2,127);for(int n=0;n<3000;++n){b.clear();allocation::begin();exportW.fill(b,2);if(allocation::finish())good.store(false);for(int c=0;c<2;++c)for(int i=0;i<127;++i)if(!std::isfinite(b.getSample(c,i)))good.store(false);}});
  for(int n=0;n<12;++n){exportW.load(n%2?source:monoFile);exportW.play(true);}audio.join();exportW.useInput();expect(wait(exportW,p->sampleRevision(),false));expect(good.load()&&!exportW.status(p->sampleRevision()).loaded);
  beginTest("BOLD is explicit, preserves genre, writes strong full-band settings");
  p->setParameterValue("simpleGenre",2);p->setParameterValue("simpleLock",1);p->makeBold();expect(p->parameterValue("simpleGenre")==2&&p->parameterValue("simpleTarget")==3&&p->parameterValue("simpleFill")==1&&p->parameterValue("simpleAmount")==1&&p->parameterValue("mix")==1&&p->parameterValue("simpleLock")==0);
  beginTest("Dragged transition files remain immutable; failed writes are not marked valid");
  auto transitions=std::make_unique<P>(kind);transitions->prepareToPlay(48000,256);transitions->setRendersFolder(root.getChildFile("transitions"));expect(transitions->useSampleForTransition(source));
  const auto first=transitions->makeTransition(0,1,{.6,.8,.5,.7},true,false);juce::MemoryBlock firstBytes;first.file.loadFileAsData(firstBytes);
  const auto second=transitions->makeTransition(0,1,{.6,.9,.5,.7},false,false);juce::MemoryBlock stillFirst;first.file.loadFileAsData(stillFirst);
  expect(first.valid&&second.valid&&first.file!=second.file&&firstBytes==stillFirst);
  transitions->setRendersFolder(blocker);const auto failed=transitions->makeTransition(0,1,{.6,.8,.5,.7},false,false);expect(!failed.valid);
  beginTest("Closing during a queued render cancels the worker safely");
  auto closing=std::make_unique<P>(kind);closing->prepareToPlay(48000,256);closing->sampleWorkspace()->load(source);expect(wait(*closing->sampleWorkspace(),closing->sampleRevision()));closing->renderSample(root.getChildFile("cancelled.wav"),8);
  const auto closeStart=juce::Time::getMillisecondCounter();closing.reset();expect(juce::Time::getMillisecondCounter()-closeStart<2000);
  beginTest("Main-page file drop and toolbar controls are reachable without moving preset screen");
  p->prepareToPlay(48000,256);std::unique_ptr<juce::AudioProcessorEditor> owned(p->createEditor());auto* editor=dynamic_cast<hpeq::standalone::ModulePluginEditor*>(owned.get());expect(editor&&editor->getSampleBar());
  if(editor)
  {
   expect(editor->isInterestedInFileDrag({source.getFullPathName()}));expect(!editor->isInterestedInFileDrag({"readme.txt"}));
   editor->filesDropped({source.getFullPathName()},1,1);expect(wait(*p->sampleWorkspace(),p->sampleRevision()));
   p->setRendersFolder(root.getChildFile("toolbar_transitions"));p->setPage(P::Page::Transition);editor->getSampleBar()->loadFile(monoFile);
   expect(p->lastTransition().valid&&p->lastTransition().sourceFile==monoFile&&p->sampleWorkspace()->status(p->sampleRevision()).source==source,"transition toolbar is separate from main sample workspace");
   for(auto size:std::array<std::pair<int,int>,3>{{{885,930},{442,465},{1770,930}}})
   {editor->setSize(size.first,size.second);auto* bar=editor->getSampleBar();for(int i=0;i<8;++i){expect(bar->getLocalBounds().contains(bar->controlBounds(i)));for(int j=0;j<i;++j)expect(!bar->controlBounds(i).intersects(bar->controlBounds(j)));}}
  }
 }
};
static SampleWorkspaceTests sampleWorkspaceTests;
}

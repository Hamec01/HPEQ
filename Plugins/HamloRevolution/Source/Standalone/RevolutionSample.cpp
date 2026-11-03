#include "Standalone/RevolutionSample.h"
#include "Standalone/ModulePluginProcessor.h"
#include <cmath>
namespace hpeq::standalone
{
namespace
{
// Offline polyphase windowed sinc. Adaptive cutoff prevents downsampling images;
// two phase rows are interpolated, avoiding a quantized fractional read head.
juce::AudioBuffer<float> resample(const juce::AudioBuffer<float>& input,double from,double to,const std::function<bool()>& cancel)
{
 if(from==to)return juce::AudioBuffer<float>(input);
 const double ratio=from/to,cutoff=.95*std::min(1.,to/from);
 const int radius=static_cast<int>(std::ceil(16/cutoff)),taps=2*radius,phases=256;
 std::vector<double> table(static_cast<size_t>((phases+1)*taps));
 const double pi=juce::MathConstants<double>::pi;
 for(int p=0;p<=phases;++p)
 {
  double sum=0;for(int j=0;j<taps;++j)
  {const double x=j-radius+1-static_cast<double>(p)/phases,v=pi*cutoff*x;const double h=(std::abs(v)<1e-14?cutoff:std::sin(v)/(pi*x))*(.5+.5*std::cos(pi*x/radius));table[static_cast<size_t>(p*taps+j)]=h;sum+=h;}
  for(int j=0;j<taps;++j)table[static_cast<size_t>(p*taps+j)]/=sum;
 }
 juce::AudioBuffer<float> out(2,std::max(1,static_cast<int>(std::llround(input.getNumSamples()/ratio))));
 for(int i=0;i<out.getNumSamples();++i)
 {
  if(i%1024==0&&cancel())return {};
  const double at=i*ratio;const int base=static_cast<int>(at),phase=std::min(phases-1,static_cast<int>((at-base)*phases));const double frac=(at-base)*phases-phase;
  double value[2]{};for(int j=0;j<taps;++j){const int n=base+j-radius+1;if(n<0||n>=input.getNumSamples())continue;const auto a=table[static_cast<size_t>(phase*taps+j)],b=table[static_cast<size_t>((phase+1)*taps+j)];const double h=a+frac*(b-a);for(int c=0;c<2;++c)value[c]+=h*input.getSample(c,n);}
  for(int c=0;c<2;++c)out.setSample(c,i,static_cast<float>(value[c]));
 }
 return out;
}
struct Head final:juce::AudioPlayHead
{
 double bpm=120,ppq=0;bool running=false;
 juce::Optional<PositionInfo> getPosition()const override{PositionInfo p;p.setBpm(bpm);p.setIsPlaying(running);if(running)p.setPpqPosition(ppq);return p;}
};
}
RevolutionSample::RevolutionSample():juce::Thread("Revolution sample files"){}
RevolutionSample::~RevolutionSample(){signalThreadShouldExit();notify();stopThread(-1);}
int RevolutionSample::acquire() noexcept
{
 for(int n=0;n<3;++n){const int i=active.load();if(i<0)return -1;slots[static_cast<size_t>(i)].readers.fetch_add(1);if(active.load()==i)return i;release(i);}return -1;
}
void RevolutionSample::release(int i) noexcept{slots[static_cast<size_t>(i)].readers.fetch_sub(1);}
void RevolutionSample::prepare(double fs) noexcept{rate.store(fs);play(false);notify();}
void RevolutionSample::play(bool on) noexcept{if(on)restart.fetch_add(1);playing.store(on);}
void RevolutionSample::useInput()
{
 const juce::ScopedLock lock(mutex);
 play(false);ticket.fetch_add(1);sourceSerial.fetch_add(1);active.store(-1);jobs.clear();info=Status{};info.message="Track input. Sample workspace is temporary.";
}
bool RevolutionSample::enqueue(Job job)
{
 const juce::ScopedLock lock(mutex);if(jobs.size()>=4){info.message="File queue busy. Try again.";return false;}if(!isThreadRunning()&&!startThread()){info.message="Cannot start sample file worker.";return false;}jobs.push_back(std::move(job));info.busy=true;info.message=jobs.back().isLoad?"Loading sample...":"Rendering current settings...";notify();return true;
}
bool RevolutionSample::load(const juce::File& f)
{
 Job j;j.isLoad=true;j.file=f;j.ticket=ticket.fetch_add(1)+1;play(false);return enqueue(std::move(j));
}
bool RevolutionSample::render(const juce::File& f,const juce::MemoryBlock& state,std::uint64_t revision,double bpm,int tail,int channels)
{
 Job j;j.file=f;j.state=state;j.revision=revision;j.bpm=bpm;j.tail=juce::jlimit(0,8,tail);j.channels=juce::jlimit(1,2,channels);j.source=sourceSerial.load();j.ticket=ticket.load();return enqueue(std::move(j));
}
RevolutionSample::Clock RevolutionSample::fill(juce::AudioBuffer<float>& output,int channels) noexcept
{
 Clock clock;if(!playing.load())return clock;
 const int index=acquire();if(index<0){playing.store(false);return clock;}
 auto& slot=slots[static_cast<size_t>(index)];const int length=slot.audio.getNumSamples();
 if(slot.rate!=rate.load()||length==0){release(index);playing.store(false);return clock;}
 const auto serial=restart.load();if(serial!=audioRestart){audioRestart=serial;audioPosition=0;audioFrame=0;}
 clock.active=true;clock.frame=audioFrame;
 for(int i=0;i<output.getNumSamples();++i)
 {
  if(audioPosition>=length&&loop.load())audioPosition=0;
  for(int c=0;c<channels;++c)output.setSample(c,i,audioPosition<length?slot.audio.getSample(c,audioPosition):0.f);
  ++audioPosition;++audioFrame;
 }
 progress.store(static_cast<double>(std::min(audioPosition,length))/length);
 if(audioPosition>=length&&!loop.load())playing.store(false);
 release(index);return clock;
}
RevolutionSample::Status RevolutionSample::status(std::uint64_t revision)const
{
 const juce::ScopedLock lock(mutex);auto s=info;const int index=active.load();s.loaded=s.loaded&&index>=0&&slots[static_cast<size_t>(index)].rate==rate.load();s.playing=playing.load();s.loop=loop.load();s.progress=progress.load();
 s.ready=s.ready&&renderRevision==revision&&renderSource==sourceSerial.load();return s;
}
void RevolutionSample::message(const juce::String& s){const juce::ScopedLock lock(mutex);info.message=s;}
bool RevolutionSample::publish(const juce::AudioBuffer<float>& audio,double fs)
{
 for(int i=0;i<3;++i)if(i!=active.load()&&slots[static_cast<size_t>(i)].readers.load()==0)
 {
  auto& slot=slots[static_cast<size_t>(i)];slot.audio.makeCopyOf(audio);slot.rate=fs;active.store(i);restart.fetch_add(1);sourceSerial.fetch_add(1);return true;
 }
 return false;
}
void RevolutionSample::doLoad(const Job& job)
{
 juce::AudioFormatManager formats;formats.registerBasicFormats();std::unique_ptr<juce::AudioFormatReader> reader(formats.createReaderFor(job.file));
 if(!reader||reader->lengthInSamples<1||reader->numChannels<1||reader->numChannels>2||!std::isfinite(reader->sampleRate)||reader->sampleRate<8000||reader->sampleRate>192000||reader->lengthInSamples>60*reader->sampleRate){message("Cannot load: mono/stereo audio, 8-192 kHz, up to 60 seconds required.");return;}
 juce::AudioBuffer<float> raw(2,static_cast<int>(reader->lengthInSamples));
 if(!reader->read(&raw,0,raw.getNumSamples(),0,true,reader->numChannels==2)){message("Audio read failed. Previous sample retained.");return;}
 if(reader->numChannels==1)raw.copyFrom(1,0,raw,0,0,raw.getNumSamples());
 for(int c=0;c<2;++c)for(int i=0;i<raw.getNumSamples();++i)if(!std::isfinite(raw.getSample(c,i))){message("Nonfinite sample rejected. Previous sample retained.");return;}
 const double fs=rate.load();auto audio=resample(raw,reader->sampleRate,fs,[&]{return threadShouldExit()||job.ticket!=ticket.load();});
 std::array<float,128> wave{};
 for(int i=0;i<audio.getNumSamples();++i){const auto n=std::min<size_t>(127,static_cast<size_t>(i)*128/static_cast<size_t>(audio.getNumSamples()));wave[n]=std::max(wave[n],std::max(std::abs(audio.getSample(0,i)),std::abs(audio.getSample(1,i))));}
 const juce::ScopedLock lock(mutex);
 if(job.ticket!=ticket.load()||threadShouldExit())return;
 if(audio.getNumSamples()==0||!publish(audio,fs)){message("Sample handoff busy. Try loading again.");return;}
 original=std::move(raw);originalRate=reader->sampleRate;
 info.wave=wave;info.source=job.file;info.loaded=true;info.ready=false;info.seconds=audio.getNumSamples()/fs;info.message="Sample ready. PLAY feeds Revolution; workspace is temporary.";
}
void RevolutionSample::doRender(const Job& job)
{
 if(job.source!=sourceSerial.load()||job.ticket!=ticket.load()){message("Source changed. Render again.");return;}
 {const juce::ScopedLock lock(mutex);if(job.file==info.source){info.message="Choose another filename; source audio is never overwritten.";return;}}
 const int index=acquire();if(index<0){message("Load a sample first.");return;}
 const double fs=slots[static_cast<size_t>(index)].rate;
 if(fs!=rate.load()){release(index);message("New host rate is still preparing. Render again shortly.");return;}
 juce::AudioBuffer<float> input(slots[static_cast<size_t>(index)].audio);release(index);
 Head head;head.bpm=job.bpm;
 auto processor=std::make_unique<ModulePluginProcessor>(ModuleKind::Revolution,true);
 auto layout=processor->getBusesLayout();layout.inputBuses.set(0,juce::AudioChannelSet::canonicalChannelSet(job.channels));layout.outputBuses.set(0,juce::AudioChannelSet::canonicalChannelSet(job.channels));
 if(!processor->setBusesLayout(layout)){message("Cannot restore render channel layout.");return;}
 processor->setStateInformation(job.state.getData(),static_cast<int>(job.state.getSize()));
 processor->setPlayHead(&head);processor->prepareToPlay(fs,256);
 juce::MidiBuffer midi;juce::AudioBuffer<float> block(job.channels,512);block.clear();
 // Settle existing wrapper ramps on silence. Starting the local clock then creates
 // a fresh Revolution epoch without changing the old processing path.
 for(int n=0;n<8;++n){block.clear();processor->processBlock(block,midi);}head.running=true;
 const int latency=processor->getLatencySamples();const int length=input.getNumSamples()+static_cast<int>(std::llround(job.tail*fs));
 juce::AudioBuffer<float> result(job.channels,length);result.clear();double peak=0;
 for(int at=0;at<length+latency;at+=512)
 {
  if(threadShouldExit()||job.ticket!=ticket.load()||job.source!=sourceSerial.load())return;
  const int count=std::min(512,length+latency-at);juce::AudioBuffer<float> view(block.getArrayOfWritePointers(),job.channels,count);view.clear();
  for(int c=0;c<job.channels;++c)if(at<input.getNumSamples())view.copyFrom(c,0,input,c,at,std::min(count,input.getNumSamples()-at));
  head.ppq=at*job.bpm/(60*fs);processor->processBlock(view,midi);
  for(int i=0;i<count;++i)if(at+i>=latency)for(int c=0;c<job.channels;++c)
  {const auto value=view.getSample(c,i);if(!std::isfinite(value)){message("Render produced invalid audio; file not replaced.");return;}result.setSample(c,at+i-latency,value);peak=std::max(peak,static_cast<double>(std::abs(value)));}
 }
 processor->setPlayHead(nullptr);
 if(!job.file.getParentDirectory().createDirectory()){message("Export folder unavailable.");return;}
 juce::TemporaryFile temporary(job.file);juce::WavAudioFormat wav;auto stream=temporary.getFile().createOutputStream();
 if(!stream){message("Cannot create WAV. Destination preserved.");return;}
 const auto options=juce::AudioFormatWriterOptions{}.withSampleRate(fs).withNumChannels(job.channels).withBitsPerSample(32).withSampleFormat(juce::AudioFormatWriterOptions::SampleFormat::floatingPoint);
 std::unique_ptr<juce::OutputStream> owned(stream.release());auto writer=wav.createWriterFor(owned,options);
 if(!writer||!writer->writeFromAudioSampleBuffer(result,0,length)){message("WAV write failed. Destination preserved.");return;}
 writer.reset();
 const juce::ScopedLock lock(mutex);
 if(threadShouldExit()||job.ticket!=ticket.load()||job.source!=sourceSerial.load())return;
 if(!temporary.overwriteTargetFileWithTemporary()){message("Cannot finish WAV. Destination preserved.");return;}
 info.rendered=job.file;info.tailSeconds=job.tail;info.ready=true;info.peakDb=20*std::log10(std::max(peak,1e-6));renderRevision=job.revision;renderSource=job.source;info.message="WAV ready: 32-bit float, "+juce::String(job.tail)+"s tail. Drag or save.";
}
void RevolutionSample::run()
{
 while(!threadShouldExit())
 {
  Job job;bool have=false;{const juce::ScopedLock lock(mutex);if(!jobs.empty()){job=std::move(jobs.front());jobs.pop_front();have=true;}}
  if(have){try{if(job.isLoad)doLoad(job);else doRender(job);}catch(...){message("File task failed; previous source/export retained.");}const juce::ScopedLock lock(mutex);info.busy=!jobs.empty();}
  else
  {
   const int index=active.load();const double fs=rate.load();
   if(index>=0&&slots[static_cast<size_t>(index)].rate!=fs&&original.getNumSamples()>0)
   {
    {const juce::ScopedLock lock(mutex);info.busy=true;info.ready=false;info.message="Preparing new host rate...";}
    const auto operation=ticket.load();auto audio=resample(original,originalRate,fs,[&]{return threadShouldExit()||operation!=ticket.load();});
    const juce::ScopedLock lock(mutex);
    if(audio.getNumSamples()>0&&operation==ticket.load()){publish(audio,fs);info.ready=false;message("Sample prepared at the new host rate. Press PLAY.");}
    info.busy=!jobs.empty();
   }
   wait(-1);
  }
 }
}
}

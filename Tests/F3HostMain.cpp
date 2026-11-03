#include "Standalone/ModulePluginProcessor.h"
#include "FaderF3Identity.h"
#include "AllocationAudit.h"
#include <juce_cryptography/juce_cryptography.h>
#include <iostream>
#include <thread>
#include <cmath>
#include <cstdlib>
#include <chrono>

using namespace hpeq::standalone;
namespace f=hamlo::fader;
using P=ModulePluginProcessor;
constexpr auto kind=ModuleKind::Fader;
namespace
{
int passed=0,failed=0;juce::Array<juce::var> results,timings;
void check(bool ok,const juce::String& name)
{
 auto row=juce::var(new juce::DynamicObject);row.getDynamicObject()->setProperty("name",name);row.getDynamicObject()->setProperty("passed",ok);results.add(row);
 (ok?passed:failed)++;std::cout<<(ok?"PASS ":"FAIL ")<<name<<std::endl;
}
void set(P& p,const char* id,double value){p.setParameterValue(id,value);}
std::unique_ptr<P> make(){return std::make_unique<P>(kind);}
juce::MemoryBlock save(P& p){juce::MemoryBlock b;p.getStateInformation(b);return b;}
void load(P& p,const juce::MemoryBlock& b){p.setStateInformation(b.getData(),static_cast<int>(b.getSize()));}
FaderStatus status(P& p){FaderStatus s;check(p.readStatus(s),"atomic snapshot available");return s;}
void layout(P& p,int channels)
{
 auto l=p.getBusesLayout();l.inputBuses.set(0,juce::AudioChannelSet::canonicalChannelSet(channels));l.outputBuses.set(0,juce::AudioChannelSet::canonicalChannelSet(channels));
 if(!p.setBusesLayout(l))throw std::runtime_error("layout rejected");
}
bool process(P& p,juce::AudioBuffer<float>& b,bool bypass=false)
{
 juce::MidiBuffer midi;hamlo::test::allocation::begin();if(bypass)p.processBlockBypassed(b,midi);else p.processBlock(b,midi);return hamlo::test::allocation::finish()==0;
}
void fill(juce::AudioBuffer<float>& b,int at=0,double rate=48000)
{
 for(int c=0;c<b.getNumChannels();++c)for(int i=0;i<b.getNumSamples();++i)
 {const auto n=at+i;const double envelope=(n/4096)%3==0?.3:1.;b.setSample(c,i,static_cast<float>(envelope*(.09*std::sin(2*juce::MathConstants<double>::pi*(c?997:701)*n/rate)+.03*std::cos(2*juce::MathConstants<double>::pi*67*n/rate))));}
}
bool equal(const juce::AudioBuffer<float>& a,const juce::AudioBuffer<float>& b)
{if(a.getNumChannels()!=b.getNumChannels()||a.getNumSamples()!=b.getNumSamples())return false;for(int c=0;c<a.getNumChannels();++c)if(std::memcmp(a.getReadPointer(c),b.getReadPointer(c),sizeof(float)*static_cast<size_t>(a.getNumSamples())))return false;return true;}
struct Head final:juce::AudioPlayHead
{
 f::Clock clock;
 juce::Optional<PositionInfo> getPosition() const override
 {PositionInfo p;p.setBpm(clock.bpm);p.setIsPlaying(clock.playing);if(clock.hasPpq)p.setPpqPosition(clock.ppq);if(clock.hasSamples)p.setTimeInSamples(clock.samplePosition);return p;}
};
f::Clock clockAt(int at,int scenario,double rate)
{
 f::Clock c;c.bpm=at<16384?113.:147.;c.playing=!(at>=24576&&at<32768);c.hasPpq=scenario!=6;c.hasSamples=scenario!=5&&scenario!=8;
 c.ppq=at<16384?at*113./(60*rate):(16384*113.+(at-16384)*147.)/(60*rate);
 if(at>=32768)c.ppq+=9.;c.samplePosition=at+(at>=32768?480000:0);return c;
}
// Explicit independent host mapping to an accepted F2 settings object, including all 32 points.
void configure(P& p,f::Settings& s,int scenario)
{
 set(p,"engine",1);s.mode=scenario==3?f::Mode::Ride:static_cast<f::Mode>(scenario<4?scenario:3);
 set(p,"mode",static_cast<int>(s.mode));
 auto number=[&](const char* id,double& target,double value){set(p,id,value);target=dynamic_cast<juce::AudioParameterFloat*>(p.parameter(id))->get();};
 number("inputDb",s.inputDb,1.25);number("gainDb",s.gainDb,-2.25);number("outputDb",s.outputDb,-1.75);number("mix",s.mix,.71);
 number("leftDb",s.leftDb,-1.5);number("rightDb",s.rightDb,.75);number("leftDelayMs",s.leftDelayMs,2.375);number("rightDelayMs",s.rightDelayMs,5.125);
 set(p,"routing",1);s.routing=f::Routing::MonoSum;set(p,"swap",1);s.swap=true;set(p,"polarity",1);s.polarity=1;
 set(p,"measure",scenario%2);s.measure=scenario%2?f::Measure::Lufs:f::Measure::Rms;
 number("targetDb",s.targetDb,-23.5);number("matchBoost",s.matchBoost,9.5);number("matchCut",s.matchCut,22.25);number("matchCeiling",s.matchCeiling,-2.25);number("matchedDb",s.matchedDb,3.125);set(p,"peakGuard",0);s.peakGuard=false;
 set(p,"profile",scenario==3?1:0);s.profile=scenario==3?f::Profile::Fast:f::Profile::Natural;
 number("strength",s.strength,.83);number("maxBoost",s.maxBoost,4.5);number("maxCut",s.maxCut,10.5);number("floorDb",s.floorDb,-57.25);
 number("holdMs",s.holdMs,33.5);number("idleMs",s.idleMs,77.25);number("riseMs",s.riseMs,123.75);number("fallMs",s.fallMs,47.125);
 s.shape=scenario==5?f::Shape::Sine:scenario==6?f::Shape::Triangle:f::Shape::Custom;set(p,"shape",static_cast<int>(s.shape));
 set(p,"points",23);s.points=23;set(p,"smooth",0);s.smooth=false;set(p,"sync",scenario==5?0:1);s.sync=scenario!=5;
 number("depthDb",s.depthDb,19.5);number("beats",s.beats,.75);number("freeMs",s.freeMs,337.125);number("phase",s.phase,.17);number("rightPhase",s.rightPhase,.31);number("edgeMs",s.edgeMs,2.5);
 for(int i=0;i<32;++i){auto id="level"+juce::String(i).paddedLeft('0',2);set(p,id.toRawUTF8(),(i%7)/6.);s.levels[static_cast<size_t>(i)]=p.parameterValue(id);}
}
struct Run{juce::MemoryBlock pcm;bool allocations=true,oracle=true;f::Reading reading;};
Run differential(int scenario,int block,int channels=2,double rate=48000)
{
 auto p=make();layout(*p,channels);f::Settings settings;configure(*p,settings,scenario);
 Head head;p->setPlayHead(scenario==7?nullptr:&head);p->prepareToPlay(rate,256);f::Kernel oracle;oracle.prepare(rate,channels);oracle.set(settings);
 // These offsets intentionally lie inside a callback/prepared chunk, not only at boundaries.
 const std::array<int,3> actionFrames{123,26001,40001};
 check(p->requestAction(FaderAction::StartLearn,123)&&p->requestAction(FaderAction::StopLearn,26001)&&p->requestAction(FaderAction::Reset,40001),"scheduled command enqueue");
 Run run;int action=0;
 for(int at=0;at<48000;)
 {
   if(at>0&&at%8192==0)
   {
     if(at==8192){set(*p,"gainDb",1.5);settings.gainDb=p->parameterValue("gainDb");set(*p,"routing",3);settings.routing=f::Routing::RightBoth;}
     if(at==16384){set(*p,"mute",1);settings.enabled=false;}
     if(at==24576){set(*p,"mute",0);settings.enabled=true;set(*p,"polarity",2);settings.polarity=2;}
     if(at==32768){set(*p,"mode",3);settings.mode=f::Mode::Motion;set(*p,"sync",0);settings.sync=false;}
     if(at==40960){set(*p,"bypass",1);settings.bypass=true;}
   }
   const int count=std::min({block,48000-at,8192-at%8192});head.clock=scenario==7?f::Clock{}:clockAt(at,scenario,rate);
   juce::AudioBuffer<float> b(channels,count);fill(b,at,rate);auto expected=b;run.allocations=process(*p,b)&&run.allocations;
   oracle.set(settings);
   for(int offset=0;offset<count;)
   {
     while(action<3&&actionFrames[static_cast<size_t>(action)]<=at+offset)
     {if(action==0)oracle.startLearn();else if(action==1)oracle.stopLearn();else oracle.reset();++action;}
     const int length=action<3?std::min(count-offset,actionFrames[static_cast<size_t>(action)]-at-offset):count-offset;
     auto c=head.clock;if(c.hasPpq)c.ppq+=offset*c.bpm/(60*rate);if(c.hasSamples)c.samplePosition+=offset;
     float* pointers[2]{};for(int ch=0;ch<channels;++ch)pointers[ch]=expected.getWritePointer(ch,offset);
     oracle.process(pointers,channels,length,c);offset+=length;
   }
   run.oracle=equal(b,expected)&&run.oracle;
   for(int i=0;i<count;++i)for(int c=0;c<channels;++c){const auto sample=b.getSample(c,i);run.pcm.append(&sample,sizeof(sample));}
   at+=count;
 }
 auto st=status(*p);run.reading=st.reading;
 run.oracle=run.oracle&&st.reading.input.samples==oracle.reading().input.samples&&st.reading.output.truePeak==oracle.reading().output.truePeak;
 p->setPlayHead(nullptr);return run;
}
void basic()
{
 auto p=make();check(p->getParameters().size()==76,"8 original +68 appended parameters");
 check(p->parameterValue("engine")==1&&p->parameterValue("mode")==0,"new instance modern Trim");
 p->prepareToPlay(48000,256);juce::AudioBuffer<float> b(2,4096);fill(b);auto before=b;
 check(process(*p,b)&&equal(b,before),"neutral modern exact, oversized callback, no allocation");
 check(p->getLatencySamples()==0&&p->getTailLengthSeconds()==.02,"modern PDC0/alignment tail20ms");
 p->setCurrentProgram(0);check(p->parameterValue("engine")==0&&p->getTailLengthSeconds()==0,"original Default selects Legacy/tail0");
 auto l=p->getBusesLayout();l.inputBuses.set(0,juce::AudioChannelSet::create5point1());l.outputBuses.set(0,juce::AudioChannelSet::create5point1());check(!p->isBusesLayoutSupported(l),"surround rejected");
}
void states()
{
 auto p=make();f::Settings s;configure(*p,s,6);p->setEditorPage(2);p->setCurrentProgram(4);configure(*p,s,6);p->setEditorPage(2);
 const auto saved=save(*p);auto q=make();load(*q,saved);check(save(*q)==saved&&q->getCurrentProgram()==4&&q->editorPage()==2,"modern state parameter/program/page roundtrip");
 p->prepareToPlay(48000,256);q->prepareToPlay(48000,256);juce::AudioBuffer<float> input(2,4096);fill(input);auto a=input,b=input;process(*p,a);process(*q,b);check(equal(a,b),"fresh restore actual audio exact");
 set(*q,"gainDb",-45);process(*q,b);q->requestAction(FaderAction::StartLearn,99999);load(*q,saved);b=input;process(*q,b);
 auto fresh=make();load(*fresh,saved);fresh->prepareToPlay(48000,256);a=input;process(*fresh,a);
 check(equal(a,b)&&!status(*q).reading.learning,"prepared modern restore resets history and stale actions");
 const auto unchanged=save(*q);auto xml=juce::AudioProcessor::getXmlFromBinary(saved.getData(),static_cast<int>(saved.getSize()));
 auto bad=[&](const juce::String& name){juce::MemoryBlock data;juce::AudioProcessor::copyXmlToBinary(*xml,data);load(*q,data);check(save(*q)==unchanged,name);};
 xml->setAttribute("faderVersion",999);bad("future state rejected");xml->setAttribute("faderVersion",3);
 auto* child=xml->getFirstChildElement();const auto originalValue=child->getStringAttribute("value");
 child->setAttribute("value","nan");bad("nonfinite modern state rejected");child->setAttribute("value","garbage");bad("nonnumeric modern state rejected");child->setAttribute("value",1000000);bad("out-of-range modern state rejected");child->setAttribute("value",originalValue);
 xml->addChildElement(new juce::XmlElement(*child));bad("duplicate modern state rejected");xml->removeChildElement(xml->getChildElement(xml->getNumChildElements()-1),true);
 xml->removeChildElement(xml->getFirstChildElement(),true);bad("incomplete modern state rejected");
 const char invalid[]{'x','m','l'};q->setStateInformation(invalid,3);check(save(*q)==unchanged,"invalid binary rejected");
 // Genuine unversioned state from paired control, not a synthetic modern state.
 juce::MemoryBlock old;juce::File("E:/HamloFader/out/f3/legacy_reference/program_3_48000_ch2_b127.state").loadFileAsData(old);
 q->setCurrentProgram(7);configure(*q,s,5);q->setEditorPage(2);load(*q,old);
 auto defaults=make();
 bool reset=q->parameterValue("engine")==0&&q->getCurrentProgram()==7&&q->editorPage()==0;
 const auto oldXml=juce::AudioProcessor::getXmlFromBinary(old.getData(),static_cast<int>(old.getSize()));
 const auto oldGain=static_cast<float>(oldXml->getChildByAttribute("id","gainDb")->getDoubleAttribute("value"));
 reset=reset&&dynamic_cast<juce::AudioParameterFloat*>(q->parameter("gainDb"))->get()==oldGain;
 for(const auto& c:faderControls())reset=reset&&q->parameterValue(c.id)==(c.id=="engine"?0:defaults->parameterValue(c.id));
 check(reset,"old load resets all68 new controls and page, retains historical program");
}
void learning()
{
 auto p=make();set(*p,"mode",1);set(*p,"peakGuard",0);set(*p,"targetDb",-18);p->prepareToPlay(48000,256);
 check(!p->applyMatch(),"Apply refuses absent proposal");
 hamlo::test::allocation::begin();const bool queued=p->requestAction(FaderAction::StartLearn);const auto alloc=hamlo::test::allocation::finish();check(queued&&alloc==0,"Learn enqueue allocation-free");
 juce::AudioBuffer<float> b(2,4096);
 for(int i=0;i<7;++i){fill(b,i*4096);check(process(*p,b),"learning callback allocation-free");}
 check(status(*p).reading.learning&&p->parameterValue("matchedDb")==0,"Learn does not apply gain");
 p->requestAction(FaderAction::StopLearn);fill(b);process(*p,b);auto st=status(*p);
 check(st.proposal.valid&&!st.reading.learning&&p->parameterValue("matchedDb")==0,"Stop publishes valid proposal only");
 check(p->applyMatch()&&std::abs(p->parameterValue("matchedDb")-st.proposal.gainDb)<1e-5,"explicit Apply persistent host parameter");
 const double applied=p->parameterValue("matchedDb");p->requestAction(FaderAction::Reset);fill(b);process(*p,b);st=status(*p);
 check(p->parameterValue("matchedDb")==applied&&!st.proposal.valid&&st.reading.input.samples==4096,"Reset clears history but retains applied gain");
 p->requestAction(FaderAction::StartLearn);fill(b);process(*p,b);set(*p,"bypass",1);fill(b);check(process(*p,b),"parameter bypass allocation-free");st=status(*p);
 check(!st.reading.learning&&st.reading.learn.samples==0&&!st.proposal.valid&&!p->applyMatch(),"parameter bypass cancels unfinished learning");
 set(*p,"bypass",0);p->requestAction(FaderAction::StartLearn);fill(b);process(*p,b);fill(b);auto dry=b;check(process(*p,b,true)&&equal(b,dry)==false,"host bypass starts its20ms fade");
 fill(b);dry=b;process(*p,b,true);st=status(*p);check(equal(b,dry)&&!st.reading.learning&&st.reading.learn.samples==0&&!p->applyMatch()&&p->parameterValue("bypass")==0,"host bypass settles exact dry, cancels Learn, leaves parameter unchanged");
 set(*p,"enabled",0);p->requestAction(FaderAction::StartLearn);fill(b);process(*p,b);check(!status(*p).reading.learning,"disabled cannot Learn");
 auto capacity=make();capacity->prepareToPlay(48000,256);bool bounded=true;for(int i=0;i<64;++i)bounded=capacity->requestAction(FaderAction::Reset,static_cast<std::uint64_t>(10000+i))&&bounded;
 check(bounded&&!capacity->requestAction(FaderAction::Reset,10064),"command64 capacity and explicit overflow refusal");
 fill(b);process(*capacity,b);capacity->setCurrentProgram(1);fill(b);process(*capacity,b);check(capacity->requestAction(FaderAction::Reset),"obsolete epoch commands drained, queue reusable");
 auto order=make();order->prepareToPlay(48000,256);check(order->requestAction(FaderAction::Reset,100)&&!order->requestAction(FaderAction::Reset,99),"decreasing command frame refused");
}
void concurrent()
{
 FaderSnapshot<FaderStatus> snapshot;std::atomic<bool> done{false},good{true};
 std::thread reader([&]{while(!done.load()){FaderStatus s;if(snapshot.read(s)&&s.frame!=s.epoch*7)good.store(false);}});
 for(std::uint64_t i=1;i<30000;++i){FaderStatus s;s.epoch=i;s.frame=i*7;snapshot.publish(s);}done.store(true);reader.join();check(good.load(),"concurrent atomic snapshot coherence");
 FaderCommands q;std::array<std::thread,3> producers;std::atomic<int> accepted{0};
 std::atomic<bool> running{true};int consumed=0;
 for(auto& t:producers)t=std::thread([&]{for(int i=0;i<10000;++i)if(q.push({FaderAction::Reset,0,1}))accepted.fetch_add(1);});
 std::thread consumer([&]{while(running.load()||q.produced()!=q.consumed()){FaderCommand c;if(q.peek(c)){if(c.epoch!=1||c.frame!=0)good.store(false);q.pop();++consumed;}}});
 for(auto& t:producers)t.join();running.store(false);consumer.join();check(good.load()&&accepted.load()==consumed,"three nonblocking producers/one consumer, accepted commands conserved");
}
void preciseActionsAndOldRestore()
{
 auto p=make();set(*p,"mode",1);set(*p,"measure",1);set(*p,"peakGuard",0);p->prepareToPlay(48000,256);
 check(p->requestAction(FaderAction::StartLearn,123)&&p->requestAction(FaderAction::StopLearn,26001),"precise Learn/Stop queued inside oversized callbacks");
 juce::AudioBuffer<float> b(2,4096);for(int n=0;n<7;++n){fill(b,n*4096);process(*p,b);}
 auto s=status(*p);check(s.proposal.valid&&s.reading.learn.samples==25878,"Learn sample interval exactly[123,26001)");
 check(p->requestAction(FaderAction::StartLearn,35000)&&!p->applyMatch(),"queued Learn makes old proposal inapplicable before consumption");
 // Re-prepare invalidates pending commands from the preceding processing timeline.
 p->prepareToPlay(48000,256);fill(b);process(*p,b);check(!status(*p).reading.learning,"reprepare invalidates old command timeline");
 auto q=make();q->setCurrentProgram(7);set(*q,"gainDb",24);set(*q,"delayMs",20);set(*q,"polarity",3);set(*q,"mute",1);
 set(*q,"engine",1);set(*q,"mode",3);set(*q,"inputDb",2.75);set(*q,"outputDb",-4.5);set(*q,"rightPhase",.73);set(*q,"leftDelayMs",9);
 auto render=[&](bool prepare)
 {
  if(prepare)q->prepareToPlay(48000,256);juce::MemoryBlock raw;
  for(int at=0;at<8192;)
  {
   const int count=std::min({127,8192-at,1024-at%1024});juce::AudioBuffer<float> audio(2,count);
   for(int ch=0;ch<2;++ch)for(int i=0;i<count;++i){const int n=at+i;const double t=n/48000.;audio.setSample(ch,i,static_cast<float>(.11*std::sin(t*juce::MathConstants<double>::twoPi*(ch?997:701))+.07*std::cos(t*juce::MathConstants<double>::twoPi*83)+(n%2003==17?.2:0)));}
   if(!process(*q,audio))throw std::runtime_error("Legacy modern-dirty allocation");
   for(int i=0;i<count;++i)for(int ch=0;ch<2;++ch){const auto x=audio.getSample(ch,i);raw.append(&x,sizeof(x));}at+=count;
  }
  return raw;
 };
 render(true);juce::MemoryBlock old,reference;const juce::File base("E:/HamloFader/out/f3/legacy_reference");
 base.getChildFile("program_3_48000_ch2_b127.state").loadFileAsData(old);
 base.getChildFile("program_3_48000_ch2_b127_2.f32").loadFileAsData(reference);load(*q,old);
 check(render(false)==reference,"old state over modified prepared modern instance matches original M0 history PCM");
}

void transition()
{
 auto p=make(),legacy=make(),modern=make();
 for(auto* q:{p.get(),legacy.get(),modern.get()}){q->setCurrentProgram(4);set(*q,"outputDb",-6);}
 set(*modern,"engine",1);
 for(auto* q:{p.get(),legacy.get(),modern.get()})q->prepareToPlay(48000,256);
 juce::AudioBuffer<float> input(2,4096);fill(input);auto a=input,b=input,c=input;
 process(*p,a);process(*legacy,b);process(*modern,c);check(equal(a,b),"stable Legacy path exact before selection change");
 set(*p,"engine",1);a=input;b=input;c=input;const bool alloc=process(*p,a);process(*legacy,b);process(*modern,c);
 bool final=true,finite=true;for(int ch=0;ch<2;++ch)for(int i=0;i<4096;++i){finite=finite&&std::isfinite(a.getSample(ch,i));if(i>=960)final=final&&a.getSample(ch,i)==c.getSample(ch,i);}
 check(alloc&&final&&finite&&!equal(a,b)&&!equal(a,c),"selection crossfade settles exactly at20ms, allocation-free");
 set(*p,"mode",1);p->requestAction(FaderAction::StartLearn);a=input;process(*p,a);set(*p,"engine",0);a=input;process(*p,a);
 check(!status(*p).reading.learning,"leaving modern cancels unfinished Learn");
 // Control thread reads atomics and posts commands while a real processor callback runs.
 auto live=make();live->prepareToPlay(48000,256);std::atomic<bool> done{false},good{true};
 std::thread audio([&]{juce::AudioBuffer<float> block(2,64);for(int i=0;i<2000;++i){fill(block,i*64);if(!process(*live,block))good.store(false);}done.store(true);});
 while(!done.load()){FaderStatus snapshot;live->readStatus(snapshot);live->requestAction(FaderAction::Reset);set(*live,"outputDb",-3);}
 audio.join();check(good.load(),"live processor/control snapshot-command stress, zero audio allocations");
}
void performance()
{
 for(int mode=-1;mode<5;++mode)
 {
  auto p=make();f::Settings settings;
  if(mode<0)p->setCurrentProgram(4);
  else configure(*p,settings,mode==3?4:mode==4?3:mode);
  p->prepareToPlay(48000,256);juce::AudioBuffer<float> b(2,256);fill(b);
  for(int i=0;i<64;++i)process(*p,b);
  double elapsed=0;bool allocated=false;
  for(int i=0;i<512;++i)
  {
   fill(b,i*256);const auto start=std::chrono::steady_clock::now();allocated=!process(*p,b)||allocated;
   elapsed+=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();
  }
  const auto percent=100*elapsed/512/(256./48000);auto row=juce::var(new juce::DynamicObject);
  row.getDynamicObject()->setProperty("mode",mode);row.getDynamicObject()->setProperty("mean_block_percent",percent);timings.add(row);
  check(!allocated&&percent<=25,"whole processor mean CPU <=25% block, mode"+juce::String(mode));
 }
}

}
int main(int argc,char** argv)
{
 juce::ScopedJuceInitialiser_GUI gui;
 try
 {
   if(argc!=2||juce::File(argv[1]).exists())throw std::runtime_error("provide NEW report.json");
   if(!juce::File("E:/HamloFader/out/f3/legacy_reference/capture.json").existsAsFile())
       throw std::runtime_error("F3 host audit requires the local archived Legacy control capture");
   basic();states();learning();concurrent();
   for(int scenario=0;scenario<9;++scenario)
   {
     auto reference=differential(scenario,512);check(reference.oracle&&reference.allocations,"host vs accepted kernel scenario"+juce::String(scenario));
     for(int block:{1,64,127,4096}){auto r=differential(scenario,block);check(r.oracle&&r.allocations&&r.pcm==reference.pcm,"exact event/block oracle scenario"+juce::String(scenario)+" block"+juce::String(block));}
   }
   for(int channels:{1,2})for(double rate:{8000.,44100.,96000.,192000.})
   {auto r=differential(4,4096,channels,rate);check(r.oracle&&r.allocations,"channel/rate oracle "+juce::String(channels)+"/"+juce::String(rate,0));}
   preciseActionsAndOldRestore();transition();performance();
   auto report=juce::var(new juce::DynamicObject);auto* o=report.getDynamicObject();
#ifdef FADER_F3_REGRESSION
   o->setProperty("source_id","frozen pre-fix Motion kernel with current host");
#else
   o->setProperty("source_id",FADER_F3_SOURCE_ID);
#endif
o->setProperty("method_id",FADER_F3_HOST_METHOD_ID);o->setProperty("passed",passed);o->setProperty("failed",failed);o->setProperty("results",results);o->setProperty("timings",timings);
   juce::File file(argv[1]);file.getParentDirectory().createDirectory();file.replaceWithText(juce::JSON::toString(report));std::cout<<passed<<" passed, "<<failed<<" failed\n";return failed?1:0;
 }
 catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 2;}
}

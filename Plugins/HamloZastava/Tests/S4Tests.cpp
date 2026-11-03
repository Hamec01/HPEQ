#include "DSP/V2Gate.h"
#include "Standalone/ModulePluginEditor.h"
#include "AllocationAudit.h"
#include "TestSignals.h"
#include <cmath>

namespace
{
using namespace hamlo::zastava;
using P=hpeq::standalone::ModulePluginProcessor;
constexpr auto kind=hpeq::standalone::ModuleKind::Zastava;
float mainSignal(int n,int c){return static_cast<float>(.12*std::sin(n*(c?.071:.11)));}
float keySignal(int n,int c){return static_cast<float>(.23*std::cos(n*(c?.097:.043)));}
juce::MemoryBlock save(P& p){juce::MemoryBlock b;p.getStateInformation(b);return b;}
void load(P& p,const juce::MemoryBlock& b){p.setStateInformation(b.getData(),static_cast<int>(b.getSize()));}
juce::Component* find(juce::Component& c,const juce::String& id)
{if(c.getComponentID()==id)return &c;for(auto* child:c.getChildren())if(auto* found=find(*child,id))return found;return nullptr;}
void click(juce::Component& c,const juce::String& id)
{if(auto* b=dynamic_cast<juce::Button*>(find(c,id))){b->triggerClick();juce::MessageManager::getInstance()->runDispatchLoopUntil(40);}}
struct Gesture:juce::AudioProcessorParameter::Listener
{void parameterValueChanged(int,float)override{++values;}void parameterGestureChanged(int,bool on)override{on?++begins:++ends;}int values=0,begins=0,ends=0;};
juce::AudioBuffer<float> events(int block,size_t& allocations)
{
    Gate gate;gate.prepare(48000,256);Settings p;p.enabled=true;p.external=true;p.detectorMs=0;
    juce::AudioBuffer<float> result(2,24000);std::array<std::array<float,4096>,2> key{};
    for(int at=0;at<24000;)
    {
        const int e=at/2000,count=std::min({block,24000-at,2000-at%2000});p.listen=e%2!=0;p.bypass=e%7==6;p.keyHpHz=e%3==1?500:0;p.keyLpHz=e%3==2?3000:0;p.outputDb=e%2?12:-24;p.mix=e%2?0:1;
        for(int c=0;c<2;++c)for(int n=0;n<count;++n){result.setSample(c,at+n,mainSignal(at+n,c));key[static_cast<size_t>(c)][static_cast<size_t>(n)]=keySignal(at+n,c);}
        float* a[]{result.getWritePointer(0,at),result.getWritePointer(1,at)};const float* k[]{key[0].data(),key[1].data()};
        Clock clock;clock.hasPpq=true;clock.ppq=at/24000.;
        hamlo::test::allocation::begin();gate.set(p);gate.process(a,2,count,e%5==4?nullptr:k,e%5==4?0:2,clock);allocations+=hamlo::test::allocation::finish();at+=count;
    }
    return result;
}
class S4Tests final:public juce::UnitTest
{
public:
    S4Tests():juce::UnitTest("S4 modern editor and audition","HamloZastava"){}
    void runTest()override
    {
        beginTest("Audition equals exactly delayed signed key across rates and layouts");
        for(double rate:{8000.,44100.,48000.,96000.,192000.})for(int channels:{1,2})for(int keys:{0,1,2})
        {
            Gate gate;gate.prepare(rate,256);Settings p;p.listen=true;p.external=keys>0;p.enabled=keys==1;p.mix=0;p.outputDb=12;p.thresholdDb=0;p.rangeDb=90;gate.set(p);
            const int frames=4096,delay=static_cast<int>(std::lround(.005*rate));juce::AudioBuffer<float> got(channels,frames),expected(channels,frames);expected.clear();std::array<std::array<float,4096>,2> key{};
            for(int c=0;c<channels;++c)for(int n=0;n<frames;++n){got.setSample(c,n,mainSignal(n,c));if(n>=delay)expected.setSample(c,n,keys?keySignal(n-delay,std::min(c,keys-1)):mainSignal(n-delay,c));}
            for(int c=0;c<keys;++c)for(int n=0;n<frames;++n)key[static_cast<size_t>(c)][static_cast<size_t>(n)]=keySignal(n,c);
            const float* k[]{key[0].data(),key[1].data()};hamlo::test::allocation::begin();gate.process(got.getArrayOfWritePointers(),channels,frames,keys?k:nullptr,keys);const auto allocations=hamlo::test::allocation::finish();
            expect(hamlo::test::bitExact(got,expected),juce::String(rate)+" main"+juce::String(channels)+" key"+juce::String(keys));expectEquals(static_cast<int>(allocations),0);
        }
        beginTest("Audition filtering discriminates and bypass/missing key override");
        double levels[3]{};
        for(int mode=0;mode<3;++mode)
        {
            Gate gate;gate.prepare(48000,256);Settings p;p.listen=true;p.keyHpHz=mode==1?8000:0;p.keyLpHz=mode==2?100:0;gate.set(p);
            juce::AudioBuffer<float> data(1,24000);for(int n=0;n<24000;++n)data.setSample(0,n,static_cast<float>(.1*std::sin(n*juce::MathConstants<double>::twoPi*1000/48000)));gate.process(data.getArrayOfWritePointers(),1,24000);
            levels[mode]=data.getRMSLevel(0,12000,12000);
        }
        expect(20*std::log10(levels[0]/levels[1])>=30);expect(20*std::log10(levels[0]/levels[2])>=30);
        logMessage("Audition HP attenuation "+juce::String(20*std::log10(levels[0]/levels[1]),6)+" dB; LP "+juce::String(20*std::log10(levels[0]/levels[2]),6)+" dB");
        for(int mode=0;mode<2;++mode)
        {
            Gate gate;gate.prepare(48000,256);Settings p;p.listen=true;p.enabled=true;p.outputDb=12;p.bypass=mode==0;p.external=mode==1;gate.set(p);
            juce::AudioBuffer<float> data(1,4096),expected(1,4096);expected.clear();for(int n=0;n<4096;++n){data.setSample(0,n,mainSignal(n,0));if(n>=240)expected.setSample(0,n,mainSignal(n-240,0));}
            gate.process(data.getArrayOfWritePointers(),1,4096);expect(hamlo::test::bitExact(data,expected));expect(gate.reading().missingKey==(mode==1));
        }
        beginTest("Audition toggle reaches the key at20ms and dry at20ms on release");
        {
            Gate gate;gate.prepare(48000,256);Settings p;p.external=true;gate.set(p);std::array<float,2000> a{},k{};a.fill(.1f);k.fill(.3f);float* audio[]{a.data()};const float* key[]{k.data()};gate.process(audio,1,2000,key,1);
            p.listen=true;gate.set(p);a.fill(.1f);gate.process(audio,1,2000,key,1);expectWithinAbsoluteError(a[0],.1f+(.3f-.1f)/960,1.e-7f);expect(a[958]<.3f);expectEquals(a[959],.3f);
            p.listen=false;gate.set(p);a.fill(.1f);gate.process(audio,1,2000,key,1);expect(a[958]>.1f);expectEquals(a[959],.1f);
        }
        beginTest("Audition/filter/bypass/key-loss automation is sample scheduled and allocation free");
        size_t allocations=0;const auto expected=events(1,allocations);for(int block:{64,127,512,4096}){auto actual=events(block,allocations);expect(hamlo::test::bitExact(actual,expected),juce::String(block));}expectEquals(static_cast<int>(allocations),0);

        beginTest("New parameter appends at77, UI state and old reset roundtrip");
        auto p=std::make_unique<P>(kind);expectEquals(p->getParameters().size(),78);auto* last=dynamic_cast<juce::RangedAudioParameter*>(p->getParameters()[77]);expect(last!=nullptr&&last->paramID=="v2_listen");
        p->setEditorView(1);p->setEditorTab(2);p->setParameterValue("v2_listen",1);const auto state=save(*p);auto q=std::make_unique<P>(kind);load(*q,state);expectEquals(q->editorView(),1);expectEquals(q->editorTab(),2);expectEquals(q->parameterValue("v2_listen"),1.);expect(save(*q)==state);
        p->setCurrentProgram(3);const auto old=save(*p);load(*q,old);expectEquals(q->editorView(),2);expectEquals(q->editorTab(),0);expectEquals(q->parameterValue("v2_listen"),0.);expect(save(*q)==old);
        expect(!p->isProgramModified());p->setParameterValue("v2_range",35);expect(p->isProgramModified());p->setParameterValue("v2_range",40);expect(!p->isProgramModified());

        beginTest("Listen host routing and prepared project restoration reach the actual output");
        for(int channels:{1,2})for(int keys:{0,1,2})
        {
            auto host=std::make_unique<P>(kind);auto layout=host->getBusesLayout();layout.inputBuses.set(0,juce::AudioChannelSet::canonicalChannelSet(channels));layout.outputBuses.set(0,juce::AudioChannelSet::canonicalChannelSet(channels));layout.inputBuses.set(1,keys?juce::AudioChannelSet::canonicalChannelSet(keys):juce::AudioChannelSet::disabled());host->setBusesLayout(layout);
            host->setParameterValue("v2_external",keys>0);host->setParameterValue("v2_listen",1);host->setParameterValue("v2_mix",0);host->setParameterValue("v2_output",12);host->prepareToPlay(48000,256);
            const auto stored=save(*host);auto restored=std::make_unique<P>(kind);restored->setBusesLayout(layout);restored->prepareToPlay(48000,256);load(*restored,stored);
            for(auto* processor:{host.get(),restored.get()})
            {
                juce::AudioBuffer<float> data(channels+keys,4096),expected(channels,4096);expected.clear();
                for(int n=0;n<4096;++n){for(int c=0;c<channels;++c){data.setSample(c,n,mainSignal(n,c));if(n>=240)expected.setSample(c,n,keys?keySignal(n-240,std::min(c,keys-1)):mainSignal(n-240,c));}for(int c=0;c<keys;++c)data.setSample(channels+c,n,keySignal(n,c));}
                juce::MidiBuffer midi;hamlo::test::allocation::begin();processor->processBlock(data,midi);const auto allocations=hamlo::test::allocation::finish();
                juce::AudioBuffer<float> output(data.getArrayOfWritePointers(),channels,4096);expect(hamlo::test::bitExact(output,expected));expectEquals(static_cast<int>(allocations),0);
                for(int n=0;n<4096;++n){for(int c=0;c<channels;++c)data.setSample(c,n,mainSignal(n,c));for(int c=0;c<keys;++c)data.setSample(channels+c,n,keySignal(n,c));}
                processor->processBlockBypassed(data,midi);bool dry=true;for(int c=0;c<channels;++c)for(int n=1200;n<4096;++n)dry=dry&&data.getSample(c,n)==mainSignal(n-240,c);expect(dry,"Host bypass overrides Listen after its20msfade");
            }
        }

        beginTest("All pages/roles scale without control or preset overlap");
        p=std::make_unique<P>(kind);std::unique_ptr<juce::AudioProcessorEditor> base(p->createEditor());auto* editor=dynamic_cast<hpeq::standalone::ModulePluginEditor*>(base.get());expect(editor!=nullptr);if(!editor)return;
        for(double scale:{.5,1.,1.5})for(int mode=1;mode<=4;++mode)for(int view:{0,1})for(int tab=0;tab<(view?3:1);++tab)
        {
            p->setParameterValue("v2_engine",mode);p->setParameterValue("v2_steps",32);p->setEditorView(view);p->setEditorTab(tab);editor->setSize(juce::roundToInt(1180*scale),juce::roundToInt(780*scale));editor->refreshEditor();
            expect(editor->getModernPanel()->layoutValid());expect(editor->getPresetScreen().getBounds()==juce::Rectangle<float>(56,20,336,116).transformedBy(juce::AffineTransform::scale(static_cast<float>(scale))).toNearestInt());
        }
        // First screenshots revealed two enabled controls with no audible role
        // effect: common Release in Drum, and Stereo Link in Rhythm.
        p->setParameterValue("v2_engine",2);p->setEditorView(1);p->setEditorTab(0);editor->refreshEditor();expect(!editor->getModernPanel()->control("v2_release")->isEnabled()&&editor->getModernPanel()->control("v2_tail")->isEnabled());
        p->setParameterValue("v2_engine",4);p->setEditorTab(1);editor->refreshEditor();expect(!editor->getModernPanel()->control("v2_link")->isEnabled());
        editor->setSize(1180,780);editor->showView(0);p->setParameterValue("v2_engine",1);editor->refreshEditor();
        beginTest("Buttons, exact numeric entry and gestures affect their intended IDs");
        for(const auto& pair:std::initializer_list<std::pair<const char*,const char*>>{{"v2_hp","OFF"},{"v2_output","0.00 dB"}})
        {auto* input=editor->getModernPanel()->control(pair.first);juce::String text;for(auto* child:input->getChildren())if(auto* box=dynamic_cast<juce::Label*>(child))text=box->getText();expect(text==pair.second,pair.first);}
        click(*editor,"modern_power");expectEquals(p->parameterValue("v2_enabled"),1.);click(*editor,"profile_hard");expectEquals(p->parameterValue("v2_profile"),1.);click(*editor,"listen_key");expectEquals(p->parameterValue("v2_listen"),1.);
        auto* slider=dynamic_cast<juce::Slider*>(editor->getModernPanel()->control("v2_threshold"));expect(slider!=nullptr);
        if(slider)
        {
            // First202checks used setValue here. Commit through the actual value
            // label now, exercising text parsing and Slider's drag notification.
            Gesture g;p->parameter("v2_threshold")->addListener(&g);juce::Label* text=nullptr;for(auto* c:slider->getChildren())if(auto* label=dynamic_cast<juce::Label*>(c))text=label;
            expect(text!=nullptr);if(text)text->setText("-25.75 dB",juce::sendNotificationSync);
            expectWithinAbsoluteError(p->parameterValue("v2_threshold"),-25.75,1.e-4);expect(g.begins==1&&g.ends==1&&g.values>0);p->parameter("v2_threshold")->removeListener(&g);
            expect(slider->getTooltip().isNotEmpty());
        }
        click(*editor,"view_advanced");expectEquals(p->editorView(),1);click(*editor,"RHYTHM");p->setEditorTab(2);editor->refreshEditor();click(*editor,"pattern_alternate");expectEquals(p->parameterValue("v2_step1"),1.);expectEquals(p->parameterValue("v2_step32"),0.);
        auto* step=dynamic_cast<juce::Slider*>(editor->getModernPanel()->control("v2_step32"));expect(step!=nullptr);
        if(step)for(auto* c:step->getChildren())if(auto* text=dynamic_cast<juce::Label*>(c))text->setText("37.5%",juce::sendNotificationSync);
        expectWithinAbsoluteError(p->parameterValue("v2_step32"),.375,1.e-6);
        click(*editor,"neutral_init");expectEquals(p->parameterValue("v2_engine"),1.);expectEquals(p->parameterValue("v2_enabled"),0.);expectEquals(p->parameterValue("v2_listen"),0.);expectEquals(p->parameterValue("bypass"),0.);
        click(*editor,"view_legacy");expectEquals(p->parameterValue("v2_engine"),0.);expect(editor->getPanel()->isVisible()&&!editor->getModernPanel()->isVisible());
    }
};
S4Tests tests;
}

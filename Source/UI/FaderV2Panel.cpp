#include "UI/FaderV2Panel.h"
#include "UI/HamloFaderPanel.h"
#include "UI/BlackSteel.h"
#include <cmath>

namespace hpeq::ui
{
namespace
{
const juce::Colour ink(0xffe6d8a6), green(0xff73ff9e), muted(0xffa6ad97);
juce::String help(const juce::String& id)
{
    if(id=="targetDb")return "RMS target in Ride; selected RMS or integrated LUFS target in Match. Learn first, Stop to propose, Apply deliberately.";
    if(id=="matchCeiling"||id=="peakGuard")return "Limits the Match proposal using learned true peak. This is not an output limiter; later input, routing or output changes can exceed the ceiling.";
    if(id=="matchedDb")return "Held Match correction. START/STOP measures a proposal; only APPLY changes this value. Saved in the project.";
    if(id=="floorDb")return "Ride activity floor. Below this floor, after Hold, correction returns to unity so pauses are not continuously boosted.";
    if(id=="profile")return "Natural uses a gentle two-pole response and 50 ms detector. Fast uses a 10 ms detector and direct response. Neither is a peak limiter.";
    if(id=="leftDelayMs"||id=="rightDelayMs")return "Deliberate channel alignment, 0 to 20 ms. Host latency remains zero. This delays audio; it cannot advance a channel.";
    if(id=="rightPhase")return "Right-channel offset in cycles. Zero links both Motion phases; nonzero offsets create alternating level movement.";
    if(id=="depthDb")return "Maximum Motion attenuation. The graph shows relative levels, from full depth at zero to unity at one. Zero depth is transparent.";
    if(id=="sync")return "Sync follows host PPQ/BPM and fades to unity when stopped. Without PPQ it uses a sample clock. Free runs independently.";
    if(id.startsWith("level"))return "Custom cyclic Motion point, 0 to 1. Drag the graph or select a point for precise input. Points are saved and automatable.";
    if(id=="measure")return "Match learning uses RMS including pauses, or gated integrated LUFS. M/S/I need enough audio. Integration stops at 72000 windows (about two hours); Reset clears it. Not full EBU Mode certification.";
    if(id=="gainDb")return "Manual gain in every mode. Input Trim precedes analysis, Gain follows the mode, Output follows mixing.";
    if(id=="mix")return "Blend aligned dry and processed signal. Zero is dry; one is fully processed. Enable and bypass are separately smoothed.";
    return "Adjust "+id+". Type an exact value, double-click for default, or hold Shift for fine movement. Saved and automatable.";
}
struct Look final : juce::LookAndFeel_V4
{
    Look(){setColour(juce::Slider::textBoxTextColourId,ink);setColour(juce::Slider::textBoxBackgroundColourId,juce::Colour(0xff131c17));setColour(juce::Slider::textBoxOutlineColourId,muted.withAlpha(.4f));setColour(juce::ComboBox::backgroundColourId,juce::Colour(0xff1b251e));setColour(juce::ComboBox::textColourId,ink);setColour(juce::TextButton::buttonColourId,juce::Colour(0xff25352a));setColour(juce::TextButton::textColourOffId,ink);setColour(juce::TextButton::textColourOnId,green);}
    void drawRotarySlider(juce::Graphics& g,int x,int y,int w,int h,float v,float,float,juce::Slider&)override
    {const juce::Point<float> c(static_cast<float>(x+w/2),static_cast<float>(y+h/2));const float s=juce::jmin(w,h)/132.f;steel::knobScale(g,c,s);steel::brassKnob(g,c,(-135+270*v)*juce::MathConstants<float>::pi/180,s,1);}
};
}
struct FaderV2Panel::Impl
{
    struct Surface final:juce::Component
    {
        juce::Image background;
        Surface(){background=juce::Image(juce::Image::RGB,1180,700,true);juce::Graphics g(background);steel::panel(g,1180,700,41);steel::nameplate(g,{420,41,402,92},"HamloFader","LEVEL / MOTION","HF-002");}
        void paint(juce::Graphics& g)override{g.drawImageAt(background,0,0);g.setColour(muted);g.setFont(steel::monoFont(11));g.drawText("HAMLOPROD  /  TRIM - MATCH - RIDE - MOTION",56,677,1068,17,juce::Justification::centred);}
    } stage;
    struct Control
    {
        juce::String id;int page=0,mode=-1,section=1;
        juce::Label label;
        std::unique_ptr<juce::Slider> slider;
        std::unique_ptr<juce::ComboBox> choice;
        juce::Component* component(){return slider?static_cast<juce::Component*>(slider.get()):choice.get();}
    };
    struct Graph final:juce::Component,juce::SettableTooltipClient
    {
        Impl& ui;int drag=-1;
        explicit Graph(Impl& i):ui(i){setName("motionGraph");setTooltip(help("level00"));}
        void paint(juce::Graphics& g)override
        {
            g.fillAll(juce::Colour(0xff0b1510));g.setColour(muted.withAlpha(.3f));g.drawRect(getLocalBounds());
            const int shape=static_cast<int>(ui.p.parameterValue("shape")),n=shape==0?static_cast<int>(ui.p.parameterValue("points")):64;const auto area=getLocalBounds().toFloat().reduced(8);
            for(int i=0;i<n;++i){const float x=area.getX()+area.getWidth()*i/n,w=area.getWidth()/n;const double phase=static_cast<double>(i)/n;const float v=static_cast<float>(shape==1?.5+.5*std::cos(2*juce::MathConstants<double>::pi*phase):shape==2?1-2*std::abs(phase-.5):ui.p.parameterValue(pointKey(i)));g.setColour(shape==0&&i==ui.point?ink:green.withAlpha(.65f));g.fillRect(x+1,area.getBottom()-v*area.getHeight(),w-2,v*area.getHeight());}
            if(ui.haveStatus){g.setColour(ink);const float x=area.getX()+static_cast<float>(ui.live.reading.phase)*area.getWidth();g.drawVerticalLine(static_cast<int>(x),area.getY(),area.getBottom());}
        }
        void mouseDown(const juce::MouseEvent& e)override{move(e);}
        void mouseDrag(const juce::MouseEvent& e)override{move(e);}
        void mouseUp(const juce::MouseEvent&)override{ui.end();drag=-1;}
        void move(const juce::MouseEvent& e)
        {
            if(ui.p.parameterValue("shape")!=0)return;
            const int n=static_cast<int>(ui.p.parameterValue("points"));const auto a=getLocalBounds().toFloat().reduced(8);
            const int i=juce::jlimit(0,n-1,static_cast<int>((e.position.x-a.getX())/a.getWidth()*n));
            if(i!=drag){ui.begin(pointKey(i));drag=i;}ui.point=i;ui.p.setParameterValue(pointKey(i),juce::jlimit(0.,1.,static_cast<double>((a.getBottom()-e.position.y)/a.getHeight())));ui.refresh();
        }
    } graph;
    FaderV2Panel& owner;standalone::ModulePluginProcessor& p;Look look;
    HamloFaderPanel legacy;
    std::vector<std::unique_ptr<Control>> controls;
    std::array<juce::TextButton,3> pages;
    std::array<juce::TextButton,4> modes;
    juce::ComboBox detail,pointChoice;
    juce::Slider pointValue;
    juce::TextButton start{"START LEARN"},stop{"STOP / PROPOSE"},applyButton{"APPLY"},reset{"RESET METERS"},bypass{"BYPASS"};
    juce::Label meters,status,message,pointLabel;
    juce::TooltipWindow tooltip{&owner,600};
    standalone::FaderStatus live;bool haveStatus=false,updating=false,pending=false;std::uint64_t waitingFrame=0,waitingEpoch=0;
    int point=0;juce::RangedAudioParameter* gesture=nullptr;
    static juce::String pointKey(int i){return "level"+juce::String(i).paddedLeft('0',2);}
    Impl(FaderV2Panel& o,standalone::ModulePluginProcessor& processor):graph(*this),owner(o),p(processor)
    {
        owner.addAndMakeVisible(stage);stage.setSize(1180,700);stage.setLookAndFeel(&look);
        stage.addChildComponent(legacy);legacy.setBounds(0,0,1180,700);
        legacy.onKnobGestureStart=[this](HamloFaderPanel::Param v){begin(HamloFaderPanel::keyFor(v));};
        legacy.onKnobChange=[this](HamloFaderPanel::Param v,double x){p.setParameterValue(HamloFaderPanel::keyFor(v),x);};legacy.onKnobGestureEnd=[this]{end();};
        legacy.onSwitchChange=[this](HamloFaderPanel::Switch v,int x){set(HamloFaderPanel::keyFor(v),x);};legacy.onPowerChange=[this](bool on){set("bypass",on?0:1);};
        legacy.readMeters=[this]{standalone::FaderStatus s;if(!p.readStatus(s))return HamloFaderPanel::Meters{};return HamloFaderPanel::Meters{static_cast<float>(s.legacyInput),static_cast<float>(s.legacyOutput),static_cast<float>(s.legacyLong)};};
        const char* pageNames[]{"QUICK","ADVANCED","LEGACY"};
        for(int i=0;i<3;++i){auto& b=pages[static_cast<size_t>(i)];stage.addAndMakeVisible(b);b.setButtonText(pageNames[i]);b.setName(pageNames[i]);b.setBounds(844+i*94,8,88,26);b.onClick=[this,i]{owner.selectPage(i);};}
        const char* names[]{"TRIM","MATCH","RIDE","MOTION"};
        for(int i=0;i<4;++i){auto& b=modes[static_cast<size_t>(i)];stage.addAndMakeVisible(b);b.setButtonText(names[i]);b.setName(names[i]);b.setBounds(56+i*190,150,178,32);b.onClick=[this,i]{set("mode",i);refresh();};}
        stage.addAndMakeVisible(bypass);bypass.setName("bypass");bypass.setBounds(986,54,138,30);bypass.onClick=[this]{set("bypass",p.isModuleOn()?1:0);refresh();};
        stage.addAndMakeVisible(detail);detail.addItem("CHANNEL TOOLS",1);detail.addItem("MODE CONTROLS",2);detail.setSelectedId(2);detail.setBounds(844,150,280,32);detail.onChange=[this]{refresh();};detail.setName("advancedSection");
        for(auto* label:{&meters,&status,&message,&pointLabel}){stage.addAndMakeVisible(*label);label->setColour(juce::Label::textColourId,ink);label->setFont(steel::monoFont(12));}
        meters.setBounds(56,192,1068,54);status.setBounds(56,248,1068,28);message.setBounds(56,636,640,32);
        const char* common[]{"inputDb","gainDb","targetDb","mix","outputDb"};
        for(int i=0;i<5;++i)add(common[i],0,-1,1,i,286);
        add("enabled",0,0,1,5,286);add("matchedDb",0,1,1,5,286);add("strength",0,2,1,5,286);add("depthDb",0,3,1,5,286);
        addRow({"routing","polarity","leftDb","rightDb","leftDelayMs","rightDelayMs"},0,0,1,418);
        addRow({"measure","matchBoost","matchCut","matchCeiling","peakGuard","enabled"},0,1,1,418);
        addRow({"profile","riseMs","maxBoost","maxCut","floorDb","holdMs"},0,2,1,418);
        addRow({"shape","sync","beats","freeMs","phase","rightPhase"},0,3,1,418);
        addRow({"inputDb","outputDb","leftDb","rightDb","leftDelayMs","rightDelayMs","routing","swap","polarity","mix","enabled","mute"},1,-1,0,286);
        addRow({"gainDb","targetDb","mix","measure","matchBoost","matchCut","matchCeiling","peakGuard","matchedDb"},1,1,1,286);
        addRow({"targetDb","profile","strength","maxBoost","maxCut","floorDb","holdMs","idleMs","riseMs","fallMs"},1,2,1,286);
        addRow({"gainDb","inputDb","outputDb","leftDb","rightDb","mix","routing","swap","polarity","enabled","mute"},1,0,1,286);
        addRow({"shape","points","smooth","sync","depthDb","beats","freeMs","phase","rightPhase","edgeMs"},1,3,1,286);
        stage.addAndMakeVisible(graph);graph.setBounds(56,550,772,76);
        stage.addAndMakeVisible(pointChoice);pointChoice.setName("selectedPoint");pointChoice.setBounds(844,568,98,28);pointChoice.onChange=[this]{point=pointChoice.getSelectedId()-1;refresh();};
        stage.addAndMakeVisible(pointValue);pointValue.setName("pointValue");pointValue.setRange(0,1,.001);pointValue.setSliderStyle(juce::Slider::LinearHorizontal);pointValue.setTextBoxStyle(juce::Slider::TextBoxRight,false,64,24);pointValue.setBounds(950,568,174,28);
        pointValue.onDragStart=[this]{begin(pointKey(point));};pointValue.onDragEnd=[this]{end();};pointValue.onValueChange=[this]{if(!updating)owner.editPoint(point,pointValue.getValue());};pointLabel.setText("CUSTOM POINT",juce::dontSendNotification);pointLabel.setBounds(844,540,280,24);
        for(auto* b:{&start,&stop,&applyButton,&reset})stage.addAndMakeVisible(*b);
        start.setName("startLearn");stop.setName("stopLearn");applyButton.setName("applyMatch");reset.setName("resetMeters");
        start.setBounds(56,636,170,32);stop.setBounds(238,636,186,32);applyButton.setBounds(436,636,140,32);reset.setBounds(926,636,198,32);
        start.onClick=[this]{owner.action(standalone::FaderAction::StartLearn);};stop.onClick=[this]{owner.action(standalone::FaderAction::StopLearn);};reset.onClick=[this]{owner.action(standalone::FaderAction::Reset);};applyButton.onClick=[this]{owner.apply();};
        start.setTooltip("Start a new learning pass. Play representative material, then Stop. Learning never applies gain automatically.");stop.setTooltip("Stop learning and propose a held correction. Inspect the proposal, then Apply.");applyButton.setTooltip("Apply the valid measured proposal as an explicit automatable gain change.");reset.setTooltip("Clear transient meters, phase and learning. Retains configured gains, including applied Match correction.");
        refresh();
    }
    ~Impl(){end();stage.setLookAndFeel(nullptr);}
    void begin(const juce::String& id){end();gesture=p.parameter(id);if(gesture)gesture->beginChangeGesture();}
    void end(){if(gesture)gesture->endChangeGesture();gesture=nullptr;}
    void set(const juce::String& id,double x){begin(id);p.setParameterValue(id,x);end();}
    void addRow(std::initializer_list<const char*> ids,int page,int mode,int section,int y){int i=0;for(auto id:ids){add(id,page,mode,section,i%6,y+(i/6)*132);++i;}}
    void add(const juce::String& id,int pg,int mode,int section,int column,int y)
    {
        auto* parameter=p.parameter(id);if(!parameter)return;
        auto c=std::make_unique<Control>();c->id=id;c->page=pg;c->mode=mode;c->section=section;
        stage.addAndMakeVisible(c->label);c->label.setColour(juce::Label::textColourId,ink);c->label.setFont(steel::monoFont(11));c->label.setJustificationType(juce::Justification::centred);c->label.setText(parameter->getName(40).toUpperCase(),juce::dontSendNotification);c->label.setBounds(56+column*180,y,168,22);
        if(auto* choice=dynamic_cast<juce::AudioParameterChoice*>(parameter))
        {c->choice=std::make_unique<juce::ComboBox>();c->choice->addItemList(choice->choices,1);c->choice->setBounds(60+column*180,y+43,160,30);c->choice->onChange=[this,id,ptr=c->choice.get()]{if(!updating){set(id,ptr->getSelectedId()-1);refresh();}};c->choice->setTooltip(help(id));}
        else
        {c->slider=std::make_unique<juce::Slider>();auto& s=*c->slider;const auto range=parameter->getNormalisableRange();s.setRange(range.start,range.end,range.interval);s.setNumDecimalPlacesToDisplay(range.interval>=1?0:id.endsWith("Ms")?1:2);s.setSliderStyle(juce::Slider::RotaryVerticalDrag);s.setTextBoxStyle(juce::Slider::TextBoxBelow,false,142,22);s.setDoubleClickReturnValue(true,parameter->convertFrom0to1(parameter->getDefaultValue()));s.setTextValueSuffix(parameter->getLabel().isEmpty()?"":" "+parameter->getLabel());s.setBounds(56+column*180,y+22,168,108);s.setTooltip(help(id));s.onDragStart=[this,id]{begin(id);};s.onDragEnd=[this]{end();};s.onValueChange=[this,id,ptr=&s]{if(!updating)p.setParameterValue(id,ptr->getValue());};}
        c->component()->setName(id);stage.addAndMakeVisible(*c->component());controls.push_back(std::move(c));
    }
    void refresh()
    {
        updating=true;const bool old=p.parameterValue("engine")<.5;const int page=old?2:juce::jmin(1,p.editorPage());const int mode=static_cast<int>(p.parameterValue("mode"));
        legacy.setVisible(old);legacy.toBack();
        if(old)
        {
            HamloFaderPanel::Values v;
            for(size_t i=0;i<v.knobs.size();++i)v.knobs[i]=p.parameterValue(HamloFaderPanel::keyFor(static_cast<HamloFaderPanel::Param>(i)));
            for(size_t i=0;i<v.switches.size();++i)v.switches[i]=static_cast<int>(p.parameterValue(HamloFaderPanel::keyFor(static_cast<HamloFaderPanel::Switch>(i))));
            v.on=p.isModuleOn();legacy.setValues(v);
        }
        for(int i=0;i<3;++i)pages[static_cast<size_t>(i)].setToggleState(i==page,juce::dontSendNotification);
        for(int i=0;i<4;++i){auto& b=modes[static_cast<size_t>(i)];b.setVisible(!old);b.setToggleState(i==mode,juce::dontSendNotification);}
        bypass.setVisible(!old);bypass.setToggleState(!p.isModuleOn(),juce::dontSendNotification);detail.setVisible(page==1);
        for(auto& c:controls)
        {
            const bool visible=c->page==page&&(c->mode<0||c->mode==mode)&&(page==0||c->section==detail.getSelectedId()-1);
            c->component()->setVisible(visible);c->label.setVisible(visible);
            if(c->slider&&!c->slider->isMouseButtonDown()&&!c->slider->hasKeyboardFocus(true))c->slider->setValue(p.parameterValue(c->id),juce::dontSendNotification);
            if(c->choice)c->choice->setSelectedId(static_cast<int>(p.parameterValue(c->id))+1,juce::dontSendNotification);
            c->component()->setEnabled(c->id!="targetDb"||mode==1||mode==2);
        }
        haveStatus=p.readStatus(live);
        if(pending&&haveStatus&&(live.epoch!=waitingEpoch||live.frame>waitingFrame))pending=false;
        const bool ready=haveStatus&&!old;
        auto value=[](double v,bool valid){return valid&&std::isfinite(v)?juce::String(v,1):juce::String("WAIT");};
        auto meter=[&](const hamlo::fader::MeterReading& m){return "RMS M/S/I "+value(m.rmsM,m.readyM)+" / "+value(m.rmsS,m.readyS)+" / "+value(m.rmsI,m.readyI)+" dBFS    LUFS "+value(m.lufsM,m.readyM)+" / "+value(m.lufsS,m.readyS)+" / "+value(m.lufsI,m.readyI)+"    SP "+value(m.samplePeak,m.samples>0)+" dBFS  TP "+value(m.truePeak,m.samples>0)+" dBTP";};
        meters.setVisible(!old);status.setVisible(!old);message.setVisible(!old);
        meters.setText(ready?"IN   "+meter(live.reading.input)+"\nOUT  "+meter(live.reading.output):"WAITING FOR AUDIO / valid measurement not available",juce::dontSendNotification);
        juce::String text;
        if(ready){text=mode==1?(live.reading.learning?"LEARNING - Stop to propose":live.proposal.valid?"PROPOSAL "+juce::String(live.proposal.gainDb,2)+" dB  / predicted "+juce::String(live.proposal.predictedPeak,1)+" dBTP"+(live.proposal.limited?" / LIMITED":""):"No valid proposal - Learn representative audio"):mode==2?"RIDE "+juce::String(live.reading.correctionDb,2)+" dB / "+(live.reading.active?"ACTIVE":"IDLE"):mode==3?"MOTION / "+juce::String(live.reading.phase,3)+" cycles / "+(live.reading.motionRunning?"RUNNING":"STOPPED"):"TRIM / neutral gains are transparent";
            if(live.reading.input.full||live.reading.output.full)text+=" / I FULL:72000 windows - RESET";
            if(live.bypassed)text+=" / BYPASSED";}
        status.setText(pending?"COMMAND PENDING - waiting for audio":text,juce::dontSendNotification);
        const bool match=page!=2&&mode==1;start.setVisible(match);stop.setVisible(match);applyButton.setVisible(match);reset.setVisible(!old);message.setBounds(match?592:56,636,match?320:852,32);
        const bool enabled=ready&&!pending&&!live.bypassed&&p.isModuleOn()&&p.parameterValue("enabled")>.5&&p.parameterValue("mute")<.5;
        start.setEnabled(enabled&&!live.reading.learning);stop.setEnabled(enabled&&live.reading.learning);applyButton.setEnabled(enabled&&p.canApplyMatch());reset.setEnabled(!pending);
        const bool motion=!old&&mode==3;graph.setVisible(motion);pointChoice.setVisible(motion);pointValue.setVisible(motion);pointLabel.setVisible(motion);
        if(motion){const int n=static_cast<int>(p.parameterValue("points"));const bool custom=p.parameterValue("shape")==0;pointChoice.setEnabled(custom);pointValue.setEnabled(custom);pointLabel.setText(custom?"CUSTOM POINT":"SHAPE PREVIEW",juce::dontSendNotification);point=juce::jlimit(0,n-1,point);if(pointChoice.getNumItems()!=n){pointChoice.clear();for(int i=0;i<n;++i)pointChoice.addItem(juce::String(i+1),i+1);}pointChoice.setSelectedId(point+1,juce::dontSendNotification);if(!pointValue.isMouseButtonDown()&&!pointValue.hasKeyboardFocus(true))pointValue.setValue(p.parameterValue(pointKey(point)),juce::dontSendNotification);graph.repaint();}
        updating=false;
    }
};
FaderV2Panel::FaderV2Panel(standalone::ModulePluginProcessor& p):impl(std::make_unique<Impl>(*this,p)){}
FaderV2Panel::~FaderV2Panel()=default;
void FaderV2Panel::refresh(){impl->refresh();}
void FaderV2Panel::selectPage(int page){impl->set("engine",page==2?0:1);impl->p.setEditorPage(page);impl->refresh();}
void FaderV2Panel::editPoint(int i,double value){i=juce::jlimit(0,31,i);const auto id=Impl::pointKey(i);if(!impl->gesture)impl->set(id,value);else impl->p.setParameterValue(id,value);impl->point=i;impl->refresh();}
bool FaderV2Panel::action(standalone::FaderAction a){const bool ok=impl->p.requestAction(a);impl->message.setText(ok?"Accepted; waiting for audio":"BUSY - command not accepted; retry",juce::dontSendNotification);if(ok){impl->pending=true;impl->waitingFrame=impl->p.nextActionFrame();impl->waitingEpoch=impl->haveStatus?impl->live.epoch:0;}impl->refresh();return ok;}
bool FaderV2Panel::apply(){const bool ok=impl->p.applyMatch();impl->message.setText(ok?"Applied held correction":"Not applied: proposal stale, pending or unavailable",juce::dontSendNotification);impl->refresh();return ok;}
juce::Component* FaderV2Panel::control(const juce::String& id)const{for(auto* c:impl->stage.getChildren())if(c->getName()==id&&c->isVisible())return c;return nullptr;}
void FaderV2Panel::resized(){const float s=juce::jmin(getWidth()/1180.f,getHeight()/700.f);impl->stage.setTransform(juce::AffineTransform::scale(s).translated((getWidth()-1180*s)/2,(getHeight()-700*s)/2));}
juce::Rectangle<float> FaderV2Panel::contentBounds()const{const float s=juce::jmin(getWidth()/1180.f,getHeight()/700.f);return {(getWidth()-1180*s)/2,(getHeight()-700*s)/2,1180*s,700*s};}
juce::String FaderV2Panel::helpKeyAt(juce::Point<float>)const{return {};}
}

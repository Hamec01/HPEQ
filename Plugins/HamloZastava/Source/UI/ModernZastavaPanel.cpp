#include "UI/ModernZastavaPanel.h"
#include <cmath>

namespace hpeq::ui
{
namespace
{
using Processor=standalone::ModulePluginProcessor;
juce::String caption(const juce::String& id)
{
    static const std::pair<const char*,const char*> names[]{
        {"v2_threshold","THRESHOLD"},{"v2_range","RANGE"},{"v2_attack","ATTACK"},{"v2_hold","HOLD"},{"v2_release","RELEASE"},
        {"v2_ratio","RATIO"},{"v2_knee","KNEE"},{"v2_hysteresis","HYSTERESIS"},{"v2_body","DRUM BODY"},{"v2_tail","DRUM TAIL"},
        {"v2_onset","ONSET CONTRAST"},{"v2_retrigger","RETRIGGER GAP"},{"v2_detectorMs","DETECTOR TIME"},{"v2_lookahead","LOOKAHEAD"},
        {"v2_hp","KEY HIGH PASS"},{"v2_lp","KEY LOW PASS"},{"v2_link","STEREO LINK"},{"v2_mix","MIX"},{"v2_output","OUTPUT"},
        {"v2_steps","STEPS"},{"v2_beats","BEATS / STEP"},{"v2_ms","MS / STEP"},{"v2_external","KEY SOURCE"},
        {"v2_expand","CLEAN SHAPE"},{"v2_rms","DETECTOR"},{"v2_sync","RHYTHM CLOCK"}};
    for(const auto& p:names)if(id==p.first)return p.second;
    return "STEP "+id.fromFirstOccurrenceOf("v2_step",false,false);
}
juce::String explanation(const juce::String& id)
{
    static const std::pair<const char*,const char*> text[]{
        {"v2_threshold","Key level that opens Clean/Drum or engages Duck. Rhythm uses its pattern instead. Lower it to retain quiet details."},
        {"v2_range","Maximum attenuation in dB. Zero leaves level unchanged. The pattern's lowest step uses this floor."},
        {"v2_attack","Opening time, or Duck onset. Clean/Duck/Rhythm use a 90% response time; Drum uses a full attack segment."},
        {"v2_hold","Keep Clean open or Duck engaged after the key falls. Drum has its separate Body control."},
        {"v2_release","Closing time, or Duck recovery, measured to 90%. Drum uses its separate finite Tail instead."},
        {"v2_ratio","Downward expansion ratio in Clean Expand. At 1:1 there is no expansion."},
        {"v2_knee","Smooth the transition around the Clean expansion threshold. Zero selects a hard knee."},
        {"v2_hysteresis","Difference between opening and closing thresholds; helps avoid chatter near threshold."},
        {"v2_body","Full-level Drum body after its attack. It is independent of Clean Hold."},
        {"v2_tail","Drum fades to the Range floor exactly at the end of this segment."},
        {"v2_onset","Drum's fast/slow envelope contrast required for another hit. This is not instrument classification."},
        {"v2_retrigger","Minimum separation between Drum triggers. Reduce it for fast rolls and ghost notes."},
        {"v2_detectorMs","Peak release or RMS averaging time. Zero gives instantaneous level detection."},
        {"v2_lookahead","Anticipate opening or Duck onset by 0-5 ms. Total audio delay stays fixed at 5 ms. Rhythm does not use lookahead."},
        {"v2_hp","Remove lows from the detector key. 0 means off. LISTEN KEY auditions the same filter."},
        {"v2_lp","Remove highs from the detector key. 0 means off. Filters affect detection, not the main audio tone."},
        {"v2_link","Link both channels to the strongest key. 100% preserves a stable shared gain; 0% responds independently."},
        {"v2_mix","Blend delayed dry and processed main audio. LISTEN KEY ignores Mix. Bypass keeps the same fixed delay."},
        {"v2_output","Main output gain. Processing Off and LISTEN KEY do not apply this gain."},
        {"v2_steps","Active pattern length, 16-32. Inactive steps retain their stored values."},
        {"v2_beats","Quarter-note beats per step. 1 = quarter note, 0.5 = eighth, 0.25 = sixteenth. Used by Tempo clock."},
        {"v2_ms","Milliseconds per step for Free clock. Tempo clock uses Beats / Step instead."},
        {"v2_external","Main uses this track; External uses the host sidechain bus. Missing External key passes dry and shows a warning."},
        {"v2_expand","Clean Gate opens/closes with hysteresis; Expand follows the Ratio/Knee curve continuously."},
        {"v2_rms","Peak reacts to instantaneous peaks; RMS follows average key energy over Detector Time."},
        {"v2_sync","Tempo follows host BPM, position and transport; stopped transport passes dry. Free runs in milliseconds."}};
    for(const auto& p:text)if(id==p.first)return p.second;
    return "Pattern level in percent. 0 uses the Range floor; 100 is full level. Type an exact value below the bar.";
}
void buttonStyle(juce::TextButton& b)
{
    b.setColour(juce::TextButton::buttonColourId,juce::Colour(0xff202923));
    b.setColour(juce::TextButton::buttonOnColourId,juce::Colour(0xff245b37));
    b.setColour(juce::TextButton::textColourOffId,steel::stencil);
    b.setColour(juce::TextButton::textColourOnId,juce::Colour(0xffb5ffca));
}
}
struct ModernZastavaPanel::Look:juce::LookAndFeel_V4
{
    void drawRotarySlider(juce::Graphics& g,int x,int y,int w,int h,float value,float start,float end,juce::Slider& slider) override
    {
        const float scale=std::min(w,h)/150.f;const juce::Point<float> centre(x+w*.5f,y+h*.5f);
        steel::knobScale(g,centre,scale*.70f);
        steel::brassKnob(g,centre,start+value*(end-start)-juce::MathConstants<float>::pi,scale*.70f,slider.isEnabled()?1.f:.35f);
    }
    juce::Font getTextButtonFont(juce::TextButton&,int height) override{return juce::Font(steel::stencilFont(std::min(15.f,height*.40f)));}
};
struct ModernZastavaPanel::Control:juce::Component
{
    Control(Processor& p,const standalone::ParameterSpec& s):processor(p),spec(s)
    {
        setComponentID("field:"+s.id);label.setText(caption(s.id),juce::dontSendNotification);label.setTooltip(explanation(s.id));label.setJustificationType(juce::Justification::centred);label.setFont(juce::Font(steel::stencilFont(s.id.startsWith("v2_step")?11.f:13.f)));label.setColour(juce::Label::textColourId,steel::stencil);addAndMakeVisible(label);
        if(s.choices.isEmpty())
        {
            const bool step=s.id.startsWith("v2_step")&&s.id!="v2_steps";
            slider.setSliderStyle(step?juce::Slider::LinearVertical:juce::Slider::RotaryHorizontalVerticalDrag);
            slider.setRange(s.minimum,s.maximum,s.interval);slider.setSkewFactor(s.skew);
            slider.setTextBoxStyle(juce::Slider::TextBoxBelow,false,step?53:115,step?20:24);slider.setDoubleClickReturnValue(true,s.defaultValue);
            slider.setColour(juce::Slider::textBoxBackgroundColourId,juce::Colour(0xff101a15));slider.setColour(juce::Slider::textBoxTextColourId,steel::phosphor);
            slider.setColour(juce::Slider::textBoxOutlineColourId,juce::Colour(0xff53604b));slider.setColour(juce::Slider::thumbColourId,steel::phosphor);slider.setColour(juce::Slider::trackColourId,juce::Colour(0xff39744b));
            const bool percent=step||s.id=="v2_link"||s.id=="v2_mix";
            slider.textFromValueFunction=[this,percent](double v)
            {
                if(std::abs(v)<.0005)v=0;
                if(percent)return juce::String(v*100,1)+"%";
                if((spec.id=="v2_hp"||spec.id=="v2_lp")&&v==0)return juce::String("OFF");
                const int digits=spec.interval>=1?0:spec.id=="v2_beats"?6:2;
                return juce::String(v,digits)+(spec.unit.isEmpty()?juce::String{}:" "+spec.unit);
            };
            slider.valueFromTextFunction=[percent](const juce::String& text){return text.trim().getDoubleValue()/(percent?100:1);};
            slider.updateText(); // Zero-valued defaults otherwise retain JUCE's old formatter.
            slider.onDragStart=[this]{dragging=true;processor.parameter(spec.id)->beginChangeGesture();};
            slider.onDragEnd=[this]{if(dragging){dragging=false;processor.parameter(spec.id)->endChangeGesture();}};
            slider.onValueChange=[this]{auto* q=processor.parameter(spec.id);if(!dragging)q->beginChangeGesture();processor.setParameterValue(spec.id,slider.getValue());if(!dragging)q->endChangeGesture();};
            slider.setComponentID(s.id);slider.setName(caption(s.id));slider.setTooltip(explanation(s.id));addAndMakeVisible(slider);
        }
        else
        {
            combo.addItemList(s.choices,1);combo.onChange=[this]{auto* q=processor.parameter(spec.id);q->beginChangeGesture();processor.setParameterValue(spec.id,combo.getSelectedId()-1);q->endChangeGesture();};
            combo.setComponentID(s.id);combo.setName(caption(s.id));combo.setTooltip(explanation(s.id));addAndMakeVisible(combo);
        }
    }
    ~Control() override{if(dragging)processor.parameter(spec.id)->endChangeGesture();}
    void refresh(){if(!dragging)slider.setValue(processor.parameterValue(spec.id),juce::dontSendNotification);combo.setSelectedId(1+juce::roundToInt(processor.parameterValue(spec.id)),juce::dontSendNotification);}
    void resized() override{label.setBounds(0,0,getWidth(),20);if(spec.choices.isEmpty())slider.setBounds(0,20,getWidth(),getHeight()-20);else combo.setBounds(4,24,getWidth()-8,32);}
    juce::Component* input(){return spec.choices.isEmpty()?static_cast<juce::Component*>(&slider):static_cast<juce::Component*>(&combo);}
    Processor& processor;const standalone::ParameterSpec& spec;juce::Label label;juce::Slider slider;juce::ComboBox combo;bool dragging=false;
};

ModernZastavaPanel::ModernZastavaPanel(Processor& p):processor(p),look(std::make_unique<Look>()),canvas(*this)
{
    setLookAndFeel(look.get());canvas.setSize(1180,780);addAndMakeVisible(canvas);canvas.addMouseListener(this,true);
    for(size_t n=standalone::modern::External;n<standalone::modern::Listen;++n)
    {
        auto c=std::make_unique<Control>(p,standalone::modern::specs()[n]);canvas.addAndMakeVisible(*c);controls.push_back(std::move(c));
    }
    const auto attach=[this](juce::TextButton& b,const char* text,const char* id,const char* tooltip)
    {b.setButtonText(text);b.setComponentID(id);b.setTooltip(tooltip);buttonStyle(b);canvas.addAndMakeVisible(b);};
    const char* names[]{"CLEAN","DRUM","DUCK","RHYTHM"};
    for(int n=0;n<4;++n){attach(modes[static_cast<size_t>(n)],names[n],names[n],"Choose this processing role. Your modern settings are retained; use INIT for a neutral start.");modes[static_cast<size_t>(n)].onClick=[this,n]{immediate("v2_engine",n+1);refresh();};}
    for(int n=0;n<2;++n){attach(profiles[static_cast<size_t>(n)],n?"HARD":"SOFT",n?"profile_hard":"profile_soft",n?"A direct one-pole gain response; linear finite Drum segments.":"A gentler two-pole response; rounded finite Drum segments.");profiles[static_cast<size_t>(n)].onClick=[this,n]{immediate("v2_profile",n);refresh();};}
    const char* tabNames[]{"ENVELOPE","KEY / MIX","RHYTHM"};
    const char* tabIds[]{"tab_envelope","tab_key","tab_rhythm"};
    for(int n=0;n<3;++n){attach(tabs[static_cast<size_t>(n)],tabNames[n],tabIds[n],"Open this Advanced section. Changing pages does not alter audio parameters.");tabs[static_cast<size_t>(n)].onClick=[this,n]{processor.setEditorTab(n);refresh();};}
    attach(power,"PROCESS OFF","modern_power","Turn modern processing on or off. Turning on also releases host bypass. Off is unity delayed dry.");
    power.onClick=[this]{const bool on=processor.isModuleOn()&&processor.parameterValue("v2_enabled")>.5;if(!on)immediate("bypass",0);immediate("v2_enabled",on?0:1);refresh();};
    attach(listen,"LISTEN KEY","listen_key","Audition the signed filtered key, delayed 5 ms, independently of Processing, Mix and Output. Bypass or missing external key returns dry.");
    listen.onClick=[this]{immediate("v2_listen",processor.parameterValue("v2_listen")>.5?0:1);refresh();};listen.setColour(juce::TextButton::buttonOnColourId,juce::Colour(0xff8c3027));
    attach(init,"INIT","neutral_init","Reset to Clean with processing off. No audible effect; build your settings from this neutral start.");
    init.onClick=[this]{processor.setCurrentProgram(0);immediate("v2_engine",1);immediate("bypass",0);processor.setEditorView(0);refresh();};
    attach(all,"ALL 100%","pattern_all","Set all 32 stored step levels to 100%.");attach(off,"ALL 0%","pattern_off","Set all 32 stored step levels to the Range floor.");attach(alternate,"ALTERNATE","pattern_alternate","Create alternating full/floor levels. Edit every step below.");
    const auto pattern=[this](int kind){for(int n=0;n<32;++n)immediate("v2_step"+juce::String(n+1),kind==2?(n%2?0:1):kind);refresh();};
    all.onClick=[pattern]{pattern(1);};off.onClick=[pattern]{pattern(0);};alternate.onClick=[pattern]{pattern(2);};
    help.setFont(juce::Font(steel::monoFont(13)));help.setColour(juce::Label::textColourId,steel::stencil);help.setJustificationType(juce::Justification::centredLeft);canvas.addAndMakeVisible(help);
    context="Hover a control for help. Type exact values below knobs; double-click a knob to reset it.";refresh();
}
ModernZastavaPanel::~ModernZastavaPanel(){canvas.removeMouseListener(this);controls.clear();setLookAndFeel(nullptr);}
void ModernZastavaPanel::immediate(const juce::String& id,double value){auto* p=processor.parameter(id);p->beginChangeGesture();processor.setParameterValue(id,value);p->endChangeGesture();}
juce::Component* ModernZastavaPanel::control(const juce::String& id) const{for(const auto& c:controls)if(c->spec.id==id)return c->input();return nullptr;}
void ModernZastavaPanel::mouseEnter(const juce::MouseEvent& e){mouseMove(e);}
void ModernZastavaPanel::mouseMove(const juce::MouseEvent& e)
{for(auto* c=e.originalComponent;c!=nullptr&&c!=this;c=c->getParentComponent())if(auto* t=dynamic_cast<juce::TooltipClient*>(c)){const auto text=t->getTooltip();if(text.isNotEmpty()){context=text;help.setText(context,juce::dontSendNotification);break;}}}
void ModernZastavaPanel::refresh()
{
    const int mode=juce::roundToInt(processor.parameterValue("v2_engine")),view=processor.editorView(),tab=processor.editorTab(),steps=juce::roundToInt(processor.parameterValue("v2_steps")),sync=juce::roundToInt(processor.parameterValue("v2_sync"));
    if(mode!=shownMode||view!=shownView||tab!=shownTab||steps!=shownSteps||sync!=shownSync){shownMode=mode;shownView=view;shownTab=tab;shownSteps=steps;shownSync=sync;arrange();}
    for(auto& c:controls)
    {
        c->refresh();const auto& id=c->spec.id;bool active=true;
        if(id=="v2_body"||id=="v2_tail"||id=="v2_onset"||id=="v2_retrigger")active=mode==2;
        if(id=="v2_ratio"||id=="v2_knee")active=mode==1&&processor.parameterValue("v2_expand")>.5;
        if(id=="v2_expand")active=mode==1;
        if(id=="v2_hysteresis")active=mode==3||(mode==1&&processor.parameterValue("v2_expand")<.5);
        if(id=="v2_hold")active=mode==1||mode==3;
        if(id=="v2_release")active=mode!=2;
        if(id=="v2_link")active=mode!=4;
        if(id=="v2_threshold")active=mode!=4;
        if(id=="v2_detectorMs"||id=="v2_rms")active=mode==1||mode==3;
        if(id=="v2_lookahead")active=mode!=4;
        if(id=="v2_sync"||id=="v2_beats"||id=="v2_ms"||id.startsWith("v2_step"))active=mode==4;
        if(!active&&c->dragging){c->dragging=false;processor.parameter(id)->endChangeGesture();}
        c->setEnabled(active);c->setAlpha(active?1.f:.35f);
    }
    all.setEnabled(mode==4);off.setEnabled(mode==4);alternate.setEnabled(mode==4);
    for(int n=0;n<4;++n)modes[static_cast<size_t>(n)].setToggleState(mode==n+1,juce::dontSendNotification);
    for(int n=0;n<2;++n)profiles[static_cast<size_t>(n)].setToggleState(processor.parameterValue("v2_profile")==n,juce::dontSendNotification);
    for(int n=0;n<3;++n)tabs[static_cast<size_t>(n)].setToggleState(tab==n,juce::dontSendNotification);
    const bool on=processor.isModuleOn()&&processor.parameterValue("v2_enabled")>.5;power.setToggleState(on,juce::dontSendNotification);power.setButtonText(on?"PROCESS ON":"PROCESS OFF");listen.setToggleState(processor.parameterValue("v2_listen")>.5,juce::dontSendNotification);
    help.setText(context,juce::dontSendNotification);canvas.repaint();
}
void ModernZastavaPanel::arrange()
{
    for(auto& c:controls)c->setVisible(false);
    const auto place=[this](const juce::String& id,juce::Rectangle<int> r){for(auto& c:controls)if(c->spec.id==id){c->setBounds(r);c->setVisible(true);break;}};
    for(int n=0;n<4;++n)modes[static_cast<size_t>(n)].setBounds(56+n*177,165,165,40);
    for(int n=0;n<2;++n)profiles[static_cast<size_t>(n)].setBounds(800+n*168,165,156,40);
    init.setBounds(814,80,106,42);power.setBounds(936,80,188,42);listen.setBounds(944,222,180,32);help.setBounds(56,728,1068,42);
    const bool advanced=shownView==1,pattern=(!advanced&&shownMode==4)||(advanced&&shownTab==2);
    for(int n=0;n<3;++n){tabs[static_cast<size_t>(n)].setVisible(advanced);tabs[static_cast<size_t>(n)].setBounds(56+n*356,265,344,30);}
    if(!advanced||pattern)
    {
        std::vector<juce::String> ids=shownMode==2?std::vector<juce::String>{"v2_threshold","v2_range","v2_attack","v2_body","v2_tail","v2_mix"}:std::vector<juce::String>{"v2_threshold","v2_range","v2_attack","v2_hold","v2_release","v2_mix"};
        if(pattern)ids={"v2_steps",shownSync?"v2_beats":"v2_ms","v2_attack","v2_release","v2_range","v2_mix"};
        for(int n=0;n<6;++n)place(ids[static_cast<size_t>(n)],{56+n*178,advanced?306:286,166,178});
        if(pattern)
        {
            place("v2_sync",{56,advanced?488:475,230,60});
            const int top=advanced?552:540,h=shownSteps>16?78:156;
            for(int n=0;n<32;++n)if(n<shownSteps)place("v2_step"+juce::String(n+1),{56+(n%16)*67,top+(n/16)*81,62,h});
        }
        else
        {
            place("v2_external",{800,498,155,65});place("v2_expand",{969,498,155,65});place("v2_output",{938,574,186,139});
        }
    }
    else if(shownTab==0)
    {
        const char* ids[]{"v2_threshold","v2_range","v2_attack","v2_hold","v2_release","v2_ratio","v2_knee","v2_hysteresis","v2_body","v2_tail","v2_onset","v2_retrigger"};
        for(int n=0;n<12;++n)place(ids[n],{56+(n%6)*178,306+(n/6)*181,166,169});
    }
    else
    {
        const char* ids[]{"v2_detectorMs","v2_lookahead","v2_hp","v2_lp","v2_link","v2_mix","v2_output"};
        for(int n=0;n<7;++n)place(ids[n],{56+(n%4)*267,306+(n/4)*195,252,181});
        place("v2_external",{857,503,267,61});place("v2_rms",{857,574,267,61});place("v2_expand",{857,645,267,61});
    }
    all.setVisible(pattern);off.setVisible(pattern);alternate.setVisible(pattern);all.setBounds(600,advanced?499:486,150,32);off.setBounds(762,advanced?499:486,150,32);alternate.setBounds(924,advanced?499:486,200,32);
}
void ModernZastavaPanel::paint(juce::Graphics& g){g.fillAll(juce::Colour(0xff090c0b));}
void ModernZastavaPanel::resized()
{const float s=std::min(getWidth()/1180.f,getHeight()/780.f);canvas.setTransform(juce::AffineTransform::scale(s).translated((getWidth()-1180*s)*.5f,(getHeight()-780*s)*.5f));}
juce::Rectangle<float> ModernZastavaPanel::contentBounds() const{return canvas.getBounds().toFloat().transformedBy(canvas.getTransform());}
juce::String ModernZastavaPanel::helpKeyAt(juce::Point<float> p) const
{const auto d=p.transformedBy(canvas.getTransform().inverted());for(const auto& c:controls)if(c->isVisible()&&c->getBounds().toFloat().contains(d))return c->spec.id;return {};}
bool ModernZastavaPanel::layoutValid() const
{
    const juce::Rectangle<int> stage(0,0,1180,780),preset(56,20,336,116);
    for(size_t i=0;i<controls.size();++i)if(controls[i]->isVisible())
    {
        const auto b=controls[i]->getBounds();if(!stage.contains(b)||preset.intersects(b))return false;
        for(size_t j=i+1;j<controls.size();++j)if(controls[j]->isVisible()&&b.intersects(controls[j]->getBounds()))return false;
    }
    return true;
}
void ModernZastavaPanel::paintStage(juce::Graphics& g)
{
    if(background.isNull())
    {
        background=juce::Image(juce::Image::RGB,1180,780,true);juce::Graphics b(background);steel::panel(b,1180,780,1946);
        steel::nameplate(b,{420,38,380,90},"HamloZastava","MODEL KPP-2","2026");
    }
    g.drawImageAt(background,0,0);const auto reading=processor.modernReading();const bool external=processor.parameterValue("v2_external")>.5;
    const bool missing=external&&(reading.missingKey||processor.getChannelCountOfBus(true,1)==0),monitor=processor.parameterValue("v2_listen")>.5;
    const bool on=processor.isModuleOn()&&processor.parameterValue("v2_enabled")>.5;
    const bool stopped=shownMode==4&&processor.parameterValue("v2_sync")>.5&&!processor.isHostPlaying();
    juce::String status=missing?"EXTERNAL KEY MISSING - DRY":!processor.isModuleOn()?"HOST BYPASS":monitor?"LISTENING TO FILTERED KEY":!on?"NEUTRAL - PROCESSING OFF":stopped?"TRANSPORT STOPPED - DRY":juce::String(external?"EXTERNAL KEY":"MAIN KEY")+"  /  "+(reading.gain<.99?"ATTENUATING":"OPEN");
    if(processor.isProgramModified())status+="  |  MODIFIED";
    steel::engraved(g,status,{56,224,870,28},14,missing||monitor?juce::Colour(0xffffa273):steel::phosphor,juce::Justification::centredLeft,0);
    const bool advanced=shownView==1,pattern=(!advanced&&shownMode==4)||(advanced&&shownTab==2);
    if(!advanced&&!pattern)
    {
        const juce::Rectangle<float> area(56,500,700,164);steel::lcdWindow(g,area);g.setColour(steel::phosphorDim);g.drawLine(80,638,730,638,1);
        const double attack=processor.parameterValue("v2_attack"),hold=processor.parameterValue(shownMode==2?"v2_body":"v2_hold"),release=processor.parameterValue(shownMode==2?"v2_tail":"v2_release"),sum=std::max(1.,attack+hold+release);
        const float attackX=80+static_cast<float>(644*std::clamp(attack/sum,.08,.4)),holdX=attackX+static_cast<float>(644*std::clamp(hold/sum,.08,.4));
        const bool duck=shownMode==3;juce::Path path;path.startNewSubPath(80,duck?532.f:632.f);path.lineTo(attackX,duck?632.f:532.f);path.lineTo(holdX,duck?632.f:532.f);
        for(int n=1;n<=64;++n){const double u=n/64.;const double shape=processor.parameterValue("v2_profile")<.5?u*u*u*(10+u*(-15+6*u)):u;path.lineTo(holdX+static_cast<float>((724-holdX)*u),static_cast<float>(duck?632-100*shape:532+100*shape));}
        steel::glow(g,path,steel::phosphor,2,1);steel::crtText(g,"ENVELOPE GUIDE - "+juce::String(shownMode==2?"ATTACK / BODY / TAIL":"ATTACK / HOLD / RELEASE"),{72,504,660,23},13,juce::Justification::centredLeft);
        steel::crtText(g,shownMode==3?"DUCK: ATTACK ENGAGES / RELEASE RECOVERS":"Use ADVANCED for detector, filters and precise shaping.",{72,667,770,24},12,juce::Justification::centredLeft);
    }
    if(!pattern)
    {
        const float y=702;const double gain=std::max(1.e-6,reading.gain),rawReduction=std::clamp(-20*std::log10(gain),0.,90.),reduction=rawReduction<.0005?0:rawReduction;
        g.setColour(juce::Colour(0xff0b1a12));g.fillRoundedRectangle(56,y,700,17,3);g.setColour(steel::phosphorDim);g.fillRoundedRectangle(56,y,static_cast<float>(700*reduction/90),17,3);
        steel::crtText(g,"KEY "+juce::String(reading.keyDb,1)+" dB   |   TARGET GR "+juce::String(reduction,1)+" dB   |   5 ms PDC",{65,y-1,680,20},12,juce::Justification::centredLeft);
    }
    else steel::crtText(g,juce::String(shownSteps)+" STEPS  |  "+(shownSync?juce::String(processor.getHostBpm(),1)+" BPM":"FREE CLOCK"),{310,advanced?503.f:490.f,285,24},12,juce::Justification::centredLeft);
}
}

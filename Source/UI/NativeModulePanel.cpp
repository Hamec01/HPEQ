#include "UI/NativeModulePanel.h"
#include "UI/Theme.h"
#include "UI/BlackSteel.h"
#include "Core/Revolution.h"

namespace hpeq::ui
{
namespace
{
juce::String blockName(int action,bool shortName)
{
    const juce::StringArray names{"Off","Repeat","Roll","Rise","Chop","Drop","Shuffle","Reverse","Stop","Gap"};
    const juce::StringArray shorts{"-","RP","RL","RI","CH","DR","SH","RV","ST","GP"};
    return (shortName?shorts:names)[juce::jlimit(0,9,action)];
}
}
struct NativeModulePanel::Control final : juce::Component
{
    native::Parameter spec;
    juce::Label label;
    juce::Slider slider{juce::Slider::RotaryHorizontalVerticalDrag,juce::Slider::TextBoxBelow};
    juce::ComboBox choice;
    explicit Control(const native::Parameter& s):spec(s)
    {
        setComponentID(s.id);label.setText(s.name,juce::dontSendNotification);
        label.setJustificationType(juce::Justification::centred);addAndMakeVisible(label);
        if(s.choices.size()>0)
        {
            choice.addItemList(s.choices,1);choice.setComponentID(s.id);addAndMakeVisible(choice);
        }
        else
        {
            slider.setComponentID(s.id);slider.setRange(s.range.start,s.range.end,s.range.interval);
            slider.setSkewFactor(s.range.skew,s.range.symmetricSkew);
            slider.setNumDecimalPlacesToDisplay(s.range.interval>=1?0:3);
            slider.setTextBoxStyle(juce::Slider::TextBoxBelow,false,110,22);
            slider.setTextValueSuffix(s.unit.isEmpty()?juce::String():" "+s.unit);
            slider.setDoubleClickReturnValue(true,s.initial);addAndMakeVisible(slider);
            slider.setVelocityModeParameters(.5,1,0,true,juce::ModifierKeys::ctrlModifier);
            slider.setTooltip(s.name+". Double-click: default. Ctrl-drag: fine adjustment.");
        }
    }
    void resized() override
    {
        auto r=getLocalBounds().reduced(5);label.setBounds(r.removeFromTop(30));
        if(spec.choices.size()>0)choice.setBounds(r.withSizeKeepingCentre(r.getWidth(),28));
        else slider.setBounds(r);
    }
    void paint(juce::Graphics& g) override
    {
        g.setColour(colours::panelRaised.withAlpha(.7f));g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(2),5);
        g.setColour(colours::outline);g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(2),5,1);
    }
};
bool NativeModulePanel::owned(const juce::String& id) const
{
    return id=="mix"||id=="bypass"||id=="v2Mix"||id=="v2_mix"
        ||(type==FxType::MonstaFx&&id=="scene")
        ||(type==FxType::Reverb&&(id=="routing"||id=="keepDry"))
        ||(type==FxType::Delay&&(id=="echoMode"||id=="scoreRouting"||id=="scoreMix"||id=="quickEnabled"));
}
NativeModulePanel::NativeModulePanel(FxType t):type(t)
{
    setName(native::productName(type));setComponentID("nativeModulePanel");
    heading.setText(native::productName(type).toUpperCase(),juce::dontSendNotification);
    heading.setFont(steel::stencilFont(25));addAndMakeVisible(heading);
    status.setColour(juce::Label::textColourId,colours::phosphor);addAndMakeVisible(status);
    search.setTextToShowWhenEmpty("Find a control...",colours::dim);addAndMakeVisible(search);
    search.onTextChange=[this]{layoutControls();};
    section.addItemList({"All controls","Tone / dynamics","Timing","Steps / motion"},1);
    section.setSelectedId(1);section.onChange=[this]{layoutControls();};addAndMakeVisible(section);
    viewport.setViewedComponent(&content,false);viewport.setScrollBarsShown(true,false);addAndMakeVisible(viewport);
    for(const auto& spec:native::layout(type))
    {
        if(owned(spec.id))continue;
        auto* c=parameters.add(new Control(spec));content.addAndMakeVisible(c);
        if(type==FxType::Reverb&&spec.id=="engine")c->choice.setItemEnabled(1,false);
        c->slider.onDragStart=[this]{if(onGesture)onGesture();};
        c->slider.onValueChange=[this,c]{if(!refreshing)parameterChanged(c->spec.id,c->slider.getValue());};
        c->choice.onChange=[this,c]{if(!refreshing){if(onGesture)onGesture();parameterChanged(c->spec.id,c->choice.getSelectedItemIndex());}};
    }
    if(type==FxType::MonstaFx)
    {
        for(const auto& p:native::layout(type))if(p.id=="scene")sceneSelect.addItemList(p.choices,1);
        sceneSelect.onChange=[this]{if(!refreshing){if(onGesture)onGesture();parameterChanged("scene",sceneSelect.getSelectedItemIndex());}};
        addAndMakeVisible(sceneSelect);
        for(int i=0;i<16;++i)
        {auto* label=stepLabels.add(new juce::Label());label->setText(juce::String(i+1),juce::dontSendNotification);label->setJustificationType(juce::Justification::centred);content.addAndMakeVisible(label);}
        for(const auto* text:{"TIME","GLITCH","MOTION","SHAPE A","SHAPE B","THROW A","THROW B"})
        {
            auto* label=laneLabels.add(new juce::Label());label->setText(text,juce::dontSendNotification);
            label->setFont(steel::stencilFont(12));content.addAndMakeVisible(label);
        }
        for(int lane=0;lane<7;++lane)for(int step=0;step<16;++step)
        {
            auto* b=steps.add(new juce::TextButton("-"));content.addAndMakeVisible(b);
            b->setComponentID("scene_"+juce::String(lane)+"_"+juce::String(step));
            b->onClick=[this,lane,step]{editStep(lane,step);};
            b->setTooltip("Choose this step's action. All seven lanes are saved with the project.");
        }
        content.addAndMakeVisible(clearScene);
        clearScene.onClick=[this] {
            if(!readSlot||!onChange)return;
            auto slot=readSlot();native::Scene empty;juce::String error;
            for(auto* id:{"timeOn","glitchOn","motionOn","shapeAOn","shapeBOn","throwAOn","throwBOn"})slot.parameters.set(id,0);
            slot.nativeState=native::editRevolutionScene(slot.nativeState,slot.parameters,empty,error);
            if(error.isEmpty()){if(onGesture)onGesture();onChange(slot);}else status.setText(error,juce::dontSendNotification);
        };
    }
    if(type==FxType::Gain)
        for(int i=0;i<4;++i)
        {
            actions[static_cast<size_t>(i)].setButtonText(juce::StringArray{"Learn","Stop","Reset","Apply Match"}[i]);
            content.addAndMakeVisible(actions[static_cast<size_t>(i)]);
            actions[static_cast<size_t>(i)].onClick=[this,i] {
                if(!faderAction||!faderAction(i))
                {actionMessage="Action unavailable: play audio and complete Learn first.";messageTicks=36;status.setText(actionMessage,juce::dontSendNotification);}
            };
        }
    startTimerHz(12);
}
NativeModulePanel::~NativeModulePanel(){stopTimer();viewport.setViewedComponent(nullptr,false);}
void NativeModulePanel::parameterChanged(const juce::String& id,double v)
{
    if(!readSlot||!onChange)return;auto slot=readSlot();
    if(slot.engineRevision!=native::kRevision||slot.type!=type)return;
    for(const auto& spec:native::layout(type))if(spec.id==id)v=spec.range.snapToLegalValue(static_cast<float>(v));
    slot.parameters.set(id,v);onChange(slot);
}
void NativeModulePanel::refresh()
{
    if(!readSlot)return;const auto slot=readSlot();
    if(slot.type!=type||slot.engineRevision!=native::kRevision)return;
    refreshing=true;
    bool nextModern=true;int nextMode=0;
    if(type==FxType::Gain){nextModern=static_cast<int>(slot.parameters["engine"])>0;nextMode=slot.parameters["mode"];}
    if(type==FxType::GateExpander)nextModern=static_cast<int>(slot.parameters["v2_engine"])>0;
    if(type==FxType::StereoProcessor)nextModern=static_cast<int>(slot.parameters["v2Mode"])>0;
    if(nextModern!=modern||nextMode!=currentMode){modern=nextModern;currentMode=nextMode;layoutControls();}
    for(auto* c:parameters)
    {
        const double value=slot.parameters.getWithDefault(c->spec.id,c->spec.initial);
        if(c->spec.choices.size()>0)c->choice.setSelectedItemIndex(juce::roundToInt(value),juce::dontSendNotification);
        else if(!c->slider.isMouseButtonDown())c->slider.setValue(value,juce::dontSendNotification);
    }
    const int sceneIndex=static_cast<int>(slot.parameters.getWithDefault("scene",0));
    if(type==FxType::MonstaFx)sceneSelect.setSelectedItemIndex(sceneIndex,juce::dontSendNotification);
    const bool nextBlock=static_cast<int>(slot.parameters.getWithDefault("glitchEngine",1))!=0;
    if(type==FxType::MonstaFx && (slot.nativeState!=lastScene||sceneIndex!=lastSceneIndex||nextBlock!=blockGlitch))
    {
        blockGlitch=nextBlock;
        juce::String error;scene={};
        if(slot.nativeState.getSize()>0&&!native::readRevolutionSceneAt(slot.nativeState,sceneIndex,scene,error))status.setText(error,juce::dontSendNotification);
        lastScene=slot.nativeState;
        lastSceneIndex=sceneIndex;
        for(int lane=0;lane<7;++lane)for(int step=0;step<16;++step)
        {
            const int value=scene.lanes[static_cast<size_t>(lane)][static_cast<size_t>(step)];
            auto* b=steps[lane*16+step];
            b->setButtonText(value==0?juce::String("-"):lane==0?revolution::valueShortName(0,value)
                :lane==1?(blockGlitch?blockName(value,true):revolution::valueShortName(1,value)):juce::String(value));
            b->setColour(juce::TextButton::buttonColourId,value?colours::accent.withAlpha(.35f):colours::panelRaised);
        }
    }
    refreshing=false;
}
void NativeModulePanel::timerCallback()
{
    refresh();
    if(readError){const auto error=readError();if(error.isNotEmpty()){status.setText(error,juce::dontSendNotification);return;}}
    if(messageTicks>0){--messageTicks;status.setText(actionMessage,juce::dontSendNotification);return;}
    if(readMeters)
    {
        const auto m=readMeters();
        const bool dynamics=type==FxType::Compressor||type==FxType::Limiter;
        status.setText("IN "+juce::String(m.input,1)+" dBFS   OUT "+juce::String(m.output,1)+" dBFS"
            +(dynamics?"   GR "+juce::String(m.reduction,1)+" dB":juce::String())
            +"   LATENCY "+juce::String(m.latency)+" samples",juce::dontSendNotification);
    }
    if(type==FxType::Gain&&readFader)
    {
        const auto view=readFader();actions[3].setEnabled(view.canApply);
        status.setText(!view.available?"Play audio to measure. Learn never applies gain automatically."
            :juce::String(view.learning?"LEARNING   ":"IN  ")+juce::String(view.input,1)+" LUFS   OUT "+juce::String(view.output,1)
             +" LUFS   GAIN "+juce::String(view.correction,1)+" dB   PROPOSAL "+juce::String(view.proposal,1)+" dB",juce::dontSendNotification);
    }
}
void NativeModulePanel::editStep(int lane,int step)
{
    juce::PopupMenu menu;const int maximum=lane==0?5:lane==1?9:15;
    for(int value=0;value<=maximum;++value)
        menu.addItem(value+1,value==0?juce::String("Off"):lane==0?revolution::valueName(0,value)
            :lane==1?(blockGlitch?blockName(value,false):revolution::valueName(1,value)):"Level "+juce::String(value),true,scene.lanes[static_cast<size_t>(lane)][static_cast<size_t>(step)]==value);
    juce::Component::SafePointer<NativeModulePanel> safe(this);
    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(steps[lane*16+step]),[safe,lane,step](int result) {
        if(!safe||result==0||!safe->readSlot||!safe->onChange)return;
        auto slot=safe->readSlot();auto pattern=safe->scene;
        pattern.lanes[static_cast<size_t>(lane)][static_cast<size_t>(step)]=static_cast<std::uint8_t>(result-1);
        const char* ids[]{"timeOn","glitchOn","motionOn","shapeAOn","shapeBOn","throwAOn","throwBOn"};
        slot.parameters.set(ids[lane],std::any_of(pattern.lanes[static_cast<size_t>(lane)].begin(),pattern.lanes[static_cast<size_t>(lane)].end(),[](auto v){return v!=0;})?1:0);
        juce::String error;slot.nativeState=native::editRevolutionScene(slot.nativeState,slot.parameters,pattern,error);
        if(error.isEmpty()){if(safe->onGesture)safe->onGesture();safe->onChange(slot);}else safe->status.setText(error,juce::dontSendNotification);
    });
}
juce::Component* NativeModulePanel::control(const juce::String& id) const
{
    for(auto* c:parameters)if(c->spec.id==id)return c->spec.choices.size()>0?static_cast<juce::Component*>(&c->choice):&c->slider;
    if(type==FxType::MonstaFx&&id=="scene")return const_cast<juce::ComboBox*>(&sceneSelect);
    for(auto* b:steps)if(b->getComponentID()==id)return b;
    return nullptr;
}
void NativeModulePanel::layoutControls()
{
    const int width=juce::jmax(320,viewport.getWidth()-viewport.getScrollBarThickness()-2);
    const int columns=juce::jmax(2,width/145),cell=width/columns;
    const bool match=type==FxType::Gain&&modern&&currentMode==1;
    int y=type==FxType::MonstaFx?304:match?44:0,index=0;
    if(type==FxType::MonstaFx)
    {
        const int stepWidth=(width-100)/16;
        for(int step=0;step<16;++step)stepLabels[step]->setBounds(100+step*stepWidth,0,stepWidth-2,20);
        for(int lane=0;lane<7;++lane)laneLabels[lane]->setBounds(4,lane*34+28,92,29);
        for(int lane=0;lane<7;++lane)for(int step=0;step<16;++step)steps[lane*16+step]->setBounds(100+step*stepWidth,lane*34+28,stepWidth-2,29);
        clearScene.setBounds(100,270,130,28);
    }
    if(type==FxType::Gain)for(int i=0;i<4;++i){actions[static_cast<size_t>(i)].setBounds(8+i*130,5,122,29);actions[static_cast<size_t>(i)].setVisible(match);}
    const auto text=search.getText().trim();const int group=section.getSelectedId();
    for(auto* c:parameters)
    {
        const auto key=c->spec.id.toLowerCase();
        bool family=true;
        if(type==FxType::GateExpander)family=key=="v2_engine"||(modern==key.startsWith("v2_"));
        if(type==FxType::StereoProcessor)family=key=="v2mode"||(modern==key.startsWith("v2"));
        if(type==FxType::Delay)family=key.startsWith("score")||key.startsWith("tap")||key.startsWith("band");
        if(type==FxType::Gain)
        {
            const juce::StringArray base{"engine","gaindb","polarity","mute","solo"};
            const juce::StringArray common{"mode","enabled","inputdb","outputdb","leftdb","rightdb","leftdelayms","rightdelayms","routing","swap"};
            const juce::StringArray matching{"targetdb","measure","matchboost","matchcut","matchceiling","matcheddb","peakguard"};
            const juce::StringArray riding{"targetdb","measure","profile","strength","maxboost","maxcut","floordb","holdms","idlems","risems","fallms"};
            const juce::StringArray motion{"shape","points","smooth","sync","depthdb","beats","freems","phase","rightphase","edgems"};
            family=base.contains(key)||(!modern&&(key=="delayms"||key=="targetdb"))
                ||(modern&&(common.contains(key)||(currentMode==1&&matching.contains(key))
                    ||(currentMode==2&&riding.contains(key))||(currentMode==3&&(motion.contains(key)||key.startsWith("level")))));
        }
        const bool stepsFamily=key.contains("step")||key.contains("node")||key.startsWith("tap")||key.startsWith("level");
        const bool timing=key.contains("time")||key.contains("attack")||key.contains("release")||key.contains("sync")||key.contains("phase")||key.contains("beat")||key.contains("delay")||key.contains("speed");
        const bool selector=key=="engine"||key=="mode"||key=="model"||key=="v2mode"||key=="v2_engine";
        const bool visible=family&&(text.isEmpty()||c->spec.name.containsIgnoreCase(text)||key.containsIgnoreCase(text))
            &&(selector||group==1||(group==2&&!timing&&!stepsFamily)||(group==3&&timing)||(group==4&&stepsFamily));
        c->setVisible(visible);if(!visible)continue;
        c->setBounds((index%columns)*cell,y+(index/columns)*132,cell,128);++index;
    }
    sceneBottom=y;content.setSize(width,y+((index+columns-1)/columns)*132+12);content.repaint();
}
void NativeModulePanel::resized()
{
    auto r=getLocalBounds().reduced(14);heading.setBounds(r.removeFromTop(35));status.setBounds(r.removeFromTop(26));
    auto filters=r.removeFromTop(30);
    if(type==FxType::MonstaFx){sceneSelect.setBounds(filters.removeFromRight(150));filters.removeFromRight(8);}
    section.setBounds(filters.removeFromRight(190));filters.removeFromRight(10);search.setBounds(filters);
    r.removeFromTop(10);viewport.setBounds(r);layoutControls();
}
void NativeModulePanel::paint(juce::Graphics& g)
{
    g.fillAll(colours::background);g.setColour(colours::outline);g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(1),6,1);
}
}

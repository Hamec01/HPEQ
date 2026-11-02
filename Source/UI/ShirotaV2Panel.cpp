#include "UI/ShirotaV2Panel.h"
#include "Standalone/V2Parameters.h"
#include <cmath>
namespace hpeq::ui
{
namespace
{
const juce::Colour green{0xff80e6ac},dark{0xff101714};
juce::String information(const juce::String& id)
{
    if(id=="v2Width")return "Source Side gain before Space/Position. 0 = mono source, 1 = original, 4 = +12 dB Side. Added space can remain stereo.";
    if(id=="v2PositionOn")return "Enable the position stage. Stereo uses panning; Binaural uses measured headphone filters. Off preserves aligned Stereo/Space output.";
    if(id=="v2Mode")return "Legacy preserves old projects. Stereo/Space and Binaural use the new controls. All modes report the same fixed host delay.";
    if(id=="v2Guard"||id=="v2Floor")return "Optional stationary Mid/Side energy budget, applied before Space. The reduction reading shows its intervention; it is not a universal correlation guarantee.";
    if(id=="v2Generate")return "Create Side from a mono source. Phase, Micro and Diffuse have different structures. Width/bands/mono bass also govern generated Side.";
    if(id=="v2Profile")return "Three SADIE II horizontal profiles for headphone audition. Select by listening; no personalized headphone correction is applied.";
    if(id=="v2Rate")return "Multiplier for all path holds and travel. 1x uses the exact node times; 2x runs the path twice as fast.";
    if(id=="v2Motion")return "Follow the editable path when Position is enabled. Hold each node, then travel to the next. Restart resets to node 1.";
    if(id.contains("Hold"))return "Hold this node before travelling. Milliseconds and quarter-note beats are stored separately; Time Unit selects the active pair.";
    if(id.contains("Travel"))return "Travel to the next node. Set Arc and Curve explicitly. A directed arc to the same position makes a full circle.";
    if(id.contains("Angle")||id=="v2Position")return "Azimuth: 0 front, +90 left, +/-180 back, -90 right. Drag the selected path point on the circular map or type an exact value.";
    if(id=="v2Predelay"||id=="v2Size")return "Spatial reflection timing. Changes crossfade between static prepared heads instead of sweeping a moving delay read.";
    if(id=="v2Source")return "Stereo Spread retains separate left/right emitters. Point explicitly collapses the input to Mid before positioning.";
    if(id=="v2Spread")return "Angular separation of the left/right emitters. 180 degrees at center 0 gives identity in Stereo; smaller values concentrate the source.";
    if(id=="v2Balance")return "Attenuate one output channel without rotating the source. Negative favors left, positive favors right.";
    return "Drag to adjust, double-click to reset, or type the exact value. This control belongs to the modern engine; Legacy keeps its original controls.";
}
}
struct ShirotaV2Panel::Look final:juce::LookAndFeel_V4
{
    Look(){setColour(juce::Slider::textBoxTextColourId,green);setColour(juce::Slider::textBoxBackgroundColourId,dark);setColour(juce::Slider::textBoxOutlineColourId,juce::Colour(0xff665c40));setColour(juce::ComboBox::backgroundColourId,dark);setColour(juce::ComboBox::textColourId,green);setColour(juce::TextButton::buttonColourId,dark);setColour(juce::TextButton::buttonOnColourId,juce::Colour(0xff30533e));setColour(juce::TextButton::textColourOffId,steel::stencil);setColour(juce::TextButton::textColourOnId,green);}
    void drawRotarySlider(juce::Graphics& g,int x,int y,int w,int h,float value,float start,float end,juce::Slider&) override
    {
        const auto center=juce::Point<float>(x+w*.5f,y+h*.5f);const float s=std::min(w,h)/132.f;
        steel::knobScale(g,center,s);steel::brassKnob(g,center,(-.75f+1.5f*value)*juce::MathConstants<float>::pi,s);
    }
};
struct ShirotaV2Panel::Cell final:juce::Component
{
    ShirotaV2Panel& owner;juce::String id;juce::Label label;juce::Slider slider;juce::ComboBox choice;juce::RangedAudioParameter* parameter=nullptr;
    Cell(ShirotaV2Panel& o,const standalone::ParameterSpec& spec):owner(o),id(spec.id)
    {
        parameter=owner.processor.parameter(id);setComponentID(id);slider.setComponentID(id);choice.setComponentID(id);label.setText(spec.name,juce::dontSendNotification);label.setJustificationType(juce::Justification::centred);label.setFont(steel::monoFont(14));label.setColour(juce::Label::textColourId,steel::stencil);addAndMakeVisible(label);
        const auto tip=information(id);slider.setTooltip(tip);choice.setTooltip(tip);
        if(spec.choices.isEmpty())
        {
            slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);slider.setTextBoxStyle(juce::Slider::TextBoxBelow,false,132,25);slider.setRange(spec.minimum,spec.maximum,spec.interval);slider.setSkewFactor(spec.skew);slider.setDoubleClickReturnValue(true,spec.defaultValue);slider.setTextValueSuffix(spec.unit.isEmpty()?juce::String{}:" "+spec.unit);slider.setNumDecimalPlacesToDisplay(spec.interval>=1?0:(id.contains("Beat")?5:3));addAndMakeVisible(slider);
            slider.onDragStart=[this]{parameter->beginChangeGesture();};slider.onDragEnd=[this]{parameter->endChangeGesture();};slider.onValueChange=[this]{owner.processor.setParameterValue(id,slider.getValue());};
        }
        else {choice.addItemList(spec.choices,1);addAndMakeVisible(choice);choice.onChange=[this]{owner.set(id,choice.getSelectedId()-1);if(id=="v2Motion"&&choice.getSelectedId()==2)owner.set("v2PositionOn",1);};}
        addMouseListener(this,true);
    }
    void mouseEnter(const juce::MouseEvent&) override {owner.help.setText(information(id),juce::dontSendNotification);}
    void resized() override {label.setBounds(0,0,getWidth(),25);if(slider.isVisible())slider.setBounds(12,26,getWidth()-24,getHeight()-26);else choice.setBounds(10,50,getWidth()-20,32);}
    void refresh(){if(slider.isVisible())slider.setValue(owner.processor.parameterValue(id),juce::dontSendNotification);else choice.setSelectedId(1+juce::roundToInt(owner.processor.parameterValue(id)),juce::dontSendNotification);}
};
ShirotaV2Panel::ShirotaV2Panel(standalone::ModulePluginProcessor& p,std::unique_ptr<juce::Component> old):processor(p),look(std::make_unique<Look>()),legacy(std::move(old))
{
    setLookAndFeel(look.get());addAndMakeVisible(canvas);canvas.addAndMakeVisible(*legacy);canvas.addAndMakeVisible(engine);canvas.addAndMakeVisible(node);canvas.addAndMakeVisible(help);canvas.addAndMakeVisible(restart);
    canvas.addAndMakeVisible(power);power.onClick=[this]{set("bypass",processor.isModuleOn()?1:0);};power.setTooltip("Global bypass, with the same fixed host delay in every engine.");
    engine.addItemList({"LEGACY","STEREO / SPACE","BINAURAL / HEADPHONES"},1);engine.setTooltip(information("v2Mode"));engine.onChange=[this]{const int m=engine.getSelectedId()-1;set("v2Mode",m);if(m==0)set("v2Page",4);else{if(processor.parameterValue("v2Page")==4)set("v2Page",0);if(m==2)set("v2PositionOn",1);}};
    const juce::StringArray names{"QUICK","STEREO","SPACE","MOTION","LEGACY"};
    for(int i=0;i<5;++i){auto& b=tabs[static_cast<size_t>(i)];canvas.addAndMakeVisible(b);b.setButtonText(names[i]);b.onClick=[this,i]{set("v2Page",i);if(i==4)set("v2Mode",0);else if(processor.parameterValue("v2Mode")==0)set("v2Mode",1);};}
    for(int i=1;i<=16;++i)node.addItem("EDIT NODE "+juce::String(i),i);node.setSelectedId(1);node.onChange=[this]{selectedNode=node.getSelectedId()-1;set("v2EditNode",selectedNode+1);page=-1;refresh();};
    restart.onClick=[this]{set("v2Restart",1-processor.parameterValue("v2Restart"));};
    help.setColour(juce::Label::textColourId,green);help.setFont(steel::monoFont(13));help.setJustificationType(juce::Justification::centredLeft);help.setText("Choose Stereo / Space or Binaural. Type exact values; the preset screen stays at the top left.",juce::dontSendNotification);
    background=juce::Image(juce::Image::RGB,1180,900,true);{juce::Graphics g(background);steel::panel(g,1180,900,1957);}
    canvas.addMouseListener(this,true);refresh();
}
ShirotaV2Panel::~ShirotaV2Panel(){if(dragging)dragging->endChangeGesture();setLookAndFeel(nullptr);}
void ShirotaV2Panel::set(const juce::String& id,double value){if(auto* p=processor.parameter(id)){p->beginChangeGesture();processor.setParameterValue(id,value);p->endChangeGesture();}}
void ShirotaV2Panel::showPage(int next)
{
    page=juce::jlimit(0,4,next);cells.clear();legacy->setVisible(page==4);help.setVisible(page!=4);node.setVisible(page==3);restart.setVisible(page==3);
    for(int i=0;i<5;++i)tabs[static_cast<size_t>(i)].setToggleState(i==page,juce::dontSendNotification);
    juce::StringArray ids;
    if(page==0)ids={"v2Width","v2Space","v2Rate","v2SpaceType","v2PositionOn","v2Motion","v2Profile","v2Mix","v2Output"};
    if(page==1)ids={"v2Width","v2LowWidth","v2MidWidth","v2HighWidth","v2LowSplit","v2HighSplit","v2MonoBass","v2Generator","v2Generate","v2Guard","v2Floor","v2PositionOn","v2Position","v2Spread","v2Balance","v2Source","v2Profile","v2Mono","v2Mix","v2Output"};
    if(page==2)ids={"v2SpaceType","v2Space","v2Size","v2Depth","v2Damping","v2Predelay","v2Generator","v2Generate","v2Guard","v2Mono","v2Mix","v2Output"};
    if(page==3)
    {
        ids={"v2PositionOn","v2Motion","v2Unit","v2Loop","v2Rate","v2Nodes","v2Source","v2Profile"};
        const auto prefix="v2Node"+juce::String(selectedNode);for(auto suffix:{"Angle","Hold","Travel","HoldBeat","TravelBeat","Arc","Curve"})ids.add(prefix+suffix);
    }
    for(const auto& id:ids)for(const auto& spec:standalone::v2Parameters())if(spec.id==id){auto c=std::make_unique<Cell>(*this,spec);canvas.addAndMakeVisible(*c);cells.push_back(std::move(c));break;}
    layoutCells();repaint();
}
void ShirotaV2Panel::layoutCells()
{
    if(page==0)for(size_t i=0;i<cells.size();++i){if(i<3)cells[i]->setBounds(56+static_cast<int>(i)*356,230,356,300);else cells[i]->setBounds(56+static_cast<int>(i-3)*178,590,178,180);}
    if(page==1||page==2)for(size_t i=0;i<cells.size();++i)cells[i]->setBounds(56+static_cast<int>(i%6)*178,190+static_cast<int>(i/6)*158,178,153);
    if(page==3)for(size_t i=0;i<cells.size();++i){if(i<8)cells[i]->setBounds(56+static_cast<int>(i%4)*267,185+static_cast<int>(i/4)*118,267,113);else cells[i]->setBounds(56+static_cast<int>(i-8)*152,475,152,155);}
}
void ShirotaV2Panel::refresh()
{
    power.setToggleState(processor.isModuleOn(),juce::dontSendNotification);power.setButtonText(processor.isModuleOn()?"ON":"BYPASSED");
    const int editing=juce::jlimit(0,15,juce::roundToInt(processor.parameterValue("v2EditNode"))-1);if(editing!=selectedNode){selectedNode=editing;page=-1;}node.setSelectedId(selectedNode+1,juce::dontSendNotification);const int wanted=juce::roundToInt(processor.parameterValue("v2Page"));if(wanted!=page)showPage(wanted);
    engine.setSelectedId(1+juce::roundToInt(processor.parameterValue("v2Mode")),juce::dontSendNotification);for(auto& c:cells){c->refresh();const bool beats=processor.parameterValue("v2Unit")>.5;if(c->id.contains("Hold")||c->id.contains("Travel"))c->setEnabled(c->id.contains("Beat")==beats);if(c->id=="v2Profile")c->setEnabled(processor.parameterValue("v2Mode")>1.5);}repaint();
}
juce::Rectangle<float> ShirotaV2Panel::contentBounds() const{return {offset.x,offset.y,1180*scale,900*scale};}
juce::Point<float> ShirotaV2Panel::design(juce::Point<float> p) const{return (p-offset)/scale;}
juce::String ShirotaV2Panel::helpKeyAt(juce::Point<float> local) const{const auto p=design(local);for(const auto& c:cells)if(c->getBounds().toFloat().contains(p))return c->id;return {};}
void ShirotaV2Panel::resized()
{
    scale=std::min(getWidth()/1180.f,getHeight()/900.f);offset={(getWidth()-1180*scale)*.5f,(getHeight()-900*scale)*.5f};canvas.setSize(1180,900);canvas.setTransform(juce::AffineTransform::scale(scale).translated(offset));
    engine.setBounds(810,35,314,33);power.setBounds(810,75,110,27);for(int i=0;i<5;++i)tabs[static_cast<size_t>(i)].setBounds(425+i*140,115,134,30);
    legacy->setBounds(0,150,1180,740);node.setBounds(56,431,240,30);restart.setBounds(310,431,210,30);help.setBounds(56,850,1068,42);layoutCells();
}
void ShirotaV2Panel::paint(juce::Graphics& graphics)
{
    auto& g=graphics;juce::Graphics::ScopedSaveState state(g);g.addTransform(juce::AffineTransform::scale(scale).translated(offset));g.drawImageAt(background,0,0);
    steel::nameplate(g,{425,20,350,82},"HamloShirota","MODEL SHR-3","No. 2026");steel::engraved(g,"ENGINE",{810,10,314,22},14);steel::engraved(g,processor.presetModified()?"MODIFIED":"LOADED",{932,77,192,20},13,processor.presetModified()?juce::Colour(0xffe5be62):steel::stencilDim);
    const auto reading=processor.modernReading();
    if(page!=4){steel::lcdWindow(g,{56,158,1068,25});steel::lcdText(g,{62,158,1056,25},"CORRELATION "+juce::String(reading.correlation,2)+"     GUARD "+juce::String(reading.guardDb,1)+" dB     POSITION "+juce::String(reading.azimuth,1)+" deg",14);}
    if(page==3)
    {
        const juce::Point<float> center{970,738};constexpr float radius=78;g.setColour(steel::stencilDim);g.drawEllipse(center.x-radius,center.y-radius,radius*2,radius*2,1);steel::engraved(g,"FRONT",{925,632,90,20},13);steel::engraved(g,"BACK",{925,820,90,20},13);steel::engraved(g,"L",{850,728,25,20},14);steel::engraved(g,"R",{1065,728,25,20},14);
        const int count=juce::roundToInt(processor.parameterValue("v2Nodes"));juce::Path route;
        auto point=[&](double degrees){const double angle=degrees*juce::MathConstants<double>::pi/180;return juce::Point<float>(center.x-radius*static_cast<float>(std::sin(angle)),center.y-radius*static_cast<float>(std::cos(angle)));};
        for(int i=0;i<count;++i)
        {
            const auto prefix="v2Node"+juce::String(i);const double angle=processor.parameterValue(prefix+"Angle");const auto at=point(angle);
            g.setColour(i==selectedNode?green:steel::stencil);g.fillEllipse(at.x-6,at.y-6,12,12);g.drawText(juce::String(i+1),juce::Rectangle<float>(center.x+(at.x-center.x)*.75f-15,center.y+(at.y-center.y)*.75f-9,30,18),juce::Justification::centred);
            if(i==count-1&&processor.parameterValue("v2Loop")<.5)continue;
            const double next=processor.parameterValue("v2Node"+juce::String((i+1)%count)+"Angle");double delta=std::fmod(next-angle+720.,360.);const int arc=juce::roundToInt(processor.parameterValue(prefix+"Arc"));
            if(arc==1)delta=delta==0?-360:delta-360;else if(arc==2)delta=delta==0?360:delta;else if(delta>180)delta-=360;
            route.startNewSubPath(at);for(int k=1;k<=32;++k)route.lineTo(point(angle+delta*k/32));
        }
        g.setColour(green.withAlpha(.45f));g.strokePath(route,juce::PathStrokeType(2));const auto now=point(reading.azimuth);g.setColour(juce::Colour(0xffdfad5a));g.drawEllipse(now.x-9,now.y-9,18,18,2);
        steel::engraved(g,"HOLD -> TRAVEL -> NEXT NODE",{56,667,720,30},19);steel::engraved(g,"0 FRONT  /  +90 LEFT  /  -90 RIGHT  /  180 BACK",{56,711,720,24},14);steel::engraved(g,"Drag the selected point; type exact times above.",{56,755,720,24},14);
    }
}
void ShirotaV2Panel::setAngle(juce::Point<float> p){const double angle=std::atan2(970-p.x,738-p.y)*180/juce::MathConstants<double>::pi;processor.setParameterValue("v2Node"+juce::String(selectedNode)+"Angle",angle);}
void ShirotaV2Panel::mouseDown(const juce::MouseEvent& e){const auto p=design(e.getEventRelativeTo(this).position);if(page==3&&juce::Rectangle<float>(835,635,280,205).contains(p)){dragging=processor.parameter("v2Node"+juce::String(selectedNode)+"Angle");dragging->beginChangeGesture();setAngle(p);}}
void ShirotaV2Panel::mouseDrag(const juce::MouseEvent& e){if(dragging)setAngle(design(e.getEventRelativeTo(this).position));}
void ShirotaV2Panel::mouseUp(const juce::MouseEvent&){if(dragging){dragging->endChangeGesture();dragging=nullptr;}}
}

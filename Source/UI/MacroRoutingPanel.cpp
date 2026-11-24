#include "UI/MacroRoutingPanel.h"
#include "Plugin/FxParameterContract.h"
#include "UI/Theme.h"
#include <cmath>
#include <cstdlib>
namespace hpeq::ui {
struct MacroRoutingPanel::Row:juce::Component {
    juce::Label name,target;
    juce::Slider value{juce::Slider::LinearHorizontal,juce::Slider::TextBoxRight};
    juce::TextButton add{"Assign"},range{"Range"},clear{"Clear"};
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    Row(FxPluginProcessor& p,int i){
        name.setText("MACRO "+juce::String(i+1).paddedLeft('0',2),juce::dontSendNotification);
        value.setComponentID(params::macroId(i));value.setTextBoxStyle(juce::Slider::TextBoxRight,false,58,22);
        value.setNumDecimalPlacesToDisplay(3);value.setDoubleClickReturnValue(true,0);
        add.setComponentID("assign"+juce::String(i+1));range.setComponentID("range"+juce::String(i+1));
        for(auto* c:std::initializer_list<juce::Component*>{&name,&target,&value,&add,&range,&clear})addAndMakeVisible(c);
        attachment=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.getParameters(),params::macroId(i),value);
        value.textFromValueFunction=[](double n){return juce::String(n,3);};value.updateText();
        value.setTooltip("Host automation, 0..1 across the assigned parameter ranges.");
        clear.setTooltip("Remove all assignments from this macro.");add.setTooltip("Add a module parameter to this macro.");range.setTooltip("Edit or remove a destination.");
    }
    void resized() override {
        auto r=getLocalBounds().reduced(6);auto top=r.removeFromTop(27);
        name.setBounds(top.removeFromLeft(86));clear.setBounds(top.removeFromRight(44));
        range.setBounds(top.removeFromRight(48));add.setBounds(top.removeFromRight(54));value.setBounds(top);
        target.setBounds(r);target.setMinimumHorizontalScale(.7f);
    }
    void paint(juce::Graphics& g)override{g.setColour(colours::panelRaised);g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(2),4);}
};
MacroRoutingPanel::MacroRoutingPanel(FxPluginProcessor& p):processor(p){
    setComponentID("macroRoutingPanel");heading.setText("HOST MACROS",juce::dontSendNotification);
    heading.setFont(juce::Font(juce::FontOptions(22.f)));hint.setText("Assign module controls. Multiple targets can share a macro; assignments travel with their module.",juce::dontSendNotification);
    for(auto* c:std::initializer_list<juce::Component*>{&heading,&hint,&close})addAndMakeVisible(c);
    close.onClick=[this]{if(onClose)onClose();};
    for(int i=0;i<control::kMacros;++i){auto& r=rows[static_cast<size_t>(i)];r=std::make_unique<Row>(p,i);addAndMakeVisible(*r);
        r->add.onClick=[this,i]{assign(i);};r->range.onClick=[this,i]{ranges(i);};r->clear.onClick=[this,i]{processor.clearMacro(i);timerCallback();};}
    timerCallback();startTimerHz(5);
}
MacroRoutingPanel::~MacroRoutingPanel()=default;
void MacroRoutingPanel::paint(juce::Graphics& g){g.fillAll(colours::background);}
void MacroRoutingPanel::resized(){auto r=getLocalBounds().reduced(14);auto header=r.removeFromTop(34);close.setBounds(header.removeFromRight(76));heading.setBounds(header);
    hint.setBounds(r.removeFromTop(30));r.removeFromTop(6);const int width=r.getWidth()/2,height=r.getHeight()/8;
    for(int i=0;i<control::kMacros;++i)rows[static_cast<size_t>(i)]->setBounds(r.getX()+(i/8)*width,r.getY()+(i%8)*height,width,height);
}
void MacroRoutingPanel::timerCallback(){for(int i=0;i<control::kMacros;++i){auto targets=processor.macroDestinations(i);auto& row=*rows[static_cast<size_t>(i)];
    row.target.setText(targets.empty()?"Unassigned":targets.front().label+(targets.size()>1?"  +"+juce::String(int(targets.size()-1)):juce::String()),juce::dontSendNotification);
    juce::StringArray names;for(const auto& t:targets)names.add(t.label);row.target.setTooltip(names.joinIntoString("\n"));row.range.setEnabled(!targets.empty());row.clear.setEnabled(!targets.empty());}}
void MacroRoutingPanel::assign(int macro){
    auto targets=processor.macroDestinations();juce::PopupMenu menu;
    int begin=0;while(begin<int(targets.size())){int end=begin+1;const auto group=targets[static_cast<size_t>(begin)].label.upToLastOccurrenceOf(" / ",false,false);
        while(end<int(targets.size())&&targets[static_cast<size_t>(end)].label.upToLastOccurrenceOf(" / ",false,false)==group)++end;
        juce::PopupMenu parameters;for(int i=begin;i<end;++i)parameters.addItem(i+1,targets[static_cast<size_t>(i)].label.fromLastOccurrenceOf(" / ",false,false));menu.addSubMenu(group,parameters);begin=end;
    }
    if(targets.empty())menu.addItem(1,"Add a module first",false);
    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&rows[static_cast<size_t>(macro)]->add),
        [safe=juce::Component::SafePointer<MacroRoutingPanel>(this),macro,targets](int id){if(safe&&id>0&&id<=int(targets.size())){if(!safe->processor.assignMacro(macro,targets[static_cast<size_t>(id-1)]))safe->hint.setText("The target changed or its routing limit was reached. Reopen Assign.",juce::dontSendNotification);safe->timerCallback();}});
}
void MacroRoutingPanel::ranges(int macro){auto targets=processor.macroDestinations(macro);if(targets.size()==1){editRange(macro,targets[0]);return;}
    juce::PopupMenu menu;for(int i=0;i<int(targets.size());++i)menu.addItem(i+1,targets[static_cast<size_t>(i)].label);
    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&rows[static_cast<size_t>(macro)]->range),
        [safe=juce::Component::SafePointer<MacroRoutingPanel>(this),macro,targets](int id){if(safe&&id>0&&id<=int(targets.size()))safe->editRange(macro,targets[static_cast<size_t>(id-1)]);});
}
void MacroRoutingPanel::editRange(int macro,FxPluginProcessor::MacroDestination target){
    auto project=processor.getProject();const FxSlotState* slot=nullptr;const auto& layer=project.layers[static_cast<size_t>(target.layer)];
    if(target.chain==0&&target.slot<int(layer.mixChain.size()))slot=&layer.mixChain[static_cast<size_t>(target.slot)];
    if(target.chain==1)for(const auto& n:layer.fxNodes)if(n.id==target.node&&target.slot<int(n.chain.size()))slot=&n.chain[static_cast<size_t>(target.slot)];
    if(target.chain==2)for(const auto& n:layer.eqNodes)if(n.id==target.node&&target.slot<int(n.chain.size()))slot=&n.chain[static_cast<size_t>(target.slot)];
    if(!slot)return;const native::Parameter* found=nullptr;for(const auto& s:native::layout(slot->type))if(s.id==target.parameter)found=&s;if(!found)return;const auto spec=*found;
    dialog=std::make_unique<juce::AlertWindow>("Macro range",target.label+"\nPhysical values at macro 0 and 1. Reverse ranges are allowed.",juce::MessageBoxIconType::NoIcon);
    dialog->addTextEditor("minimum",juce::String(spec.range.convertFrom0to1(target.minimum),6),"At 0 ("+spec.unit+")");
    dialog->addTextEditor("maximum",juce::String(spec.range.convertFrom0to1(target.maximum),6),"At 1 ("+spec.unit+")");
    dialog->addButton("Apply",1);dialog->addButton("Unassign",2);dialog->addButton("Cancel",0);
    dialog->enterModalState(true,juce::ModalCallbackFunction::create([safe=juce::Component::SafePointer<MacroRoutingPanel>(this),macro,target,spec](int result){
        if(!safe)return;
        const auto stale=[&]{safe->hint.setText("The target changed. Reopen Range to edit its current assignment.",juce::dontSendNotification);};
        if(result==2&&!safe->processor.removeMacroDestination(macro,target))stale();
        if(result==1){
            const auto parse=[&](const char* id,float& value){const auto text=safe->dialog->getTextEditorContents(id).trim();const auto* start=text.toRawUTF8();char* end=nullptr;value=std::strtof(start,&end);
                return end!=start&&*end=='\0'&&std::isfinite(value)&&value>=spec.range.start&&value<=spec.range.end;};
            float low=0,high=0;
            if(!parse("minimum",low)||!parse("maximum",high))
                safe->hint.setText("Enter numbers from "+juce::String(spec.range.start)+" to "+juce::String(spec.range.end)+" "+spec.unit+". The range was kept.",juce::dontSendNotification);
            else if(!safe->processor.assignMacro(macro,target,spec.range.convertTo0to1(spec.range.snapToLegalValue(low)),spec.range.convertTo0to1(spec.range.snapToLegalValue(high))))stale();
        }safe->dialog.reset();safe->timerCallback();}),false);
}
}

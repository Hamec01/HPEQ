#include "UI/ScorePanel.h"
#include "Standalone/QuickControls.h"
#include <algorithm>

namespace hpeq::ui
{
namespace
{
bool black(int n){const int k=n%12;return k==1||k==3||k==6||k==8||k==10;}
juce::Colour green(){return juce::Colour(0xff76f6a0);}
}
ScorePanel::HardwareLook::HardwareLook()
{
    setColour(juce::Slider::textBoxTextColourId,green());setColour(juce::Slider::textBoxBackgroundColourId,juce::Colour(0xff09130e));
    setColour(juce::Slider::textBoxOutlineColourId,steel::stencilDim);setColour(juce::Slider::thumbColourId,steel::stencil);
    setColour(juce::Slider::trackColourId,juce::Colour(0xff499567));setColour(juce::Slider::backgroundColourId,juce::Colour(0xff070b09));
    setColour(juce::ComboBox::backgroundColourId,juce::Colour(0xff101712));setColour(juce::ComboBox::textColourId,green());
    setColour(juce::ComboBox::outlineColourId,steel::stencilDim);setColour(juce::PopupMenu::backgroundColourId,juce::Colour(0xff111914));
    setColour(juce::PopupMenu::textColourId,steel::stencil);setColour(juce::TextButton::buttonColourId,juce::Colour(0xff26312b));
    setColour(juce::TextButton::buttonOnColourId,juce::Colour(0xff285d3c));setColour(juce::TextButton::textColourOffId,steel::stencil);
    setColour(juce::TextButton::textColourOnId,green());
}
juce::Font ScorePanel::HardwareLook::getComboBoxFont(juce::ComboBox&){return juce::Font(steel::monoFont(14));}
juce::Label* ScorePanel::HardwareLook::createSliderTextBox(juce::Slider& slider)
{auto* label=juce::LookAndFeel_V4::createSliderTextBox(slider);label->setFont(juce::Font(steel::monoFont(15)));return label;}
void ScorePanel::HardwareLook::drawRotarySlider(juce::Graphics& g,int x,int y,int w,int h,float position,float start,float end,juce::Slider& slider)
{
    const juce::Point<float> centre{static_cast<float>(x)+w*.5f,static_cast<float>(y)+h*.48f};
    const float scale=juce::jmin(w,h)/118.f;
    steel::knobScale(g,centre,scale);steel::brassKnob(g,centre,start+(end-start)*position,scale,slider.isEnabled()?1.f:.4f);
}
ScorePanel::ScorePanel(standalone::ModulePluginProcessor& p):processor(p),canvas(*this),tooltip(this,600)
{
    setLookAndFeel(&look);addAndMakeVisible(canvas);canvas.setBounds(0,0,1180,980);
    const char* pages[]{"REFLECTIONS","LOOP / COLOR","FREQUENCY BANDS","SPACE","GLITCH / DETAILS","QUICK"};
    const char* groups[]{"VOCAL","DRUMS","SAMPLES","SYNTH","ATMOSPHERE"};
    for(int i=0;i<6;++i)
    {
        auto& button=tabs[static_cast<size_t>(i)];canvas.addAndMakeVisible(button);button.setButtonText(pages[i]);button.setBounds(32+186*(i==5?0:i+1),234,178,30);
        button.onClick=[this,i]{selectPage(i);};
    }
    for(int i=0;i<5;++i)
    {
        auto& group=categories[static_cast<size_t>(i)];canvas.addAndMakeVisible(group);group.setButtonText(groups[i]);group.setBounds(574+108*i,176,102,26);
        group.setTooltip("Filter the preset library without changing the sound. LOAD START loads the first matching recipe.");
        group.onClick=[this,i]{const juce::String names[]{"Vocal","Drums","Samples / Keys","Synth","Atmosphere / FX"};category=category==names[i]?juce::String{}:names[i];if(onCategory)onCategory(category);};
    }
    add("scoreSource",-1,{32,150,230,58},juce::Slider::IncDecButtons);
    add("scoreOctaves",-1,{300,150,230,58});
    canvas.addAndMakeVisible(fillMode);fillMode.addItem("Same note",1);fillMode.addItem("Step pattern",2);fillMode.setSelectedId(2);fillMode.setBounds(565,207,175,23);
    canvas.addAndMakeVisible(fillStep);fillStep.setRange(-12,12,1);fillStep.setValue(-12);fillStep.setSliderStyle(juce::Slider::IncDecButtons);
    fillStep.setTextBoxStyle(juce::Slider::TextBoxLeft,false,75,23);fillStep.setTextValueSuffix(" st");fillStep.setBounds(752,207,155,23);
    fillStep.setTooltip("Semitone interval between consecutive notes when Step pattern is selected.");
    canvas.addAndMakeVisible(fillNotes);fillNotes.setBounds(920,207,205,23);
    fillNotes.setTooltip("Fill all eight note cells starting from the selected reflection note. Enabled switches and rhythm stay as set. Notes beyond +/-24 st are visibly muted.");
    fillNotes.onClick=[this]{const int first=juce::roundToInt(processor.parameterValue("tap"+juce::String(selectedTap+1)+"Note"));
        const int step=fillMode.getSelectedId()==1?0:juce::roundToInt(fillStep.getValue());
        for(int t=0;t<8;++t){auto* p=processor.parameter("tap"+juce::String(t+1)+"Note");p->beginChangeGesture();processor.setParameterValue(p->paramID,juce::jlimit(0,127,first+t*step));p->endChangeGesture();}};
    add("scoreMix",-1,{45,805,150,130});add("scoreOutput",-1,{230,805,150,130});
    add("scoreWidth",-1,{415,805,150,130});add("scoreDuck",-1,{600,805,150,130});
    add("scoreFreeze",-1,{800,818,145,55});add("scoreRouting",-1,{800,880,145,55});add("scoreMixLock",-1,{975,818,150,55});
    const char* fields[]{"On","Note","Time","Sync","Gain","Pan","Shape"};
    const int y[]{313,353,413,480,523,591,651},heights[]{50,55,61,50,61,55,50};
    for(int t=0;t<8;++t)
    {
        auto& select=tapSelectors[static_cast<size_t>(t)];canvas.addAndMakeVisible(select);select.setButtonText("ECHO "+juce::String(t+1));
        select.setBounds(38+139*t,281,127,27);select.onClick=[this,t]{selectedTap=t;detailTap.setSelectedId(t+1,juce::dontSendNotification);showPage(page);};
        for(int field=0;field<7;++field)
        {
            const auto style=field==1?juce::Slider::IncDecButtons:juce::Slider::LinearHorizontal;
            add("tap"+juce::String(t+1)+fields[field],0,{42+139*t,y[field],119,heights[field]},style,t);
        }
        const char* detailFields[]{"Fragment","Repeats","Low","High"};
        for(int d=0;d<4;++d)add("tap"+juce::String(t+1)+detailFields[d],4,{690+215*(d%2),350+180*(d/2),185,145},juce::Slider::RotaryHorizontalVerticalDrag,t);
        add("tap"+juce::String(t+1)+"SpaceSend",4,{690,700,400,65},juce::Slider::LinearHorizontal,t);
    }
    const char* loop[]{"scoreFeedback","scoreFeedbackMs","scoreFeedbackPitch","scoreDrive","scoreLowCut","scoreHighCut","scoreFeedbackSync","scoreColor"};
    for(int i=0;i<8;++i)add(loop[i],1,{75+270*(i%4),315+205*(i/4),215,150});
    add("scoreSwing",1,{420,680,340,55},juce::Slider::LinearHorizontal);
    add("scoreBands",2,{65,280,220,55});add("scoreSplitLow",2,{390,280,185,90});add("scoreSplitHigh",2,{650,280,185,90});
    const char* bandFields[]{"Time","Feedback","Gain","Pan"};
    for(int b=0;b<3;++b)for(int i=0;i<4;++i)add("band"+juce::String(b+1)+bandFields[i],2,{65+365*b+145*(i%2),435+160*(i/2),135,135});
    for(int b=0;b<3;++b)add("band"+juce::String(b+1)+"On",2,{215+365*b,371,135,50});
    const char* space[]{"scoreSpaceMix","scoreSpaceDecay","scoreSpaceSize","scoreSpaceDamping","scoreSpaceType","scoreSpaceRoute"};
    for(int i=0;i<6;++i)add(space[i],3,{110+335*(i%3),320+210*(i/3),245,160});
    add("scoreGlitch",4,{65,300,245,60});add("scoreGlitchTarget",4,{350,300,245,60});
    add("scoreGlitchMs",4,{65,415,230,170});add("scoreGlitchAmount",4,{350,415,230,170});
    canvas.addAndMakeVisible(detailTap);detailTap.setBounds(685,288,400,30);
    for(int t=0;t<8;++t)detailTap.addItem("Reflection "+juce::String(t+1)+" details",t+1);
    detailTap.setSelectedId(1);detailTap.onChange=[this]{selectedTap=detailTap.getSelectedId()-1;showPage(page);};
    canvas.addAndMakeVisible(loadStart);loadStart.setBounds(820,94,170,28);loadStart.onClick=[this]{if(onLoadStart)onLoadStart();};
    loadStart.setTooltip("Load the first preset matching the current genre and category. Filtering alone does not load a recipe.");
    canvas.addAndMakeVisible(power);power.setBounds(1040,94,90,28);power.setClickingTogglesState(true);power.setButtonText("BYPASS");
    powerLink=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor.valueTreeState(),"bypass",power);
    for(int i=0;i<8;++i)
    {
        auto& b=quickMoves[static_cast<size_t>(i)];canvas.addAndMakeVisible(b);
        b.setButtonText(standalone::quickName(i));b.setBounds(45+137*i,290,130,32);
        b.setTooltip(standalone::quickDescription(i));b.onClick=[this,i]{auto* param=processor.parameter("quickStyle");param->beginChangeGesture();processor.setParameterValue("quickStyle",i);param->endChangeGesture();canvas.repaint();};
    }
    add("quickTime",5,{85,390,270,270});add("quickMotion",5,{455,390,270,270});add("quickSpace",5,{825,390,270,270});
    add("quickSync",5,{105,678,230,60});
    add("scoreMix",5,{75,810,650,95},juce::Slider::LinearHorizontal);
    add("scoreOutput",5,{975,880,150,60},juce::Slider::LinearHorizontal);
    canvas.addAndMakeVisible(expandQuick);expandQuick.setBounds(418,710,346,38);
    expandQuick.setTooltip("Transfer this exact move into the full Note Score editor. Mix and the running sound are retained. Full-page tabs without this button restore your stored detailed score.");
    expandQuick.onClick=[this]{processor.expandQuickToScore();lastQuick=false;showPage(0);};
    lastQuick=processor.parameterValue("quickEnabled")>.5;showPage(lastQuick?5:0);startTimerHz(10);
}
ScorePanel::~ScorePanel(){stopTimer();powerLink.reset();controls.clear();setLookAndFeel(nullptr);}
void ScorePanel::add(juce::String id,int targetPage,juce::Rectangle<int> bounds,juce::Slider::SliderStyle style,int tap)
{
    const auto& specs=standalone::scoreParameterSpecs();const auto found=std::find_if(specs.begin(),specs.end(),[&](const auto& s){return s.id==id;});
    jassert(found!=specs.end());if(found==specs.end())return;
    auto c=std::make_unique<Control>();c->id=id;c->page=targetPage;c->tap=tap;c->bounds=bounds;
    juce::String title=found->name;if(title.startsWith("Score "))title=title.substring(6);
    if(tap>=0)title=title.fromFirstOccurrenceOf(" ",false,false).fromFirstOccurrenceOf(" ",false,false);
    c->caption.setText(title.toUpperCase(),juce::dontSendNotification);c->caption.setFont(juce::Font(steel::monoFont(12)));
    c->caption.setColour(juce::Label::textColourId,steel::stencil);c->caption.setJustificationType(juce::Justification::centred);
    const int labelHeight=tap>=0&&targetPage==0?14:20;
    c->caption.setBounds(bounds.withHeight(labelHeight));canvas.addAndMakeVisible(c->caption);
    if(found->choices.isEmpty())
    {
        c->slider=std::make_unique<juce::Slider>();auto& slider=*c->slider;
        slider.setSliderStyle(style);slider.setTextBoxStyle(style==juce::Slider::IncDecButtons?juce::Slider::TextBoxLeft:juce::Slider::TextBoxBelow,false,style==juce::Slider::IncDecButtons?bounds.getWidth()-38:bounds.getWidth()-4,24);
        slider.setComponentID(id);slider.setBounds(bounds.withTrimmedTop(labelHeight+1));slider.setRange(found->minimum,found->maximum,found->interval);
        slider.setDoubleClickReturnValue(true,found->defaultValue);slider.setTooltip(help(id));
        if(id=="scoreSource"||id.endsWith("Note"))
        {
            slider.textFromValueFunction=[this](double value){return standalone::noteName(juce::roundToInt(value),processor.parameterValue("scoreOctaves")<.5);};
            slider.valueFromTextFunction=[this,id](const juce::String& text){const int note=standalone::parseNote(text,processor.parameterValue("scoreOctaves")<.5);return note<0?processor.parameterValue(id):static_cast<double>(note);};
        }
        else {slider.setNumDecimalPlacesToDisplay(found->interval>=1?0:found->unit=="ms"||found->unit=="Hz"?1:2);slider.setTextValueSuffix(found->unit.isEmpty()?juce::String{}:" "+found->unit);}
        if(tap>=0)slider.onDragStart=[this,tap]{selectedTap=tap;showPage(page);};
        canvas.addAndMakeVisible(slider);c->sliderLink=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.valueTreeState(),id,slider);
        // JUCE's attachment installs its own text callbacks. Apply note/precision
        // formatting afterwards so typed notes do not revert to numeric MIDI text.
        if(id=="scoreSource"||id.endsWith("Note"))
        {
            slider.setTextValueSuffix({});
            slider.textFromValueFunction=[this](double value){return standalone::noteName(juce::roundToInt(value),processor.parameterValue("scoreOctaves")<.5);};
            slider.valueFromTextFunction=[this,id](const juce::String& text){const int note=standalone::parseNote(text,processor.parameterValue("scoreOctaves")<.5);return note<0?processor.parameterValue(id):static_cast<double>(note);};
        }
        else
        {
            const bool percent=id=="quickMotion"||id=="quickSpace"||id=="scoreMix"||id=="scoreWidth"||id=="scoreDuck"||id=="scoreFeedback"||id=="scoreSpaceMix"||id=="scoreGlitchAmount"||id=="scoreSwing"||id.endsWith("SpaceSend")||(id.startsWith("band")&&id.endsWith("Feedback"));
            if(percent)
            {
                slider.setTextValueSuffix({});slider.textFromValueFunction=[](double value){return juce::String(value*100,0)+" %";};
                slider.valueFromTextFunction=[](const juce::String& text){const double value=text.getDoubleValue();return text.containsChar('%')||std::abs(value)>1?value/100:value;};
            }
            else
            {
                const int decimals=found->interval>=1?0:found->unit=="ms"||found->unit=="Hz"?1:found->unit=="dB"?1:2;
                slider.textFromValueFunction=[decimals](double value){return juce::String(value,decimals);};
            }
        }
        if(id=="quickTime")
        {
            slider.textFromValueFunction=[this](double value){const int division=standalone::quickDivision(static_cast<float>(value));return processor.parameterValue("quickSync")>.5?standalone::scoreParameterSpecs()[standalone::scoreparam::FeedbackSync].choices[division]:juce::String(standalone::quickMilliseconds(static_cast<float>(value)),1)+" ms";};
            slider.valueFromTextFunction=[this](const juce::String& text)->double{if(processor.parameterValue("quickSync")>.5){const auto& labels=standalone::scoreParameterSpecs()[standalone::scoreparam::FeedbackSync].choices;const int d=labels.indexOf(text.trim());return d>=1?standalone::quickTimeForDivision(d):processor.parameterValue("quickTime");}return juce::jlimit(0.,1.,std::log(juce::jlimit(1.,2000.,text.getDoubleValue()))/std::log(2000.));};
        }
        slider.updateText();
    }
    else
    {
        c->combo=std::make_unique<juce::ComboBox>();c->combo->setComponentID(id);c->combo->addItemList(found->choices,1);c->combo->setBounds(bounds.withTrimmedTop(labelHeight+2).withHeight(tap>=0&&targetPage==0?24:27));c->combo->setTooltip(help(id));
        canvas.addAndMakeVisible(*c->combo);c->comboLink=std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(processor.valueTreeState(),id,*c->combo);
    }
    controls.push_back(std::move(c));
}
juce::String ScorePanel::help(const juce::String& id) const
{
    if(id=="quickTime")return "Set the primary delay. Host Sync follows tempo; Milliseconds covers 1-2000 ms. Pitched echoes need at least 80 ms; captured actions add their fragment window.";
    if(id=="quickMotion")return "Add reflections, loop feedback and movement for the selected move. Note Fall/Rise add source-relative intervals and an accumulating pitch step. No automatic note detection.";
    if(id=="quickSpace")return "Blend in the existing Note Score spatial field and lengthen its decay. Zero adds no spatial effect.";
    if(id=="quickSync")return "Choose host-tempo divisions or free milliseconds for TIME. MIX is independent; zero Mix is dry in Insert.";
    if(id=="scoreSource")return "Declare the incoming source note. Echo transposes the complete audio relative to this reference; it does not identify notes or separate chords.";
    if(id=="scoreOctaves")return "Labels only: FL Studio C5 = MIDI 60; Standard C4 = MIDI 60. Stored MIDI notes and audio do not change.";
    if(id.endsWith("Note"))return "Type a note such as C5, F#4, Db4 or a MIDI number. Supported interval is +/-24 semitones from SOURCE. Out-of-range reflections are muted and marked red.";
    if(id=="scoreMix")return "Ordinary Insert dry/wet balance. Send is wet-only and retains this Insert value for your return to Insert.";
    if(id=="scoreMixLock")return "Keep the current Mix when loading factory presets. Loading a project restores its complete saved state.";
    if(id=="scoreFeedbackPitch")return "One pitch step per feedback round. A -12 step descends one octave each time; 0 repeats at the same pitch. Independent of the initial reflection notes.";
    if(id=="scoreSwing")return "Delay synced reflections 2, 4, 6 and 8 by this fraction of a sixteenth note. Free-time reflections are unchanged.";
    if(id.endsWith("SpaceSend"))return "Send this reflection into the spatial field in After Echo or In Feedback routing. Before Echo applies space before all reflections and uses its global Amount.";
    if(id=="scoreFreeze")return "Close the input and hold the rendered loop texture. Output filtering, Width, Mix and Wet Output remain available.";
    if(id.contains("SpaceRoute"))return "Place the spatial field after the echoes, in their feedback loop, or before the note/reflection engine.";
    if(id.startsWith("band"))return "Independent timing, feedback, level and placement. Neutral bands recombine exactly. A shared feedback safety trim bounds the combined band returns when their total feedback is high.";
    if(id.endsWith("Shape")||id.contains("Glitch"))return "Transform captured audio: backwards playback, repeated fragments, slowing tape or omissions. Tail targeting can transform only the spatial field.";
    if(id.endsWith("Time")||id.endsWith("Sync"))return "Requested delay or host-tempo division. Forward pitched repeats have an 80 ms minimum capture delay. Reverse and fragment actions need an additional captured window.";
    if(id.endsWith("Fragment"))return "Captured fragment duration, 50-2000 ms. Reverse preserves fragment playback duration even when its already-transposed source is pitched.";
    if(id.endsWith("Low")||id.endsWith("High")||id.contains("Cut"))return "Thin or darken this wet signal. Minimum Low Cut and maximum High Cut bypass their filters.";
    return "Shape the wet repeats and their spatial field. Double-click a slider to restore its default; hover timing, note and routing controls for capture details.";
}
void ScorePanel::showPage(int next)
{
    page=next;
    for(auto& c:controls){const bool common=c->page<0&&(page!=5||c->id=="scoreFreeze"||c->id=="scoreRouting"||c->id=="scoreMixLock");const bool visible=common||(c->page==page&&(page!=4||c->tap<0||c->tap==selectedTap));c->caption.setVisible(visible);if(c->slider)c->slider->setVisible(visible);if(c->combo)c->combo->setVisible(visible);}
    for(int i=0;i<6;++i)tabs[static_cast<size_t>(i)].setToggleState(i==page,juce::dontSendNotification);
    for(int i=0;i<8;++i){auto& b=quickMoves[static_cast<size_t>(i)];b.setVisible(page==5);b.setToggleState(i==juce::roundToInt(processor.parameterValue("quickStyle")),juce::dontSendNotification);}expandQuick.setVisible(page==5);
    for(auto& b:categories)b.setVisible(page!=5);
    fillMode.setVisible(page!=5);fillStep.setVisible(page!=5);fillNotes.setVisible(page!=5);
    for(int t=0;t<8;++t){tapSelectors[static_cast<size_t>(t)].setVisible(page==0);tapSelectors[static_cast<size_t>(t)].setToggleState(t==selectedTap,juce::dontSendNotification);}
    detailTap.setVisible(page==4);canvas.repaint();
}
void ScorePanel::selectPage(int p)
{
    const int next=juce::jlimit(0,5,p);processor.setQuickEnabled(next==5);lastQuick=next==5;showPage(next);
}
juce::Rectangle<float> ScorePanel::contentBounds() const
{
    return juce::Rectangle<float>(0,0,1180,980).withSizeKeepingCentre(1180,980).transformedBy(canvas.getTransform());
}
void ScorePanel::resized()
{
    const float scale=juce::jmin(getWidth()/1180.f,getHeight()/980.f);
    canvas.setTransform(juce::AffineTransform::scale(scale).translated((getWidth()-1180*scale)*.5f,(getHeight()-980*scale)*.5f));
}
void ScorePanel::Canvas::paint(juce::Graphics& g)
{
    if(background.isNull())
    {background=juce::Image(juce::Image::RGB,1180,980,true);juce::Graphics bg(background);steel::panel(bg,1180,980,4321);steel::nameplate(bg,{410,42,350,94},"HamloEcho","SCORE / STEREO","0421");}
    g.drawImageAt(background,0,0);owner.draw(g);
}
void ScorePanel::Canvas::mouseDown(const juce::MouseEvent& e){owner.keyboardClick(e.position);}
void ScorePanel::draw(juce::Graphics& g)
{
    const int source=juce::roundToInt(processor.parameterValue("scoreSource"));
    steel::lcdWindow(g,{820,43,310,38});steel::lcdText(g,{829,45,292,34},"WET "+juce::String(processor.scoreOutputDb(),1)+" dB",16);
    g.setColour(green());g.setFont(juce::Font(steel::monoFont(13)));
    if(page!=5)g.drawText("MIDI "+juce::String(source)+" / "+juce::String(440*std::pow(2.,(source-69)/12.),2)+" Hz",32,209,240,20,juce::Justification::centred);
    else g.drawText("QUICK NOTE SCORE   /   Choose a move, shape it, then raise MIX.",45,174,1090,40,juce::Justification::centred);
    if(page==0)
    {
        for(int t=0;t<8;++t)
        {
            const auto id="tap"+juce::String(t+1);const bool unsupported=processor.parameterValue(id+"On")>.5&&std::abs(processor.parameterValue(id+"Note")-source)>24;
            g.setColour(juce::Colour(0xff0d1510).withAlpha(.85f));g.fillRoundedRectangle({32.f+139*t,275,139-8.f,436},5);
            g.setColour(unsupported?juce::Colour(0xffef776b):t==selectedTap?green():steel::stencilDim);g.drawRoundedRectangle({32.f+139*t,275,131,436},5,1);
            if(unsupported)g.drawText("RANGE +/-24",37+139*t,705,122,20,juce::Justification::centred);
            else if(processor.parameterValue(id+"On")>.5&&processor.parameterValue(id+"Shape")>0&&processor.parameterValue(id+"Shape")<4)
                g.drawText("CAP +"+juce::String(processor.parameterValue(id+"Fragment"),0)+"ms",37+139*t,693,122,18,juce::Justification::centred);
        }
        drawKeyboard(g);
    }
    else
    {
        g.setColour(juce::Colour(0xff101813).withAlpha(.65f));g.fillRoundedRectangle({32,275,1115,510},8);
        g.setColour(steel::stencil);g.setFont(juce::Font(steel::monoFont(15)));
        if(page==1)g.drawText("SOURCE -> NOTE REFLECTIONS -> LOOP CHARACTER -> PITCH STEP -> REPEAT",55,750,1070,30,juce::Justification::centred);
        if(page==2){const char* titles[]{"LOW","MID","HIGH"};for(int b=0;b<3;++b)g.drawText(titles[b],65+365*b,401,135,28,juce::Justification::centred);}
        if(page==3)g.drawText("ROOM: discrete space   /   PLATE: dense short loops   /   CLOUD: expanded field",55,735,1070,30,juce::Justification::centred);
        if(page==4)g.drawFittedText("TEXTURE CAPTURE: "+juce::String(processor.parameterValue("scoreGlitchMs"),0)+" ms\nREPEAT: paired fragments  /  STUTTER: x4",65,655,530,105,juce::Justification::centred,3);
        if(page==5)
        {
            const int style=juce::roundToInt(processor.parameterValue("quickStyle"));
            g.setColour(green());g.drawFittedText(standalone::quickDescription(style),60,338,1060,40,juce::Justification::centred,2);
            g.setColour(steel::stencilDim);g.setFont(juce::Font(steel::monoFont(12)));
            g.drawText("RHYTHM",85,665,270,20,juce::Justification::centred);g.drawText("REPEATS + MOVEMENT",455,665,270,20,juce::Justification::centred);g.drawText("AMOUNT + DECAY",825,665,270,20,juce::Justification::centred);
            g.drawText("Full tabs recall your stored score. EDIT transfers this move without changing its sound.",45,758,1090,20,juce::Justification::centred);
        }
    }
    g.setColour(steel::stencilDim);g.drawHorizontalLine(797,32,1147);
    g.setFont(juce::Font(steel::monoFont(13)));g.setColour(green());
    g.drawText("HOST "+juce::String(processor.getHostBpm(),1)+" BPM   |   PITCH CAPTURE: MIN 80 ms   |   DRY / HOST LATENCY: 0 samples",38,948,1100,23,juce::Justification::centred);
}
void ScorePanel::drawKeyboard(juce::Graphics& g)
{
    const int root=juce::roundToInt(processor.parameterValue("scoreSource")),first=juce::jmax(0,root-24),last=juce::jmin(127,root+24);
    const int chosen=juce::roundToInt(processor.parameterValue("tap"+juce::String(selectedTap+1)+"Note"));
    int whites=0;for(int n=first;n<=last;++n)if(!black(n))++whites;
    const float width=1110.f/juce::jmax(1,whites);float x=35;
    for(int n=first;n<=last;++n)
        if(!black(n))
        {g.setColour(n==chosen?green():juce::Colour(0xffc9c7b6));g.fillRect(x,727.f,width-1,61.f);g.setColour(juce::Colours::black);
            if(n%12==0)g.drawText(standalone::noteName(n,processor.parameterValue("scoreOctaves")<.5),static_cast<int>(x),762,static_cast<int>(width),22,juce::Justification::centred);x+=width;}
    x=35;
    for(int n=first;n<=last;++n)
        if(!black(n))x+=width;else{g.setColour(n==chosen?green():juce::Colour(0xff151b17));g.fillRect(x-width*.31f,727.f,width*.62f,37.f);}
}
void ScorePanel::keyboardClick(juce::Point<float> p)
{
    if(page!=0||p.y<727||p.y>788||p.x<35||p.x>1145)return;
    const int root=juce::roundToInt(processor.parameterValue("scoreSource")),first=juce::jmax(0,root-24),last=juce::jmin(127,root+24);
    int whites=0;for(int n=first;n<=last;++n)if(!black(n))++whites;const float width=1110.f/juce::jmax(1,whites);
    int chosen=-1;float x=35;
    for(int n=first;n<=last;++n)if(!black(n)){if(p.x>=x&&p.x<x+width)chosen=n;x+=width;}
    if(p.y<764){x=35;for(int n=first;n<=last;++n)if(!black(n))x+=width;else if(p.x>=x-width*.31f&&p.x<x+width*.31f)chosen=n;}
    if(chosen>=0){const auto id="tap"+juce::String(selectedTap+1)+"Note";auto* parameter=processor.parameter(id);parameter->beginChangeGesture();processor.setParameterValue(id,chosen);parameter->endChangeGesture();}
}
void ScorePanel::timerCallback()
{
    const bool quick=processor.parameterValue("quickEnabled")>.5;
    if(quick!=lastQuick){lastQuick=quick;showPage(quick?5:0);}
    for(int i=0;i<8;++i)quickMoves[static_cast<size_t>(i)].setToggleState(i==juce::roundToInt(processor.parameterValue("quickStyle")),juce::dontSendNotification);
    for(auto& c:controls)if(c->id=="quickTime"&&c->slider)c->slider->updateText();
    const bool fl=processor.parameterValue("scoreOctaves")<.5;
    if(fl!=lastFl){lastFl=fl;for(auto& c:controls)if(c->slider&&(c->id=="scoreSource"||c->id.endsWith("Note")))c->slider->updateText();}
    for(int i=0;i<5;++i){const juce::String names[]{"Vocal","Drums","Samples / Keys","Synth","Atmosphere / FX"};categories[static_cast<size_t>(i)].setToggleState(category==names[i],juce::dontSendNotification);}
    canvas.repaint();
}
}

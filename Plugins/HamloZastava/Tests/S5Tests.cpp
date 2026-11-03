#include "Standalone/ModulePluginEditor.h"
#include "Core/ModulePresets.h"
#include "AllocationAudit.h"
#include <set>

namespace
{
using P=hpeq::standalone::ModulePluginProcessor;
constexpr auto kind=hpeq::standalone::ModuleKind::Zastava;
juce::MemoryBlock save(P& p){juce::MemoryBlock b;p.getStateInformation(b);return b;}
void load(P& p,const juce::MemoryBlock& b){p.setStateInformation(b.getData(),static_cast<int>(b.getSize()));}
juce::Component* find(juce::Component& c)
{if(dynamic_cast<hpeq::ui::PresetScreen*>(&c))return &c;for(auto* child:c.getChildren())if(auto* found=find(*child))return found;return nullptr;}
class S5Tests final:public juce::UnitTest
{
public:
    S5Tests():juce::UnitTest("S5 library identity and filtering","HamloZastava"){}
    void runTest()override
    {
        auto p=std::make_unique<P>(kind);
        beginTest("207 programs retain neutral startup and original prefix");
        expectEquals(p->getNumPrograms(),207);expectEquals(p->parameterValue("v2_engine"),1.);expectEquals(p->parameterValue("v2_enabled"),0.);
        const auto& recipes=hpeq::presets::factoryModulePresets(p->getSpec().type);
        expectEquals(static_cast<int>(recipes.size()),206);
        std::set<juce::String> ids,names;bool metadata=true,values=true,unmodified=true,allocationFree=true;
        p->prepareToPlay(48000,256);juce::AudioBuffer<float> data(2,512);juce::MidiBuffer midi;
        for(int index=0;index<207;++index)
        {
            p->setParameterValue("v2_listen",1);p->setParameterValue("bypass",1);
            hamlo::test::allocation::begin();p->setCurrentProgram(index);const auto allocated=hamlo::test::allocation::finish();allocationFree=allocationFree&&allocated==0;
            ids.insert(p->programIdentifier(index));names.insert(p->getProgramName(index));
            unmodified=unmodified&&!p->isProgramModified();values=values&&p->parameterValue("v2_listen")==0;
            // Host bypass is independent of factory program loading.
            values=values&&p->parameterValue("bypass")==1;
            if(index>=7)
            {
                const auto& r=recipes[static_cast<size_t>(index-1)];metadata=metadata&&juce::String(r.id).isNotEmpty()&&juce::String(r.genre).isNotEmpty()&&juce::String(r.group).isNotEmpty()&&juce::String(r.description).isNotEmpty()&&juce::String(r.material).isNotEmpty()&&r.bpm>0;
                for(const auto& v:r.values)values=values&&std::abs(p->parameterValue(v.first)-v.second)<=.011;
                const auto state=save(*p);auto q=std::make_unique<P>(kind);load(*q,state);values=values&&q->programIdentifier(q->getCurrentProgram())==p->programIdentifier(index)&&save(*q)==state&&!q->isProgramModified();
            }
            data.clear();hamlo::test::allocation::begin();p->processBlock(data,midi);allocationFree=allocationFree&&hamlo::test::allocation::finish()==0;
        }
        expectEquals(static_cast<int>(ids.size()),207);expectEquals(static_cast<int>(names.size()),207);expect(metadata);expect(values);expect(unmodified);expect(allocationFree);
        beginTest("Stable ID overrides stale index; unknown IDs retain settings with index fallback");
        p->setCurrentProgram(73);p->setParameterValue("v2_range",22);auto state=save(*p);auto xml=juce::AudioProcessor::getXmlFromBinary(state.getData(),static_cast<int>(state.getSize()));
        xml->setAttribute("v2_program",7);juce::MemoryBlock modified;juce::AudioProcessor::copyXmlToBinary(*xml,modified);load(*p,modified);expectEquals(p->getCurrentProgram(),73);expectEquals(p->parameterValue("v2_range"),22.);expect(p->isProgramModified());
        xml->setAttribute("v2_preset_id","future.unknown");juce::AudioProcessor::copyXmlToBinary(*xml,modified);load(*p,modified);expectEquals(p->getCurrentProgram(),7);expectEquals(p->parameterValue("v2_range"),22.);
        p->setCurrentProgram(206);expect(!p->isProgramModified());const auto original=p->parameterValue("v2_range");p->setParameterValue("v2_range",original+1);expect(p->isProgramModified());p->setParameterValue("v2_range",original);expect(!p->isProgramModified());
        beginTest("Four genre filters, role counts, navigation and restored selected program");
        p->setCurrentProgram(145);const auto selected=save(*p);auto q=std::make_unique<P>(kind);load(*q,selected);std::unique_ptr<juce::AudioProcessorEditor> editor(q->createEditor());
        auto* screen=dynamic_cast<hpeq::ui::PresetScreen*>(find(*editor));expect(screen!=nullptr);
        if(screen)
        {
            for(const auto* genre:{"Boom Bap","Trap","Techno","DnB"})
            {
                screen->setFilter(genre,"All");expectEquals(static_cast<int>(screen->filtered().size()),50);
                int role=0;for(const auto* group:{"Clean","Drum","Duck","Rhythm"})
                {screen->setFilter(genre,group);expectEquals(static_cast<int>(screen->filtered().size()),std::array<int,4>{16,14,8,12}[static_cast<size_t>(role++)]);}
            }
            screen->setFilter("DnB","Rhythm");screen->step(1);expect(q->getCurrentProgram()>=195&&q->getCurrentProgram()<=206);
            expect(editor->getLocalBounds().contains(screen->getBounds()));expect(screen->getBounds().getX()<editor->getWidth()/2);
        }
    }
} s5Tests;
}

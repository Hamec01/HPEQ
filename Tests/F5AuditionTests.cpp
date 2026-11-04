#include "Standalone/ModulePluginEditor.h"
#include "Core/ModulePresets.h"
#include "UI/FaderV2Panel.h"
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_cryptography/juce_cryptography.h>

namespace hamlo::test
{
class F5AuditionTests final:public juce::UnitTest
{
    static juce::String pcm(const juce::AudioBuffer<float>& x)
    {juce::MemoryBlock b;for(int c=0;c<2;++c)b.append(x.getReadPointer(c),static_cast<size_t>(x.getNumSamples())*sizeof(float));return juce::SHA256(b).toHexString();}
public:
    F5AuditionTests():juce::UnitTest("F5 library workflow","HamloFader"){}
    void runTest()override
    {
        using P=hpeq::standalone::ModulePluginProcessor;
        beginTest("Genre/group filters only filter; selected modern recipe recalls its real mode");
        auto p=std::make_unique<P>(hpeq::standalone::ModuleKind::Fader);
        auto owned=std::unique_ptr<juce::AudioProcessorEditor>(p->createEditor());auto* editor=dynamic_cast<hpeq::standalone::ModulePluginEditor*>(owned.get());expect(editor!=nullptr);if(!editor)return;
        auto& screen=editor->getPresetScreen();juce::MemoryBlock a,b;p->getStateInformation(a);screen.setFilter("DnB","Keys / Piano");p->getStateInformation(b);expect(a==b);expectEquals(static_cast<int>(screen.filtered().size()),4);
        screen.step(1);expect(p->getCurrentProgram()>=8);expectWithinAbsoluteError(p->parameterValue("engine"),1.,0.);expect(!p->isProgramModified());
        beginTest("Alignment and phase readouts expose sub-sample and cycle precision");
        auto* panel=dynamic_cast<hpeq::ui::FaderV2Panel*>(editor->getPanel());expect(panel!=nullptr);if(!panel)return;
        panel->selectPage(1);p->setParameterValue("mode",3);panel->refresh();
        auto* phase=dynamic_cast<juce::Slider*>(panel->control("phase"));expect(phase&&phase->getNumDecimalPlacesToDisplay()==3);
        auto* section=dynamic_cast<juce::ComboBox*>(panel->control("advancedSection"));if(section)section->setSelectedId(1,juce::sendNotificationSync);
        auto* delay=dynamic_cast<juce::Slider*>(panel->control("leftDelayMs"));expect(delay&&delay->getNumDecimalPlacesToDisplay()==3);
        if(delay){delay->setValue(.013,juce::sendNotificationSync);expectWithinAbsoluteError(p->parameterValue("leftDelayMs"),.013,1e-5);}
        beginTest("Compensated analysis staging is visible and neutral as a fixed gain; Match loads without applied gain");
        const auto& bank=hpeq::presets::factoryModulePresets(hpeq::FxType::Gain);
        for(int i=8;i<208;++i)
        {p->setCurrentProgram(i);expectWithinAbsoluteError(p->parameterValue("inputDb")+p->parameterValue("gainDb"),0.,1e-5);expectWithinAbsoluteError(p->parameterValue("matchedDb"),0.,0.);expect(juce::String(bank[static_cast<size_t>(i-1)].note).isNotEmpty());}
        beginTest("Optional matched private auditions reconstruct the exact prepared input");
        const auto checkName=juce::SystemStats::getEnvironmentVariable("HAMLO_FADER_DEMO_CHECK",{});
        if(checkName.isEmpty()){logMessage("PRIVATE_AUDITIONS=not requested");return;}
        const juce::File check(checkName),out(juce::SystemStats::getEnvironmentVariable("HAMLO_FADER_DEMO_OUT",{}));
        expect(!out.exists());if(out.exists())return;expect(out.createDirectory().wasOk());
        const auto report=juce::JSON::parse(check.getChildFile("result.json"));const auto materialIds=report["identity"]["materials"];
        juce::AudioFormatManager formats;formats.registerBasicFormats();juce::Array<juce::var> records;
        for(int genre=0;genre<4;++genre)for(int offset:{22,33,46})
        {
            const int index=genre*50+offset,program=index+8;const auto& recipe=bank[static_cast<size_t>(program-1)];
            const auto m=materialIds[recipe.material];const int count=static_cast<int>(m["source_length"]),length=static_cast<int>(m["samples"]),start=static_cast<int>(m["start"]);
            std::unique_ptr<juce::AudioFormatReader> reader(formats.createReaderFor(juce::File(m["source"].toString())));expect(reader!=nullptr);if(!reader)return;
            juce::AudioBuffer<float> source(2,count+8);source.clear();reader->read(&source,0,count,start,true,true);if(reader->numChannels==1)source.copyFrom(1,0,source,0,0,count);
            juce::AudioBuffer<float> dry(2,length);for(int c=0;c<2;++c){juce::LagrangeInterpolator resample;resample.process(static_cast<double>(m["source_rate"])/48000.,source.getReadPointer(c),dry.getWritePointer(c),length);}dry.applyGain(static_cast<float>(static_cast<double>(m["gain"])));
            expectEquals(pcm(dry),m["prepared_sha256"].toString());
            const auto wetPath=check.getChildFile("audio/"+juce::String(recipe.id)+".wav");std::unique_ptr<juce::AudioFormatReader> processedReader(formats.createReaderFor(wetPath));expect(processedReader!=nullptr);if(!processedReader)return;
            juce::AudioBuffer<float> processed(2,length);processedReader->read(&processed,0,length,0,true,true);
            const int gap=19200;juce::AudioBuffer<float> pair(2,length*2+gap);pair.clear();for(int c=0;c<2;++c){pair.copyFrom(c,0,dry,c,0,length);pair.copyFrom(c,length+gap,processed,c,0,length);}
            const float gain=std::min(1.f,.5f/pair.getMagnitude(0,pair.getNumSamples()));pair.applyGain(gain);
            const auto path=out.getChildFile(juce::String(recipe.id)+"_dry_then_processed.wav");auto stream=path.createOutputStream();juce::WavAudioFormat wav;std::unique_ptr<juce::AudioFormatWriter> writer(wav.createWriterFor(stream.get(),48000,2,24,{},0));expect(writer!=nullptr);if(!writer)return;stream.release();expect(writer->writeFromAudioSampleBuffer(pair,0,pair.getNumSamples()));writer.reset();
            auto row=juce::var(new juce::DynamicObject);auto* data=row.getDynamicObject();data->setProperty("id",recipe.id);data->setProperty("name",recipe.name);data->setProperty("file",path.getFileName());data->setProperty("sha256",juce::SHA256(path).toHexString());data->setProperty("prepared_sha256",m["prepared_sha256"]);data->setProperty("common_gain",gain);data->setProperty("order","Dry,0.4s silence,processed;single common gain");records.add(row);
        }
        expectEquals(records.size(),12);expect(out.getChildFile("comparisons.json").replaceWithText(juce::JSON::toString(juce::var(records))));
    }
};static F5AuditionTests f5AuditionTests;
}

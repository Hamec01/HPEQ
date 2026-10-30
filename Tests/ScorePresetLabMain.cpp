#include <juce_audio_utils/juce_audio_utils.h>
#include "Standalone/ModulePluginProcessor.h"
#include "Core/ModulePresets.h"
#include "SourceIdentity.h"
#include <chrono>
#include <fstream>
#include <iostream>

using hpeq::standalone::ModulePluginProcessor;
using hpeq::standalone::ModuleKind;
struct Playhead:juce::AudioPlayHead
{
    double bpm=120;int64_t time=0;
    juce::Optional<PositionInfo> getPosition()const override{PositionInfo p;p.setBpm(bpm);p.setTimeInSamples(time);p.setIsPlaying(true);return p;}
};
juce::var object(){return new juce::DynamicObject;}
void set(juce::var& x,const char* key,const juce::var& value){x.getDynamicObject()->setProperty(key,value);}
void write(const juce::File& f,const juce::var& value){f.getParentDirectory().createDirectory();if(!f.replaceWithText(juce::JSON::toString(value,true)))throw std::runtime_error("Cannot write report");}
int main(int argc,char** argv)
{
    juce::ScopedJuceInitialiser_GUI juce;
    try
    {
        if(argc<3)throw std::runtime_error("manifest <json> | render <jobs.json>");
        ModulePluginProcessor p(ModuleKind::Echo);const auto& bank=hpeq::presets::factoryModulePresets(p.getSpec().type);
        if(std::string(argv[1])=="bench")
        {
            const bool dense=argc>3&&std::string(argv[3])=="dense";
            juce::Array<juce::var> rows;
            for(int paths:{1,2})
            {
                p.setParameterValue("echoMode",1);p.setParameterValue("scoreBands",1);p.setParameterValue("scoreFeedbackPitch",-12);
                p.setParameterValue("scoreSpaceRoute",1);p.setParameterValue("scoreSpaceMix",.6);p.setParameterValue("scoreGlitch",3);p.setParameterValue("scoreGlitchTarget",2);p.setParameterValue("scoreGlitchAmount",.4);
                const int notes[]{36,41,43,48,55,60,67,72};
                for(int t=0;t<8;++t){const auto id="tap"+juce::String(t+1);p.setParameterValue(id+"On",1);p.setParameterValue(id+"Note",notes[t]);p.setParameterValue(id+"Time",150+120*t);p.setParameterValue(id+"Gain",-18);p.setParameterValue(id+"Shape",t%3==0?1:0);}
                p.prepareToPlay(48000,256);juce::AudioBuffer<float> block(2,256);juce::MidiBuffer midi;double total=0,maximum=0;uint32_t random=1;
                for(int b=0;b<1500;++b)
                {
                    if(b==100&&paths==2)p.setParameterValue("tap1Note",37);
                    for(int i=0;i<256;++i){const int frame=b*256+i;const double time=frame/48000.;random^=random<<13;random^=random>>17;random^=random<<5;
                        const int phase=frame%384;const double burst=dense&&phase<288?.08*(static_cast<double>(random)/4294967295.*2-1)*(.5-.5*std::cos(2*juce::MathConstants<double>::pi*phase/288)):0;
                        const float sample=static_cast<float>(.12*std::sin(2*juce::MathConstants<double>::pi*220*time)+.06*std::sin(2*juce::MathConstants<double>::pi*997*time)+burst);block.setSample(0,i,sample);block.setSample(1,i,sample);}
                    const auto begin=std::chrono::steady_clock::now();p.processBlock(block,midi);
                    const double duration=std::chrono::duration<double>(std::chrono::steady_clock::now()-begin).count();if(b>=100){total+=duration;maximum=std::max(maximum,duration);}
                }
                auto row=object();set(row,"paths",paths);set(row,"mean_percent",100*total/1400/(256./48000));set(row,"max_percent",100*maximum/(256./48000));rows.add(row);
            }
            auto result=object();set(result,"dsp_source",HAMLO_ECHO_SOURCE_ID);set(result,"workload",dense?"tones plus 6 ms noise bursts every 8 ms":"tones");set(result,"rows",rows);write(juce::File(argv[2]),result);return 0;
        }
        if(std::string(argv[1])=="manifest")
        {
            juce::Array<juce::var> rows;
            for(int program=8;program<p.getNumPrograms();++program)
            {
                p.setCurrentProgram(program);const auto& recipe=bank[static_cast<size_t>(program-1)];auto row=object(),values=object();
                set(row,"program",program);set(row,"id",recipe.id);set(row,"name",recipe.name);set(row,"genre",recipe.genre);set(row,"group",recipe.group);
                set(row,"material",recipe.material);set(row,"depth",recipe.depth);set(row,"bpm",recipe.bpm);set(row,"note",recipe.note);
                for(const auto& spec:hpeq::standalone::scoreParameterSpecs())values.getDynamicObject()->setProperty(spec.id,p.parameterValue(spec.id));
                set(row,"values",values);rows.add(row);
            }
            auto root=object();set(root,"dsp_source",HAMLO_ECHO_SOURCE_ID);set(root,"program_count",p.getNumPrograms());set(root,"presets",rows);write(juce::File(argv[2]),root);return 0;
        }
        juce::var jobs;const auto parsed=juce::JSON::parse(juce::File(argv[2]).loadFileAsString(),jobs);if(parsed.failed()||!jobs.isArray())throw std::runtime_error("Invalid jobs");
        juce::AudioFormatManager formats;formats.registerBasicFormats();Playhead head;p.setPlayHead(&head);
        for(const auto& job:*jobs.getArray())
        {
            const juce::File report(job["report"].toString());
            if(job.hasProperty("expected_dsp")&&job["expected_dsp"].toString()!=HAMLO_ECHO_SOURCE_ID)throw std::runtime_error("Stale requested DSP source");
            std::unique_ptr<juce::AudioFormatReader> reader(formats.createReaderFor(juce::File(job["input"].toString())));
            if(!reader||reader->sampleRate!=48000||reader->lengthInSamples>48000*30)throw std::runtime_error("Expected prepared 48 kHz material <=30 s");
            juce::AudioBuffer<float> input(2,static_cast<int>(reader->lengthInSamples));reader->read(&input,0,input.getNumSamples(),0,true,true);
            const int program=static_cast<int>(job["program"]);p.setParameterValue("scoreMixLock",0);p.setParameterValue("bypass",0);p.setCurrentProgram(program);
            head.bpm=static_cast<double>(job["bpm"]);head.time=0;
            if(job.hasProperty("output_db"))p.setParameterValue("scoreOutput",static_cast<double>(job["output_db"]));
            const int routing=static_cast<int>(job["routing"]);p.setParameterValue("scoreRouting",routing);
            p.prepareToPlay(48000,256);
            const int samples=input.getNumSamples()+static_cast<int>(std::ceil(48000*p.getTailLengthSeconds()));
            juce::MemoryBlock saved;p.getStateInformation(saved);std::vector<float> before;
            for(auto* parameter:p.getParameters())before.push_back(parameter->getValue());
            p.setParameterValue("tap1Note",127);p.setStateInformation(saved.getData(),static_cast<int>(saved.getSize()));
            bool state=p.getCurrentProgram()==program;for(int i=0;i<p.getParameters().size();++i)state&=before[static_cast<size_t>(i)]==p.getParameters()[i]->getValue();
            p.prepareToPlay(48000,256);
            std::ofstream raw(job["raw"].toString().toStdString(),std::ios::binary);if(!raw)throw std::runtime_error("Cannot open raw output");
            juce::AudioBuffer<float> block(2,256);juce::MidiBuffer midi;float packed[512];double energy=0,wetEnergy=0,dryEnergy=0,peak=0,lastEnergy=0;bool finite=true;
            const double mix=p.parameterValue("scoreMix");
            for(int start=0;start<samples;start+=256)
            {
                const int count=std::min(256,samples-start);block.clear();
                if(start<input.getNumSamples())for(int ch=0;ch<2;++ch)block.copyFrom(ch,0,input,ch,start,std::min(count,input.getNumSamples()-start));
                head.time=start;juce::AudioBuffer<float> view(block.getArrayOfWritePointers(),2,count);p.processBlock(view,midi);
                for(int i=0;i<count;++i)for(int ch=0;ch<2;++ch)
                {
                    const double dry=start+i<input.getNumSamples()?input.getSample(ch,start+i):0,y=block.getSample(ch,i),wet=y-(routing==0?(1-mix)*dry:0);
                    packed[2*i+ch]=static_cast<float>(y);energy+=y*y;wetEnergy+=wet*wet;dryEnergy+=dry*dry;peak=std::max(peak,std::abs(y));finite&=std::isfinite(y);if(start+i>=samples-48000)lastEnergy+=y*y;
                }
                raw.write(reinterpret_cast<const char*>(packed),count*2*sizeof(float));
            }
            raw.close();auto result=object();set(result,"dsp_source",HAMLO_ECHO_SOURCE_ID);set(result,"program",program);set(result,"routing",routing);set(result,"samples",samples);
            set(result,"energy",energy);set(result,"wet_energy",wetEnergy);set(result,"dry_energy",dryEnergy);set(result,"last_second_energy",lastEnergy);
            set(result,"peak",peak);set(result,"finite",finite);set(result,"state_exact",state);set(result,"output_db",p.parameterValue("scoreOutput"));set(result,"mix",mix);
            write(report,result);std::cout<<program<<" "<<routing<<" "<<report.getFullPathName()<<std::endl;
        }
        p.setPlayHead(nullptr);return 0;
    }
    catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 1;}
}

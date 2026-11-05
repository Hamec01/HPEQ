#include "DSP/TransitionRender.h"
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_cryptography/juce_cryptography.h>
#include <set>
#include <iostream>
namespace tr=hpeq::dsp::transition;
namespace
{
juce::var object(){return juce::var(new juce::DynamicObject);}
void put(juce::var& v,const char* id,const juce::var& x){v.getDynamicObject()->setProperty(id,x);}
juce::String pcm(const juce::AudioBuffer<float>& x){juce::MemoryBlock b;for(int c=0;c<2;++c)b.append(x.getReadPointer(c),static_cast<size_t>(x.getNumSamples())*sizeof(float));return juce::SHA256(b).toHexString();}
}
int main(int argc,char** argv)
{
    if(argc!=3)return 2;const juce::File manifest(argv[1]),folder(argv[2]);if(folder.exists())return 3;folder.createDirectory();
    const auto input=juce::JSON::parse(manifest);juce::AudioFormatManager formats;formats.registerBasicFormats();juce::Array<juce::var> rows,structures;
    for(const auto& source:*input["sources"].getArray())
    {
        const juce::File file(source["path"].toString());std::unique_ptr<juce::AudioFormatReader> reader(formats.createReaderFor(file));if(!reader)return 4;
        juce::AudioBuffer<float> x(2,static_cast<int>(reader->lengthInSamples));reader->read(&x,0,x.getNumSamples(),0,true,true);
        const auto render=[&](const tr::Settings& s,bool compatibility)
        {
            const auto start=juce::Time::getHighResolutionTicks();const auto y=tr::render(x,s);const double ms=juce::Time::highResolutionTicksToSeconds(juce::Time::getHighResolutionTicks()-start)*1000;
            const auto key=source["name"].toString()+"_"+juce::String(s.kind)+"_"+juce::String(s.recipe)+"_"+juce::String(s.bars)+"_"+juce::String(static_cast<int>(s.seed));
            auto row=object();put(row,"key",key);put(row,"source",source["name"]);put(row,"kind",s.kind);put(row,"recipe",s.recipe);put(row,"bars",s.bars);put(row,"seed",static_cast<int>(s.seed));put(row,"compatibility",compatibility);put(row,"pcm_sha256",pcm(y));put(row,"samples",y.getNumSamples());put(row,"render_ms",ms);
            const int bar=static_cast<int>(std::lround(s.sampleRate*240/s.bpm)),first=std::max(0,y.getNumSamples()-bar);float peak=0;bool finite=true;double prefixError=0;
            for(int c=0;c<2;++c)for(int n=0;n<y.getNumSamples();++n){const float v=y.getSample(c,n);peak=std::max(peak,std::abs(v));finite=finite&&std::isfinite(v);if(n<first&&n<x.getNumSamples())prefixError=std::max(prefixError,std::abs(static_cast<double>(v-x.getSample(c,n))));}
            juce::Array<juce::var> envelope;
            for(int cell=0;cell<128;++cell){const int a=first+(y.getNumSamples()-first)*cell/128,b=first+(y.getNumSamples()-first)*(cell+1)/128;double e=0;for(int c=0;c<2;++c)for(int n=a;n<b;++n)e+=y.getSample(c,n)*static_cast<double>(y.getSample(c,n));envelope.add(std::sqrt(e/std::max(1,2*(b-a))));}
            put(row,"finite",finite);put(row,"peak",peak);put(row,"prefix_error",prefixError);put(row,"rms128",envelope);
            if(!compatibility&&s.bars==1)
            {const auto path=folder.getChildFile(key+".wav");std::unique_ptr<juce::OutputStream> stream=path.createOutputStream();juce::WavAudioFormat wav;auto writer=wav.createWriterFor(stream,juce::AudioFormatWriterOptions{}.withSampleRate(s.sampleRate).withNumChannels(2).withBitsPerSample(32).withSampleFormat(juce::AudioFormatWriterOptions::SampleFormat::floatingPoint));if(!writer||!writer->writeFromAudioSampleBuffer(y,0,y.getNumSamples()))throw std::runtime_error("WAV failed");}
            rows.add(row);
        };
        for(int kind:{tr::Rise,tr::Fill})for(int recipe=0;recipe<=7;++recipe)for(int bars:{1,2})
        {tr::Settings s;s.kind=kind;s.recipe=recipe;s.bars=bars;s.bpm=source["bpm"];s.knobs=tr::defaultKnobs(kind);s.seed=17;render(s,true);}
        std::vector<int> recipes{1};
#ifdef HAMLO_DRUM_FILL_V3
        recipes.push_back(8);
#endif
        for(int recipe:recipes)for(int bars:{1,2})for(int seed=1;seed<=8;++seed)
        {tr::Settings s;s.kind=tr::Fill;s.recipe=recipe;s.bars=bars;s.bpm=source["bpm"];s.knobs=tr::defaultKnobs(tr::Fill);s.seed=static_cast<std::uint64_t>(seed);render(s,false);}
        for(int recipe=1;recipe<=20;++recipe)
        {
#ifndef HAMLO_DRUM_FILL_V3
            if(recipe>7)continue;
#endif
            tr::Settings s;s.kind=tr::Fill;s.recipe=recipe;s.bars=1;s.bpm=source["bpm"];s.knobs=tr::defaultKnobs(tr::Fill);
            std::set<juce::String> plans;std::set<std::uint32_t> masks;const int seeds=recipe==8?64:32;
            for(int seed=1;seed<=seeds;++seed)
            {
                s.seed=static_cast<std::uint64_t>(seed);const auto plan=tr::makePlan(x,s);juce::String signature;std::uint32_t mask=0;const int length=tr::lengthSamples(s);
                for(const auto& event:plan.events){signature<<event.at<<":"<<event.length<<":"<<event.source<<";";mask|=1u<<std::clamp(static_cast<int>(event.at*32LL/length),0,31);}
                plans.insert(signature);masks.insert(mask);
            }
            auto row=object();put(row,"source",source["name"]);put(row,"recipe",recipe);put(row,"seeds",seeds);put(row,"plans",static_cast<int>(plans.size()));put(row,"rhythm_masks32",static_cast<int>(masks.size()));structures.add(row);
        }
        std::cout<<source["name"].toString()<<" done"<<std::endl;
    }
    auto result=object();put(result,"rows",rows);put(result,"structures",structures);
#ifdef HAMLO_DRUM_FILL_V3
    put(result,"new_features","available");
#else
    put(result,"new_features","n/a");
#endif
    folder.getChildFile("results.json").replaceWithText(juce::JSON::toString(result));return 0;
}

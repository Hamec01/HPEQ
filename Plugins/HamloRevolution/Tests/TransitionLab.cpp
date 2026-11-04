#include "DSP/TransitionRender.h"
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_cryptography/juce_cryptography.h>
#include <iostream>
namespace tr=hpeq::dsp::transition;
int main(int argc,char**argv)
{
    if(argc!=3)return 2;const juce::File manifest(argv[1]),folder(argv[2]);if(folder.exists())return 3;folder.createDirectory();
    const auto data=juce::JSON::parse(manifest);juce::AudioFormatManager formats;formats.registerBasicFormats();juce::Array<juce::var> rows;
    for(const auto& source:*data["sources"].getArray())
    {
        std::unique_ptr<juce::AudioFormatReader> reader(formats.createReaderFor(juce::File(source["path"].toString())));if(!reader)return 4;
        juce::AudioBuffer<float> input(2,static_cast<int>(reader->lengthInSamples));reader->read(&input,0,input.getNumSamples(),0,true,true);
        for(int kind=0;kind<2;++kind)for(int bars:{1,2})for(int seed=1;seed<=8;++seed)
        {
            int versions=1;
#ifdef HAMLO_TRANSITION_V2
            versions=2;
#endif
            for(int version=0;version<versions;++version)
            {
                tr::Settings s;s.kind=kind;s.bars=bars;s.seed=static_cast<std::uint64_t>(seed);s.bpm=source["bpm"];s.knobs=tr::defaultKnobs(kind);
#ifdef HAMLO_TRANSITION_V2
                s.recipe=version;
#endif
                const auto start=juce::Time::getHighResolutionTicks();const auto y=tr::render(input,s);
                const double ms=juce::Time::highResolutionTicksToSeconds(juce::Time::getHighResolutionTicks()-start)*1000;
                juce::MemoryBlock pcm;for(int c=0;c<2;++c)pcm.append(y.getReadPointer(c),static_cast<size_t>(y.getNumSamples())*sizeof(float));
                auto row=juce::var(new juce::DynamicObject);auto* o=row.getDynamicObject();const auto key=source["name"].toString()+"_"+juce::String(kind)+"_"+juce::String(bars)+"_"+juce::String(seed)+"_"+juce::String(version);
                o->setProperty("key",key);o->setProperty("source",source["name"]);o->setProperty("kind",kind);o->setProperty("bars",bars);o->setProperty("seed",seed);o->setProperty("version",version);o->setProperty("samples",y.getNumSamples());o->setProperty("pcm_sha256",juce::SHA256(pcm).toHexString());o->setProperty("render_ms",ms);
                juce::Array<juce::var> envelope;double first=0,last=0;float peak=0;bool finite=true;
                for(int bin=0;bin<64;++bin){double e=0;const int from=y.getNumSamples()*bin/64,to=y.getNumSamples()*(bin+1)/64;
                    for(int c=0;c<2;++c)for(int n=from;n<to;++n){const double a=y.getSample(c,n);e+=a*a;peak=std::max(peak,static_cast<float>(std::abs(a)));finite=finite&&std::isfinite(a);}
                    envelope.add(std::sqrt(e/std::max(1,2*(to-from))));if(bin<16)first+=e;if(bin>=48)last+=e;}
                o->setProperty("rms64",envelope);o->setProperty("peak",peak);o->setProperty("finite",finite);o->setProperty("growth_db",10*std::log10(std::max(last,1e-30)/std::max(first,1e-30)));
                if(seed==1||seed==8)
                {juce::WavAudioFormat wav;std::unique_ptr<juce::OutputStream> stream=folder.getChildFile(key+".wav").createOutputStream();
                    auto writer=wav.createWriterFor(stream,juce::AudioFormatWriterOptions{}.withSampleRate(s.sampleRate).withNumChannels(2).withBitsPerSample(32).withSampleFormat(juce::AudioFormatWriterOptions::SampleFormat::floatingPoint));
                    if(!writer||!writer->writeFromAudioSampleBuffer(y,0,y.getNumSamples()))return 5;}
                rows.add(row);
            }
        }
        std::cout<<source["name"].toString()<<" done"<<std::endl;
    }
    folder.getChildFile("results.json").replaceWithText(juce::JSON::toString(rows));return 0;
}

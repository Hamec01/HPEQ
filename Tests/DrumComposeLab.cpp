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
juce::String hash(const juce::AudioBuffer<float>& x){juce::MemoryBlock b;for(int c=0;c<2;++c)b.append(x.getReadPointer(c),static_cast<size_t>(x.getNumSamples())*sizeof(float));return juce::SHA256(b).toHexString();}
void wav(const juce::File& file,const juce::AudioBuffer<float>& x)
{std::unique_ptr<juce::OutputStream> stream=file.createOutputStream();juce::WavAudioFormat f;auto w=f.createWriterFor(stream,juce::AudioFormatWriterOptions{}.withSampleRate(48000).withNumChannels(2).withBitsPerSample(32).withSampleFormat(juce::AudioFormatWriterOptions::SampleFormat::floatingPoint));if(!w||!w->writeFromAudioSampleBuffer(x,0,x.getNumSamples()))throw std::runtime_error("WAV write");}
}
int main(int argc,char** argv)
{
    if(argc!=3)return 2;const juce::File material(argv[1]),folder(argv[2]);if(folder.exists()||folder.createDirectory().failed())return 3;
    juce::AudioFormatManager formats;formats.registerBasicFormats();const auto sources=juce::JSON::parse(material)["sources"];juce::Array<juce::var> rows,structures;
    for(const auto& source:*sources.getArray())
    {
        std::unique_ptr<juce::AudioFormatReader> reader(formats.createReaderFor(juce::File(source["path"].toString())));if(!reader)return 4;
        juce::AudioBuffer<float> x(2,static_cast<int>(reader->lengthInSamples));reader->read(&x,0,x.getNumSamples(),0,true,true);
        const auto render=[&](const tr::Settings& s,bool compatibility)
        {
            const auto tick=juce::Time::getHighResolutionTicks();const auto y=tr::render(x,s);const double ms=juce::Time::highResolutionTicksToSeconds(juce::Time::getHighResolutionTicks()-tick)*1000;
            const auto key=source["name"].toString()+"_"+juce::String(s.kind)+"_"+juce::String(s.recipe)+"_"+juce::String(s.bars)+"_"+juce::String(static_cast<int>(s.seed));
            auto r=object();put(r,"key",key);put(r,"source",source["name"]);put(r,"kind",s.kind);put(r,"recipe",s.recipe);put(r,"bars",s.bars);put(r,"seed",static_cast<int>(s.seed));put(r,"compatibility",compatibility);put(r,"pcm_sha256",hash(y));put(r,"samples",y.getNumSamples());put(r,"render_ms",ms);
            double prefix=0,peak=0;bool finite=true;const int phrase=y.getNumSamples()-static_cast<int>(std::lround(s.sampleRate*240/s.bpm));
            for(int c=0;c<2;++c)for(int n=0;n<y.getNumSamples();++n){const auto v=y.getSample(c,n);finite=finite&&std::isfinite(v);peak=std::max(peak,std::abs(static_cast<double>(v)));if(n<phrase)prefix=std::max(prefix,std::abs(static_cast<double>(v-x.getSample(c,n))));}
            put(r,"finite",finite);put(r,"peak",peak);put(r,"prefix_error",prefix);if(!compatibility&&s.bars==1)wav(folder.getChildFile(key+".wav"),y);rows.add(r);
        };
        for(int kind:{tr::Rise,tr::Fill})for(int recipe=0;recipe<=(kind==tr::Rise?7:20);++recipe)for(int bars:{1,2})
        {tr::Settings s;s.kind=kind;s.recipe=recipe;s.bars=bars;s.bpm=source["bpm"];s.knobs=tr::defaultKnobs(kind);s.seed=17;render(s,true);}
        std::vector<int> recipes{8};
#ifdef HAMLO_DRUM_COMPOSE
        recipes.push_back(21);
#endif
        for(int recipe:recipes)for(int bars:{1,2})for(int seed=1;seed<=8;++seed)
        {tr::Settings s;s.kind=tr::Fill;s.recipe=recipe;s.bars=bars;s.bpm=source["bpm"];s.knobs=tr::defaultKnobs(tr::Fill);s.seed=static_cast<std::uint64_t>(seed);render(s,false);}
        tr::Settings original;original.bars=1;original.bpm=source["bpm"];juce::AudioBuffer<float> dry(2,tr::lengthSamples(original));for(int c=0;c<2;++c)dry.copyFrom(c,0,x,c,0,dry.getNumSamples());wav(folder.getChildFile(source["name"].toString()+"_source.wav"),dry);
        std::vector<int> probeRecipes{8};
#ifdef HAMLO_DRUM_COMPOSE
        for(int recipe=21;recipe<=29;++recipe)probeRecipes.push_back(recipe);
#endif
        for(int recipe:probeRecipes)
        {
            std::set<juce::String> plans,masks,orders;juce::Array<juce::var> similarities,counts;
            for(int seed=1;seed<=(recipe==8||recipe==21?64:32);++seed)
            {
                auto s=original;s.kind=tr::Fill;s.recipe=recipe;s.seed=static_cast<std::uint64_t>(seed);s.knobs=tr::defaultKnobs(tr::Fill);const auto p=tr::makePlan(x,s);juce::String plan,order;std::array<std::uint32_t,3> mask{};int events[3]{};
                for(const auto& e:p.events){plan<<e.at<<":"<<e.length<<":"<<e.source<<":"<<e.role<<";";order<<e.source<<":"<<e.role<<";";const int role=std::clamp(e.role,0,2);mask[static_cast<size_t>(role)]|=1u<<std::clamp(static_cast<int>(e.at*32LL/tr::lengthSamples(s)),0,31);++events[role];}
                plans.insert(plan);orders.insert(order);masks.insert(juce::String(mask[0])+":"+juce::String(mask[1])+":"+juce::String(mask[2]));juce::Array<juce::var> count;for(int v:events)count.add(v);counts.add(count);
#ifdef HAMLO_DRUM_COMPOSE
                if(recipe>=21)similarities.add(p.inputSimilarity);
#endif
            }
            auto r=object();put(r,"source",source["name"]);put(r,"recipe",recipe);put(r,"plans",static_cast<int>(plans.size()));put(r,"rhythm_masks",static_cast<int>(masks.size()));put(r,"source_orders",static_cast<int>(orders.size()));put(r,"role_counts",counts);put(r,"input_similarity",similarities);structures.add(r);
        }
        std::cout<<source["name"].toString()<<" done"<<std::endl;
    }
    auto result=object();put(result,"rows",rows);put(result,"structures",structures);
#ifdef HAMLO_DRUM_COMPOSE
    put(result,"composition","available");
#else
    put(result,"composition","n/a");
#endif
    folder.getChildFile("results.json").replaceWithText(juce::JSON::toString(result));return 0;
}

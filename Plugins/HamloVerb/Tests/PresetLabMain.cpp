// Complete-render reverb calibration. MIX is solved from wet/dry energy, then measured again
// through the actual Insert path. A saturation or gain-reduction proxy is intentionally not used.
#include <iostream>
#include <map>
#include <set>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_cryptography/juce_cryptography.h>
#include <juce_dsp/juce_dsp.h>
#include "VerbLabHelpers.h"
#include "Core/ModulePresets.h"

using namespace hamlo::verblab;
namespace
{
constexpr double fs=48000;
struct Tempo final:juce::AudioPlayHead
{
    double bpm=120;
    juce::Optional<PositionInfo> getPosition() const override
    {PositionInfo pos;pos.setBpm(bpm);pos.setIsPlaying(true);return pos;}
};
juce::String sha(const juce::File& f){return juce::SHA256(f).toHexString();}
juce::String sourceHash()
{
    juce::MemoryOutputStream data;
    const auto root=juce::File::getCurrentWorkingDirectory();
    juce::Array<juce::File> files;
    root.getChildFile("Source/DSP").findChildFiles(files,juce::File::findFiles,true,"*.cpp");
    root.getChildFile("Source/DSP").findChildFiles(files,juce::File::findFiles,true,"*.h");
    files.add(root.getChildFile("Source/Standalone/ModulePluginProcessor.cpp"));
    files.add(root.getChildFile("Source/Standalone/ModulePluginProcessor.h"));
    files.add(root.getChildFile("Source/Standalone/ModuleVariant.cpp"));
    files.add(root.getChildFile("Source/Standalone/VerbParameters.h"));
    files.add(root.getChildFile("Tests/PresetLabMain.cpp"));
    files.add(root.getChildFile("Tests/VerbLabHelpers.h"));
    std::sort(files.begin(),files.end(),[](const auto&a,const auto&b){return a.getFullPathName()<b.getFullPathName();});
    for(const auto& f:files){data.writeString(f.getRelativePathFrom(root));data.writeString(sha(f));}
    return juce::SHA256(data.getData(),data.getDataSize()).toHexString();
}
juce::AudioBuffer<float> material(const juce::File& path)
{
    juce::AudioFormatManager formats;formats.registerBasicFormats();
    std::unique_ptr<juce::AudioFormatReader> reader(formats.createReaderFor(path));
    if(!reader)throw std::runtime_error("Cannot read material: "+path.getFullPathName().toStdString());
    const int length=static_cast<int>(reader->lengthInSamples);
    if(length<1)throw std::runtime_error("Empty source audio");
    juce::AudioBuffer<float> raw(2,length);reader->read(&raw,0,length,0,true,true);
    if(reader->numChannels==1)raw.copyFrom(1,0,raw,0,0,length);
    const int window=std::min(length,static_cast<int>(reader->sampleRate*10));
    std::vector<double> integral(static_cast<size_t>(length+1),0);
    for(int i=0;i<length;++i)integral[static_cast<size_t>(i+1)]=integral[static_cast<size_t>(i)]+raw.getSample(0,i)*static_cast<double>(raw.getSample(0,i))+raw.getSample(1,i)*static_cast<double>(raw.getSample(1,i));
    int start=0;double best=-1;
    const int stride=std::max(1,static_cast<int>(reader->sampleRate*.1));
    for(int i=0;i+window<=length;i+=stride)
    {const double e=integral[static_cast<size_t>(i+window)]-integral[static_cast<size_t>(i)];if(e>best){best=e;start=i;}}
    const int outLength=std::max(1,static_cast<int>(window*fs/reader->sampleRate));
    juce::AudioBuffer<float> result(2,outLength);
    if(reader->sampleRate==fs)
        for(int ch=0;ch<2;++ch)result.copyFrom(ch,0,raw,ch,start,outLength);
    else
    {
        // Four guard samples permit the interpolator to read the end of a selected window safely.
        juce::AudioBuffer<float> segment(2,window+8);segment.clear();
        for(int ch=0;ch<2;++ch){segment.copyFrom(ch,0,raw,ch,start,window);juce::LagrangeInterpolator interp;interp.process(reader->sampleRate/fs,segment.getReadPointer(ch),result.getWritePointer(ch),outLength);}
    }
    const double rms=std::sqrt(energy(result)/(2.*outLength));
    if(rms<1e-12)throw std::runtime_error("Silent source audio");
    const double peak=result.getMagnitude(0,outLength);
    result.applyGain(static_cast<float>(std::min(std::pow(10.,-18./20)/rms,std::pow(10.,-3./20)/std::max(1e-20,peak))));
    return result;
}
juce::String signature(const Processor& p,bool includeMix)
{
    juce::String result;
    for(const auto& spec:p.getSpec().parameters)
    {
        if(spec.id=="routing"||spec.id=="bypass"||(!includeMix&&(spec.id=="mix"||spec.id=="outputDb")))continue;
        result+=spec.id+"="+juce::String(p.parameterValue(spec.id),9)+";";
    }
    return juce::SHA256(result.toRawUTF8(),static_cast<size_t>(result.getNumBytesAsUTF8())).toHexString();
}
std::unique_ptr<Processor> presetProcessor(int program,int routing,double mix,double output,Tempo& tempo)
{
    auto p=std::make_unique<Processor>(hpeq::standalone::ModuleKind::Verb);
    p->setCurrentProgram(program);p->setParameterValue("routing",routing);p->setParameterValue("mix",mix);p->setParameterValue("outputDb",output);
    p->setPlayHead(&tempo);p->prepareToPlay(fs,4096);settle(*p,fs);return p;
}
juce::AudioBuffer<float> complete(Processor& p,const juce::AudioBuffer<float>& input,int total)
{
    juce::AudioBuffer<float> x(2,total);x.clear();
    for(int ch=0;ch<2;++ch)x.copyFrom(ch,0,input,ch,0,input.getNumSamples());
    return render(p,x,4096);
}
double truePeak(const juce::AudioBuffer<float>& audio)
{
    juce::dsp::Oversampling<float> oversampler(2,3,juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple,true);
    oversampler.initProcessing(4096);oversampler.reset();
    juce::AudioBuffer<float> block(2,4096);double peak=0;
    // Zero padding also flushes the reconstruction filters after the measured render.
    for(int pos=0;pos<audio.getNumSamples()+4096;pos+=4096)
    {
        block.clear();const int n=std::min(4096,audio.getNumSamples()-pos);
        if(n>0)for(int ch=0;ch<2;++ch)block.copyFrom(ch,0,audio,ch,pos,n);
        const auto up=oversampler.processSamplesUp(juce::dsp::AudioBlock<const float>(block));
        for(size_t ch=0;ch<up.getNumChannels();++ch)
            for(size_t i=0;i<up.getNumSamples();++i)peak=std::max(peak,std::abs(static_cast<double>(up.getChannelPointer(ch)[i])));
    }
    return peak;
}
double mixedRatio(const juce::AudioBuffer<float>& out,const juce::AudioBuffer<float>& dry,double mix,double output)
{
    const double gain=std::pow(10.,output/20);double wet=0;
    for(int ch=0;ch<2;++ch)for(int i=0;i<out.getNumSamples();++i)
    {const double d=i<dry.getNumSamples()?dry.getSample(ch,i):0;const double w=out.getSample(ch,i)/gain-(1-mix)*d;wet+=w*w;}
    return 10*std::log10(wet/((1-mix)*(1-mix)*energy(dry)+1e-300)+1e-300);
}
bool stateRoundTrip(Processor& p,const juce::AudioBuffer<float>& source,Tempo& tempo)
{
    juce::MemoryBlock state;p.getStateInformation(state);auto q=processor({});q->setPlayHead(&tempo);
    q->setStateInformation(state.getData(),static_cast<int>(state.getSize()));
    for(const auto& spec:p.getSpec().parameters)
        if(std::abs(p.parameterValue(spec.id)-q->parameterValue(spec.id))>1e-6)return false;
    p.prepareToPlay(fs,4096);q->prepareToPlay(fs,4096);
    juce::AudioBuffer<float> excerpt(2,std::min(4096,source.getNumSamples()));
    for(int ch=0;ch<2;++ch)excerpt.copyFrom(ch,0,source,ch,0,excerpt.getNumSamples());
    return exact(render(p,excerpt,127),render(*q,excerpt,127));
}
void wave(const juce::File& file,const juce::AudioBuffer<float>& audio)
{
    file.getParentDirectory().createDirectory();file.deleteFile();juce::WavAudioFormat format;
    std::unique_ptr<juce::FileOutputStream> stream(file.createOutputStream());
    if(!stream)throw std::runtime_error("Cannot write WAV");
    std::unique_ptr<juce::AudioFormatWriter> writer(format.createWriterFor(stream.get(),fs,2,24,{},0));
    if(!writer)throw std::runtime_error("Cannot create WAV writer");stream.release();
    if(!writer->writeFromAudioSampleBuffer(audio,0,audio.getNumSamples()))throw std::runtime_error("Cannot finish WAV");
}
}
int main(int argc,char** argv)
{
    juce::ScopedJuceInitialiser_GUI gui;
    if(argc<5){std::cerr<<"Usage: HamloVerb_PresetLab calibrate|check material-dir cache-dir result.json [genre]\n";return 2;}
    try
    {
        const bool calibrate=juce::String(argv[1])=="calibrate";
        const auto cwd=juce::File::getCurrentWorkingDirectory();
        const auto materialDir=cwd.getChildFile(argv[2]),cache=cwd.getChildFile(argv[3]),output=cwd.getChildFile(argv[4]);
        const juce::String genre=argc>5?argv[5]:"";
        cache.createDirectory();const auto dspHash=sourceHash();
        const auto& presets=hpeq::presets::factoryModulePresets(hpeq::FxType::Reverb);
        juce::Array<juce::var> results;int passed=0,failed=0;std::set<std::string> unique;
        std::map<std::string,juce::AudioBuffer<float>> audioCache;
        std::set<std::string> demoGroups;
        for(size_t index=7;index<presets.size();++index)
        {
            const auto& recipe=presets[index];
            if(!genre.isEmpty()&&juce::String(recipe.genre)!=genre)continue;
            const juce::String id=recipe.id;
            const auto sourceFile=materialDir.getChildFile(juce::String(recipe.material)+".wav");
            Tempo tempo;tempo.bpm=recipe.tempo;
            auto probe=presetProcessor(static_cast<int>(index+1),0,recipe.mix,0,tempo);
            const auto soundSignature=signature(*probe,false);
            const bool isUnique=unique.insert(soundSignature.toStdString()).second;
            const auto key=dspHash+":"+sha(sourceFile)+":"+signature(*probe,!calibrate)+":"+juce::String(recipe.depth)+":"+juce::String(recipe.tempo);
            const auto cacheFile=cache.getChildFile(id+(calibrate?"_cal.json":"_check.json"));
            auto cached=juce::JSON::parse(cacheFile);
            if(calibrate&&cached.isObject()&&cached["cacheKey"].toString()==key&&static_cast<bool>(cached["pass"]))
            {results.add(cached);++passed;std::cout<<id<<" resumed PASS"<<std::endl;continue;}
            const auto materialKey=std::string(recipe.material);
            if(audioCache.count(materialKey)==0)audioCache.emplace(materialKey,material(sourceFile));
            const auto& input=audioCache.at(materialKey);
            const double tail=std::clamp(2*probe->getTailLengthSeconds()+1.,2.,120.);
            const int total=input.getNumSamples()+static_cast<int>(std::ceil(tail*fs));
            double lower=-24,upper=-16;
            if(juce::String(recipe.depth)=="Medium"){lower=-16;upper=-8;}
            else if(juce::String(recipe.depth)=="Heavy"){lower=-8;upper=6;}
            const double desired=(lower+upper)*.5;
            auto send=presetProcessor(static_cast<int>(index+1),1,recipe.mix,0,tempo);
            auto wet=complete(*send,input,total);
            const double dryEnergy=energy(input),wetEnergy=energy(wet);
            const double target=std::pow(10.,desired/20);
            double mix=calibrate?std::clamp(target/(std::sqrt(wetEnergy/(dryEnergy+1e-300))+target),.001,.995):probe->parameterValue("mix");
            double trim=0;
            if(calibrate)
            {
                juce::AudioBuffer<float> estimated(wet);
                for(int ch=0;ch<2;++ch)for(int i=0;i<total;++i)
                    estimated.setSample(ch,i,static_cast<float>(mix*wet.getSample(ch,i)+(i<input.getNumSamples()?(1-mix)*input.getSample(ch,i):0)));
                const double maximum=std::max(truePeak(estimated),truePeak(wet));
                trim=std::clamp(std::min(0.,-1.1-20*std::log10(maximum+1e-300)),-24.,12.);
            }
            else
            {
                // Read the actual compiled output value, not the untrimmed probe used for signature.
                auto p=processor({});p->setCurrentProgram(static_cast<int>(index+1));trim=p->parameterValue("outputDb");
            }
            auto insert=presetProcessor(static_cast<int>(index+1),0,mix,trim,tempo);
            const auto mixed=complete(*insert,input,total);
            const double depth=mixedRatio(mixed,input,mix,trim);
            const double insertPeak=20*std::log10(truePeak(mixed)+1e-300);
            // Output is a common linear trim; verify the final Send by an independent actual render in check mode.
            if(calibrate)wet.applyGain(static_cast<float>(std::pow(10.,trim/20)));
            else {send=presetProcessor(static_cast<int>(index+1),1,mix,trim,tempo);wet=complete(*send,input,total);}
            const double sendPeak=20*std::log10(truePeak(wet)+1e-300);
            const bool stateOk=stateRoundTrip(*insert,input,tempo);
            const bool success=finite(mixed)&&finite(wet)&&wetEnergy>1e-15&&depth>=lower-.01&&depth<=upper+.01&&insertPeak<=-1&&sendPeak<=-1&&isUnique&&stateOk;
            auto* item=new juce::DynamicObject;
            item->setProperty("id",id);item->setProperty("name",recipe.name);item->setProperty("genre",recipe.genre);item->setProperty("group",recipe.group);
            item->setProperty("material",recipe.material);item->setProperty("depth",recipe.depth);item->setProperty("cacheKey",key);
            item->setProperty("dspHash",dspHash);item->setProperty("materialHash",sha(sourceFile));item->setProperty("signature",soundSignature);
            item->setProperty("mix",mix);item->setProperty("outputDb",trim);item->setProperty("measuredDepthDb",depth);
            item->setProperty("insertTruePeakDb",insertPeak);item->setProperty("sendTruePeakDb",sendPeak);item->setProperty("tailSeconds",tail);
            item->setProperty("unique",isUnique);item->setProperty("stateRoundTrip",stateOk);item->setProperty("pass",success);
            const juce::var value(item);results.add(value);cacheFile.replaceWithText(juce::JSON::toString(value,true));
            success?++passed:++failed;
            std::cout<<id<<" "<<(success?"PASS":"FAIL")<<" depth="<<depth<<" dB, mix="<<mix<<", trim="<<trim<<", TP Insert="<<insertPeak<<", Send="<<sendPeak<<std::endl;
            // One real A/B example per material section and genre, selected deterministically from the bank.
            const auto demoKey=std::string(recipe.genre)+"/"+recipe.group;
            if(!calibrate&&success&&demoGroups.insert(demoKey).second)
            {const auto demo=cwd.getChildFile("build/demos").getChildFile(id);wave(demo.getChildFile("dry.wav"),input);wave(demo.getChildFile("insert.wav"),mixed);wave(demo.getChildFile("send.wav"),wet);demo.getChildFile("description.txt").replaceWithText(juce::String(recipe.name)+"\n"+recipe.note+"\nInput: "+sourceFile.getFullPathName()+"\nDepth: "+juce::String(depth,3)+" dB\n");}
        }
        auto* root=new juce::DynamicObject;root->setProperty("mode",calibrate?"calibrate":"check");root->setProperty("genre",genre);
        root->setProperty("dspHash",dspHash);root->setProperty("passed",passed);root->setProperty("failed",failed);root->setProperty("results",juce::var(results));
        output.getParentDirectory().createDirectory();output.replaceWithText(juce::JSON::toString(juce::var(root),true));
        std::cout<<passed<<" passed, "<<failed<<" failed"<<std::endl;return failed==0?0:1;
    }
    catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 2;}
}

#include "Standalone/ModulePluginProcessor.h"
#include "Core/ModulePresets.h"
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_cryptography/juce_cryptography.h>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <set>

namespace hamlo::test
{
using Fader=hpeq::standalone::ModulePluginProcessor;
namespace
{
juce::var obj(){return juce::var(new juce::DynamicObject);}
void put(juce::var& v,const char* key,const juce::var& x){v.getDynamicObject()->setProperty(key,x);}
void json(const juce::File& f,const juce::var& v){f.getParentDirectory().createDirectory();if(!f.replaceWithText(juce::JSON::toString(v)))throw std::runtime_error("Cannot write "+f.getFullPathName().toStdString());}
juce::String hash(const juce::String& text){return juce::SHA256(text.toRawUTF8(),text.getNumBytesAsUTF8()).toHexString();}
juce::String normalizedHash(const juce::File& f){return hash(f.loadFileAsString().replace("\r\n","\n"));}
std::unique_ptr<Fader> make(){return std::make_unique<Fader>(hpeq::standalone::ModuleKind::Fader);}
struct Head final:juce::AudioPlayHead
{
    double bpm=120;int at=0;
    juce::Optional<PositionInfo> getPosition()const override{PositionInfo i;i.setBpm(bpm);i.setPpqPosition(at*bpm/(60.*48000));i.setTimeInSamples(at);i.setIsPlaying(true);return i;}
};
juce::AudioBuffer<float> render(Fader& p,const juce::AudioBuffer<float>& x,Head& head,int block=256)
{
    juce::AudioBuffer<float> y(x);juce::MidiBuffer midi;
    for(int at=0;at<y.getNumSamples();at+=block){const int n=std::min(block,y.getNumSamples()-at);head.at=at;juce::AudioBuffer<float> view(y.getArrayOfWritePointers(),2,at,n);p.processBlock(view,midi);}
    return y;
}
juce::String pcmHash(const juce::AudioBuffer<float>& x)
{juce::MemoryBlock bytes;for(int c=0;c<x.getNumChannels();++c)bytes.append(x.getReadPointer(c),static_cast<size_t>(x.getNumSamples())*sizeof(float));return juce::SHA256(bytes).toHexString();}
}
class F5LibraryTests final:public juce::UnitTest
{
public:
    F5LibraryTests():juce::UnitTest("F5 preset library","HamloFader"){}
    void runTest()override
    {
#ifndef HAMLO_FADER_LIBRARY
        beginTest("Factory library absent in F4 baseline");logMessage("F5_LIBRARY=n/a");
#else
        try{library();}catch(const std::exception& e){expect(false,e.what());}
#endif
    }
#ifdef HAMLO_FADER_LIBRARY
    struct Material{juce::AudioBuffer<float> audio;juce::var identity;};
    std::map<juce::String,std::unique_ptr<Material>> materials;
    struct Result{juce::AudioBuffer<float> audio;double work=0,peak=-120,applied=0,measured=-120;bool finite=true,state=true,valid=true;};
    Material& material(const juce::File& folder,const juce::String& name)
    {
        auto& cached=materials[name];if(cached)return *cached;
        auto m=std::make_unique<Material>();const auto path=folder.getChildFile(name+".wav");
        juce::AudioFormatManager formats;formats.registerBasicFormats();std::unique_ptr<juce::AudioFormatReader> reader(formats.createReaderFor(path));
        if(!reader)throw std::runtime_error("Missing material "+path.getFullPathName().toStdString());
        const int length=static_cast<int>(reader->lengthInSamples),window=std::min(length,static_cast<int>(6*reader->sampleRate));
        juce::AudioBuffer<float> raw(2,length+8);raw.clear();reader->read(&raw,0,length,0,true,true);if(reader->numChannels==1)raw.copyFrom(1,0,raw,0,0,length);
        std::vector<double> sum(static_cast<size_t>(length+1));for(int i=0;i<length;++i)sum[static_cast<size_t>(i+1)]=sum[static_cast<size_t>(i)]+raw.getSample(0,i)*static_cast<double>(raw.getSample(0,i))+raw.getSample(1,i)*static_cast<double>(raw.getSample(1,i));
        int start=0;double best=0;const int stride=std::max(1,static_cast<int>(reader->sampleRate*.1));
        for(int i=0;i+window<=length;i+=stride){const double e=sum[static_cast<size_t>(i+window)]-sum[static_cast<size_t>(i)];if(e>best){best=e;start=i;}}
        const int count=static_cast<int>(window*48000./reader->sampleRate);m->audio.setSize(2,count);juce::AudioBuffer<float> segment(2,window+8);segment.clear();
        for(int c=0;c<2;++c){segment.copyFrom(c,0,raw,c,start,window);juce::LagrangeInterpolator rs;rs.process(reader->sampleRate/48000.,segment.getReadPointer(c),m->audio.getWritePointer(c),count);}
        double power=0;for(int c=0;c<2;++c)for(int i=0;i<count;++i)power+=m->audio.getSample(c,i)*static_cast<double>(m->audio.getSample(c,i));
        if(power<1e-20)throw std::runtime_error("Silent material");
        const double gain=std::min(std::pow(10.,-18./20)/std::sqrt(power/(2.*count)),std::pow(10.,-9./20)/m->audio.getMagnitude(0,count));m->audio.applyGain(static_cast<float>(gain));
        m->identity=obj();put(m->identity,"source",path.getFullPathName());put(m->identity,"source_sha256",juce::SHA256(path).toHexString());put(m->identity,"source_rate",reader->sampleRate);put(m->identity,"start",start);put(m->identity,"source_length",window);put(m->identity,"samples",count);put(m->identity,"gain",gain);put(m->identity,"prepared_sha256",pcmHash(m->audio));
        cached=std::move(m);return *cached;
    }
    Result measure(int program,const hpeq::presets::ModulePreset& recipe,const Material& source,const juce::String& parameter,double value,bool verifyState=false)
    {
        auto p=make();p->setCurrentProgram(program);if(parameter.isNotEmpty())p->setParameterValue(parameter,value);
        Head head;head.bpm=recipe.bpm;p->setPlayHead(&head);p->prepareToPlay(48000,256);Result r;
        if(p->parameterValue("mode")==1)
        {
            r.valid=p->requestAction(hpeq::standalone::FaderAction::StartLearn);render(*p,source.audio,head);
            r.valid=p->requestAction(hpeq::standalone::FaderAction::StopLearn)&&r.valid;
            juce::AudioBuffer<float> silence(2,256);silence.clear();juce::MidiBuffer midi;head.at=source.audio.getNumSamples();p->processBlock(silence,midi);
            hpeq::standalone::FaderStatus learned;r.valid=p->readStatus(learned)&&r.valid;r.measured=learned.proposal.measuredDb;
            r.valid=p->applyMatch()&&r.valid;r.applied=p->parameterValue("matchedDb");p->prepareToPlay(48000,256);
        }
        r.audio=render(*p,source.audio,head);hpeq::standalone::FaderStatus status;r.valid=p->readStatus(status)&&r.valid;r.peak=status.reading.output.truePeak;
        double work=0;int windows=0;
        for(int at=24000;at+960<=source.audio.getNumSamples();at+=960)
        {
            double in=0,out=0;for(int c=0;c<2;++c)for(int i=at;i<at+960;++i){const double a=source.audio.getSample(c,i),b=r.audio.getSample(c,i);in+=a*a;out+=b*b;}
            if(in/(2*960)>1e-6){work+=std::abs(10*std::log10(std::max(1e-30,out)/in));++windows;}
        }
        r.valid=windows>0&&r.valid;r.work=windows?work/windows:0;
        for(int c=0;c<2;++c)for(int i=0;i<r.audio.getNumSamples();++i)r.finite=r.finite&&std::isfinite(r.audio.getSample(c,i));
        if(verifyState)
        {
            juce::MemoryBlock state;p->getStateInformation(state);auto q=make();q->setStateInformation(state.getData(),static_cast<int>(state.getSize()));
            r.state=q->getCurrentProgram()==program&&q->editorPage()==p->editorPage();
            for(auto* base:p->getParameters()){auto* par=dynamic_cast<juce::RangedAudioParameter*>(base);r.state=r.state&&std::abs(p->parameterValue(par->paramID)-q->parameterValue(par->paramID))<.0003;}
            q->setPlayHead(&head);q->prepareToPlay(48000,256);r.state=r.state&&pcmHash(render(*q,source.audio,head,127))==pcmHash(r.audio)&&q->getLatencySamples()==0;
        }
        return r;
    }
    juce::String signature(Fader& p,bool skeleton)
    {
        const int mode=static_cast<int>(p.parameterValue("mode"));juce::StringArray ids;
        if(mode==1)ids={"mode","measure","targetDb","matchBoost","matchCut","matchCeiling","peakGuard"};
        else if(mode==2)ids={"mode","targetDb","profile","strength","maxBoost","maxCut","floorDb","holdMs","idleMs","riseMs","fallMs"};
        else{ids={"mode","shape","points","smooth","sync","depthDb","beats","freeMs","phase","rightPhase","edgeMs"};for(int i=0;i<32;++i)ids.add("level"+juce::String(i).paddedLeft('0',2));}
        juce::String result;for(const auto& id:ids)if(!skeleton||(id!="targetDb"&&id!="depthDb"))result+=id+"="+juce::String(p.parameterValue(id),9)+";";return hash(result);
    }
    void library()
    {
        beginTest("208 programs;200 permanent IDs,group counts,unique effective recipes and neutral held Match");
        const auto& bank=hpeq::presets::factoryModulePresets(hpeq::FxType::Gain);auto p=make();expectEquals(p->getNumPrograms(),208);expectEquals(static_cast<int>(bank.size()),207);
        std::set<juce::String> ids,signatures;std::map<juce::String,int> genres,groups,modes;juce::String skeleton;
        for(int program=8;program<208;++program)
        {
            const auto& recipe=bank[static_cast<size_t>(program-1)];p->setCurrentProgram(program);
            expect(ids.insert(recipe.id).second&&juce::String(recipe.id).isNotEmpty());expect(juce::String(recipe.note).isNotEmpty()&&juce::String(recipe.material).isNotEmpty());
            expect(p->parameterValue("engine")==1&&p->parameterValue("mix")==1&&p->parameterValue("outputDb")==0&&p->parameterValue("matchedDb")==0);
            expect(!p->isProgramModified());expect(signatures.insert(juce::String(recipe.group)+":"+signature(*p,false)).second,"Duplicate effective recipe "+juce::String(recipe.id));
            ++genres[recipe.genre];++groups[juce::String(recipe.genre)+":"+recipe.group];++modes[juce::String(recipe.genre)+":"+juce::String(p->parameterValue("mode"),0)];
            for(auto* base:p->getParameters()){auto* par=dynamic_cast<juce::RangedAudioParameter*>(base);const double v=p->parameterValue(par->paramID);const auto range=par->getNormalisableRange();expect(std::isfinite(v)&&v>=range.start-.001&&v<=range.end+.001);}
            skeleton+=juce::String(recipe.id)+":"+recipe.material+":"+juce::String(recipe.bpm,4)+":"+juce::String(recipe.goalDb,5)+":"+signature(*p,true);
        }
        expectEquals(static_cast<int>(genres.size()),4);for(const auto& pair:genres)expectEquals(pair.second,50);
        const std::map<juce::String,int> distribution{{"Kick",6},{"Snare / Clap",6},{"Hats / Cymbals",5},{"Percussion",4},{"Drum Bus",7},{"808 / Bass",5},{"Vocals",5},{"Keys / Piano",4},{"Samples / Melody",4},{"Master",4}};
        for(const auto& genre:genres){for(const auto& group:distribution)expectEquals(groups[genre.first+":"+group.first],group.second);expectEquals(modes[genre.first+":1"],11);expectEquals(modes[genre.first+":2"],19);expectEquals(modes[genre.first+":3"],20);}
        const auto mode=juce::SystemStats::getEnvironmentVariable("HAMLO_FADER_LIBRARY_MODE",{});if(mode.isEmpty()){logMessage("MATERIAL_PASS=not requested (explicit calibration/check run required)");return;}
        if(mode!="calibrate"&&mode!="check")throw std::runtime_error("Unknown library mode");
        const juce::File output(juce::SystemStats::getEnvironmentVariable("HAMLO_FADER_LIBRARY_DIR",{})),sourceFolder(juce::SystemStats::getEnvironmentVariable("HAMLO_FADER_MATERIALS",{}));
        const auto root=juce::File::getCurrentWorkingDirectory();juce::Array<juce::File> sourceFiles;root.getChildFile("Source/DSP").findChildFiles(sourceFiles,juce::File::findFiles,true);std::sort(sourceFiles.begin(),sourceFiles.end(),[](const auto& a,const auto& b){return a.getFullPathName()<b.getFullPathName();});
        juce::String dsp="FaderPresetDSPV1";for(const auto& f:sourceFiles)dsp+=normalizedHash(f);
        for(const auto* name:{"Source/Standalone/ModulePluginProcessor.cpp","Source/Standalone/ModulePluginProcessor.h","Source/Standalone/FaderHostControls.cpp","Source/Standalone/FaderHostControls.h","Source/Standalone/ModuleVariant.cpp"})dsp+=normalizedHash(root.getChildFile(name));
        juce::String method="FaderPresetMethodV1";for(const auto* name:{"Tests/F5LibraryTests.cpp","docs/F5_PRESETS.md"})method+=normalizedHash(root.getChildFile(name));
        auto identity=obj();put(identity,"dsp_sha256",hash(dsp));put(identity,"method_sha256",hash(method));put(identity,"skeleton_sha256",hash(skeleton));put(identity,"mode",mode);
        auto materialIds=obj();for(int i=7;i<207;++i){const auto& recipe=bank[static_cast<size_t>(i)];put(materialIds,recipe.material,material(sourceFolder,recipe.material).identity);}put(identity,"materials",materialIds);
        const auto manifest=output.getChildFile("identity.json");if(manifest.existsAsFile()){if(juce::JSON::toString(juce::JSON::parse(manifest))!=juce::JSON::toString(identity))throw std::runtime_error("Identity changed; use a new output folder, do not resume another DSP/method/material/recipe");}else json(manifest,identity);
        beginTest("Actual owner-material calibration/check:work,peak,finite,state and command workflow");
        juce::Array<juce::var> rows;int passed=0;
        for(int program=8;program<208;++program)
        {
            const auto& recipe=bank[static_cast<size_t>(program-1)];const auto record=output.getChildFile("records/"+juce::String(recipe.id)+".json");
            if(record.existsAsFile())
            {auto row=juce::JSON::parse(record);const bool valid=static_cast<bool>(row["passed"]);expect(valid,"Resumed failed recipe "+juce::String(recipe.id));if(valid)++passed;rows.add(row);continue;}
            p->setCurrentProgram(program);const int role=static_cast<int>(p->parameterValue("mode"));const juce::String parameter=role==3?"depthDb":"targetDb";auto& input=material(sourceFolder,recipe.material);
            double value=p->parameterValue(parameter);Result result;
            if(mode=="calibrate")
            {
                if(role==1)
                {
                    result=measure(program,recipe,input,{},0);
                    value=std::clamp(result.measured-recipe.goalDb,-40.,0.);
                }
                else
                {
                    double lo=role==3?0:-40,hi=role==3?90:0,best=value,bestError=1e30;bool bracket=false;double left=lo,right=hi,fl=0;
                    double previous=lo,previousError=0;
                    const int steps=role==3?1:10;
                    for(int i=0;i<=steps;++i)
                    {
                        const double at=lo+(hi-lo)*i/steps;const auto trial=measure(program,recipe,input,parameter,at);const double error=trial.work-recipe.goalDb;
                        if(std::abs(error)<bestError){bestError=std::abs(error);best=at;}
                        if(i>0&&error*previousError<=0){left=previous;right=at;fl=previousError;bracket=true;break;}previous=at;previousError=error;
                    }
                    if(bracket)for(int i=0;i<12;++i){const double at=(left+right)*.5;const auto trial=measure(program,recipe,input,parameter,at);const double error=trial.work-recipe.goalDb;if(std::abs(error)<bestError){bestError=std::abs(error);best=at;}if(error*fl<=0)right=at;else{left=at;fl=error;}}
                    value=best;
                }
            }
            result=measure(program,recipe,input,parameter,value,true);
            const juce::String depth(recipe.depth);const double low=depth=="Gentle"?1:depth=="Medium"?3:6,high=depth=="Gentle"?3:depth=="Medium"?6:12;
            const bool pass=result.valid&&result.finite&&result.state&&result.work>=low&&result.work<=high&&result.peak<=-1;
            auto row=obj();put(row,"id",recipe.id);put(row,"program",program);put(row,"name",recipe.name);put(row,"mode",role);put(row,"depth",depth);put(row,"goal_db",recipe.goalDb);put(row,"parameter",parameter);put(row,"value",value);put(row,"work_db",result.work);put(row,"peak_dbtp",result.peak);put(row,"finite",result.finite);put(row,"state_exact",result.state);put(row,"workflow_valid",result.valid);put(row,"passed",pass);put(row,"pcm_sha256",pcmHash(result.audio));
            if(mode=="check")
            {
                const auto wave=output.getChildFile("audio/"+juce::String(recipe.id)+".wav");wave.getParentDirectory().createDirectory();auto stream=wave.createOutputStream();juce::WavAudioFormat format;std::unique_ptr<juce::AudioFormatWriter> writer(format.createWriterFor(stream.get(),48000,2,24,{},0));if(!writer)throw std::runtime_error("WAV writer failed");stream.release();if(!writer->writeFromAudioSampleBuffer(result.audio,0,result.audio.getNumSamples()))throw std::runtime_error("WAV export failed");writer.reset();put(row,"wav_sha256",juce::SHA256(wave).toHexString());
            }
            json(record,row);rows.add(row);if(pass)++passed;expect(pass,juce::JSON::toString(row,true));
            std::cout<<recipe.id<<" "<<(pass?"PASS":"FAIL")<<" work="<<result.work<<" peak="<<result.peak<<std::endl;
        }
        auto report=obj();put(report,"identity",identity);put(report,"passed",passed);put(report,"total",200);put(report,"recipes",rows);json(output.getChildFile("result.json"),report);logMessage("LIBRARY "+mode+" "+juce::String(passed)+"/200");
    }
#endif
};static F5LibraryTests f5LibraryTests;
}

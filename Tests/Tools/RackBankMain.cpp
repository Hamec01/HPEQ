// Rack bank laboratory. Thresholds: RACK0_DESIGN A3-A7, fixed before the first
// render. Synthetic fixtures are labeled; they never stand in for owner listening.
#include "Rack/RackPresets.h"
#include "Plugin/FxPluginProcessor.h"
#include "Plugin/FxPluginEditor.h"
#include "UI/PresetBrowser.h"
#include "Analysis/LoudnessMeter.h"
#include <BinaryData.h>
#include <juce_dsp/juce_dsp.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_cryptography/juce_cryptography.h>
#include <iostream>
#include <cmath>
#include <memory>
#include <map>

namespace
{
constexpr double fs=48000;
juce::File materialFolder;bool materialMode=false;
struct Head:juce::AudioPlayHead
{
    int sample=0;double bpm=120;
    juce::Optional<PositionInfo> getPosition() const override
    {PositionInfo p;p.setBpm(bpm);p.setTimeInSamples(sample);p.setPpqPosition(sample*bpm/(60*fs));p.setIsPlaying(true);return p;}
};
juce::var object(){return juce::var(new juce::DynamicObject);}
void put(juce::var& o,const juce::Identifier& k,const juce::var& v){o.getDynamicObject()->setProperty(k,v);}
void save(const juce::File& file,const juce::var& value)
{file.getParentDirectory().createDirectory();if(!file.replaceWithText(juce::JSON::toString(value)))throw std::runtime_error("Cannot save receipt");}
double db(double x){return 20*std::log10(std::max(1.e-12,x));}
juce::String pcmHash(const juce::AudioBuffer<float>& x)
{juce::MemoryBlock bytes;for(int c=0;c<x.getNumChannels();++c)bytes.append(x.getReadPointer(c),static_cast<size_t>(x.getNumSamples())*sizeof(float));return juce::SHA256(bytes).toHexString();}
struct Metrics {double lufs=-120,peak=-120,truePeak=-120,rms=-120,side=-120,correlation=1,monoFoldDb=0,lowCorrelation=1,lowMonoFoldDb=0,lowRms=-120;bool finite=true;};
Metrics metrics(const juce::AudioBuffer<float>& x)
{
    hpeq::LoudnessMeter meter;meter.prepare(fs,false);meter.process(x.getArrayOfReadPointers(),2,x.getNumSamples());
    Metrics m;m.lufs=meter.integratedLufs();double energy=0,side=0,ll=0,rr=0,lr=0,peak=0;
    double loLL=0,loRR=0,loLR=0,l1=0,l2=0,r1=0,r2=0;const double lowAlpha=1-std::exp(-2*juce::MathConstants<double>::pi*120/fs);
    for(int i=0;i<x.getNumSamples();++i)
    {
        const double l=x.getSample(0,i),r=x.getSample(1,i);m.finite=m.finite&&std::isfinite(l)&&std::isfinite(r);
        energy+=l*l+r*r;side+=(l-r)*(l-r)*.25;ll+=l*l;rr+=r*r;lr+=l*r;peak=std::max({peak,std::abs(l),std::abs(r)});
        l1+=lowAlpha*(l-l1);l2+=lowAlpha*(l1-l2);r1+=lowAlpha*(r-r1);r2+=lowAlpha*(r1-r2);
        loLL+=l2*l2;loRR+=r2*r2;loLR+=l2*r2;
    }
    m.peak=db(peak);m.rms=db(std::sqrt(energy/(2*x.getNumSamples())));m.side=db(std::sqrt(side/x.getNumSamples()));
    m.correlation=ll*rr>1.e-24?lr/std::sqrt(ll*rr):1;
    m.monoFoldDb=db(std::sqrt(std::max(1.e-24,(ll+rr+2*lr)*.25)/std::max(1.e-24,energy*.5)));
    m.lowCorrelation=loLL*loRR>1.e-24?loLR/std::sqrt(loLL*loRR):1;
    m.lowRms=db(std::sqrt((loLL+loRR)/(2*x.getNumSamples())));
    m.lowMonoFoldDb=db(std::sqrt(std::max(1.e-24,(loLL+loRR+2*loLR)*.5)/std::max(1.e-24,loLL+loRR)));
    juce::dsp::Oversampling<float> up(2,3,juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple,true);
    up.initProcessing(512);
    for(int start=0;start<x.getNumSamples();start+=512)
    {
        const auto n=std::min(512,x.getNumSamples()-start);
        juce::dsp::AudioBlock<const float> block(x);auto high=up.processSamplesUp(block.getSubBlock(static_cast<size_t>(start),static_cast<size_t>(n)));
        for(size_t c=0;c<high.getNumChannels();++c)for(size_t i=0;i<high.getNumSamples();++i)peak=std::max(peak,static_cast<double>(std::abs(high.getSample(static_cast<int>(c),static_cast<int>(i)))));
    }
    m.truePeak=db(peak);return m;
}
juce::var data(const Metrics& m)
{auto v=object();put(v,"lufs",m.lufs);put(v,"rms_dbfs",m.rms);put(v,"sample_peak_dbfs",m.peak);put(v,"true_peak_8x_dbtp",m.truePeak);put(v,"side_rms_dbfs",m.side);put(v,"correlation",m.correlation);put(v,"mono_fold_db",m.monoFoldDb);put(v,"low_120hz_rms_dbfs",m.lowRms);put(v,"low_120hz_correlation",m.lowCorrelation);put(v,"low_120hz_mono_fold_db",m.lowMonoFoldDb);put(v,"finite",m.finite);return v;}
juce::AudioBuffer<float> fixture(const hpeq::rack::Recipe& recipe,int variant,double bpm)
{
    if(materialMode)
    {
        const auto file=materialFolder.getChildFile(recipe.category.replaceCharacter(' ','_')+"-"+juce::String(variant)+".wav");
        juce::AudioFormatManager formats;formats.registerBasicFormats();
        auto reader=std::unique_ptr<juce::AudioFormatReader>(formats.createReaderFor(file));
        if(!reader||reader->sampleRate!=fs||reader->numChannels!=2||reader->lengthInSamples<fs*3)
            throw std::runtime_error("Missing/invalid 48 kHz stereo material: "+file.getFullPathName().toStdString());
        juce::AudioBuffer<float> x(2,static_cast<int>(std::min<juce::int64>(reader->lengthInSamples,fs*8)));
        if(!reader->read(&x,0,x.getNumSamples(),0,true,true))throw std::runtime_error("Material read failed");
        double energy=0,peak=0;
        for(int c=0;c<2;++c)for(int i=0;i<x.getNumSamples();++i){const double v=x.getSample(c,i);energy+=v*v;peak=std::max(peak,std::abs(v));}
        if(energy<1.e-12)throw std::runtime_error("Material is silent");
        const double target=static_cast<double>(recipe.data["calibration"]["reference_input_dbfs"]);
        const double gain=std::min(std::pow(10.,target/20)/std::sqrt(energy/(2*x.getNumSamples())),std::pow(10.,-9./20)/peak);
        x.applyGain(static_cast<float>(gain));return x;
    }
    const int length=static_cast<int>(std::max(3.0,8*60/bpm)*fs);
    juce::AudioBuffer<float> x(2,length);std::uint32_t random=0x5193ab17u+static_cast<unsigned>(variant)*31;
    const auto category=recipe.category.toLowerCase();double previous=0,energy=0,peak=0;
    const double pi=juce::MathConstants<double>::pi;
    for(int i=0;i<length;++i)
    {
        const double t=i/fs,beat=t*bpm/60,quarter=std::fmod(beat,1.),half=std::fmod(beat,2.);
        random=random*1664525u+1013904223u;const double noise=(static_cast<double>(random>>8)/8388608)-1;
        const double kick=std::sin(2*pi*((variant?57:48)*quarter*60/bpm+7*(1-std::exp(-quarter*18))))*std::exp(-quarter*(variant?12:17));
        const double snare=half>1?(.6*noise+.4*std::sin(2*pi*(variant?190:165)*t))*std::exp(-(half-1)*22):0;
        const double hat=(noise-previous)*std::exp(-std::fmod(beat*4,1.)*35);previous=noise;
        const double gate=std::fmod(beat*.5,1.)<.8?1:.03;
        const double bass=gate*(std::sin(2*pi*(variant?73.416:55)*t)+.15*std::sin(2*pi*(variant?146.832:110)*t));
        const double pitch=(variant?196:146.832)*(1+.006*std::sin(2*pi*5.2*t));
        const double vocal=gate*(.5+.5*std::sin(2*pi*1.7*t))*(std::sin(2*pi*pitch*t)+.3*std::sin(2*pi*pitch*3*t)+.12*noise);
        const double chord=(std::sin(2*pi*(variant?293.665:261.626)*t)+.5*std::sin(2*pi*(variant?440:391.995)*t)+.25*std::sin(2*pi*(variant?587.33:523.252)*t))*std::exp(-quarter*(variant?2:5));
        double left=0,right=0;
        if(category=="kick")left=kick;
        else if(category.contains("snare"))left=snare;
        else if(category.contains("hat"))left=hat;
        else if(category=="percussion")left=.6*hat+.4*snare;
        else if(category=="bass")left=bass;
        else if(category.contains("vocal"))left=vocal;
        else if(category=="keys"||category=="guitar"||category=="synth"||category=="samples")left=chord;
        else if(category=="drum bus")left=.65*kick+.4*snare+.12*hat;
        else left=.45*kick+.24*snare+.08*hat+.25*bass+.15*vocal+.15*chord;
        right=left;
        if(category=="master"||category=="mix bus"||category=="keys"||category=="samples"||category=="synth")right=.92*left+.08*std::sin(2*pi*(variant?349.228:329.628)*t)*gate;
        const double fade=std::min({1.,i/(fs*.01),(length-i-1)/(fs*.01)});
        left*=fade;right*=fade;x.setSample(0,i,static_cast<float>(left));x.setSample(1,i,static_cast<float>(right));
        energy+=left*left+right*right;peak=std::max({peak,std::abs(left),std::abs(right)});
    }
    const double target=static_cast<double>(recipe.data["calibration"]["reference_input_dbfs"]);
    const double gain=std::min(std::pow(10.,target/20)/std::sqrt(energy/(2*length)),std::pow(10.,-9./20)/std::max(1.e-12,peak));
    x.applyGain(static_cast<float>(gain));return x;
}
struct Render {juce::AudioBuffer<float> audio;double comp=0,limiter=0;std::map<int,double> reductions;};
Render render(const hpeq::FxProjectState& p,const juce::AudioBuffer<float>& input,double bpm)
{
    auto processor=std::make_unique<hpeq::FxPluginProcessor>();
    if(!processor->setProject(p))throw std::runtime_error(processor->getProjectError().toStdString());
    processor->prepareToPlay(fs,256);Head head;head.bpm=bpm;processor->setPlayHead(&head);
    const int latency=processor->getLatencySamples(),length=input.getNumSamples()+latency;
    juce::AudioBuffer<float> result(2,length),block(2,127);juce::MidiBuffer midi;
    Render out;
    std::vector<std::pair<int,hpeq::native::Runtime*>> runtimes;
    for(int layer=0;layer<hpeq::kNumLayers;++layer)for(int node=0;node<9;++node)for(int slot=0;slot<6;++slot)
        if(auto* runtime=processor->getEngine().nativeRuntime(layer,node,slot))runtimes.push_back({(layer*9+node)*6+slot,runtime});
    for(int start=0;start<length;start+=127)
    {
        const int n=std::min(127,length-start);block.setSize(2,n,false,false,true);block.clear();
        const int available=std::min(n,input.getNumSamples()-start);
        if(available>0)for(int c=0;c<2;++c)block.copyFrom(c,0,input,c,start,available);
        head.sample=start;processor->processBlock(block,midi);
        for(int c=0;c<2;++c)result.copyFrom(c,start,block,c,0,n);
        for(auto [key,runtime]:runtimes)
            {if(runtime->kind()==hpeq::FxType::Compressor)out.comp=std::max(out.comp,static_cast<double>(runtime->meters().reduction));
             if(runtime->kind()==hpeq::FxType::Limiter)out.limiter=std::max(out.limiter,static_cast<double>(runtime->meters().reduction));
             out.reductions[key]=std::max(out.reductions[key],static_cast<double>(runtime->meters().reduction));}
    }
    out.audio.setSize(2,input.getNumSamples());for(int c=0;c<2;++c)out.audio.copyFrom(c,0,result,c,latency,input.getNumSamples());
    processor->setPlayHead(nullptr);return out;
}
void learnFader(hpeq::FxProjectState& p,const juce::AudioBuffer<float>& source,double bpm)
{
    std::vector<int> slots;
    for(size_t i=0;i<p.layers[0].mixChain.size();++i)
        if(p.layers[0].mixChain[i].type==hpeq::FxType::Gain&&static_cast<int>(p.layers[0].mixChain[i].parameters["mode"])==1)slots.push_back(static_cast<int>(i));
    if(slots.empty())return;
    auto processor=std::make_unique<hpeq::FxPluginProcessor>();
    if(!processor->setProject(p))throw std::runtime_error("Match project rejected");processor->prepareToPlay(fs,256);
    Head head;head.bpm=bpm;processor->setPlayHead(&head);juce::MidiBuffer midi;juce::AudioBuffer<float> block(2,256);
    for(int slot:slots)if(!processor->nativeFaderAction(0,8,slot,0))throw std::runtime_error("Match Learn rejected");
    for(int start=0;start<source.getNumSamples();start+=256)
    {
        const int n=std::min(256,source.getNumSamples()-start);block.setSize(2,n,false,false,true);
        for(int c=0;c<2;++c)block.copyFrom(c,0,source,c,start,n);head.sample=start;processor->processBlock(block,midi);
    }
    for(int slot:slots)if(!processor->nativeFaderAction(0,8,slot,1))throw std::runtime_error("Match Stop rejected");
    block.setSize(2,256,false,false,true);block.clear();head.sample=source.getNumSamples();processor->processBlock(block,midi);
    for(int slot:slots)if(!processor->nativeFaderAction(0,8,slot,3))throw std::runtime_error("Match needs valid active audio; no unmeasured gain was applied");
    p=processor->getProject();processor->setPlayHead(nullptr);
}
void wave(const juce::File& file,const juce::AudioBuffer<float>& audio)
{
    file.getParentDirectory().createDirectory();auto stream=file.createOutputStream();juce::WavAudioFormat format;
    if(!stream||!stream->setPosition(0)||stream->truncate().failed())throw std::runtime_error("Cannot replace audition wave");
    auto writer=std::unique_ptr<juce::AudioFormatWriter>(format.createWriterFor(stream.get(),fs,2,24,{},0));
    if(!writer)throw std::runtime_error("Cannot create audition wave");stream.release();if(!writer->writeFromAudioSampleBuffer(audio,0,audio.getNumSamples()))throw std::runtime_error("Cannot write audition");
}
struct Slot {int layer,node,index;hpeq::FxSlotState* state;int key() const{return (layer*9+node)*6+index;}};
std::vector<Slot> slots(hpeq::FxProjectState& p)
{
    std::vector<Slot> result;
    for(int l=0;l<hpeq::kNumLayers;++l)
    {
        auto add=[&](int node,std::vector<hpeq::FxSlotState>& chain){for(int k=0;k<static_cast<int>(chain.size());++k)result.push_back({l,node,k,&chain[static_cast<size_t>(k)]});};
        auto& layer=p.layers[static_cast<size_t>(l)];add(8,layer.mixChain);
        for(auto& n:layer.fxNodes)add(n.engineSlot,n.chain);for(auto& n:layer.eqNodes)add(n.engineSlot,n.chain);
    }
    return result;
}
juce::var calibrationData(hpeq::FxProjectState& p,hpeq::FxProjectState& original)
{
    auto value=object();put(value,"input_trim_db",p.inputTrimDb);put(value,"output_trim_db",p.outputTrimDb);
    const auto old=slots(original);juce::Array<juce::var> overrides;
    for(const auto& slot:slots(p))
    {
        const auto before=std::find_if(old.begin(),old.end(),[&](const Slot& s){return s.key()==slot.key();});
        if(before==old.end())throw std::runtime_error("Calibration changed topology");
        auto parameters=object();bool changed=false;
        for(const auto& parameter:slot.state->parameters)
            if(parameter.value!=before->state->parameters[parameter.name]){put(parameters,parameter.name,parameter.value);changed=true;}
        if(changed){auto o=object();put(o,"layer",slot.layer);put(o,"node",slot.node);put(o,"slot",slot.index);put(o,"parameters",parameters);overrides.add(o);}
    }
    put(value,"overrides",overrides);return value;
}
const std::vector<hpeq::rack::Recipe>& taskRecipes()
{
    static const auto bank=[] {
        std::vector<hpeq::rack::Recipe> result;
        for(const auto& recipe:hpeq::rack::recipes())if(recipe.id.startsWith("HR"))result.push_back(recipe);
        return result;
    }();
    return bank;
}
int calibrate(const juce::File& folder,int first,int count)
{
    const auto& bank=taskRecipes();int failures=0;
    for(int i=first;i<std::min(first+count,static_cast<int>(bank.size()));++i)
    {
        const auto& recipe=bank[static_cast<size_t>(i)];const auto file=folder.getChildFile("records/"+recipe.id+".json");
        hpeq::FxProjectState p;juce::String error;if(!hpeq::rack::makeProject(recipe,p,error,false))throw std::runtime_error(error.toStdString());
        auto original=p;auto source=fixture(recipe,0,recipe.bpm);
        const auto in=metrics(source);auto firstRender=render(p,source,recipe.bpm);auto initial=metrics(firstRender.audio);
        // Method v1: individual canonical-clip compressor targets. Parallel
        // heavy (amount < .75, ratio >= 4) aims at 8 dB; ratio >= 3 aims at
        // 4.5 dB; the remaining gentle controls aim at 2 dB. A5 bands stay fixed.
        juce::Array<juce::var> dynamics;
        for(const auto& s:slots(p))if(s.state->type==hpeq::FxType::Compressor)
        {
            const double ratio=s.state->parameters["ratio"];
            const double target=s.state->mix<.75&&ratio>=4?8:ratio>=3?4.5:2;
            const double lower=target==8?6:target==4.5?3:1,upper=target==8?12:target==4.5?6:3;
            double best=s.state->parameters["thresholdDb"],distance=1.e9,achieved=0;
            for(int iteration=0;iteration<6;++iteration)
            {
                const auto r=render(p,source,recipe.bpm);const double gr=r.reductions.at(s.key());
                if(std::abs(gr-target)<distance){distance=std::abs(gr-target);best=s.state->parameters["thresholdDb"];achieved=gr;}
                if(gr>=lower&&gr<=upper)break;
                const double threshold=s.state->parameters["thresholdDb"];
                const double next=juce::jlimit(-60.,0.,threshold+juce::jlimit(-12.,12.,(gr-target)/std::max(.1,1-1/ratio)));
                if(std::abs(next-threshold)<.01)break;s.state->parameters.set("thresholdDb",next);
            }
            s.state->parameters.set("thresholdDb",best);
            auto d=object();put(d,"slot_key",s.key());put(d,"target_gr_db",target);put(d,"achieved_gr_db",achieved);put(d,"pass",achieved>=lower&&achieved<=upper);dynamics.add(d);
        }
        learnFader(p,source,recipe.bpm);
        const auto tuned=render(p,source,recipe.bpm);const auto tunedMetrics=metrics(tuned.audio);
        double trim=juce::jlimit(-48.,24.,in.lufs-tunedMetrics.lufs);
        if(p.masterRoute)
        {
            // Master trim belongs before the final limiter; post-ceiling gain is
            // never used to satisfy loudness. Re-measure the nonlinear result.
            p.inputTrimDb=trim;
            for(int iteration=0;iteration<4;++iteration)
            {
                auto test=render(p,source,recipe.bpm);const auto m=metrics(test.audio);
                double worst=test.limiter;
                for(int variant=0;variant<2;++variant){auto hot=fixture(recipe,variant,recipe.bpm);hot.applyGain(std::pow(10.f,6.f/20));worst=std::max(worst,render(p,hot,recipe.bpm).limiter);}
                if(worst>3)p.inputTrimDb-=worst-3+.05;
                else if(std::abs(m.lufs-in.lufs)>.1)p.inputTrimDb+=juce::jlimit(-3.,3.,in.lufs-m.lufs);
                else break;
            }
            // Preserve the GR limit on the hot independent clip, even when the
            // equal-loudness target cannot then be reached.
            double worst=0;
            for(int variant=0;variant<2;++variant){auto hot=fixture(recipe,variant,recipe.bpm);hot.applyGain(std::pow(10.f,6.f/20));worst=std::max(worst,render(p,hot,recipe.bpm).limiter);}
            if(worst>3)p.inputTrimDb-=worst-3+.05;
        }
        else if(!p.wetOnly)
        {
            for(int variant=0;variant<2;++variant)for(double level:{-6.,0.,6.})
            {
                auto x=fixture(recipe,variant,recipe.bpm);x.applyGain(static_cast<float>(std::pow(10.,level/20)));
                trim=std::min(trim,metrics(x).peak+3-.001-metrics(render(p,x,recipe.bpm).audio).truePeak);
            }
            p.outputTrimDb=trim;
        }
        auto row=object();put(row,"id",recipe.id);put(row,"name",recipe.name);
        put(row,"bank_sha256",hpeq::rack::bankSha256());
        put(row,"fixture_method",materialMode?"rack-material-v2; two category-specific local recordings/sample arrangements; owner listening pending":"rack-fixtures-v1; category-specific synthetic, two variants; owner listening pending");
        if(materialMode)put(row,"material_manifest_sha256",juce::SHA256(materialFolder.getChildFile("sources.json")).toHexString());
        put(row,"calibration",calibrationData(p,original));put(row,"dynamics",dynamics);put(row,"initial_input",data(in));put(row,"initial_output",data(initial));
        put(row,"executable_sha256",juce::SHA256(juce::File::getSpecialLocation(juce::File::currentExecutableFile)).toHexString());
        put(row,"definition_sha256",juce::SHA256(juce::JSON::toString(recipe.data).toRawUTF8(),static_cast<size_t>(juce::JSON::toString(recipe.data).getNumBytesAsUTF8())).toHexString());
        juce::Array<juce::var> checks;bool passed=true;
        for(int variant=0;variant<2;++variant)for(double level:{-6.,0.,6.})
        {
            auto x=fixture(recipe,variant,recipe.bpm);x.applyGain(static_cast<float>(std::pow(10.,level/20)));
            const auto input=metrics(x);const auto result=render(p,x,recipe.bpm);const auto output=metrics(result.audio);
            auto check=object();put(check,"variant",variant);put(check,"level_offset_db",level);put(check,"bpm",recipe.bpm);
            put(check,"input",data(input));put(check,"output",data(output));put(check,"compressor_peak_gr_db",result.comp);put(check,"limiter_peak_gr_db",result.limiter);
            put(check,"input_pcm_sha256",pcmHash(x));put(check,"output_pcm_sha256",pcmHash(result.audio));
            const bool finite=output.finite&&output.peak> -110&&input.peak<=-2.999;
            const bool ceiling=!p.masterRoute||output.truePeak<=-1;
            const bool peak=p.wetOnly||p.masterRoute||output.truePeak<=input.peak+3;
            put(check,"finite_active",finite);put(check,"ceiling_pass",ceiling);put(check,"peak_growth_pass",peak);
            put(check,"loudness_delta_lu",output.lufs-input.lufs);
            put(check,"loudness_match_pass",std::abs(output.lufs-input.lufs)<=.5);
            put(check,"master_gr_pass",!p.masterRoute||result.limiter<=3);
            passed=passed&&finite&&ceiling&&peak&&(!p.masterRoute||result.limiter<=3);
            checks.add(check);
            if(variant==0&&level==0)
            {
                wave(folder.getChildFile("audio/"+recipe.id+"-dry.wav"),x);
                wave(folder.getChildFile("audio/"+recipe.id+"-processed.wav"),result.audio);
                auto matched=result.audio;matched.applyGain(static_cast<float>(std::pow(10.,(input.lufs-output.lufs)/20)));
                wave(folder.getChildFile("audio/"+recipe.id+"-matched.wav"),matched);
                put(row,"input_pcm_sha256",pcmHash(x));put(row,"output_pcm_sha256",pcmHash(result.audio));
            }
        }
        for(double multiplier:{.8,1.2})
        {
            auto x=fixture(recipe,1,recipe.bpm*multiplier);const auto result=render(p,x,recipe.bpm*multiplier);const auto output=metrics(result.audio);
            auto check=object();put(check,"bpm",recipe.bpm*multiplier);put(check,"output",data(output));
            const bool safe=output.finite&&output.peak> -110&&(!p.masterRoute||output.truePeak<=-1);
            put(check,"safety_pass",safe);checks.add(check);passed=passed&&safe;
        }
        put(row,"checks",checks);put(row,"safety_pass",passed);put(row,"intent_verdict","owner audition pending; numerical metrics do not certify intent");
        save(file,row);if(!passed)++failures;
        std::cout<<recipe.id<<" "<<(passed?"safe":"FAIL")<<" "<<recipe.name<<std::endl;
    }
    return failures?1:0;
}
int powerBank(const juce::File& folder,bool verifyOnly=false)
{
    // Rack Power method v1, fixed before its first render: preserve every DSP
    // parameter; tune output trim only. Two independent local clips, -6/0/+6 dB,
    // true peak <= -0.3 dBTP, finite active output, plus +/-20% tempo and an
    // eight-second silent input tail. Numerical character is not a listening verdict.
    juce::Array<juce::var> records;int failures=0;
    for(const auto& recipe:hpeq::rack::recipes())if(recipe.id.startsWith("PW"))
    {
        hpeq::FxProjectState p;juce::String error;
        if(!hpeq::rack::makeProject(recipe,p,error,false))throw std::runtime_error(error.toStdString());
        if(verifyOnly)
        {
            const auto row=juce::JSON::parse(folder.getChildFile("records/"+recipe.id+".json"));
            const auto source=fixture(recipe,0,recipe.bpm);const auto actual=render(p,source,recipe.bpm);
            const bool exact=std::abs(p.outputTrimDb-static_cast<double>(row["output_trim_db"]))<1.e-9
                &&pcmHash(source)==row["checks"][1]["input_pcm_sha256"].toString()
                &&pcmHash(actual.audio)==row["checks"][1]["output_pcm_sha256"].toString();
            if(!exact)++failures;std::cout<<recipe.id<<(exact?" exact":" FAIL")<<std::endl;continue;
        }
        auto x=fixture(recipe,0,recipe.bpm);const auto input=metrics(x);
        const auto first=render(p,x,recipe.bpm);const auto initial=metrics(first.audio);
        double trim=p.outputTrimDb+input.lufs-initial.lufs;
        for(int variant=0;variant<2;++variant)for(double level:{-6.,0.,6.})
        {
            auto hot=fixture(recipe,variant,recipe.bpm);hot.applyGain(static_cast<float>(std::pow(10.,level/20)));
            const auto m=metrics(render(p,hot,recipe.bpm).audio);
            trim=std::min(trim,p.outputTrimDb-.35-m.truePeak);
        }
        p.outputTrimDb=juce::jlimit(-48.,24.,trim);
        auto row=object();put(row,"id",recipe.id);put(row,"name",recipe.name);put(row,"output_trim_db",p.outputTrimDb);
        put(row,"input",data(input));put(row,"initial_output",data(initial));
        put(row,"definition_sha256",juce::SHA256(juce::JSON::toString(recipe.data).toRawUTF8(),static_cast<size_t>(juce::JSON::toString(recipe.data).getNumBytesAsUTF8())).toHexString());
        juce::Array<juce::var> checks;bool safe=true;
        for(int variant=0;variant<2;++variant)for(double level:{-6.,0.,6.})
        {
            auto source=fixture(recipe,variant,recipe.bpm);source.applyGain(static_cast<float>(std::pow(10.,level/20)));
            const auto result=render(p,source,recipe.bpm);const auto m=metrics(result.audio);
            const bool pass=m.finite&&m.peak> -110&&m.truePeak<=-.3;
            auto check=object();put(check,"variant",variant);put(check,"level_db",level);put(check,"output",data(m));
            put(check,"compressor_peak_gr_db",result.comp);put(check,"limiter_peak_gr_db",result.limiter);
            put(check,"input_pcm_sha256",pcmHash(source));put(check,"output_pcm_sha256",pcmHash(result.audio));put(check,"pass",pass);
            checks.add(check);safe=safe&&pass;
            if(variant==0&&level==0)
            {
                wave(folder.getChildFile("audio/"+recipe.id+"-dry.wav"),source);
                wave(folder.getChildFile("audio/"+recipe.id+"-processed.wav"),result.audio);
                auto matched=result.audio;
                const double matchDb=input.lufs-m.lufs;
                const double common=std::min(0.,-1.5-std::max(input.truePeak,m.truePeak+matchDb));
                auto dryMatched=source;dryMatched.applyGain(static_cast<float>(std::pow(10.,common/20)));
                matched.applyGain(static_cast<float>(std::pow(10.,(matchDb+common)/20)));
                wave(folder.getChildFile("audio/"+recipe.id+"-matched-dry.wav"),dryMatched);
                wave(folder.getChildFile("audio/"+recipe.id+"-matched.wav"),matched);
                double difference=0,dryEnergy=0;
                for(int c=0;c<2;++c)for(int i=0;i<source.getNumSamples();++i)
                {const double d=source.getSample(c,i),w=result.audio.getSample(c,i)*std::pow(10.,matchDb/20);difference+=(w-d)*(w-d);dryEnergy+=d*d;}
                put(row,"loudness_delta_lu",m.lufs-input.lufs);put(row,"matched_difference_db",10*std::log10(std::max(1.e-24,difference/std::max(1.e-24,dryEnergy))));
            }
        }
        for(double multiplier:{.8,1.2})
        {
            auto source=fixture(recipe,1,recipe.bpm);const auto m=metrics(render(p,source,recipe.bpm*multiplier).audio);
            const bool pass=m.finite&&m.peak> -110&&m.truePeak<=-.3;
            auto check=object();put(check,"bpm",recipe.bpm*multiplier);put(check,"output",data(m));put(check,"pass",pass);checks.add(check);safe=safe&&pass;
        }
        auto tail=fixture(recipe,0,recipe.bpm);const int active=tail.getNumSamples();tail.setSize(2,active+static_cast<int>(fs*8),true,true);
        const auto tailResult=render(p,tail,recipe.bpm);const auto tailMetrics=metrics(tailResult.audio);
        double tailEnd=0;
        for(int c=0;c<2;++c)for(int i=tailResult.audio.getNumSamples()-static_cast<int>(fs);i<tailResult.audio.getNumSamples();++i)
            tailEnd=std::max(tailEnd,static_cast<double>(std::abs(tailResult.audio.getSample(c,i))));
        const bool tailPass=tailMetrics.finite&&tailMetrics.truePeak<=-.3&&db(tailEnd)<=-45;
        put(row,"tail_render",data(tailMetrics));put(row,"tail_final_second_peak_dbfs",db(tailEnd));put(row,"tail_pass",tailPass);safe=safe&&tailPass;
        put(row,"checks",checks);put(row,"safety_pass",safe);put(row,"intent_verdict","Owner listening pending; numerical difference does not certify musical intent");
        save(folder.getChildFile("records/"+recipe.id+".json"),row);records.add(row);if(!safe)++failures;
        std::cout<<recipe.id<<" "<<(safe?"safe":"FAIL")<<" "<<recipe.name<<std::endl;
    }
    if(verifyOnly)return failures?1:0;
    auto receipt=object();put(receipt,"format","HamloRackPowerVerification");put(receipt,"version",1);
    put(receipt,"material_manifest_sha256",juce::SHA256(materialFolder.getChildFile("sources.json")).toHexString());
    put(receipt,"executable_sha256",juce::SHA256(juce::File::getSpecialLocation(juce::File::currentExecutableFile)).toHexString());
    int size=0;const auto* bytes=HpeqAssets::getNamedResource("power_json",size);
    const auto definition=juce::JSON::parse(juce::String::fromUTF8(bytes,size));
    put(receipt,"authoring_sha256",definition["authoring_sha256"]);put(receipt,"records",records);put(receipt,"failures",failures);
    save(folder.getChildFile("receipt.json"),receipt);return failures?1:0;
}
int browserSnapshots(const juce::File& folder)
{
    folder.createDirectory();auto processor=std::make_unique<hpeq::FxPluginProcessor>();
    auto editor=std::make_unique<hpeq::FxPluginEditor>(*processor);editor->showPresetBrowser();
    hpeq::ui::PresetBrowser* browser=nullptr;
    for(auto* child:editor->getChildren())if(auto* panel=dynamic_cast<hpeq::ui::PresetBrowser*>(child))browser=panel;
    if(!browser)return 1;
    const auto capture=[&](const juce::String& name){
        auto image=editor->createComponentSnapshot(editor->getLocalBounds());
        auto stream=folder.getChildFile(name+".png").createOutputStream();juce::PNGImageFormat png;
        return stream&&stream->setPosition(0)&&stream->truncate().wasOk()&&png.writeImageToStream(image,*stream);
    };
    for(auto size:{juce::Point<int>{1000,700},juce::Point<int>{1280,840},juce::Point<int>{1920,1200}})
    {editor->setSize(size.x,size.y);if(!capture("library-"+juce::String(size.x)))return 1;}
    editor->setSize(1280,840);hpeq::rack::BrowserFilter filter;filter.collection="Rack Power";filter.genre="Trap";filter.character="Aggressive";
    browser->setFilter(filter);if(!capture("filtered"))return 1;
    browser->loadSelected();if(!capture("loaded"))return 1;
    filter.query="no such preset";browser->setFilter(filter);if(!capture("empty"))return 1;
    return 0;
}
int snapshots(const juce::File& folder)
{
    folder.createDirectory();
    for(auto type:{hpeq::FxType::Compressor,hpeq::FxType::Limiter,hpeq::FxType::Reverb,hpeq::FxType::Delay,hpeq::FxType::Gain,hpeq::FxType::Saturator,
        hpeq::FxType::TransientShaper,hpeq::FxType::StereoProcessor,hpeq::FxType::GateExpander,hpeq::FxType::MonstaFx})
    {
        auto p=std::make_unique<hpeq::FxPluginProcessor>();hpeq::FxProjectState project;
        project.layers[0].mixChain={hpeq::makeNativeModule(type)};if(!p->setProject(project))return 1;p->prepareToPlay(fs,256);
        auto editor=std::make_unique<hpeq::FxPluginEditor>(*p);editor->selectMix(0);
        for(const auto size:{juce::Point<int>{1000,700},juce::Point<int>{1280,840},juce::Point<int>{1920,1200}})
        {
        editor->setSize(size.x,size.y);
        auto image=editor->createComponentSnapshot(editor->getLocalBounds());
        auto stream=folder.getChildFile(hpeq::native::productName(type)+"-"+juce::String(size.x)+".png").createOutputStream();juce::PNGImageFormat png;
        if(!stream||!stream->setPosition(0)||stream->truncate().failed()||!png.writeImageToStream(image,*stream))return 1;
        }
    }
    return 0;
}
int verify(const juce::File& folder,int first=0,int count=250)
{
    int failures=0,verified=0;const auto& bank=taskRecipes();
    if(first<0||count<1||first>=static_cast<int>(bank.size()))throw std::runtime_error("Invalid replay range");
    for(int index=first;index<std::min(first+count,static_cast<int>(bank.size()));++index)
    {
        const auto& recipe=bank[static_cast<size_t>(index)];++verified;
        const auto row=juce::JSON::parse(folder.getChildFile("records/"+recipe.id+".json"));
        hpeq::FxProjectState p;juce::String error;
        if(row["bank_sha256"].toString()!=hpeq::rack::bankSha256()||!hpeq::rack::makeProject(recipe,p,error))
            throw std::runtime_error("Stale/missing calibration: "+recipe.id.toStdString()+" "+error.toStdString());
        const auto x=fixture(recipe,0,recipe.bpm);const auto result=render(p,x,recipe.bpm);
        const bool exact=pcmHash(x)==row["input_pcm_sha256"].toString()&&pcmHash(result.audio)==row["output_pcm_sha256"].toString();
        if(!exact)++failures;
        std::cout<<recipe.id<<(exact?" exact":" FAIL")<<std::endl;
    }
    std::cout<<"Embedded bank replay: "<<verified-failures<<" exact, "<<failures<<" failed\n";return failures?1:0;
}
}
int main(int argc,char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    try
    {
        if(argc==4&&(juce::String(argv[1])=="power"||juce::String(argv[1])=="power-verify"))
        {materialMode=true;materialFolder=juce::File(argv[3]);return powerBank(juce::File(argv[2]),juce::String(argv[1])=="power-verify");}
        if(argc==3&&juce::String(argv[1])=="snapshots")return snapshots(juce::File(argv[2]));
        if(argc==3&&juce::String(argv[1])=="browser-snapshots")return browserSnapshots(juce::File(argv[2]));
        if((argc==4||argc==6)&&juce::String(argv[1])=="verify")
        {materialMode=true;materialFolder=juce::File(argv[3]);return verify(juce::File(argv[2]),argc==6?std::stoi(argv[4]):0,argc==6?std::stoi(argv[5]):250);}
        if((argc==5||argc==6)&&juce::String(argv[1])=="calibrate")
        {if(argc==6){materialMode=true;materialFolder=juce::File(argv[5]);}return calibrate(juce::File(argv[2]),std::stoi(argv[3]),std::stoi(argv[4]));}
        std::cerr<<"HPEQ_RackBank snapshots <directory> | calibrate <directory> <first index> <count> [material directory] | verify <directory> <material directory> [first index count]\n";return 2;
    }
    catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}
}

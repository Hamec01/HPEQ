#include "DSP/ProjectEngine.h"
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_dsp/juce_dsp.h>
#include <juce_cryptography/juce_cryptography.h>
#include <iostream>
#include <complex>

namespace {
juce::var object() { return juce::var(new juce::DynamicObject); }
void put(juce::var& v,const char* k,const juce::var& x) { v.getDynamicObject()->setProperty(k,x); }
hpeq::FxProjectState project(hpeq::LayerRouting route, hpeq::ChannelMode mode,float mix,int count)
{
    hpeq::FxProjectState p;p.routing=route;
    for(auto& l:p.layers)l.enabled=false;
    for(int j=0;j<count;++j) {
        auto& l=p.layers[static_cast<size_t>(j)];l.enabled=true;l.mix=mix;l.channel=mode;
        hpeq::FxNodeState n;n.id=p.nextNodeId++;n.lowHz=j?900:250;n.highHz=j?3600:700;
        n.chain[0].type=hpeq::FxType::Gain;n.chain[0].parameters.set("gainDb",0.0);
        l.fxNodes.push_back(n);
    }
    hpeq::sanitize(p);return p;
}
juce::AudioBuffer<float> render(const hpeq::FxProjectState& p,double fs,int block,const juce::AudioBuffer<float>& input)
{
    auto engine=std::make_unique<hpeq::dsp::ProjectEngine>();engine->prepare(fs,256);
    if(!engine->publish(p))throw std::runtime_error(engine->publicationError().toStdString());
    auto y=input;
    for(int i=0;i<y.getNumSamples();i+=block) {
        float* data[]{y.getWritePointer(0,i),y.getWritePointer(1,i)};
        engine->process(data,2,std::min(block,y.getNumSamples()-i));
    }
    return y;
}
void wave(const juce::File& f,const juce::AudioBuffer<float>& x,double fs)
{
    auto stream=f.createOutputStream();if(!stream)throw std::runtime_error("WAV open failed");
    stream->setPosition(0);stream->truncate();juce::WavAudioFormat format;
    auto writer=std::unique_ptr<juce::AudioFormatWriter>(format.createWriterFor(stream.get(),fs,2,24,{},0));
    if(!writer)throw std::runtime_error("WAV writer failed");stream.release();
    if(!writer->writeFromAudioSampleBuffer(x,0,x.getNumSamples()))throw std::runtime_error("WAV write failed");
}
}
int main(int argc,char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    try {
        if(argc!=2)throw std::runtime_error("HPEQ_PhaseAudit <new output directory>");
        juce::File folder(argv[1]);if(folder.exists())throw std::runtime_error("Output already exists");
        folder.createDirectory();juce::Array<juce::var> rows;int failures=0;
        for(double fs:{44100.,48000.,96000.,192000.}) {
            constexpr int order=16,size=1<<order;
            const int warm=static_cast<int>(fs*.3);
            juce::AudioBuffer<float> impulse(2,warm+size);impulse.clear();
            impulse.setSample(0,warm,.25f);impulse.setSample(1,warm,.25f);
            for(auto route:{hpeq::LayerRouting::Parallel,hpeq::LayerRouting::Serial})
            for(auto mode:{hpeq::ChannelMode::Stereo,hpeq::ChannelMode::Left,hpeq::ChannelMode::Right,hpeq::ChannelMode::Mid,hpeq::ChannelMode::Side})
            for(float mix:{0.f,.25f,.5f,.75f,1.f})for(int count:{1,2}) {
                const auto p=project(route,mode,mix,count);const auto y=render(p,fs,127,impulse);
                double worst=0,stereoError=0;bool finite=true;juce::Array<juce::var> spectra;
                for(int ch=0;ch<2;++ch) {
                    std::vector<std::complex<float>> in(size),out(size);
                    for(int i=0;i<size;++i) {const auto v=y.getSample(ch,warm+i)*4;finite=finite&&std::isfinite(v);in[static_cast<size_t>(i)]={v,0};}
                    juce::dsp::FFT fft(order);fft.perform(in.data(),out.data(),false);
                    for(int bin=1;bin<size/2;++bin) {
                        const double hz=fs*bin/size;if(hz<20||hz>20000)continue;
                        worst=std::max(worst,std::abs(20*std::log10(std::max(1.e-12,static_cast<double>(std::abs(out[static_cast<size_t>(bin)]))))));
                    }
                    if(ch==0)for(double hz:{20.,100.,250.,500.,1000.,3000.,10000.,20000.}) {
                        auto v=object();const auto bin=static_cast<size_t>(std::round(hz*size/fs));
                        put(v,"hz",fs*static_cast<double>(bin)/size);put(v,"magnitude_db",20*std::log10(std::max(1.e-12,static_cast<double>(std::abs(out[bin])))));
                        put(v,"phase_radians",static_cast<double>(std::arg(out[bin])));spectra.add(v);
                    }
                }
                for(int i=warm;i<y.getNumSamples();++i)stereoError=std::max(stereoError,std::abs(static_cast<double>(y.getSample(0,i)-y.getSample(1,i))));
                auto row=object();put(row,"sample_rate",fs);put(row,"routing",route==hpeq::LayerRouting::Parallel?"parallel":"serial");
                put(row,"mode",static_cast<int>(mode));put(row,"mix",mix);put(row,"layers",count);put(row,"magnitude_error_db",worst);
                put(row,"mono_input_channel_difference",stereoError);put(row,"finite",finite);put(row,"spectrum_left",spectra);
                const bool pass=finite&&worst<=.1&&stereoError<=1.e-5;put(row,"pass",pass);if(!pass)++failures;rows.add(row);
            }
        }
        auto root=object();put(root,"format","HamloRackPhaseAudit1");put(root,"cases",rows);put(root,"failed",failures);put(root,"total",rows.size());
        if(!folder.getChildFile("phase.json").replaceWithText(juce::JSON::toString(root)))throw std::runtime_error("Receipt write failed");
        juce::AudioBuffer<float> x(2,144000);juce::Random random(0x481105);
        for(int i=0;i<x.getNumSamples();++i) {
            const double t=i/48000.,beat=std::fmod(t,.5);const auto v=static_cast<float>(.12*std::sin(2*juce::MathConstants<double>::pi*63*t)*std::exp(-beat*25)+.035*random.nextFloat()*std::exp(-beat*70));
            x.setSample(0,i,v);x.setSample(1,i,v*.9f);
        }
        wave(folder.getChildFile("input.wav"),x,48000);
        wave(folder.getChildFile("parallel-half.wav"),render(project(hpeq::LayerRouting::Parallel,hpeq::ChannelMode::Stereo,.5f,2),48000,127,x),48000);
        std::cout<<rows.size()<<" cases, "<<failures<<" failed\n";return failures?1:0;
    }catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 2;}
}

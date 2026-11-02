#include "DSP/ScoreEngine.h"
#include "SourceIdentity.h"
#include <chrono>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <string>
#include <filesystem>
#include <immintrin.h>

using namespace hamlo::echo;
constexpr double pi=3.14159265358979323846;
ScoreSettings base()
{
    ScoreSettings s;s.mix=1;s.feedback=0;s.spaceMix=0;
    s.taps[0].enabled=true;s.taps[0].gainDb=0;s.taps[0].timeMs=100;
    return s;
}
void render(const std::string& path,ScoreSettings s,double seconds,int signal)
{
    ScoreEngine engine;engine.prepare(48000,256);engine.start(s);
    std::ofstream out(path,std::ios::binary);float left[256],right[256],interleaved[512];float* p[]{left,right};
    for(int start=0;start<static_cast<int>(48000*seconds);start+=256)
    {
        const int n=std::min(256,static_cast<int>(48000*seconds)-start);
        for(int i=0;i<n;++i)
        {const double t=static_cast<double>(start+i)/48000;double x=0;
            if(signal==0)x=start+i==4800?.25:0;
            if(signal==1&&t>=.1&&t<1.3)x=.1*std::sin(2*pi*440*(t-.1))*std::min({1.,(t-.1)*100,(1.3-t)*100});
            if(signal==2)x=.1*(2*std::fmod(t,.05)/.05-1);
            left[i]=static_cast<float>(x);right[i]=static_cast<float>(.7*x);}
        engine.process(p,2,n);
        for(int i=0;i<n;++i){interleaved[2*i]=left[i];interleaved[2*i+1]=right[i];}out.write(reinterpret_cast<const char*>(interleaved),n*2*sizeof(float));
    }
}
int main(int argc,char** argv)
{
    const std::string dir=argc>1?argv[1]:"out/score";std::filesystem::create_directories(dir);
    _mm_setcsr(_mm_getcsr()|0x8040); // Match the production callback's FTZ/DAZ policy.
    const auto identity=dir+"/dsp_source.sha256";std::ifstream existing(identity);std::string previous;existing>>previous;
    if(!previous.empty()&&previous!=HAMLO_ECHO_SOURCE_ID){std::cerr<<"Refusing stale DSP results in "<<dir<<"\n";return 3;}
    std::ofstream(identity)<<HAMLO_ECHO_SOURCE_ID<<"\n";
    const std::string mode=argc>2?argv[2]:"effects";
    std::cout<<std::setprecision(12);
    if(mode=="effects")
    {
        auto s=base();render(dir+"/unity.f32",s,4,0);
        for(int note:{36,48,55,67,72,84})
        {auto a=s;a.taps[0].note=note;a.taps[0].fragmentMs=100;render(dir+"/combined_forward_"+std::to_string(note)+".f32",a,2,1);a.taps[0].action=Action::Reverse;render(dir+"/combined_reverse_"+std::to_string(note)+".f32",a,2,1);}
        s.bands=true;for(auto& b:s.band)b.feedback=0;render(dir+"/bands_neutral.f32",s,4,0);
        for(int b=0;b<3;++b){auto a=s;for(int k=0;k<3;++k)a.band[k].enabled=k==b;a.band[b].timeScale=.5*(b+1);render(dir+"/band_"+std::to_string(b)+".f32",a,4,0);}
        s=base();s.feedback=.65;s.feedbackMs=1600;
        for(int initial:{0,-12})for(int step:{-12,12}){s.taps[0].note=60+initial;s.feedbackPitch=step;render(dir+"/pitch_"+std::to_string(initial)+"_"+std::to_string(step)+".f32",s,8,1);}
        s=base();s.taps[0].fragmentMs=100;
        for(int action=1;action<=4;++action){s.taps[0].action=static_cast<Action>(action);render(dir+"/action_"+std::to_string(action)+".f32",s,2,2);}
        s=base();s.glitchMs=100;s.glitchAmount=1;s.glitchTarget=0;
        for(int action=1;action<=5;++action){s.glitch=action;render(dir+"/glitch_"+std::to_string(action)+".f32",s,2,2);}
        s=base();s.spaceMix=.8;s.feedback=.35;s.spaceDecay=1.5;
        for(int type=0;type<3;++type)for(int route=0;route<3;++route)
        {s.spaceType=type;s.spaceRoute=route;render(dir+"/space_"+std::to_string(type)+"_"+std::to_string(route)+".f32",s,14,0);}
        std::cout<<"effects rendered\n";return 0;
    }
    if(mode=="timing")
    {
        std::ofstream csv(dir+"/timing.csv");csv<<"rate,sync,requested_samples,measured_samples,max_error\n";
        for(double rate:{44100.,48000.,96000.})for(int sync:{0,5,6,7,8})
        {
            auto s=base();s.bpm=137;s.taps[0].sync=sync;s.taps[0].timeMs=127.35;
            ScoreEngine engine;engine.prepare(rate,256);engine.start(s);float a[256],b[256];float* p[]{a,b};
            const int input=static_cast<int>(rate*.1),expected=input+static_cast<int>(std::lround(timeMs(s.taps[0].timeMs,sync,s.bpm)*.001*rate));int measured=-1;double error=0;
            for(int start=0;start<static_cast<int>(rate*1.1);start+=256)
            {for(int i=0;i<256;++i)a[i]=b[i]=start+i==input?.25f:0;engine.process(p,2,256);
                for(int i=0;i<256;++i){if(std::abs(a[i])>1e-8&&measured<0)measured=start+i;error=std::max(error,std::abs(a[i]-(start+i==expected?.25:0)));}}
            csv<<rate<<","<<sync<<","<<expected-input<<","<<measured-input<<","<<error<<"\n";
        }
        return 0;
    }
    if(mode=="bandstress"||mode=="bandstress_raw")
    {
        std::ofstream csv(dir+"/bandstress.csv");csv<<"second,energy,peak\n";auto s=base();s.bands=true;s.spaceMix=.8;s.spaceRoute=1;s.spaceType=1;s.feedbackMs=100;
        for(int b=0;b<3;++b){s.band[b].feedback=.9;s.band[b].timeScale=b==0?.25:b==1?1.125:4;}
        if(mode=="bandstress_raw"){s.spaceMix=0;s.feedbackMs=500;s.band[0].timeScale=.998;s.band[1].timeScale=1.00047;s.band[2].timeScale=1.00183;}
        ScoreEngine engine;engine.prepare(48000,256);engine.start(s);float a[256],b[256];float* p[]{a,b};
        for(int second=0;second<30;++second)
        {double energy=0,peak=0;for(int start=0;start<48000;start+=256){const int n=std::min(256,48000-start);for(int i=0;i<n;++i)a[i]=b[i]=second==0&&start+i==4800?.25f:0;engine.process(p,2,n);for(int i=0;i<n;++i){energy+=a[i]*a[i]+b[i]*b[i];peak=std::max({peak,std::abs(static_cast<double>(a[i])),std::abs(static_cast<double>(b[i]))});}}
            csv<<second<<","<<energy<<","<<peak<<"\n";}
        return 0;
    }
    if(mode=="freeze")
    {
        auto s=base();s.feedback=.9;s.spaceMix=.8;s.spaceRoute=1;s.bands=true;
        for(auto& b:s.band)b.feedback=.9;
        ScoreEngine engine;engine.prepare(48000,256);engine.start(s);
        float a[256],b[256];float* p[]{a,b};double peak=0,sum=0;int finite=1;std::ofstream csv(dir+"/freeze.csv");csv<<"second,energy,peak,finite\n";
        for(int second=0;second<310;++second)
        {
            if(second==2){s.freeze=true;engine.configure(s);}
            if(second==304){s.freeze=false;engine.configure(s);}
            sum=0;double secondPeak=0;
            for(int start=0;start<48000;start+=256)
            {
                const int n=std::min(256,48000-start);
                for(int i=0;i<n;++i){const double t=second+(start+i)/48000.;a[i]=second<2?static_cast<float>(.12*std::sin(2*pi*220*t)+.08*std::sin(2*pi*997*t)):0;b[i]=.8f*a[i];}
                engine.process(p,2,n);
                for(int i=0;i<n;++i)for(auto* ch:p){finite&=std::isfinite(ch[i]);sum+=ch[i]*ch[i];secondPeak=std::max(secondPeak,std::abs(static_cast<double>(ch[i])));}
            }
            peak=std::max(peak,secondPeak);csv<<second<<","<<std::setprecision(14)<<sum<<","<<secondPeak<<","<<finite<<"\n";
        }
        std::cout<<"freeze peak "<<peak<<", finite "<<finite<<"\n";return finite?0:1;
    }
    if(mode=="bench")
    {
        auto s=base();s.bands=true;s.feedback=.7;s.feedbackPitch=-12;s.spaceRoute=1;s.spaceMix=.6;s.glitch=3;s.glitchAmount=.4;s.glitchTarget=2;
        const int notes[]{36,41,43,48,55,60,67,72};
        for(int t=0;t<8;++t){s.taps[t].enabled=true;s.taps[t].note=notes[t];s.taps[t].timeMs=150+120*t;s.taps[t].gainDb=-18;s.taps[t].action=t%3==0?Action::Reverse:Action::Forward;}
        ScoreEngine one,two;one.prepare(48000,256);two.prepare(48000,256);one.start(s);two.start(s);
        for(int paths:{1,2})
        {
            double total=0,maximum=0;float a[256],b[256],c[256],d[256];float* p[]{a,b};float* q[]{c,d};
            for(int block=0;block<1500;++block)
            {
                for(int i=0;i<256;++i){const auto t=(block*256+i)/48000.;a[i]=c[i]=static_cast<float>(.12*std::sin(2*pi*220*t)+.06*std::sin(2*pi*997*t));b[i]=d[i]=a[i];}
                const auto begin=std::chrono::steady_clock::now();one.process(p,2,256);if(paths==2)two.process(q,2,256);
                const double duration=std::chrono::duration<double>(std::chrono::steady_clock::now()-begin).count();
                if(block>=100){total+=duration;maximum=std::max(maximum,duration);}
            }
            std::cout<<paths<<" paths mean "<<100*total/1400/(256./48000)<<"%, max "<<100*maximum/(256./48000)<<"%\n";
        }
        return 0;
    }
    return 2;
}

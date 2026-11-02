#include "DSP/V2Gate.h"
// First run: 49 cases missed independent Hold/knee/RMS/HP, clock-phase,
// recovery and full-window expiry checks. Expanded to 66 before the final pair;
// original criteria are unchanged. See S2_METHODS.md and the S2 report.
#include "AllocationAudit.h"
#include "ZastavaS2Identity.h"
#define NOMINMAX
#include <windows.h>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>
namespace
{
using namespace hamlo::zastava;
struct Case {std::string name;Settings p;int signal=0,block=256,channels=2;double rate=48000,seconds=.8;bool key=true;};
std::vector<Case> cases()
{
    std::vector<Case> all;
    for(double rate:{8000.,44100.,48000.,96000.,192000.})for(int kind=0;kind<4;++kind)
    {Case c;c.name="neutral_"+std::to_string(static_cast<int>(rate))+"_"+std::to_string(kind);c.rate=rate;c.signal=1;c.channels=kind%2+1;c.p.enabled=kind!=0;c.p.bypass=kind==1;c.p.mix=kind==2?0:1;c.p.external=kind==3;c.key=kind!=3;all.push_back(c);}
    auto basic=[](const std::string& name){Case c;c.name=name;c.p.enabled=true;c.p.external=true;c.p.detectorMs=0;c.p.lookaheadMs=0;c.p.kneeDb=0;c.p.attackMs=4;c.p.holdMs=10;c.p.releaseMs=20;c.p.profile=Profile::Hard;return c;};
    for(int i=0;i<3;++i){auto c=basic("static_"+std::to_string(i));c.signal=i+2;c.p.expand=i==2;c.p.ratio=2;all.push_back(c);}
    for(int i=0;i<3;++i){auto c=basic("knee_"+std::to_string(i));c.signal=14+i;c.p.expand=true;c.p.ratio=2;c.p.kneeDb=12;all.push_back(c);}
    {auto c=basic("rms");c.signal=9;c.p.rms=true;c.p.detectorMs=20;c.p.expand=true;c.p.ratio=2;c.p.thresholdDb=-20;all.push_back(c);}
    for(int profile=0;profile<2;++profile){auto c=basic("timing_"+std::to_string(profile));c.signal=5;c.p.profile=static_cast<Profile>(profile);all.push_back(c);}
    for(int mode=0;mode<2;++mode)for(int profile=0;profile<2;++profile)for(int look=0;look<2;++look){auto c=basic("window_"+std::to_string(mode)+"_"+std::to_string(profile)+"_"+std::to_string(look));c.signal=5;c.p.mode=mode?Mode::Duck:Mode::Clean;c.p.profile=static_cast<Profile>(profile);c.p.lookaheadMs=look*5;all.push_back(c);}
    for(int profile=0;profile<2;++profile)for(int look=0;look<2;++look){auto c=basic("look_"+std::to_string(profile)+"_"+std::to_string(look));c.signal=6;c.p.profile=static_cast<Profile>(profile);c.p.attackMs=1;c.p.lookaheadMs=look*5;all.push_back(c);}
    for(int profile=0;profile<2;++profile)for(int fixture=0;fixture<2;++fixture){auto c=basic("drum_"+std::to_string(profile)+"_"+std::to_string(fixture));c.signal=fixture?8:7;c.p.mode=Mode::Drum;c.p.profile=static_cast<Profile>(profile);c.p.attackMs=1;c.p.bodyMs=15;c.p.tailMs=40;all.push_back(c);}
    for(int i=0;i<2;++i){auto c=basic("duck_"+std::to_string(i));c.signal=i?3:2;c.p.mode=Mode::Duck;all.push_back(c);}
    for(int i=0;i<2;++i){auto c=basic("filter_"+std::to_string(i));c.signal=9;c.p.keyLpHz=i?200:0;c.p.detectorMs=10;all.push_back(c);}
    {auto c=basic("filter_hp");c.signal=9;c.p.keyHpHz=6000;c.p.detectorMs=10;all.push_back(c);}
    for(int i=0;i<2;++i){auto c=basic("link_"+std::to_string(i));c.signal=10;c.p.stereoLink=i;all.push_back(c);}
    for(int size:{16,32})for(int sync=0;sync<2;++sync){auto c=basic("rhythm_"+std::to_string(size)+"_"+std::to_string(sync));c.signal=11;c.seconds=2.2;c.p.mode=Mode::Rhythm;c.p.steps=size;c.p.sync=sync!=0;c.p.stepBeats=size==16?.25:.125;c.p.stepMs=size==16?125:62.5;c.p.attackMs=c.p.releaseMs=1;for(int n=0;n<32;++n)c.p.levels[static_cast<size_t>(n)]=n%2;all.push_back(c);}
    for(int block:{1,64,127,512,4096}){auto c=basic("events_"+std::to_string(block));c.signal=12;c.seconds=2;c.block=block;all.push_back(c);}
    for(int kind=0;kind<4;++kind){auto c=basic("recovery_"+std::to_string(kind));c.signal=17+kind;c.seconds=1;c.p.mode=kind==2?Mode::Rhythm:Mode::Clean;c.p.levels.fill(0);all.push_back(c);}
    {auto c=basic("stress");c.signal=13;c.rate=192000;c.seconds=300;c.block=4096;all.push_back(c);}
    return all;
}
void input(const Case& c,int index,int channel,float& audio,float& key)
{
    const double t=index/c.rate;audio=.1f;key=.2f;
    if(c.signal==1||c.signal==12||c.signal==13){audio=static_cast<float>(.1*std::sin(t*6264.335)+.03*std::sin(t*(channel?8293:3217)));key=std::fmod(t,.14)<.04?.2f:.000001f;}
    if(c.signal==3)key=.000001f;
    if(c.signal==4)key=static_cast<float>(std::pow(10.,-50./20));
    if(c.signal==5)key=t>=.1&&t<.2?.2f:.000001f;
    if(c.signal==6){audio=index==static_cast<int>(c.rate*.1)?1.f:0.f;key=t>=.1&&t<.11?.2f:.000001f;}
    if(c.signal==7||c.signal==8){key=.000001f;for(double hit:{.1,.24,.38,.52}){const int start=static_cast<int>(hit*c.rate);if(index>=start&&index<start+8)key=.2f;if(c.signal==7){const int ghost=start+static_cast<int>(.03*c.rate);if(index>=ghost&&index<ghost+8)key=.035f;}}if(c.signal==8&&t>.15)key=.000001f;}
    if(c.signal==9)key=static_cast<float>(.05*std::sin(t*6283.185307179586));
    if(c.signal==10)key=channel==0?.2f:.000001f;
    if(c.signal>=14&&c.signal<=16)key=static_cast<float>(std::pow(10.,(-50+10*(c.signal-14))/20.));
    if(c.signal>=17&&c.signal<=20)key=t>=.4?.2f:.000001f;
}
}
int main(int argc,char** argv)
{
    try
    {
        if(argc!=2)throw std::runtime_error("V2Lab NEW_output_directory");const std::filesystem::path folder(argv[1]);
        if(std::filesystem::exists(folder))throw std::runtime_error("Refusing to overwrite laboratory run");std::filesystem::create_directories(folder);
        std::ofstream manifest(folder/"manifest.json");manifest<<std::setprecision(17)<<"{\"source_id\":\""<<Gate::sourceIdentity()<<"\",\"method_id\":\""<<ZASTAVA_S2_METHOD_ID<<"\",\"implemented\":"<<(Gate::implemented()?"true":"false")<<",\"cases\":[";
        bool first=true;
        for(const auto& c:cases())
        {
            Gate gate;gate.prepare(c.rate,256);gate.set(c.p);const int frames=static_cast<int>(c.seconds*c.rate),cap=4096;
            std::array<std::vector<float>,2> audio,key;for(auto* bank:{&audio,&key})for(auto& a:*bank)a.resize(cap);
            std::vector<float> interleaved(cap*2);std::ofstream raw(folder/(c.name+".f32"),std::ios::binary);Settings settings=c.p;
            bool finite=true,missingSeen=false;double maximum=0,seconds=0,maxWallFraction=0;size_t allocations=0;int event=-1;
            ULONG64 cyclesAtMaxWall=0;std::vector<ULONG64> cycles;cycles.reserve(static_cast<size_t>(frames/c.block+frames/2400+32));
            for(int position=0;position<frames;)
            {
                const int interval=static_cast<int>(c.rate*(c.signal==13?.05:.2));const int current=position/interval;
                if((c.signal==12||c.signal==13)&&current!=event)
                {event=current;settings.mode=static_cast<Mode>(event%4);settings.profile=static_cast<Profile>(event%2);settings.keyHpHz=event%2?300:0;settings.keyLpHz=event%3?3500:0;settings.lookaheadMs=event%2?5:0;settings.bypass=event%7==6;settings.outputDb=c.signal==13?(event%2?12:-24):0;settings.expand=event%3==0;for(int n=0;n<32;++n)settings.levels[static_cast<size_t>(n)]=(n+event)%3*.5;}
                if(c.signal==13){settings.rangeDb=event%2?90:0;settings.ratio=event%2?100:1;settings.attackMs=event%2?200:.01;settings.holdMs=event%2?2000:0;settings.releaseMs=event%2?5000:.01;settings.bodyMs=event%2?2000:0;settings.tailMs=event%2?5000:.01;settings.onsetDb=event%2?24:0;settings.retriggerMs=event%2?200:0;settings.rms=event%2!=0;}
                const int count=std::min({c.block,frames-position,interval-position%interval});
                for(int ch=0;ch<c.channels;++ch)for(int i=0;i<count;++i)input(c,position+i,ch,audio[static_cast<size_t>(ch)][static_cast<size_t>(i)],key[static_cast<size_t>(ch)][static_cast<size_t>(i)]);
                float* a[]{audio[0].data(),audio[1].data()};const float* k[]{key[0].data(),key[1].data()};Clock clock;clock.hasPpq=true;
                const double t=position/c.rate;clock.bpm=(c.signal==12||c.signal==13)&&t>=1?174:120;clock.ppq=t<1?t*2:2+(t-1)*clock.bpm/60;
                if((c.signal==12||c.signal==13)&&t>=1.4)clock.ppq+=3;
                bool hasKey=c.key;
                if(c.signal==17)hasKey=!(t>=.2&&t<.4);
                if(c.signal==18)settings.bypass=t>=.2&&t<.4;
                if(c.signal==19)clock.playing=!(t>=.2&&t<.4);
                ULONG64 before=0,after=0;QueryThreadCycleTime(GetCurrentThread(),&before);
                hamlo::test::allocation::begin();const auto start=std::chrono::steady_clock::now();
                if(c.signal==20&&position==static_cast<int>(.6*c.rate))gate.reset();
                gate.set(settings);gate.process(a,c.channels,count,hasKey?k:nullptr,hasKey?c.channels:0,clock);
                const double elapsed=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();allocations+=hamlo::test::allocation::finish();
                QueryThreadCycleTime(GetCurrentThread(),&after);cycles.push_back(after-before);seconds+=elapsed;
                const double fraction=elapsed*c.rate/count;if(fraction>maxWallFraction){maxWallFraction=fraction;cyclesAtMaxWall=after-before;}
                missingSeen=missingSeen||gate.reading().missingKey;
                for(int i=0;i<count;++i)for(int ch=0;ch<c.channels;++ch){const float sample=a[ch][i];finite=finite&&std::isfinite(sample);maximum=std::max(maximum,std::abs(static_cast<double>(sample)));interleaved[static_cast<size_t>(i*c.channels+ch)]=sample;}
                raw.write(reinterpret_cast<const char*>(interleaved.data()),static_cast<std::streamsize>(count*c.channels*sizeof(float)));position+=count;
            }
            std::sort(cycles.begin(),cycles.end());const auto reading=gate.reading();if(!first)manifest<<",";first=false;
            manifest<<"{\"name\":\""<<c.name<<"\",\"rate\":"<<c.rate<<",\"frames\":"<<frames<<",\"channels\":"<<c.channels<<",\"finite\":"<<(finite?"true":"false")<<",\"peak\":"<<maximum<<",\"allocations\":"<<allocations<<",\"latency\":"<<gate.latency()<<",\"hits\":"<<reading.hits<<",\"missing_key\":"<<(reading.missingKey?"true":"false")<<",\"missing_seen\":"<<(missingSeen?"true":"false")<<",\"mean_fraction\":"<<seconds/c.seconds<<",\"maximum_wall_fraction\":"<<maxWallFraction<<",\"cycles_at_maximum_wall\":"<<cyclesAtMaxWall<<",\"median_cycles\":"<<cycles[cycles.size()/2]<<",\"maximum_cycles\":"<<cycles.back()<<"}";
            std::cout<<c.name<<" finite="<<finite<<" allocations="<<allocations<<std::endl;
        }manifest<<"]}\n";return 0;
    }
    catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}
}

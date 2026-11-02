#include "DSP/SpatialPrototype.h"
#include "AllocationAudit.h"
#include "SpatialIdentity.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

namespace
{
using namespace hamlo::shirota;
constexpr double pi=3.14159265358979323846;
struct Job
{std::string id,signal="complex";double fs=48000,seconds=2;int block=127;SpatialSettings settings;bool events=false;};
void raw(const std::filesystem::path& path,const std::vector<float>& l,const std::vector<float>& r)
{
    std::vector<float> values(l.size()*2);for(size_t i=0;i<l.size();++i){values[i*2]=l[i];values[i*2+1]=r[i];}
    std::ofstream out(path,std::ios::binary);out.write(reinterpret_cast<const char*>(values.data()),static_cast<std::streamsize>(values.size()*sizeof(float)));
    if(!out)throw std::runtime_error("Cannot write raw render");
}
std::pair<std::vector<float>,std::vector<float>> input(const Job& j)
{
    const int n=static_cast<int>(j.fs*j.seconds);std::vector<float> l(static_cast<size_t>(n)),r(l.size());uint32_t seed=0x484d5332;
    double pink1=0,pink2=0;
    for(int i=0;i<n;++i)
    {
        seed=seed*1664525u+1013904223u;const double noise=static_cast<double>(seed)/4294967296.-.5;
        pink1=.98*pink1+.02*noise;pink2=.7*pink2+.3*noise;
        const double t=i/j.fs;double a=.12*std::sin(2*pi*997*t)+.07*std::sin(2*pi*83*t)+.02*noise;
        double b=.12*std::sin(2*pi*1319*t)+.07*std::sin(2*pi*83*t)+.02*noise;
        if(j.signal=="mono"){a=b=.13*(noise+2*pink1+pink2)+.04*std::sin(2*pi*373*t);}
        if(j.signal=="mono60"){a=b=.1*std::sin(2*pi*60*t);}
        if(j.signal=="impulse"){a=i==static_cast<int>(j.fs*.25)?.5:0;b=0;}
        if(j.signal=="tail"){a=b=i<static_cast<int>(j.fs*.1)?.15*noise:0;}
        if(j.signal.rfind("anti",0)==0){const double hz=std::stod(j.signal.substr(4));a=.1*std::sin(2*pi*hz*t);b=-a;}
        l[static_cast<size_t>(i)]=static_cast<float>(a);r[static_cast<size_t>(i)]=static_cast<float>(b);
    }
    return {std::move(l),std::move(r)};
}
std::vector<Job> jobs()
{
    std::vector<Job> out;
    const auto add=[&](std::string id,std::string signal,SpatialSettings p,double fs=48000,double seconds=2,int block=127,bool events=false)
    {out.push_back({id,signal,fs,seconds,block,p,events});};
    SpatialSettings neutral,active;active.enabled=true;
    for(double fs:{44100.,48000.,96000.})
    {add("neutral_"+std::to_string(static_cast<int>(fs)),"complex",active,fs);auto extreme=active;extreme.enabled=false;extreme.width=4;extreme.generate=1.5;extreme.space=1;extreme.guard=true;add("disabled_"+std::to_string(static_cast<int>(fs)),"complex",extreme,fs);}
    for(double width:{0.,1.,2.,4.}){auto p=active;p.width=width;add("width_"+std::to_string(static_cast<int>(width)),"complex",p);}
    for(int g=0;g<3;++g){auto p=active;p.generator=static_cast<Generator>(g);p.generate=.75;add("generate_"+std::to_string(g),"mono",p);}
    // First 139-check run omitted routing combinations. Added before correcting DSP;
    // P02/P03/P05/P06 limits remain unchanged, and the original jobs stay intact.
    for(int g=0;g<3;++g)
    {
        auto p=active;p.generator=static_cast<Generator>(g);p.generate=.75;
        p.width=0;add("generate_zero_"+std::to_string(g),"mono",p);
        p.width=2;add("generate_double_"+std::to_string(g),"mono",p);
        p.width=1;add("generate_ref60_"+std::to_string(g),"mono60",p);
        p.monoBassHz=150;add("generate_bass60_"+std::to_string(g),"mono60",p);
        p.monoBassHz=0;p.bandWidth[0]=0;add("generate_band60_"+std::to_string(g),"mono60",p);
    }
    for(const auto& tone:{"anti60","anti1000"}){auto p=active;p.monoBassHz=150;add(std::string("bass_")+tone,tone,p);add(std::string("ref_")+tone,tone,active);}
    for(const auto& tone:{"anti60","anti8000"}){auto p=active;p.bandWidth[0]=0;add(std::string("band_")+tone,tone,p);if(std::string(tone)!="anti60")add(std::string("ref_")+tone,tone,active);}
    for(double floor:{0.,.4}){auto p=active;p.width=4;p.guard=true;p.correlationFloor=floor;add("guard_"+std::to_string(static_cast<int>(floor*10)),"complex",p);}
    for(int c=0;c<3;++c)
    {
        auto p=active;p.space=.7;p.character=static_cast<Space>(c);
        add("space_noise_"+std::to_string(c),"mono",p);add("space_impulse_"+std::to_string(c),"impulse",p,48000,4);
        p.size=2;p.space=1;p.depth=1;p.predelayMs=60;add("tail_"+std::to_string(c),"tail",p,48000,4);
    }
    for(double fs:{44100.,48000.,96000.})for(double pre:{0.,23.})
    {auto p=active;p.space=.7;p.predelayMs=pre;add("arrival_"+std::to_string(static_cast<int>(fs))+"_"+std::to_string(static_cast<int>(pre)),"impulse",p,fs);}
    for(int block:{1,64,127,512,4096})add("events_"+std::to_string(block),"complex",active,48000,2,block,true);
    auto stress=active;stress.width=4;stress.bandWidth={4,4,4};stress.generate=1.5;stress.generator=Generator::Diffuse;stress.space=1;stress.character=Space::Scatter;stress.size=2;stress.depth=1;
    add("stress","complex",stress,48000,30,256);
    return out;
}
void event(SpatialSettings& p,int index)
{
    p.enabled=true;p.width=index%2?4:1;p.generate=index%3*.55;p.generator=static_cast<Generator>(index%3);
    p.space=index%3*.4;p.character=static_cast<Space>(index%3);p.depth=index%4/3.;p.size=.5+index%3*.7;p.predelayMs=index%3*13.;
    p.guard=index%2==0;p.correlationFloor=.2;p.bandWidth={index%2?.3:1,1,index%2?3.:1};
    if(index==7)p.enabled=false;
}
void render(const Job& j,const std::filesystem::path& folder,std::ostream& manifest,bool& comma)
{
    auto [left,right]=input(j);const auto originalLeft=left,originalRight=right;
    SpatialPrototype p;p.prepare(j.fs,256);p.setTargets(j.settings);auto settings=j.settings;
    const int n=static_cast<int>(left.size());size_t allocations=0;double largest=0,total=0;int callbacks=0;
    const int times[]{0,10001,20003,30001,45007,61009,71013,84001,n};
    const int sections=j.events?8:1;
    for(int e=0;e<sections;++e)
    {
        if(j.events){event(settings,e);p.setTargets(settings);}
        const int first=j.events?times[e]:0,last=j.events?times[e+1]:n;
        for(int pos=first;pos<last;pos+=j.block)
        {
            const int count=std::min(j.block,last-pos);const auto start=std::chrono::steady_clock::now();
            hamlo::test::allocation::begin();p.process(left.data()+pos,right.data()+pos,count);allocations+=hamlo::test::allocation::finish();
            const double seconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();total+=seconds;largest=std::max(largest,seconds);++callbacks;
        }
    }
    bool finite=true;double peak=0;for(size_t i=0;i<left.size();++i){finite=finite&&std::isfinite(left[i])&&std::isfinite(right[i]);peak=std::max({peak,std::abs(static_cast<double>(left[i])),std::abs(static_cast<double>(right[i]))});}
    raw(folder/(j.id+".f32"),left,right);raw(folder/(j.id+".input.f32"),originalLeft,originalRight);
    if(comma)manifest<<",\n";comma=true;
    manifest<<"{\"id\":\""<<j.id<<"\",\"signal\":\""<<j.signal<<"\",\"rate\":"<<j.fs<<",\"frames\":"<<n<<",\"block\":"<<j.block<<",\"latency\":"<<p.latency()<<",\"finite\":"<<(finite?"true":"false")<<",\"peak\":"<<peak<<",\"allocations\":"<<allocations<<",\"guard_db\":"<<p.guardDb()<<",\"cpu_fraction\":"<<total/(n/j.fs)<<",\"largest_callback_seconds\":"<<largest<<",\"callbacks\":"<<callbacks<<"}";
    std::cout<<j.id<<" allocations="<<allocations<<" finite="<<finite<<std::endl;
}
void benchmark(const std::filesystem::path& folder)
{
    Job j;j.signal="complex";j.seconds=8;j.settings.enabled=true;j.settings.width=4;j.settings.generate=1.5;j.settings.space=1;j.settings.character=Space::Scatter;j.settings.size=2;
    const auto [originalL,originalR]=input(j);std::ofstream out(folder/"benchmark.json");out<<std::setprecision(17)<<"[";
    for(int run=0;run<3;++run)
    {
        auto l=originalL,r=originalR;SpatialPrototype p;p.prepare(48000,256);p.setTargets(j.settings);double total=0,largest=0;int count=0;
        for(int pos=0;pos<static_cast<int>(l.size());pos+=256)
        {const auto begin=std::chrono::steady_clock::now();p.process(l.data()+pos,r.data()+pos,std::min(256,static_cast<int>(l.size())-pos));const double time=std::chrono::duration<double>(std::chrono::steady_clock::now()-begin).count();if(pos>=48000){total+=time;largest=std::max(largest,time);++count;}}
        if(run)out<<",";out<<"{\"mean_fraction\":"<<total/(count*256./48000)<<",\"largest_fraction\":"<<largest/(256./48000)<<"}";
    }out<<"]\n";
}
}
int main(int argc,char** argv)
{
    try
    {
        if(argc!=2)throw std::runtime_error("Usage: SpatialLab <new-output-directory>");
        if(std::string(SpatialPrototype::compiledIdentity())!=SHIROTA_SPATIAL_SOURCE_ID)throw std::runtime_error("Mixed incremental build: DSP and laboratory fingerprints differ");
        const auto folder=std::filesystem::absolute(argv[1]);if(std::filesystem::exists(folder))throw std::runtime_error("Refusing to overwrite a laboratory run");std::filesystem::create_directories(folder);
        std::ofstream manifest(folder/"manifest.json");manifest<<std::setprecision(17)<<"{\"method\":\"Shirota S2 laboratory v1\",\"source_id\":\""<<SHIROTA_SPATIAL_SOURCE_ID<<"\",\"dsp_identity\":\""<<SpatialPrototype::compiledIdentity()<<"\",\"implemented\":"<<(SpatialPrototype::implemented()?"true":"false")<<",\"cases\":[\n";
        bool comma=false;for(const auto& j:jobs())render(j,folder,manifest,comma);manifest<<"]}\n";manifest.close();benchmark(folder);return 0;
    }
    catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 2;}
}

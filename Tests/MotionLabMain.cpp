#include "DSP/MotionPrototype.h"
#include "AllocationAudit.h"
#include "MotionIdentity.h"
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
MotionPath path(bool beats=false)
{
    MotionPath p;p.count=3;p.unit=beats?MotionUnit::Beats:MotionUnit::Milliseconds;
    p.nodes[0]={90,beats?.25:125,beats?.75:375,MotionArc::Counterclockwise,MotionCurve::Smooth};
    p.nodes[1]={270,beats?.5:250,beats?.5:250,MotionArc::Shortest,MotionCurve::Linear};
    p.nodes[2]={0,0,beats?1.:500,MotionArc::Shortest,MotionCurve::Smooth};return p;
}
MotionPath orbit(bool clockwise=false)
{MotionPath p;p.count=1;p.nodes[0]={0,0,1000,clockwise?MotionArc::Clockwise:MotionArc::Counterclockwise,MotionCurve::Linear};return p;}
struct Job
{
    std::string id,signal="complex",scenario;
    double fs=48000,seconds=2,bpm=120;
    int block=127;
    bool trace=false,events=false;
    MotionSettings settings;
    MotionPath route=path();
};
std::vector<Job> jobs()
{
    std::vector<Job> out;MotionSettings active;active.enabled=true;
    const auto add=[&](std::string id,std::string signal,MotionSettings p,double fs=48000){Job j;j.id=id;j.signal=signal;j.settings=p;j.fs=fs;out.push_back(j);};
    for(double fs:{44100.,48000.,96000.})for(const auto& signal:{"complex","mono"})
    {
        add("neutral_"+std::to_string(static_cast<int>(fs))+"_"+signal,signal,active,fs);
        auto p=active;p.enabled=false;p.renderer=MotionRenderer::Binaural;p.center=153;p.spread=12;p.balance=.7;p.profile=2;
        add("disabled_"+std::to_string(static_cast<int>(fs))+"_"+signal,signal,p,fs);
    }
    for(int angle:{0,90,270}){auto p=active;p.source=MotionSource::Point;p.center=angle;add("pan_"+std::to_string(angle),"mono",p);}
    for(int b:{-1,1}){auto p=active;p.balance=b;add("balance_"+std::to_string(b),"complex",p);}
    for(int s=0;s<2;++s){auto p=active;p.source=s?MotionSource::Point:MotionSource::Spread;p.spread=60;add(s?"point_anti":"spread_anti","anti",p);}
    for(int profile=0;profile<3;++profile)
    {
        auto p=active;p.renderer=MotionRenderer::Binaural;p.profile=profile;p.source=MotionSource::Point;
        for(double angle:{0.,90.,180.,270.,2.5}){p.center=angle;add("hrir_"+std::to_string(profile)+"_"+std::to_string(static_cast<int>(angle*10)),"impulse",p);}
        p.source=MotionSource::Spread;p.spread=60;p.center=0;add("binaural_spread_anti_"+std::to_string(profile),"anti",p);
        // Post-first-run coverage audit: independently prove both emitter channels.
        for(int channel=0;channel<2;++channel)add("spread_hrir_"+std::to_string(profile)+"_"+std::to_string(channel),channel==0?"impulse_left":"impulse_right",p);
    }
    for(double fs:{44100.,96000.}){auto p=active;p.renderer=MotionRenderer::Binaural;p.source=MotionSource::Point;p.center=90;add("hrir_rate_"+std::to_string(static_cast<int>(fs)),"impulse",p,fs);}
    for(double fs:{44100.,48000.,96000.}){Job j;j.id="path_free_"+std::to_string(static_cast<int>(fs));j.scenario="free";j.trace=true;j.fs=fs;j.seconds=4;out.push_back(j);}
    for(double bpm:{90.,140.,174.}){Job j;j.id="path_beats_"+std::to_string(static_cast<int>(bpm));j.scenario="beats";j.route=path(true);j.bpm=bpm;j.trace=true;j.seconds=4;out.push_back(j);}
    for(const auto& scenario:{"tempo","reset","cw","ccw","zero","nonloop"})
    {
        Job j;j.scenario=scenario;j.id="path_"+j.scenario;j.trace=true;j.seconds=4;
        if(j.scenario=="tempo"){j.route=path(true);j.bpm=90;}
        if(j.scenario=="cw"||j.scenario=="ccw")j.route=orbit(j.scenario=="cw");
        if(j.scenario=="zero")for(auto& n:j.route.nodes){n.hold=0;n.travel=0;}
        if(j.scenario=="nonloop")j.route.loop=false;
        out.push_back(j);
    }
    for(int block:{1,64,127,512,4096})
    {Job j;j.id="events_"+std::to_string(block);j.events=true;j.settings=active;j.settings.usePath=true;j.settings.renderer=MotionRenderer::Binaural;j.settings.spread=60;j.block=block;j.seconds=4;out.push_back(j);}
    for(const auto& scenario:{"pre","post","change","sweep","enable"})
    {
        Job j;j.scenario=scenario;j.id="transition_"+j.scenario;j.signal="tone";j.seconds=3;j.settings=active;j.settings.renderer=MotionRenderer::Binaural;j.settings.source=MotionSource::Point;
        if(j.scenario=="post"){j.settings.profile=1;j.settings.center=180;}
        if(j.scenario=="sweep"){j.settings.usePath=true;j.route=orbit();j.route.nodes[0].travel=3000;}
        out.push_back(j);
    }
    return out;
}
std::pair<std::vector<float>,std::vector<float>> input(const Job& j)
{
    const auto n=static_cast<size_t>(j.seconds*j.fs);std::vector<float> l(n),r(n);uint32_t seed=0x53484933;
    for(size_t i=0;i<n;++i)
    {
        seed=seed*1664525u+1013904223u;const double noise=static_cast<double>(seed)/4294967296.-.5,t=i/j.fs;
        double a=.1*std::sin(2*pi*997*t)+.04*noise,b=.1*std::sin(2*pi*1319*t)+.04*noise;
        if(j.signal=="mono")a=b=.08*std::sin(2*pi*373*t)+.1*noise;
        if(j.signal=="impulse")a=b=i==static_cast<size_t>(.25*j.fs)?.5:0;
        if(j.signal=="impulse_left"||j.signal=="impulse_right")
        {a=i==static_cast<size_t>(.25*j.fs)&&j.signal=="impulse_left"?.5:0;b=i==static_cast<size_t>(.25*j.fs)&&j.signal=="impulse_right"?.5:0;}
        if(j.signal=="tone")a=b=.1*std::sin(2*pi*997*t);
        if(j.signal=="anti"){a=.1*std::sin(2*pi*997*t);b=-a;}
        l[i]=static_cast<float>(a);r[i]=static_cast<float>(b);
    }return {std::move(l),std::move(r)};
}
void raw(const std::filesystem::path& file,const std::vector<float>& l,const std::vector<float>& r)
{
    std::vector<float> v(l.size()*2);for(size_t i=0;i<l.size();++i){v[i*2]=l[i];v[i*2+1]=r[i];}
    std::ofstream out(file,std::ios::binary);out.write(reinterpret_cast<const char*>(v.data()),static_cast<std::streamsize>(v.size()*4));if(!out)throw std::runtime_error("Raw write failed");
}
void event(MotionPrototype& p,MotionSettings& s,int i)
{
    if(i==1)s.profile=1;
    if(i==2){s.center=180;s.usePath=false;}
    if(i==3){p.setPath(path(true));p.setTempo(174);s.usePath=true;}
    if(i==4)s.renderer=MotionRenderer::Stereo;
    if(i==5)p.resetPath();
    if(i==6)s.source=MotionSource::Point;
    if(i==7)s.enabled=false;
    if(i==8){s.enabled=true;s.renderer=MotionRenderer::Binaural;s.profile=2;}
    p.setTargets(s);
}
void render(const Job& j,const std::string& bank,const std::filesystem::path& folder,std::ostream& manifest,bool& comma)
{
    auto [l,r]=input(j);const auto originalL=l,originalR=r;const int n=static_cast<int>(l.size());
    std::vector<MotionPosition> trace(j.trace?l.size():0);
    MotionPrototype p;p.prepare(j.fs,bank);p.setPath(j.route);p.setTempo(j.bpm);p.setTargets(j.settings);auto s=j.settings;
    std::vector<int> times{0,n};
    if(j.events)times={0,20003,40009,60013,80021,100003,120007,140009,160003,n};
    else if(j.scenario=="tempo")times={0,36001,n};
    else if(j.scenario=="reset")times={0,10001,n};
    else if(j.scenario=="change")times={0,48000,n};
    else if(j.scenario=="enable")times={0,48000,96000,n};
    size_t allocations=0;double total=0,largest=0;
    for(size_t e=0;e+1<times.size();++e)
    {
        hamlo::test::allocation::begin();
        if(j.events)event(p,s,static_cast<int>(e));
        else if(e>0&&j.scenario=="tempo")p.setTempo(174);
        else if(e>0&&j.scenario=="reset")p.resetPath();
        else if(e>0&&j.scenario=="change"){s.profile=1;s.center=180;p.setTargets(s);}
        else if(e>0&&j.scenario=="enable"){s.enabled=e==2;p.setTargets(s);}
        allocations+=hamlo::test::allocation::finish();
        for(int pos=times[e];pos<times[e+1];pos+=j.block)
        {
            const int count=std::min(j.block,times[e+1]-pos);const auto begin=std::chrono::steady_clock::now();hamlo::test::allocation::begin();
            p.process(l.data()+pos,r.data()+pos,count,j.trace?trace.data()+pos:nullptr);allocations+=hamlo::test::allocation::finish();
            const double t=std::chrono::duration<double>(std::chrono::steady_clock::now()-begin).count();total+=t;largest=std::max(largest,t);
        }
    }
    double peak=0;bool finite=true;for(size_t i=0;i<l.size();++i){finite=finite&&std::isfinite(l[i])&&std::isfinite(r[i]);peak=std::max({peak,std::abs(static_cast<double>(l[i])),std::abs(static_cast<double>(r[i]))});}
    raw(folder/(j.id+".f32"),l,r);raw(folder/(j.id+".input.f32"),originalL,originalR);
    if(j.trace)
    {std::vector<double> values;values.reserve(trace.size()*4);for(auto t:trace){values.push_back(t.azimuth);values.push_back(t.node);values.push_back(t.travelling?1:0);values.push_back(t.progress);}std::ofstream out(folder/(j.id+".trace.f64"),std::ios::binary);out.write(reinterpret_cast<const char*>(values.data()),static_cast<std::streamsize>(values.size()*8));if(!out)throw std::runtime_error("Trace write failed");}
    if(comma)manifest<<",\n";comma=true;
    manifest<<"{\"id\":\""<<j.id<<"\",\"signal\":\""<<j.signal<<"\",\"scenario\":\""<<j.scenario<<"\",\"rate\":"<<j.fs<<",\"frames\":"<<n<<",\"latency\":"<<p.latency()<<",\"trace\":"<<(j.trace?"true":"false")<<",\"bpm\":"<<j.bpm<<",\"finite\":"<<(finite?"true":"false")<<",\"peak\":"<<peak<<",\"allocations\":"<<allocations<<",\"cpu_fraction\":"<<total/(n/j.fs)<<",\"largest_seconds\":"<<largest<<"}";
    std::cout<<j.id<<" finite="<<finite<<" allocations="<<allocations<<std::endl;
}
void stress(const std::string& bank,const std::filesystem::path& folder)
{
    MotionPrototype p;p.prepare(48000,bank);std::vector<float> l(256),r(256);MotionSettings s;s.enabled=true;s.usePath=true;s.renderer=MotionRenderer::Binaural;s.spread=60;p.setTargets(s);p.setPath(path());
    size_t allocations=0;bool finite=true;double peak=0;constexpr int frames=48000*300;uint32_t seed=917;
    for(int pos=0;pos<frames;pos+=256)
    {
        for(int i=0;i<256;++i){seed=seed*1664525u+1013904223u;l[static_cast<size_t>(i)]=static_cast<float>(.2*(static_cast<double>(seed)/4294967296.-.5));r[static_cast<size_t>(i)]=.5f*l[static_cast<size_t>(i)];}
        hamlo::test::allocation::begin();
        if(pos%4096==0)
        {const int k=pos/4096;s.profile=k%5-1;s.renderer=k%3==0?MotionRenderer::Stereo:MotionRenderer::Binaural;s.enabled=k%7!=0;s.balance=k%11==0?NAN:0;s.spread=k%13==0?INFINITY:60;p.setTargets(s);p.setTempo(k%7==0?NAN:90+k%100);if(k%19==0){auto q=path();for(auto& v:q.nodes)v.hold=v.travel=0;p.setPath(q);}else if(k%19==1)p.setPath(path());}
        p.process(l.data(),r.data(),std::min(256,frames-pos));allocations+=hamlo::test::allocation::finish();
        for(int i=0;i<std::min(256,frames-pos);++i){finite=finite&&std::isfinite(l[static_cast<size_t>(i)])&&std::isfinite(r[static_cast<size_t>(i)]);peak=std::max({peak,std::abs(static_cast<double>(l[static_cast<size_t>(i)])),std::abs(static_cast<double>(r[static_cast<size_t>(i)]))});}
    }
    std::ofstream out(folder/"stress.json");out<<std::setprecision(17)<<"{\"frames\":"<<frames<<",\"finite\":"<<(finite?"true":"false")<<",\"peak\":"<<peak<<",\"allocations\":"<<allocations<<"}\n";
}
void benchmark(const std::string& bank,const std::filesystem::path& folder)
{
    Job j;j.seconds=8;const auto [a,b]=input(j);std::ofstream out(folder/"benchmark.json");out<<std::setprecision(17)<<"[";
    for(int run=0;run<3;++run)
    {
        auto l=a,r=b;MotionPrototype p;p.prepare(48000,bank);MotionSettings s;s.enabled=true;s.usePath=true;s.renderer=MotionRenderer::Binaural;s.spread=60;s.profile=2;p.setTargets(s);p.setPath(orbit());double total=0,largest=0;int count=0;
        for(int pos=0;pos<static_cast<int>(l.size());pos+=256)
        {const auto begin=std::chrono::steady_clock::now();p.process(l.data()+pos,r.data()+pos,std::min(256,static_cast<int>(l.size())-pos));const double t=std::chrono::duration<double>(std::chrono::steady_clock::now()-begin).count();if(pos>=48000){total+=t;largest=std::max(largest,t);++count;}}
        if(run)out<<",";out<<"{\"mean_fraction\":"<<total/(count*256./48000)<<",\"largest_fraction\":"<<largest/(256./48000)<<"}";
    }out<<"]\n";
}
void demos(const std::string& bank,const std::filesystem::path& folder,const std::filesystem::path& file)
{
    const auto bytes=std::filesystem::file_size(file);if(bytes==0||bytes%8||bytes>48000*12*8)throw std::runtime_error("Expected <=12s48k stereo f32");
    std::vector<float> interleaved(static_cast<size_t>(bytes/4));std::ifstream inputFile(file,std::ios::binary);inputFile.read(reinterpret_cast<char*>(interleaved.data()),static_cast<std::streamsize>(bytes));if(!inputFile)throw std::runtime_error("Demo input failed");
    MotionSettings p;std::vector<std::pair<std::string,MotionSettings>> scenes{{"dry",p}};p.enabled=true;p.renderer=MotionRenderer::Binaural;p.source=MotionSource::Point;
    for(int profile=0;profile<3;++profile)
    {p.profile=profile;p.usePath=false;for(int angle:{0,90,180,270}){p.center=angle;scenes.push_back({"profile"+std::to_string(profile)+"_"+std::to_string(angle),p});}p.usePath=true;scenes.push_back({"profile"+std::to_string(profile)+"_orbit",p});}
    p.source=MotionSource::Spread;p.spread=60;p.profile=0;scenes.push_back({"spread_orbit",p});
    std::ofstream manifest(folder/"demo_manifest.json");manifest<<"{\"source_id\":\""<<SHIROTA_MOTION_SOURCE_ID<<"\",\"scenes\":[";bool comma=false;
    for(const auto& scene:scenes)
    {
        std::vector<float> l(interleaved.size()/2),r(l.size());for(size_t i=0;i<l.size();++i){l[i]=interleaved[2*i];r[i]=interleaved[2*i+1];}
        MotionPrototype renderer;renderer.prepare(48000,bank);renderer.setTargets(scene.second);auto q=orbit();q.nodes[0].travel=4000;renderer.setPath(q);
        for(size_t i=0;i<l.size();i+=256)renderer.process(l.data()+i,r.data()+i,static_cast<int>(std::min(size_t(256),l.size()-i)));
        raw(folder/(scene.first+".f32"),l,r);if(comma)manifest<<",";comma=true;manifest<<"\""<<scene.first<<"\"";
    }manifest<<"]}\n";
}
}
int main(int argc,char** argv)
{
    try
    {
        if(argc!=3&&argc!=5)throw std::runtime_error("MotionLab <hrtf-bank> <new-output-dir> [--demo <48k-stereo-f32>]");
        if(std::string(MotionPrototype::compiledIdentity())!=SHIROTA_MOTION_SOURCE_ID)throw std::runtime_error("Mixed motion translation units");
        const auto folder=std::filesystem::absolute(argv[2]);if(std::filesystem::exists(folder))throw std::runtime_error("Refusing to overwrite a run");
        // Validate the bank before creating any output, including demo mode.
        MotionPrototype probe;probe.prepare(48000,argv[1]);std::filesystem::create_directories(folder);
        if(argc==5){if(std::string(argv[3])!="--demo"||!MotionPrototype::implemented())throw std::runtime_error("Expected implemented demo");demos(argv[1],folder,argv[4]);return 0;}
        std::ofstream manifest(folder/"manifest.json");manifest<<std::setprecision(17)<<"{\"source_id\":\""<<SHIROTA_MOTION_SOURCE_ID<<"\",\"dsp_identity\":\""<<MotionPrototype::compiledIdentity()<<"\",\"implemented\":"<<(MotionPrototype::implemented()?"true":"false")<<",\"cases\":[\n";
        bool comma=false;for(const auto& j:jobs())render(j,argv[1],folder,manifest,comma);manifest<<"]}\n";manifest.close();stress(argv[1],folder);benchmark(argv[1],folder);return 0;
    }
    catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 2;}
}

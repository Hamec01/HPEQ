#include "DSP/V2Gate.h"
#include "AllocationAudit.h"
#include "ZastavaS2Identity.h"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

using namespace hamlo::zastava;
namespace
{
std::vector<float> read(const std::filesystem::path& path)
{
    const auto bytes=std::filesystem::file_size(path);
    if(bytes==0||bytes%8!=0)throw std::runtime_error("Expected interleaved stereo float32");
    std::vector<float> data(static_cast<size_t>(bytes/4));
    std::ifstream file(path,std::ios::binary);
    file.read(reinterpret_cast<char*>(data.data()),static_cast<std::streamsize>(bytes));
    if(!file)throw std::runtime_error("Incomplete input");
    return data;
}
Settings recipe(const std::string& name,Profile profile)
{
    Settings p;p.enabled=true;p.profile=profile;p.lookaheadMs=3;p.thresholdDb=-36;
    p.rangeDb=24;p.attackMs=3;p.holdMs=35;p.releaseMs=110;p.detectorMs=3;
    p.keyHpHz=90;p.keyLpHz=12000;
    if(name=="Clean"){p.expand=true;p.ratio=3;p.kneeDb=8;p.rms=true;}
    else if(name=="Drum")
    {
        p.mode=Mode::Drum;p.thresholdDb=-32;p.rangeDb=36;p.attackMs=.5;
        p.bodyMs=40;p.tailMs=140;p.onsetDb=3;p.retriggerMs=12;p.detectorMs=0;
    }
    else if(name=="Duck")
    {
        p.mode=Mode::Duck;p.external=true;p.thresholdDb=-24;p.rangeDb=15;
        p.attackMs=100;p.holdMs=20;p.releaseMs=2;p.detectorMs=4;
    }
    else if(name=="Rhythm")
    {
        p.mode=Mode::Rhythm;p.rangeDb=40;p.attackMs=2;p.releaseMs=12;
        const double levels[]{1,.2,.5,0,1,0,.65,0,1,.3,.5,0,.8,0,.45,0};
        for(int n=0;n<32;++n)p.levels[static_cast<size_t>(n)]=levels[n%16];
    }
    else throw std::runtime_error("Unknown recipe");
    return p;
}
}
int main(int argc,char** argv)
{
    try
    {
        if(argc!=6)throw std::runtime_error("V2Render input.f32 key.f32|- NEW_directory Clean|Drum|Duck|Rhythm Soft|Hard");
        const std::filesystem::path folder(argv[3]);
        if(std::filesystem::exists(folder))throw std::runtime_error("Refusing to overwrite render");
        const auto input=read(argv[1]);const auto key=std::string(argv[2])=="-"?std::vector<float>{}:read(argv[2]);
        const auto settings=recipe(argv[4],std::string(argv[5])=="Soft"?Profile::Soft:Profile::Hard);
        if(settings.external&&key.size()!=input.size())throw std::runtime_error("Key length must match input");
        std::filesystem::create_directories(folder);
        std::array<std::array<float,256>,2> audio{},side{};std::array<float,512> result{};
        Gate gate;gate.prepare(48000,256);gate.set(settings);
        std::ofstream raw(folder/"output.f32",std::ios::binary);
        size_t allocations=0;bool finite=true;
        for(size_t position=0;position<input.size()/2;)
        {
            const int count=static_cast<int>(std::min<size_t>(256,input.size()/2-position));
            for(int i=0;i<count;++i)for(size_t c=0;c<2;++c)
            {
                const auto at=(position+static_cast<size_t>(i))*2+c;
                audio[c][static_cast<size_t>(i)]=input[at];side[c][static_cast<size_t>(i)]=key.empty()?0:key[at];
            }
            float* a[]{audio[0].data(),audio[1].data()};const float* k[]{side[0].data(),side[1].data()};
            Clock clock;clock.hasPpq=true;clock.ppq=position/24000.;
            hamlo::test::allocation::begin();gate.set(settings);gate.process(a,2,count,key.empty()?nullptr:k,key.empty()?0:2,clock);
            allocations+=hamlo::test::allocation::finish();
            for(int i=0;i<count;++i)for(int c=0;c<2;++c)
            {
                const float value=a[c][i];finite=finite&&std::isfinite(value);result[static_cast<size_t>(2*i+c)]=value;
            }
            raw.write(reinterpret_cast<const char*>(result.data()),count*2*sizeof(float));position+=static_cast<size_t>(count);
        }
        std::ofstream meta(folder/"manifest.json");
        meta<<"{\"source_id\":\""<<Gate::sourceIdentity()<<"\",\"render_id\":\""<<ZASTAVA_S2_RENDER_ID
            <<"\",\"recipe\":\""<<argv[4]<<"\",\"profile\":\""<<argv[5]<<"\",\"frames\":"<<input.size()/2
            <<",\"latency\":"<<gate.latency()<<",\"allocations\":"<<allocations<<",\"finite\":"<<(finite?"true":"false")<<",\"hits\":"<<gate.reading().hits<<"}\n";
        return finite&&allocations==0?0:1;
    }
    catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}
}

#include "DSP/SpatialPrototype.h"
#include "SpatialIdentity.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

using namespace hamlo::shirota;
int main(int argc,char** argv)
{
    try
    {
        if(argc!=3)throw std::runtime_error("SpatialDemo <48kHz-stereo-interleaved-f32> <output-dir>");
        if(!SpatialPrototype::implemented()||std::string(SpatialPrototype::compiledIdentity())!=SHIROTA_SPATIAL_SOURCE_ID)throw std::runtime_error("Unimplemented or mixed DSP build");
        const auto bytes=std::filesystem::file_size(argv[1]);if(bytes%8!=0||bytes>48000*12*8||bytes==0)throw std::runtime_error("Expected <=12s stereo float32");
        std::vector<float> input(static_cast<size_t>(bytes/4));std::ifstream file(argv[1],std::ios::binary);file.read(reinterpret_cast<char*>(input.data()),static_cast<std::streamsize>(bytes));if(!file)throw std::runtime_error("Cannot read input");
        const std::filesystem::path folder=std::filesystem::absolute(argv[2]);std::filesystem::create_directories(folder);
        SpatialSettings p;p.enabled=true;std::vector<std::pair<std::string,SpatialSettings>> scenes{{"dry",p}};
        p.width=2;scenes.push_back({"width2",p});p.width=1;
        for(int g=0;g<3;++g){p.generate=.75;p.generator=static_cast<Generator>(g);scenes.push_back({std::string(g==0?"phase":g==1?"micro":"diffuse"),p});}
        p.generate=0;
        for(int c=0;c<3;++c){p.space=.7;p.character=static_cast<Space>(c);scenes.push_back({std::string(c==0?"tight":c==1?"open":"scatter"),p});}
        p.generate=.75;p.generator=Generator::Phase;p.character=Space::Open;p.space=.55;scenes.push_back({"phase_open",p});
        p.generator=Generator::Diffuse;p.character=Space::Scatter;p.space=.7;scenes.push_back({"diffuse_scatter",p});
        std::ofstream manifest(folder/"demo_manifest.json");manifest<<"{\"source_id\":\""<<SHIROTA_SPATIAL_SOURCE_ID<<"\",\"scenes\":[";
        bool first=true;
        for(const auto& scene:scenes)
        {
            std::vector<float> l(input.size()/2),r(l.size());for(size_t i=0;i<l.size();++i){l[i]=input[2*i];r[i]=input[2*i+1];}
            SpatialPrototype engine;engine.prepare(48000,256);engine.setTargets(scene.second);
            for(size_t pos=0;pos<l.size();pos+=256)engine.process(l.data()+pos,r.data()+pos,static_cast<int>(std::min(size_t(256),l.size()-pos)));
            std::vector<float> out(input.size());for(size_t i=0;i<l.size();++i){out[2*i]=l[i];out[2*i+1]=r[i];}
            std::ofstream audio(folder/(scene.first+".f32"),std::ios::binary);audio.write(reinterpret_cast<const char*>(out.data()),static_cast<std::streamsize>(bytes));if(!audio)throw std::runtime_error("Cannot save demo");
            if(!first)manifest<<",";first=false;manifest<<"\""<<scene.first<<"\"";
        }
        manifest<<"]}\n";return 0;
    }
    catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 2;}
}

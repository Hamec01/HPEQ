#include "Standalone/ModulePluginProcessor.h"
#include <windows.h>
#include <algorithm>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <vector>
int main(int argc,char** argv)
{
    juce::ScopedJuceInitialiser_GUI gui;if(argc!=2)return 2;std::ofstream out(argv[1]);out<<std::setprecision(17)<<"[";
    for(int run=0;run<3;++run)
    {
        hpeq::standalone::ModulePluginProcessor p(hpeq::standalone::ModuleKind::Shirota);
        for(const auto& value:std::initializer_list<std::pair<const char*,double>>{{"v2Mode",2},{"v2PositionOn",1},{"v2Width",4},{"v2Generate",1.5},{"v2Generator",2},{"v2Space",1},{"v2SpaceType",2},{"v2Size",2},{"v2Predelay",60},{"v2Motion",1},{"v2Spread",60},{"v2Profile",2}})p.setParameterValue(value.first,value.second);
        p.prepareToPlay(48000,256);juce::AudioBuffer<float> audio(2,256);juce::MidiBuffer midi;
        std::vector<double> wall;std::vector<unsigned long long> cycles;wall.reserve(1500);cycles.reserve(1500);
        for(int pos=0;pos<48000*8;pos+=256)
        {
            for(int ch=0;ch<2;++ch)for(int i=0;i<256;++i)audio.setSample(ch,i,static_cast<float>(.1*std::sin((pos+i)*(.09+ch*.01))));
            ULONG64 a=0,b=0;QueryThreadCycleTime(GetCurrentThread(),&a);const auto start=std::chrono::steady_clock::now();p.processBlock(audio,midi);const double elapsed=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();QueryThreadCycleTime(GetCurrentThread(),&b);
            if(pos>=48000){wall.push_back(elapsed/(256./48000));cycles.push_back(b-a);}
        }
        const auto worst=static_cast<size_t>(std::max_element(wall.begin(),wall.end())-wall.begin());auto sorted=cycles;std::sort(sorted.begin(),sorted.end());double mean=0;for(double w:wall)mean+=w;mean/=wall.size();
        if(run)out<<",";out<<"{\"mean_fraction\":"<<mean<<",\"largest_fraction\":"<<wall[worst]<<",\"cycles_at_largest_wall\":"<<cycles[worst]<<",\"median_cycles\":"<<sorted[sorted.size()/2]<<",\"maximum_cycles\":"<<sorted.back()<<"}";
    }out<<"]\n";return 0;
}

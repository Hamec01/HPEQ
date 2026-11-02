// Fault injection in this disposable test process only; no file/DLL mutation.
#include "Standalone/ModulePluginProcessor.h"
#include "ShirotaHrtf.h"
#include <windows.h>
#include <iostream>
int main()
{
    juce::ScopedJuceInitialiser_GUI gui;
    auto* byte=const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(ShirotaHrtf::production_bin))+128;
    DWORD protection=0;if(!VirtualProtect(byte,1,PAGE_READWRITE,&protection))return 2;*byte^=1;DWORD ignored=0;VirtualProtect(byte,1,protection,&ignored);
    try{hpeq::standalone::ModulePluginProcessor p(hpeq::standalone::ModuleKind::Shirota);p.prepareToPlay(48000,256);}
    catch(const std::exception& e){const bool correct=std::string(e.what()).find("Stale embedded HRIR bank")!=std::string::npos;std::cout<<e.what()<<"\n";return correct?0:1;}
    return 1;
}

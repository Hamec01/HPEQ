// Preserve the original bank laboratory's renderer and measurement functions.
// This separate executable changes only failing master ceiling parameters.
#define main originalRackBankMain
#include "RackBankMain.cpp"
#undef main

int main(int argc,char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    try
    {
        if(argc!=4)throw std::runtime_error("HPEQ_RackRepair <record directory> <material directory> <ID>");
        const juce::File folder(argv[1]);materialMode=true;materialFolder=juce::File(argv[2]);const juce::String id(argv[3]);
        const auto& bank=hpeq::rack::recipes();const auto found=std::find_if(bank.begin(),bank.end(),[&](const auto& r){return r.id==id;});
        if(found==bank.end()||found->category!="Master")throw std::runtime_error("Ceiling repair requires a master recipe");
        const auto& recipe=*found;const auto file=folder.getChildFile("records/"+id+".json");auto row=juce::JSON::parse(file);
        if(row["bank_sha256"].toString()!=hpeq::rack::bankSha256()||row.hasProperty("repair"))throw std::runtime_error("Missing/stale/already repaired receipt");
        const auto before=folder.getChildFile("originals/"+id+".json");before.getParentDirectory().createDirectory();
        if(before.existsAsFile()||!file.copyFileTo(before))throw std::runtime_error("Original receipt must be immutable");
        for(const auto suffix:{"-processed.wav","-matched.wav"})
        {
            const auto old=folder.getChildFile("audio/"+id+suffix),backup=folder.getChildFile("originals/audio/"+id+suffix);
            backup.getParentDirectory().createDirectory();if(backup.existsAsFile()||!old.copyFileTo(backup))throw std::runtime_error("Cannot preserve original audition");
        }
        hpeq::FxProjectState p;juce::String error;if(!hpeq::rack::makeProject(recipe,p,error,false))throw std::runtime_error(error.toStdString());
        auto original=p;if(!hpeq::rack::applyCalibration(row["calibration"],p,error))throw std::runtime_error(error.toStdString());
        auto& limiter=p.layers.back().mixChain.back();const double prior=limiter.parameters["ceilingDb"];
        limiter.parameters.set("ceilingDb",prior-.05);
        juce::Array<juce::var> checks;bool passed=true;
        for(int variant=0;variant<2;++variant)for(double level:{-6.,0.,6.})
        {
            auto x=fixture(recipe,variant,recipe.bpm);x.applyGain(static_cast<float>(std::pow(10.,level/20)));
            const auto input=metrics(x);const auto result=render(p,x,recipe.bpm);const auto output=metrics(result.audio);
            auto c=object();put(c,"variant",variant);put(c,"level_offset_db",level);put(c,"bpm",recipe.bpm);put(c,"input",data(input));put(c,"output",data(output));
            put(c,"compressor_peak_gr_db",result.comp);put(c,"limiter_peak_gr_db",result.limiter);
            const bool finite=output.finite&&output.peak> -110&&input.peak<=-2.999,ceiling=output.truePeak<=-1,gr=result.limiter<=3;
            put(c,"finite_active",finite);put(c,"ceiling_pass",ceiling);put(c,"peak_growth_pass",true);put(c,"master_gr_pass",gr);
            put(c,"loudness_delta_lu",output.lufs-input.lufs);put(c,"loudness_match_pass",std::abs(output.lufs-input.lufs)<=.5);
            put(c,"input_pcm_sha256",pcmHash(x));put(c,"output_pcm_sha256",pcmHash(result.audio));checks.add(c);passed=passed&&finite&&ceiling&&gr;
            if(variant==0&&level==0)
            {
                wave(folder.getChildFile("audio/"+id+"-processed.wav"),result.audio);auto matched=result.audio;
                matched.applyGain(static_cast<float>(std::pow(10.,(input.lufs-output.lufs)/20)));wave(folder.getChildFile("audio/"+id+"-matched.wav"),matched);
                put(row,"input_pcm_sha256",pcmHash(x));put(row,"output_pcm_sha256",pcmHash(result.audio));
            }
        }
        for(double multiplier:{.8,1.2})
        {
            const auto result=render(p,fixture(recipe,1,recipe.bpm*multiplier),recipe.bpm*multiplier);const auto output=metrics(result.audio);
            auto c=object();put(c,"bpm",recipe.bpm*multiplier);put(c,"output",data(output));const bool safe=output.finite&&output.peak> -110&&output.truePeak<=-1;
            put(c,"safety_pass",safe);checks.add(c);passed=passed&&safe;
        }
        auto repair=object();put(repair,"kind","master_ceiling_margin_only");put(repair,"original_executable_sha256",row["executable_sha256"]);
        put(repair,"original_receipt_sha256",juce::SHA256(before).toHexString());put(repair,"ceiling_before_db",prior);put(repair,"ceiling_after_db",prior-.05);
        put(row,"repair",repair);put(row,"executable_sha256",juce::SHA256(juce::File::getSpecialLocation(juce::File::currentExecutableFile)).toHexString());
        put(row,"checks",checks);put(row,"calibration",calibrationData(p,original));put(row,"safety_pass",passed);save(file,row);
        std::cout<<id<<(passed?" repaired safely":" REPAIR FAILED")<<std::endl;return passed?0:1;
    }
    catch(const std::exception& error){std::cerr<<error.what()<<std::endl;return 1;}
}

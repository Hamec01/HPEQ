#include "Rack/RackPresets.h"
#include "Rack/NativeApi.h"
#include <HamloRackDspIdentity.h>
#include <BinaryData.h>
#include <juce_cryptography/juce_cryptography.h>
#include <cmath>
#include <stdexcept>

namespace hpeq::rack
{
namespace
{
FxType typeFor(const juce::String& name)
{
    for(auto type:{FxType::Compressor,FxType::Limiter,FxType::Reverb,FxType::Delay,
        FxType::TransientShaper,FxType::Saturator,FxType::MonstaFx,
        FxType::StereoProcessor,FxType::GateExpander,FxType::Gain})
        if(native::productName(type)==name)return type;
    throw std::invalid_argument("Unknown rack module: "+name.toStdString());
}
std::vector<FxSlotState> chain(const juce::var& list)
{
    if(!list.isArray() || list.size()>kMaxFxPerNode)throw std::invalid_argument("Invalid rack chain");
    std::vector<FxSlotState> result;
    for(const auto& m:*list.getArray())
    {
        auto slot=makeNativeModule(typeFor(m["plugin"].toString()));
        const auto amount=static_cast<double>(m["amount"]);
        if(!std::isfinite(amount)||amount<0||amount>1)throw std::invalid_argument("Invalid module amount");
        slot.mix=static_cast<float>(amount);
        const auto* params=m["parameters"].getDynamicObject();
        if(params==nullptr)throw std::invalid_argument("Missing native parameters");
        for(const auto& p:params->getProperties())slot.parameters.set(p.name,p.value);
        if(slot.type==FxType::MonstaFx)
        {
            native::Scene scene;
            const auto s=m["scene"];
            if(s["lanes"].size()!=7||s["locks"].size()!=3||s["allow_masks"].size()!=3)
                throw std::invalid_argument("Invalid Revolution scene dimensions");
            for(int lane=0;lane<7;++lane)
            {
                if(s["lanes"][lane].size()!=16)throw std::invalid_argument("Invalid scene length");
                for(int step=0;step<16;++step)
                {
                    const int action=s["lanes"][lane][step];
                    if(action<0||action>255)throw std::invalid_argument("Invalid scene action");
                    scene.lanes[static_cast<size_t>(lane)][static_cast<size_t>(step)]=static_cast<std::uint8_t>(action);
                }
                if(lane<3)
                {
                    scene.locks[static_cast<size_t>(lane)]=static_cast<std::uint16_t>(static_cast<int>(s["locks"][lane]));
                    scene.allow[static_cast<size_t>(lane)]=static_cast<std::uint16_t>(static_cast<int>(s["allow_masks"][lane]));
                }
            }
            juce::String error;
            slot.nativeState=native::makeRevolutionScene(slot.parameters,scene,error);
            if(slot.nativeState.getSize()==0)throw std::invalid_argument(error.toStdString());
        }
        juce::String error;
        if(!isModuleSupported(slot,&error))throw std::invalid_argument(error.toStdString());
        result.push_back(std::move(slot));
    }
    return result;
}
}
const std::vector<Recipe>& recipes()
{
    static const auto bank=[] {
        int size=0;
        const auto* bytes=HpeqAssets::getNamedResource("recipes_json",size);
        const auto root=juce::JSON::parse(juce::String::fromUTF8(bytes,size));
        if(root["format"].toString()!="HamloRackDesign"||root["presets"].size()!=250)
            throw std::runtime_error("Embedded HamloRack bank is invalid");
        std::vector<Recipe> out;
        int powerSize=0;const auto* powerBytes=HpeqAssets::getNamedResource("power_json",powerSize);
        const auto power=juce::JSON::parse(juce::String::fromUTF8(powerBytes,powerSize));
        if(power["format"].toString()!="HamloRackPower"||power["presets"].size()!=20)
            throw std::runtime_error("Embedded Rack Power bank is invalid");
        for(const auto& row:*power["presets"].getArray())
            out.push_back({row["id"],"Rack Power / "+row["bank"].toString(),row["category"],row["name"],row["intent"],row["reject_if"],row["bpm"],row});
        for(const auto& row:*root["presets"].getArray())
            out.push_back({row["id"],row["bank"],row["category"],row["name"],row["intent"],row["reject_if"],row["bpm"],row});
        return out;
    }();
    return bank;
}
juce::String bankSha256()
{
    int size=0;const auto* bytes=HpeqAssets::getNamedResource("recipes_json",size);
    return juce::SHA256(bytes,static_cast<size_t>(size)).toHexString();
}
juce::String dspSourceSha256() { return HPEQ_RACK_DSP_SOURCE_SHA256; }
bool applyCalibration(const juce::var& entry,FxProjectState& p,juce::String& error)
{
    if(entry.isVoid())return true;
    p.inputTrimDb=static_cast<double>(entry.getProperty("input_trim_db",0.0));
    p.outputTrimDb=static_cast<double>(entry.getProperty("output_trim_db",0.0));
    if(auto* overrides=entry["overrides"].getArray())for(const auto& o:*overrides)
    {
        const int layer=o["layer"],node=o["node"],slot=o["slot"];
        if(layer<0||layer>=kNumLayers){error="Invalid calibration layer";return false;}
        auto& l=p.layers[static_cast<size_t>(layer)];std::vector<FxSlotState>* c=node==8?&l.mixChain:nullptr;
        for(auto& n:l.fxNodes)if(n.engineSlot==node)c=&n.chain;
        for(auto& n:l.eqNodes)if(n.engineSlot==node)c=&n.chain;
        if(c==nullptr||slot<0||slot>=static_cast<int>(c->size())||o["parameters"].getDynamicObject()==nullptr)
        {error="Invalid calibration slot";return false;}
        auto& target=(*c)[static_cast<size_t>(slot)];
        for(const auto& kv:o["parameters"].getDynamicObject()->getProperties())target.parameters.set(kv.name,kv.value);
        if(!isModuleSupported(target,&error))return false;
    }
    if(!std::isfinite(p.inputTrimDb)||!std::isfinite(p.outputTrimDb)||p.inputTrimDb< -48||p.inputTrimDb>24||p.outputTrimDb< -48||p.outputTrimDb>24)
    {error="Invalid calibration trim";return false;}
    return true;
}
bool makeProject(const Recipe& recipe,FxProjectState& output,juce::String& error,bool calibrated)
{
    try
    {
        if(const auto* layers=recipe.data["layers"].getArray())
        {
            if(layers->isEmpty()||layers->size()>kNumLayers)throw std::invalid_argument("Invalid Rack Power layer count");
            FxProjectState p;for(auto& l:p.layers)l.enabled=false;
            p.routing=recipe.data["routing"].toString()=="Serial"?LayerRouting::Serial:LayerRouting::Parallel;
            for(int i=0;i<layers->size();++i)
            {
                Recipe sub=recipe;sub.category="Power layer";sub.data=(*layers)[i].clone();
                sub.data.getDynamicObject()->setProperty("output_mode","insert");
                FxProjectState one;if(!makeProject(sub,one,error,false))return false;
                auto& layer=p.layers[static_cast<size_t>(i)];layer=std::move(one.layers[0]);
                layer.name=(*layers)[i]["name"].toString();layer.mix=static_cast<float>((*layers)[i]["mix"]);
                if(!std::isfinite(layer.mix)||layer.mix<0||layer.mix>1)throw std::invalid_argument("Invalid Power layer mix");
                for(auto& n:layer.eqNodes)n.id=p.nextNodeId++;for(auto& n:layer.fxNodes)n.id=p.nextNodeId++;
            }
            p.referenceBpm=recipe.bpm;p.rackPresetId=recipe.id;
            p.outputTrimDb=static_cast<double>(recipe.data["output_trim_db"]);
            sanitize(p);output=std::move(p);error.clear();return true;
        }
        if(!recipe.data["eq"].isArray()||recipe.data["eq"].size()>kMaxEqNodesPerLayer
            ||!recipe.data["regions"].isArray()||recipe.data["regions"].size()>kMaxFxNodesPerLayer)
            throw std::invalid_argument("Invalid rack geometry");
        if(recipe.data["output_mode"].toString()!="wet_only"&&recipe.data["output_mode"].toString()!="insert")
            throw std::invalid_argument("Invalid rack output mode");
        FxProjectState p;
        for(auto& layer:p.layers)layer.enabled=false;
        auto& layer=p.layers[0];layer.enabled=true;layer.name=recipe.name;
        for(const auto& e:*recipe.data["eq"].getArray())
        {
            EqNodeState n;n.id=p.nextNodeId++;
            bool found=false;
            for(auto shape:{EqShape::Bell,EqShape::LowShelf,EqShape::HighShelf,EqShape::LowCut,EqShape::HighCut,
                EqShape::Notch,EqShape::BandPass,EqShape::Tilt,EqShape::AllPass})
                if(toString(shape)==e["shape"].toString()){n.shape=shape;found=true;break;}
            if(!found)throw std::invalid_argument("Unknown rack EQ shape");
            n.frequencyHz=e["hz"];n.gainDb=e["gain_db"];n.q=e["q"];n.slopeDbPerOct=e["slope_db_oct"];
            if(e.hasProperty("dynamic"))
            {
                const auto d=e["dynamic"];n.dynamic=true;
                n.thresholdDb=d["threshold_db"];n.ratio=d["ratio"];n.attackMs=d["attack_ms"];
                n.releaseMs=d["release_ms"];n.rangeDb=d["range_db"];
            }
            layer.eqNodes.push_back(std::move(n));
        }
        layer.mixChain=chain(recipe.data["chain"]);
        if(recipe.category=="Master")
        {
            if(layer.mixChain.empty()||layer.mixChain.back().type!=FxType::Limiter)
                throw std::invalid_argument("Master recipe has no final limiter");
            p.routing=LayerRouting::Serial;p.masterRoute=true;
            auto& final=p.layers.back();final.enabled=true;final.name="Master ceiling";
            final.mixChain={layer.mixChain.back()};layer.mixChain.pop_back();
        }
        for(const auto& z:*recipe.data["regions"].getArray())
        {
            FxNodeState n;n.id=p.nextNodeId++;n.lowHz=z["low_hz"];n.highHz=z["high_hz"];n.chain=chain(z["chain"]);
            layer.fxNodes.push_back(std::move(n));
        }
        p.wetOnly=recipe.data["output_mode"].toString()=="wet_only";
        p.referenceBpm=recipe.bpm;
        p.outputTrimDb=static_cast<double>(recipe.data.getProperty("output_trim_db",0.0));
        p.inputTrimDb=static_cast<double>(recipe.data.getProperty("input_trim_db",0.0));
        p.rackPresetId=recipe.id;
        sanitize(p);
        if(calibrated)
        {
            static const auto calibration=[] {int size=0;const auto* bytes=HpeqAssets::getNamedResource("calibration_json",size);
                return bytes?juce::JSON::parse(juce::String::fromUTF8(bytes,size)):juce::var();}();
            const auto* entries=calibration["entries"].getDynamicObject();
            if(calibration["format"].toString()!="HamloRackCalibration"||static_cast<int>(calibration["version"])!=3
                ||calibration["dsp_source_sha256"].toString()!=dspSourceSha256()
                ||calibration["bank_sha256"].toString()!=bankSha256()||entries==nullptr||entries->getProperties().size()!=250)
                throw std::invalid_argument("Embedded rack calibration is missing or stale");
            const auto entry=calibration["entries"][juce::Identifier(recipe.id)];
            if(!entry.isObject())throw std::invalid_argument("Missing calibration for "+recipe.id.toStdString());
            if(!applyCalibration(entry,p,error))return false;
        }
        output=std::move(p);error.clear();return true;
    }
    catch(const std::exception& e){error=e.what();return false;}
}
}

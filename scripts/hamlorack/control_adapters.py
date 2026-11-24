"""Rack-only external detector extension; pinned standalone payloads stay intact.

Keep original compressor kernels verbatim. Generate separate external-key kernels
from them so the disabled path continues to call exactly the original methods.
"""
def replace_once(text, old, new):
    if text.count(old) != 1:
        raise ValueError('Native Comp adapter anchor: '+old[:100])
    return text.replace(old, new, 1)

def function(text, signature):
    start=text.index(signature); opening=text.index('{',start); depth=1; end=opening+1
    while depth:
        depth += (text[end]=='{')-(text[end]=='}'); end+=1
    return text[start:end]

def comp(text, path):
    if path=='Source/Plugin/HamloCompProcessor.h':
        return replace_once(text,'\npublic:\n','\npublic:\n    const float* const* rackKey=nullptr; int rackKeyChannels=0;\n')
    if path=='Source/Plugin/HamloCompProcessor.cpp':
        return replace_once(text,'        compressor.process(wet, channels, count, settings, true);',
            '''        if(rackKey && rackKeyChannels>0) {
            const float* key[2]{rackKey[0]+start,rackKeyChannels>1?rackKey[1]+start:nullptr};
            compressor.processExternal(wet,channels,count,settings,true,key,rackKeyChannels);
        } else compressor.process(wet, channels, count, settings, true);''')
    if path=='Source/DSP/Compressor.h':
        anchor='    void process(float* const* channels, int numChannels, int numSamples, const CompressorSettings&, bool linked = true) noexcept;'
        return replace_once(text,anchor,anchor+'''
    void processExternal(float* const*,int,int,const CompressorSettings&,bool,const float* const*,int) noexcept;
    void processModelExternal(float* const*,int,int,const CompressorSettings&,bool,int,const float* const*,int) noexcept;''')
    if path!='Source/DSP/Compressor.cpp':return text
    classic=function(text,'void CompressorProcessor::process(')
    external=replace_once(classic,'bool linked) noexcept','bool linked,const float* const* key,int keyChannels) noexcept')
    external=replace_once(external,'CompressorProcessor::process(','CompressorProcessor::processExternal(')
    external=replace_once(external,'processModel(channels, numChannels, numSamples, p, linked, model);',
                          'processModelExternal(channels,numChannels,numSamples,p,linked,model,key,keyChannels);')
    external=replace_once(external,'for (int ch = 0; ch < numChannels; ++ch)\n                for (int i',
                          'for (int ch = 0; ch < keyChannels; ++ch)\n                for (int i')
    external=replace_once(external,'sidechain(ch, channels[ch][start + i])','sidechain(ch, key[ch][start + i])')
    external=replace_once(external,'for (int ch = 0; ch < numChannels; ++ch)\n                {',
                          'for (int ch = 0; ch < keyChannels; ++ch)\n                {')
    external=replace_once(external,': channels[ch] + start',': key[ch] + start')
    external=replace_once(external,': channels[d] + start',': key[std::min(d,keyChannels-1)] + start')
    model=function(text,'void CompressorProcessor::processModel(')
    keyed=replace_once(model,'bool linked, int model) noexcept','bool linked,int model,const float* const* key,int keyChannels) noexcept')
    keyed=replace_once(keyed,'CompressorProcessor::processModel(','CompressorProcessor::processModelExternal(')
    keyed=replace_once(keyed,'for (int ch = first; ch < last; ++ch)\n            {\n                const float heard',
                       'for (int ch = first; ch < (linked?keyChannels:last); ++ch)\n            {\n                const float heard')
    keyed=replace_once(keyed,'model == kFet ? static_cast<float>(lastOut[static_cast<size_t>(ch)]) : channels[ch][i]',
        'key[std::min(ch,keyChannels-1)][i] * (model==kFet?static_cast<float>(std::pow(10.,-smoothedReductionDb[static_cast<size_t>(d)]/20.)):1.f)')
    # Insert inside the isolated DSP namespace, without editing original kernels.
    return text.replace(classic,classic+'\n\n'+external,1).replace(model,model+'\n\n'+keyed,1)

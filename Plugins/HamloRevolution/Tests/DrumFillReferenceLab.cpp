// R15 reference measurement: runs the PHRASE FILL analyser over a manifest of reference fills and dumps every
// detected hit of the LAST 4 BEATS of each file (the fill bar). Statistics are aggregated by Tools/r15_reference.py.
// Manifest lines: group<TAB>bpm<TAB>path. Output lines: group, file, bpm, tick (0..96), role, conf, peak / file max,
// band shares (sub, low-mid, mid, high), decay ms, secondary flag, grid phase ms.
// Usage: HamloRevolution_DrumFillReferenceLab <manifest.tsv> <hits.tsv>
#include "DSP/DrumPhraseFill.h"
#include <juce_audio_formats/juce_audio_formats.h>
#include <iostream>

namespace ph = hpeq::dsp::transition::phrase;

int main(int argc, char** argv)
{
    if (argc != 3)
        return 2;
    juce::AudioFormatManager formats;
    formats.registerBasicFormats();
    juce::StringArray lines;
    juce::File(juce::String::fromUTF8(argv[1])).readLines(lines);
    juce::FileOutputStream out(juce::File(juce::String::fromUTF8(argv[2])));
    if (!out.openedOk())
        return 3;
    out.setPosition(0);
    out.truncate();
    out << "group\tfile\tbpm\ttick\trole\tconf\tpeak\tsub\tlowmid\tmid\thigh\tdecay\tsecondary\tphase_ms\n";
    int done = 0, skipped = 0;
    for (const auto& line : lines)
    {
        const auto parts = juce::StringArray::fromTokens(line, "\t", "");
        if (parts.size() != 3)
            continue;
        const double bpm = parts[1].getDoubleValue();
        const juce::File file(parts[2]);
        std::unique_ptr<juce::AudioFormatReader> reader(formats.createReaderFor(file));
        if (!reader || bpm <= 0)
        {
            ++skipped;
            std::cerr << "unreadable: " << file.getFullPathName() << "\n";
            continue;
        }
        const double fs = reader->sampleRate, beat = fs * 60.0 / bpm;
        const int total = static_cast<int>(reader->lengthInSamples), bar = static_cast<int>(std::lround(4 * beat));
        if (total < static_cast<int>(3.5 * beat)) // shorter than a bar: not a 4-beat fill
        {
            ++skipped;
            continue;
        }
        // the fill bar: the last 4 beats, rounded to whole beats from the file start
        const int beats = std::max(4, static_cast<int>(std::lround(total / beat)));
        const int start = std::max(0, static_cast<int>(std::lround((beats - 4) * beat)));
        juce::AudioBuffer<float> x(2, bar);
        x.clear();
        juce::AudioBuffer<float> raw(static_cast<int>(reader->numChannels), total);
        reader->read(&raw, 0, total, 0, true, true);
        for (int c = 0; c < 2; ++c)
            x.copyFrom(c, 0, raw, std::min(c, raw.getNumChannels() - 1), start, std::min(bar, total - start));
        const float top = x.getMagnitude(0, x.getNumSamples());
        if (top <= 0)
        {
            ++skipped;
            continue;
        }
        const auto a = ph::analyse(x, fs, bpm);
        for (const auto& o : a.onsets)
        {
            const double sum = o.novelty[0] + o.novelty[1] + o.novelty[2] + o.novelty[3] + 1e-30;
            out << parts[0] << "\t" << file.getFileName() << "\t" << bpm << "\t" << juce::String(o.tick, 3) << "\t" << o.role << "\t"
                << juce::String(o.conf, 3) << "\t" << juce::String(o.peak / top, 4) << "\t" << juce::String(o.novelty[0] / sum, 4) << "\t"
                << juce::String(o.novelty[1] / sum, 4) << "\t" << juce::String(o.novelty[2] / sum, 4) << "\t" << juce::String(o.novelty[3] / sum, 4) << "\t"
                << juce::String(o.decayMs, 1) << "\t" << (o.secondary ? 1 : 0) << "\t" << juce::String(a.phase / fs * 1000, 2) << "\n";
        }
        ++done;
    }
    std::cout << "analysed " << done << " files, skipped " << skipped << std::endl;
    return 0;
}

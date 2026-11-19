#include "UI/HamloRevolutionTransitionPage.h"

#include <cmath>
#include <iterator>

#include "DSP/TransitionRender.h"
#include "UI/BlackSteel.h"
#include "UI/RevolutionHelp.h"

namespace hpeq::ui
{
namespace
{
using namespace hpeq::ui::steel;
namespace tr = hpeq::dsp::transition;

constexpr float kW = 1180.0f, kH = 1080.0f;
constexpr float kPi = juce::MathConstants<float>::pi;
const juce::Rectangle<float> kBack { 960.0f, 28.0f, 180.0f, 44.0f }, kUseInput { 800.0f, 28.0f, 150.0f, 44.0f };
juce::Rectangle<float> kindRect(int k) { return { 60.0f + 216.0f * static_cast<float>(k), 196.0f, 200.0f, 48.0f }; }
juce::Rectangle<float> barsRect(int i) { return { 700.0f + 110.0f * static_cast<float>(i), 196.0f, 96.0f, 48.0f }; }
constexpr std::array<int, 3> kBarChoices { 1, 2, 4 };
const std::array<juce::Point<float>, 4> kKnobs { { { 170.0f, 380.0f }, { 420.0f, 380.0f }, { 670.0f, 380.0f }, { 920.0f, 380.0f } } };
constexpr float kKnobScale = 0.85f;
const juce::Point<float> kGenerate { 140.0f, 556.0f };
const juce::Rectangle<float> kPlay { 230.0f, 532.0f, 170.0f, 48.0f }, kShowFile { 420.0f, 532.0f, 190.0f, 48.0f };
const juce::Rectangle<float> kStatus { 640.0f, 516.0f, 500.0f, 80.0f };
const juce::Rectangle<float> kWave { 60.0f, 640.0f, 1060.0f, 330.0f };
const juce::Rectangle<float> kRecipe { 420.0f, 118.0f, 720.0f, 30.0f };
const juce::Rectangle<float> kPhrase { 60.0f, 260.0f, 330.0f, 28.0f }, kFx { 420.0f, 260.0f, 330.0f, 28.0f };
const juce::Rectangle<float> kVariation { 800.0f, 602.0f, 320.0f, 28.0f };
juce::Rectangle<float> kitRect(int role,int count=3){const float step=1080.0f/static_cast<float>(count);return {60.0f+step*static_cast<float>(role),480.0f,step-30.0f,28.0f};}
constexpr const char* kitNames[]{"KICK","SNARE","HAT","PERC","TOM","SAMPLE"}; // R16: SAMPLE = not a drum (marks only)
const juce::Colour kitColours[]{juce::Colour(0xffffc75d),juce::Colour(0xffff75b9),juce::Colour(0xff6ddfe8),juce::Colour(0xffb48cff),juce::Colour(0xffff9a4d),juce::Colour(0xffc9d3dc)};
static_assert(std::size(kitNames) == tr::kSampleMark + 1);
bool phraseRecipe(int kind,int recipe){return kind==tr::Fill&&recipe>=tr::kPhraseRecipe;}
int kitCount(int kind,int recipe){return phraseRecipe(kind,recipe)?tr::kKitRoles:3;}
// R16: GRID / BPM / CLEAR row under the controls, left of NEW VARIATION
const juce::Rectangle<float> kGrid { 60.0f, 602.0f, 200.0f, 28.0f }, kBpm { 275.0f, 602.0f, 250.0f, 28.0f }, kClear { 540.0f, 602.0f, 240.0f, 28.0f };
constexpr const char* gridNames[] { "OFF", "1/4", "1/8", "1/16", "1/32", "1/8 T", "1/16 T" };
constexpr int gridPerBeat[] { 0, 1, 2, 4, 8, 3, 6 };
constexpr int kGrids = 7;
constexpr double kMinView = 1.0 / 64.0; // the deepest zoom: 1/64 of the wave
} // namespace

// waveform columns run from kWave.x + 4 over width - 8 (as the peaks are drawn), over the shown view
float HamloRevolutionTransitionPage::fractionToX(double fraction) const
{
    return kWave.getX() + 4.0f + (kWave.getWidth() - 8.0f) * static_cast<float>((fraction - viewStart) / std::max(1.0e-9, viewEnd - viewStart));
}

double HamloRevolutionTransitionPage::xToFraction(float x) const
{
    return viewStart + (viewEnd - viewStart) * juce::jlimit(0.0, 1.0, static_cast<double>((x - kWave.getX() - 4.0f) / (kWave.getWidth() - 8.0f)));
}

int HamloRevolutionTransitionPage::xToSample(float x, int length) const
{
    return juce::jlimit(0, std::max(0, length - 1), static_cast<int>(xToFraction(x) * length));
}

int HamloRevolutionTransitionPage::snapToGrid(int sample) const
{
    const int grid = juce::jlimit(0, kGrids - 1, values.grid);
    if (grid == 0 || sourceLength <= 0)
        return sample;
    const double steps = static_cast<double>(values.bars * 4 * gridPerBeat[grid]), step = sourceLength / steps;
    return juce::jlimit(0, sourceLength, static_cast<int>(std::lround(std::round(sample / step) * step)));
}

double HamloRevolutionTransitionPage::bpmFromName(const juce::String& fileName)
{
    // the digits right before "bpm" (fill_88bpm, Loop 92 BPM, 140bpm_...)
    const auto lower = fileName.toLowerCase();
    for (int at = lower.indexOf("bpm"); at >= 0; at = lower.indexOf(at + 1, "bpm"))
    {
        int end = at;
        while (end > 0 && lower[end - 1] == ' ')
            --end;
        int start = end;
        while (start > 0 && juce::CharacterFunctions::isDigit(lower[start - 1]))
            --start;
        const double bpm = lower.substring(start, end).getDoubleValue();
        if (end - start >= 2 && bpm >= 40.0 && bpm <= 250.0)
            return bpm;
    }
    return 0.0;
}

void HamloRevolutionTransitionPage::updatePeaks()
{
    const int columns = static_cast<int>(kWave.getWidth()) - 8;
    const auto peaksOf = [&](const juce::AudioBuffer<float>& b, std::vector<std::pair<float, float>>& out)
    {
        const int n = b.getNumSamples();
        out.assign(static_cast<size_t>(columns), { 0.0f, 0.0f });
        if (n == 0 || b.getNumChannels() == 0)
            return;
        const double first = viewStart * n, span = (viewEnd - viewStart) * n;
        for (int c = 0; c < columns; ++c)
        {
            const int from = juce::jlimit(0, n - 1, static_cast<int>(first + span * c / columns));
            const int to = juce::jlimit(from + 1, n, static_cast<int>(first + span * (c + 1) / columns));
            float lo = 0.0f, hi = 0.0f;
            for (int i = from; i < to; ++i)
                for (int ch = 0; ch < b.getNumChannels(); ++ch)
                {
                    lo = std::min(lo, b.getSample(ch, i));
                    hi = std::max(hi, b.getSample(ch, i));
                }
            out[static_cast<size_t>(c)] = { lo, hi };
        }
    };
    peaksOf(clipBuffer, peaks);
    peaksOf(sourceBuffer, sourcePeaks);
    if (sourceBuffer.getNumSamples() == 0)
        sourcePeaks.clear();
}

void HamloRevolutionTransitionPage::setView(double start, double end)
{
    const double width = juce::jlimit(kMinView, 1.0, end - start);
    viewStart = juce::jlimit(0.0, 1.0 - width, start);
    viewEnd = viewStart + width;
    updatePeaks();
    repaint();
}

void HamloRevolutionTransitionPage::zoomAround(double fraction, double factor)
{
    const double width = juce::jlimit(kMinView, 1.0, (viewEnd - viewStart) * factor);
    const double at = (fraction - viewStart) / std::max(1.0e-9, viewEnd - viewStart); // where the mouse sits in the view
    setView(fraction - at * width, fraction - at * width + width);
}

HamloRevolutionTransitionPage::HamloRevolutionTransitionPage() { setOpaque(true); }

void HamloRevolutionTransitionPage::setValues(const Values& v)
{
    values = v;
    repaint();
}

void HamloRevolutionTransitionPage::setSource(const juce::AudioBuffer<float>& source)
{
    if (source.getNumSamples() != sourceLength) // another source length: the old zoom and selection mean nothing
    {
        viewStart = 0.0;
        viewEnd = 1.0;
        selection = {};
    }
    sourceLength = source.getNumSamples();
    sourceBuffer.makeCopyOf(source);
    updatePeaks();
    repaint();
}

juce::Range<int> HamloRevolutionTransitionPage::hitAt(int sample) const
{
    juce::Range<int> found;
    for (const auto& h : values.hits)
    {
        if (h.contains(sample))
            return h;
        if (h.getStart() <= sample)
            found = h;
    }
    return found;
}

void HamloRevolutionTransitionPage::markRegion(int start, int end, int role)
{
    if (end < start)
        std::swap(start, end);
    if (end - start < 8)
        return;
    auto& marks = values.marks;
    markHistory.push_back(marks);
    if (markHistory.size() > 64)
        markHistory.erase(markHistory.begin());
    marks.erase(std::remove_if(marks.begin(), marks.end(), [&](const tr::Mark& m) { return m.start < end && start < m.end; }), marks.end());
    selection = {};
    if (role >= 0 && role <= tr::kSampleMark)
        marks.push_back({ start, end, role });
    std::sort(marks.begin(), marks.end(), [](const tr::Mark& a, const tr::Mark& b) { return a.start < b.start; });
    if (onMarks)
        onMarks();
    repaint();
}

int HamloRevolutionTransitionPage::markAt(int sample) const
{
    for (size_t i = 0; i < values.marks.size(); ++i)
        if (sample >= values.marks[i].start && sample < values.marks[i].end)
            return static_cast<int>(i);
    return -1;
}

void HamloRevolutionTransitionPage::undoMark()
{
    if (markHistory.empty())
        return;
    values.marks = markHistory.back();
    markHistory.pop_back();
    if (onMarks)
        onMarks();
    repaint();
}

void HamloRevolutionTransitionPage::clearMarks()
{
    if (values.marks.empty())
        return;
    markHistory.push_back(values.marks);
    values.marks.clear();
    if (onMarks)
        onMarks();
    repaint();
}

void HamloRevolutionTransitionPage::setClip(const juce::AudioBuffer<float>& clip)
{
    clipBuffer.makeCopyOf(clip);
    updatePeaks();
    repaint();
}

juce::Point<float> HamloRevolutionTransitionPage::toDesign(juce::Point<float> p) const { return (p - offset) / scale; }

void HamloRevolutionTransitionPage::resized()
{
    scale = juce::jmin(static_cast<float>(getWidth()) / kW, static_cast<float>(getHeight()) / kH);
    offset = { (static_cast<float>(getWidth()) - kW * scale) * 0.5f, (static_cast<float>(getHeight()) - kH * scale) * 0.5f };
    staticLayer = {};
    if (getWidth() <= 0 || getHeight() <= 0)
        return;
    staticLayer = juce::Image(juce::Image::ARGB, getWidth(), getHeight(), true);
    juce::Graphics g(staticLayer);
    g.fillAll(juce::Colours::black);
    g.addTransform(juce::AffineTransform::scale(scale).translated(offset));
    panel(g, kW, kH, 13);
    engraved(g, "TRANSITION", { 420.0f, 30.0f, 360.0f, 44.0f }, 30.0f);
    engraved(g, "FROM THE LAST BARS", { 420.0f, 78.0f, 360.0f, 24.0f }, 13.0f, stencilDim);
    engraved(g, "BARS", { 700.0f, 166.0f, 200.0f, 22.0f }, 13.0f, stencil, juce::Justification::centredLeft);
    for (const auto& k : kKnobs)
        knobScale(g, k, kKnobScale);
    engraved(g, "GENERATE", { kGenerate.x - 70.0f, kGenerate.y + 40.0f, 140.0f, 20.0f }, 12.0f);
    lcdWindow(g, kStatus);
    for (const float y : { 160.0f, 270.0f, 500.0f, 620.0f })
    {
        g.setColour(juce::Colours::black);
        g.drawHorizontalLine(juce::roundToInt(y), 20.0f, kW - 20.0f);
        g.setColour(juce::Colours::white.withAlpha(0.07f));
        g.drawHorizontalLine(juce::roundToInt(y) - 1, 20.0f, kW - 20.0f);
    }
    crtBezel(g, kWave.expanded(10.0f), kWave, "", kWave.getBottom() + 10.0f);
    engraved(g, "DRAG THE WAVE INTO YOUR DAW  /  DROP A SAMPLE ONTO THE PAGE", { kWave.getX(), kWave.getBottom() + 18.0f, kWave.getWidth(), 22.0f }, 14.0f);
}

void HamloRevolutionTransitionPage::paint(juce::Graphics& g)
{
    if (staticLayer.isValid())
        g.drawImageAt(staticLayer, 0, 0);
    g.addTransform(juce::AffineTransform::scale(scale).translated(offset));
    crtButton(g, kBack, "BACK", false);
    crtButton(g, kUseInput, "USE INPUT", values.sourceName.isEmpty());
    crtButton(g, kindRect(0), "RISE", values.kind == tr::Rise);
    crtButton(g, kindRect(1), "DRUM FILL", values.kind == tr::Fill);
    tr::Settings recipeSettings;recipeSettings.kind=values.kind;recipeSettings.recipe=values.recipe;recipeSettings.seed=values.seed;
    juce::String recipeLabel="STYLE / ";recipeLabel<<tr::recipeName(values.kind,values.recipe);
    if(values.recipe==1||values.recipe==8||values.recipe==21)recipeLabel<<" - "<<tr::recipeName(values.kind,tr::resolvedRecipe(recipeSettings));
    crtButton(g,kRecipe,recipeLabel+"  v",values.recipe!=0);
    if(values.kind==tr::Fill&&values.recipe>=8)
    {
        crtButton(g,kPhrase,values.fillBeats==tr::kWholeLoop?juce::String("EDIT / WHOLE LOOP  v"):"EDIT LAST / "+juce::String(values.fillBeats)+" BEAT"+(values.fillBeats==1?"":"S")+"  v",true);
        crtButton(g,kFx,"FILTER + ROOM / "+juce::String(juce::roundToInt(values.fillFx*100))+" %  v",values.fillFx>0);
    }
    crtButton(g,kVariation,"NEW VARIATION / SAME SOURCE",false);
    crtButton(g, kGrid, juce::String("GRID / ") + gridNames[juce::jlimit(0, kGrids - 1, values.grid)] + "  v", values.grid != 0);
    crtButton(g, kBpm, values.sourceName.isEmpty() ? "BPM / HOST " + juce::String(juce::roundToInt(values.hostBpm))
                       : "BPM / " + (values.sampleBpm > 0.0 ? juce::String(values.sampleBpm, values.sampleBpm == std::round(values.sampleBpm) ? 0 : 1) : "HOST " + juce::String(juce::roundToInt(values.hostBpm))) + "  v",
              values.sampleBpm > 0.0 && values.sourceName.isNotEmpty());
    crtButton(g, kClear, "CLEAR / START OVER", false);
    if(values.kind==tr::Fill&&values.recipe>=21)for(int r=0;r<kitCount(values.kind,values.recipe);++r)
        crtButton(g,kitRect(r,kitCount(values.kind,values.recipe)),juce::String(kitNames[r])+" / "+(values.kitEnabled[static_cast<size_t>(r)]?"ON":"OFF")+" / "+juce::String(values.kitCounts[static_cast<size_t>(r)])+" SLICES",values.kitEnabled[static_cast<size_t>(r)]);
    for (int i = 0; i < 3; ++i)
        crtButton(g, barsRect(i), juce::String(kBarChoices[static_cast<size_t>(i)]), values.bars == kBarChoices[static_cast<size_t>(i)]);
    for (int k = 0; k < 4; ++k)
    {
        const auto c = kKnobs[static_cast<size_t>(k)];
        const double v = juce::jlimit(0.0, 1.0, values.knobs[static_cast<size_t>(k)]);
        brassKnob(g, c, (-135.0f + static_cast<float>(v) * 270.0f) * kPi / 180.0f, kKnobScale, 1.0f, KnobAsset::LatheSteelRust);
        engraved(g, tr::knobName(values.kind, k), { c.x - 90.0f, c.y - kKnobScale * 60.0f - 34.0f, 180.0f, 20.0f }, 15.0f);
        engraved(g, juce::String(juce::roundToInt(100.0 * v)) + " %", { c.x - 90.0f, c.y + kKnobScale * 60.0f + 12.0f, 180.0f, 22.0f }, 15.0f, stencilDim);
    }
    {
        juce::ColourGradient red(juce::Colour(0xffff5a4a), kGenerate.x - 10.0f, kGenerate.y - 20.0f, juce::Colour(0xff7a0d08), kGenerate.x + 20.0f,
                                 kGenerate.y + 30.0f, true);
        g.setColour(juce::Colour(0xff0b0c0b));
        g.fillEllipse(kGenerate.x - 36.0f, kGenerate.y - 36.0f, 72.0f, 72.0f);
        g.setGradientFill(red);
        g.fillEllipse(kGenerate.x - 29.0f, kGenerate.y - 29.0f, 58.0f, 58.0f);
    }
    crtButton(g, kPlay, values.previewing ? "STOP" : "PLAY", values.previewing);
    crtButton(g, kShowFile, "SHOW FILE", false);
    const juce::String line1 = juce::String(values.kind == tr::Rise ? "RISE" : "FILL") + "  /  " + juce::String(values.bars) + (values.bars == 1 ? " BAR" : " BARS")
                               + "  /  " + juce::String(juce::roundToInt(values.bpm)) + " BPM";
    const juce::String line2 = values.sourceName.isNotEmpty() ? "FROM THE SAMPLE: " + values.sourceName.toUpperCase()
                               : !values.valid      ? juce::String("PRESS GENERATE")
                               : !values.complete ? juce::String("NOT ENOUGH HEARD - PLAY THE TRACK, THEN GENERATE")
                               : values.aligned   ? juce::String("FROM THE LAST BARS, ON THE BAR LINE")
                                                  : juce::String("FROM THE LAST AUDIO (TRANSPORT STOPPED)");
    lcdText(g, kStatus.withHeight(kStatus.getHeight() * 0.5f).translated(0.0f, 4.0f), line1, 16.0f);
    lcdText(g, kStatus.withTrimmedTop(kStatus.getHeight() * 0.5f).translated(0.0f, -4.0f), line2, 12.5f);

    // the waveform
    crtBegin(g, kWave);
    if(values.kind==tr::Fill&&values.recipe>=8)
    {
        const float fraction=values.fillBeats==tr::kWholeLoop?1.0f:static_cast<float>(values.fillBeats)/(values.bars*4.0f);
        g.setColour(juce::Colour(0xffffbc61).withAlpha(0.10f));
        const float from=std::max(kWave.getX(),fractionToX(1.0-fraction));
        if(from<kWave.getRight())g.fillRect(kWave.withLeft(fraction>=1.0f?kWave.getX():from));
    }
    const float mid = kWave.getCentreY(), half = kWave.getHeight() * 0.45f;
    float peak = 1.0e-6f;
    for (const auto& [lo, hi] : peaks)
        peak = std::max({ peak, -lo, hi });
    const bool phrase = phraseRecipe(values.kind, values.recipe) && sourceLength > 0 && !sourcePeaks.empty();
    if (phrase)
    {
        // R15: the source behind the result (what the marks point at), then the user's marks
        for (const auto& [lo, hi] : sourcePeaks)
            peak = std::max({ peak, -lo, hi });
        for (size_t c = 0; c < sourcePeaks.size(); ++c)
        {
            g.setColour(juce::Colours::white.withAlpha(0.16f));
            g.drawVerticalLine(juce::roundToInt(kWave.getX() + 4.0f + static_cast<float>(c)), mid - half * sourcePeaks[c].second / peak, mid - half * sourcePeaks[c].first / peak + 1.0f);
        }
        for (const auto& m : values.marks)
        {
            const float x0 = sampleToX(m.start, sourceLength), x1 = std::max(x0 + 2.0f, sampleToX(m.end, sourceLength));
            const int role = juce::jlimit(0, tr::kSampleMark, m.role);
            const auto colour = kitColours[role];
            const juce::Rectangle<float> box(x0, kWave.getY() + 44.0f, x1 - x0, kWave.getHeight() - 110.0f);
            g.setColour(colour.withAlpha(role == tr::kSampleMark ? 0.12f : 0.20f));
            g.fillRect(box);
            g.setColour(colour.withAlpha(0.9f));
            g.drawRect(box, 1.0f);
            g.setFont(10.0f);
            g.drawText(kitNames[role], juce::Rectangle<float>(x0 + 2.0f, kWave.getY() + 46.0f, 60.0f, 12.0f), juce::Justification::centredLeft);
        }
        if (!selection.isEmpty()) // R16: left-drag selection, waiting for a role
        {
            const float x0 = sampleToX(selection.getStart(), sourceLength), x1 = std::max(x0 + 2.0f, sampleToX(selection.getEnd(), sourceLength));
            const juce::Rectangle<float> box(x0, kWave.getY() + 2.0f, x1 - x0, kWave.getHeight() - 64.0f);
            g.setColour(juce::Colours::white.withAlpha(0.13f));
            g.fillRect(box);
            g.setColour(juce::Colours::white.withAlpha(0.85f));
            const float dash[] { 4.0f, 3.0f };
            for (const auto& edge : { juce::Line<float>(box.getTopLeft(), box.getTopRight()), juce::Line<float>(box.getBottomLeft(), box.getBottomRight()),
                                      juce::Line<float>(box.getTopLeft(), box.getBottomLeft()), juce::Line<float>(box.getTopRight(), box.getBottomRight()) })
                g.drawDashedLine(edge, dash, 2, 1.2f);
            g.setFont(10.0f);
            g.drawText("RIGHT-CLICK: WHAT IS IT?", juce::Rectangle<float>(x0 + 3.0f, box.getBottom() - 15.0f, 160.0f, 12.0f), juce::Justification::centredLeft);
        }
        if (marking)
        {
            g.setColour(juce::Colours::white.withAlpha(0.14f));
            g.fillRect(juce::Rectangle<float>(std::min(markFrom, markTo), kWave.getY() + 2.0f, std::abs(markTo - markFrom) + 1.0f, kWave.getHeight() - 4.0f));
        }
    }
    const auto ink = juce::Colour(0xff6dff9a);
    for (size_t c = 0; c < peaks.size(); ++c)
    {
        const float x = kWave.getX() + 4.0f + static_cast<float>(c);
        g.setColour(ink.withAlpha(0.75f));
        g.drawVerticalLine(juce::roundToInt(x), mid - half * peaks[c].second / peak, mid - half * peaks[c].first / peak + 1.0f);
    }
    const int perBeat = gridPerBeat[juce::jlimit(0, kGrids - 1, values.grid)];
    if (perBeat > 0) // R16 GRID: the finer lines (the beat lines are drawn below)
    {
        const int steps = values.bars * 4 * perBeat;
        for (int i = 1; i < steps; ++i)
        {
            const float x = fractionToX(static_cast<double>(i) / steps);
            if (i % perBeat != 0 && x > kWave.getX() && x < kWave.getRight())
            {
                g.setColour(ink.withAlpha(0.06f));
                g.drawVerticalLine(juce::roundToInt(x), kWave.getY() + 2.0f, kWave.getBottom() - 2.0f);
            }
        }
    }
    for (int b = 1; b < values.bars * 4; ++b) // beat lines
    {
        const float x = fractionToX(static_cast<double>(b) / (values.bars * 4));
        if (x <= kWave.getX() || x >= kWave.getRight())
            continue;
        g.setColour(ink.withAlpha(b % 4 == 0 ? 0.35f : 0.12f));
        g.drawVerticalLine(juce::roundToInt(x), kWave.getY() + 2.0f, kWave.getBottom() - 2.0f);
    }
    if (values.playhead >= 0.0)
    {
        g.setColour(juce::Colours::white.withAlpha(0.8f));
        g.drawVerticalLine(juce::roundToInt(fractionToX(values.playhead)), kWave.getY(), kWave.getBottom());
    }
    if (viewEnd - viewStart < 0.999) // R16 zoom: how deep, and the way back
        crtText(g, "ZOOM x" + juce::String((1.0 / (viewEnd - viewStart)), 1) + "  /  DOUBLE-CLICK: WHOLE WAVE",
                { kWave.getRight() - 330.0f, kWave.getY() + 6.0f, 320.0f, 16.0f }, 11.0f, juce::Justification::centredRight, 0.8f);
    if (values.valid)
        crtText(g, phrase ? values.fileName + "   <- DRAG THIS LINE (OR ALT + DRAG) INTO THE DAW" : values.fileName, { kWave.getX() + 8.0f, kWave.getY() + 6.0f, 600.0f, 16.0f }, 11.0f, juce::Justification::centredLeft, 0.7f);
    if (values.valid && values.kind == tr::Fill && values.recipe >= tr::kPhraseRecipe && values.fillInfo.isNotEmpty())
        crtText(g, values.fillInfo, { kWave.getX() + 8.0f, kWave.getY() + 24.0f, kWave.getWidth() - 16.0f, 16.0f }, 11.0f, juce::Justification::centredLeft, 0.95f);
    crtEnd(g, kWave);
    if(values.kind==tr::Fill&&values.recipe>=21)
    {
        const int rows=kitCount(values.kind,values.recipe);const float rowStep=rows>=5?11.5f:rows==4?14.0f:18.0f;const float cellHeight=std::min(11.0f,rowStep-2.0f);const auto* colours=kitColours;
        g.setColour(juce::Colours::black.withAlpha(.88f));g.fillRect(kWave.withTrimmedTop(kWave.getHeight()-60));
        for(int r=0;r<rows;++r){const float y=kWave.getBottom()-58+r*rowStep;g.setColour(colours[r]);g.setFont(10.0f);g.drawText(kitNames[r],juce::Rectangle<float>(kWave.getX()+8,y,52,cellHeight+3.0f),juce::Justification::centredLeft);
            for(int cell=0;cell<32;++cell){const bool on=(values.kitMask[static_cast<size_t>(r)]&(1u<<cell))!=0;g.setColour(colours[r].withAlpha(on?.9f:.08f));g.fillRect(kWave.getX()+65+cell*(kWave.getWidth()-74)/32,y+2,(kWave.getWidth()-74)/32-2,cellHeight);}}
    }
    if (dropHover)
    {
        g.setColour(juce::Colour(0xff6dff9a).withAlpha(0.12f));
        g.fillRect(kWave);
        g.setColour(juce::Colour(0xff6dff9a).withAlpha(0.8f));
        g.drawRect(kWave, 2.0f);
        crtText(g, "DROP THE SAMPLE: RISE / FILL ARE MADE FROM IT", kWave, 20.0f, juce::Justification::centred, 0.95f);
    }
}

void HamloRevolutionTransitionPage::pressGenerate() { if (onGenerate) onGenerate(); }
void HamloRevolutionTransitionPage::pressVariation() { if(onVariation)onVariation(); }
void HamloRevolutionTransitionPage::toggleKit(int role)
{if(role<0||role>=kitCount(values.kind,values.recipe))return;values.kitEnabled[static_cast<size_t>(role)]=!values.kitEnabled[static_cast<size_t>(role)];if(onFillOptions)onFillOptions();repaint();}
void HamloRevolutionTransitionPage::chooseFillBeats(int beats)
{values.fillBeats=beats==1||beats==2||(beats==tr::kWholeLoop&&phraseRecipe(values.kind,values.recipe))?beats:4;if(onFillOptions)onFillOptions();repaint();}
void HamloRevolutionTransitionPage::chooseFillFx(double amount)
{values.fillFx=std::isfinite(amount)?juce::jlimit(0.,1.,amount):0.;if(onFillOptions)onFillOptions();repaint();}
void HamloRevolutionTransitionPage::pressClear()
{
    // R16: start over on the same source - no marks, no selection, the whole wave; the owner sets the defaults
    markHistory.clear();
    values.marks.clear();
    selection = {};
    setView(0.0, 1.0);
    if (onClear)
        onClear();
    repaint();
}
void HamloRevolutionTransitionPage::chooseGrid(int grid)
{
    values.grid = juce::jlimit(0, kGrids - 1, grid);
    repaint();
}
void HamloRevolutionTransitionPage::chooseBpm(double bpm)
{
    values.sampleBpm = std::isfinite(bpm) && bpm >= 40.0 && bpm <= 250.0 ? bpm : 0.0;
    if (onBpm)
        onBpm(values.sampleBpm);
    repaint();
}
void HamloRevolutionTransitionPage::selectRegion(int start, int end)
{
    if (end < start)
        std::swap(start, end);
    start = juce::jlimit(0, std::max(0, sourceLength), start);
    end = juce::jlimit(0, std::max(0, sourceLength), end);
    selection = end - start >= 8 ? juce::Range<int>(start, end) : juce::Range<int>();
    repaint();
}

void HamloRevolutionTransitionPage::showRoleMenu(juce::Range<int> region, int current)
{
    bool marked = false;
    for (const auto& m : values.marks)
        marked = marked || (m.start < region.getEnd() && region.getStart() < m.end);
    juce::PopupMenu menu;
    menu.addSectionHeader(current >= 0 ? "CHANGE THIS MARK" : "THIS IS ...");
    for (int r = 0; r < tr::kKitRoles; ++r)
        menu.addItem(r + 1, kitNames[r], true, r == current);
    menu.addItem(tr::kSampleMark + 1, "SAMPLE (NOT A DRUM)", true, current == tr::kSampleMark);
    menu.addSeparator();
    menu.addItem(10, "REMOVE MARK", marked);
    menu.addItem(13, "CANCEL SELECTION", !selection.isEmpty());
    menu.addItem(12, "UNDO LAST MARK", !markHistory.empty());
    menu.addItem(11, "CLEAR ALL MARKS", !values.marks.empty());
    juce::Component::SafePointer<HamloRevolutionTransitionPage> safe(this);
    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(this).withMousePosition(), [safe, region](int result)
    {
        if (!safe || result <= 0)
            return;
        if (result == 13)
            return safe->selectRegion(0, 0);
        if (result == 11)
            return safe->clearMarks();
        if (result == 12)
            return safe->undoMark();
        safe->markRegion(region.getStart(), region.getEnd(), result == 10 ? -1 : result - 1);
    });
}

void HamloRevolutionTransitionPage::showBpmMenu()
{
    juce::PopupMenu menu;
    const bool sample = values.sourceName.isNotEmpty();
    if (!sample)
    {
        menu.addSectionHeader("THE TRACK'S INPUT FOLLOWS THE HOST TEMPO");
        menu.addItem(1, "HOST  " + juce::String(juce::roundToInt(values.hostBpm)) + " BPM", false, true);
        menu.addSectionHeader("DROP A SAMPLE TO SET ITS OWN BPM");
    }
    else
    {
        menu.addSectionHeader("THE SAMPLE'S TEMPO");
        menu.addItem(1, "HOST  " + juce::String(juce::roundToInt(values.hostBpm)) + " BPM", true, values.sampleBpm <= 0.0);
        const double named = bpmFromName(values.sourceName);
        menu.addItem(2, named > 0.0 ? "FROM THE FILE NAME  " + juce::String(named, 0) + " BPM" : juce::String("FROM THE FILE NAME  (NONE)"), named > 0.0,
                     named > 0.0 && std::abs(values.sampleBpm - named) < 1.0e-6);
        const double fit = values.sampleSeconds > 0.0 ? values.bars * 240.0 / values.sampleSeconds : 0.0;
        const bool fits = fit >= 40.0 && fit <= 250.0;
        menu.addItem(3, fits ? "FIT THE SAMPLE TO " + juce::String(values.bars) + (values.bars == 1 ? " BAR  " : " BARS  ") + juce::String(fit, 1) + " BPM"
                             : juce::String("FIT THE SAMPLE TO THE BARS  (OUT OF RANGE)"),
                     fits, fits && std::abs(values.sampleBpm - fit) < 0.05);
        menu.addSeparator();
        for (int from = 60; from < 200; from += 20)
        {
            juce::PopupMenu sub;
            for (int bpm = from; bpm < from + 20; ++bpm)
                sub.addItem(1000 + bpm, juce::String(bpm) + " BPM", true, std::abs(values.sampleBpm - bpm) < 1.0e-6);
            menu.addSubMenu(juce::String(from) + " - " + juce::String(from + 19), sub);
        }
        menu.addItem(4, "TYPE A VALUE ...");
    }
    juce::Component::SafePointer<HamloRevolutionTransitionPage> safe(this);
    const auto area = kBpm.transformedBy(juce::AffineTransform::scale(scale).translated(offset)).toNearestInt();
    const double named = bpmFromName(values.sourceName), fit = values.sampleSeconds > 0.0 ? values.bars * 240.0 / values.sampleSeconds : 0.0;
    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(this).withTargetScreenArea(localAreaToGlobal(area)), [safe, named, fit](int r)
    {
        if (!safe || r <= 0)
            return;
        if (r == 1)
            return safe->chooseBpm(0.0);
        if (r == 2)
            return safe->chooseBpm(named);
        if (r == 3)
            return safe->chooseBpm(std::round(fit * 10.0) / 10.0);
        if (r >= 1000)
            return safe->chooseBpm(r - 1000);
        auto* window = new juce::AlertWindow("BPM", "The sample's tempo, 40 .. 250", juce::MessageBoxIconType::NoIcon);
        window->addTextEditor("bpm", safe->values.sampleBpm > 0.0 ? juce::String(safe->values.sampleBpm, 1) : juce::String(juce::roundToInt(safe->values.hostBpm)));
        window->addButton("OK", 1, juce::KeyPress(juce::KeyPress::returnKey));
        window->addButton("CANCEL", 0, juce::KeyPress(juce::KeyPress::escapeKey));
        window->enterModalState(true, juce::ModalCallbackFunction::create([safe, window](int ok)
        {
            const double bpm = window->getTextEditorContents("bpm").getDoubleValue();
            if (safe && ok == 1 && bpm >= 40.0 && bpm <= 250.0)
                safe->chooseBpm(bpm);
        }), true);
    });
}

void HamloRevolutionTransitionPage::pressPlay() { if (onPlay) onPlay(); }
void HamloRevolutionTransitionPage::pressBack() { if (onBack) onBack(); }
void HamloRevolutionTransitionPage::pressShowFile() { if (onShowFile) onShowFile(); }
void HamloRevolutionTransitionPage::pressUseInput() { if (onUseInput) onUseInput(); }

bool HamloRevolutionTransitionPage::isInterestedInFileDrag(const juce::StringArray& files)
{
    for (const auto& f : files)
        if (juce::File(f).hasFileExtension("wav;aif;aiff;flac;mp3;ogg;wma;m4a"))
            return true;
    return false;
}

void HamloRevolutionTransitionPage::fileDragEnter(const juce::StringArray&, int, int)
{
    dropHover = true;
    repaint();
}

void HamloRevolutionTransitionPage::fileDragExit(const juce::StringArray&)
{
    dropHover = false;
    repaint();
}

void HamloRevolutionTransitionPage::filesDropped(const juce::StringArray& files, int, int)
{
    dropHover = false;
    for (const auto& f : files)
        if (juce::File(f).hasFileExtension("wav;aif;aiff;flac;mp3;ogg;wma;m4a"))
        {
            if (onSampleDropped)
                onSampleDropped(juce::File(f));
            break;
        }
    repaint();
}

void HamloRevolutionTransitionPage::chooseKind(int k)
{
    values.kind = juce::jlimit(0, 1, k);
    if (onKind)
        onKind(values.kind);
    repaint();
}

void HamloRevolutionTransitionPage::chooseBars(int bars)
{
    values.bars = bars == 1 || bars == 4 ? bars : 2;
    if (onBars)
        onBars(values.bars);
    repaint();
}

void HamloRevolutionTransitionPage::chooseRecipe(int recipe)
{
    values.recipe=juce::jlimit(0,tr::maximumRecipe(values.kind),recipe);
    if(onRecipe)onRecipe(values.recipe);
    repaint();
}

void HamloRevolutionTransitionPage::setKnob(int knob, double v, bool done)
{
    values.knobs[static_cast<size_t>(juce::jlimit(0, 3, knob))] = juce::jlimit(0.0, 1.0, v);
    if (onKnob)
        onKnob(knob, values.knobs[static_cast<size_t>(juce::jlimit(0, 3, knob))]);
    if (done && onKnobsDone)
        onKnobsDone();
    repaint();
}

juce::String HamloRevolutionTransitionPage::helpKeyAt(juce::Point<float> local) const
{
    const auto d = toDesign(local);
    if(kRecipe.contains(d))return "transitionRecipe";
    if(kGrid.contains(d))return "transitionGrid";
    if(kBpm.contains(d))return "transitionBpm";
    if(kClear.contains(d))return "transitionClear";
    if(kVariation.contains(d))return "transitionVariation";
    if(values.kind==tr::Fill&&values.recipe>=21)for(int r=0;r<kitCount(values.kind,values.recipe);++r)if(kitRect(r,kitCount(values.kind,values.recipe)).contains(d))return "transitionKit";
    if(values.kind==tr::Fill&&values.recipe>=8)
    {if(kPhrase.contains(d))return "transitionPhrase";if(kFx.contains(d))return "transitionFillFx";}
    if (kBack.contains(d))
        return "transitionBack";
    if (kUseInput.contains(d))
        return "transitionUseInput";
    if (kindRect(0).contains(d) || kindRect(1).contains(d))
        return "transitionKind";
    for (int i = 0; i < 3; ++i)
        if (barsRect(i).contains(d))
            return "transitionBars";
    for (int k = 0; k < 4; ++k)
        if (d.getDistanceFrom(kKnobs[static_cast<size_t>(k)]) <= 60.0f * kKnobScale)
            return juce::String("transition") + (values.kind == tr::Rise ? "Rise" : "Fill") + juce::String(k);
    if (d.getDistanceFrom(kGenerate) <= 36.0f)
        return "transitionGenerate";
    if (kPlay.contains(d))
        return "transitionPlay";
    if (kShowFile.contains(d))
        return "transitionShowFile";
    if (kWave.contains(d))
        return "transitionWave";
    return {};
}

juce::String HamloRevolutionTransitionPage::getTooltip() { return revolutionHelp(helpKeyAt(getMouseXYRelative().toFloat())); }

void HamloRevolutionTransitionPage::mouseDown(const juce::MouseEvent& e)
{
    const auto d = toDesign(e.position);
    waveDown = dragged = false;
    if(kRecipe.contains(d))
    {
        juce::PopupMenu menu;
        if(values.kind==tr::Fill)
        {
            for(int recipe=tr::kPhraseRecipe;recipe<=tr::kPhraseRecipe+4;++recipe)menu.addItem(recipe+1,tr::recipeName(values.kind,recipe),true,values.recipe==recipe);
            juce::PopupMenu composer;
            for(int recipe=21;recipe<=29;++recipe)composer.addItem(recipe+1,tr::recipeName(values.kind,recipe),true,values.recipe==recipe);
            menu.addSubMenu("R14 AUTO COMPOSE RECIPES",composer);
            juce::PopupMenu slicer;
            for(int recipe=8;recipe<=20;++recipe)slicer.addItem(recipe+1,tr::recipeName(values.kind,recipe),true,values.recipe==recipe);
            menu.addSubMenu("R13 SLICER RECIPES",slicer);
            juce::PopupMenu old;
            for(int recipe=0;recipe<8;++recipe)old.addItem(recipe+1,tr::recipeName(values.kind,recipe),true,values.recipe==recipe);
            menu.addSubMenu("CLASSIC / R12 RECIPES",old);
        }
        else for(int recipe=0;recipe<8;++recipe)menu.addItem(recipe+1,tr::recipeName(values.kind,recipe),true,values.recipe==recipe);
        juce::Component::SafePointer<HamloRevolutionTransitionPage> safe(this);
        const auto target=kRecipe.transformedBy(juce::AffineTransform::scale(scale).translated(offset)).toNearestInt();
        menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(this).withTargetScreenArea(localAreaToGlobal(target)),[safe](int result){if(safe&&result>0)safe->chooseRecipe(result-1);});
        return;
    }
    if(kVariation.contains(d))return pressVariation();
    if(kClear.contains(d))return pressClear();
    if(kBpm.contains(d))return showBpmMenu();
    if(kGrid.contains(d))
    {
        juce::PopupMenu menu;
        menu.addSectionHeader("GRID: LINES ON THE WAVE, SELECTIONS SNAP");
        for(int i=0;i<kGrids;++i)menu.addItem(i+1,i==0?juce::String("OFF (BEATS, NO SNAP)"):juce::String(gridNames[i]),true,values.grid==i);
        juce::Component::SafePointer<HamloRevolutionTransitionPage> safe(this);
        const auto area=kGrid.transformedBy(juce::AffineTransform::scale(scale).translated(offset)).toNearestInt();
        menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(this).withTargetScreenArea(localAreaToGlobal(area)),[safe](int r){if(safe&&r>0)safe->chooseGrid(r-1);});
        return;
    }
    if(values.kind==tr::Fill&&values.recipe>=21)for(int r=0;r<kitCount(values.kind,values.recipe);++r)if(kitRect(r,kitCount(values.kind,values.recipe)).contains(d))return toggleKit(r);
    if(values.kind==tr::Fill&&values.recipe>=8&&(kPhrase.contains(d)||kFx.contains(d)))
    {
        const bool phrase=kPhrase.contains(d);juce::PopupMenu menu;
        if(phrase){for(int i=0;i<3;++i)menu.addItem(i+1,juce::String(kBarChoices[static_cast<size_t>(i)])+" BEAT(S)",true,values.fillBeats==kBarChoices[static_cast<size_t>(i)]);
            if(phraseRecipe(values.kind,values.recipe))menu.addItem(4,"WHOLE LOOP (EVERY BAR)",true,values.fillBeats==tr::kWholeLoop);}
        else for(int i=0;i<=20;++i)menu.addItem(i+1,juce::String(i*5)+" %",true,juce::roundToInt(values.fillFx*20)==i);
        juce::Component::SafePointer<HamloRevolutionTransitionPage> safe(this);
        const auto area=(phrase?kPhrase:kFx).transformedBy(juce::AffineTransform::scale(scale).translated(offset)).toNearestInt();
        menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(this).withTargetScreenArea(localAreaToGlobal(area)),[safe,phrase](int r){if(safe&&r>0){if(phrase)safe->chooseFillBeats(r==4?tr::kWholeLoop:kBarChoices[static_cast<size_t>(r-1)]);else safe->chooseFillFx((r-1)*.05);}});
        return;
    }
    if (kBack.contains(d))
        return pressBack();
    if (kUseInput.contains(d))
        return pressUseInput();
    for (int k = 0; k < 2; ++k)
        if (kindRect(k).contains(d))
            return chooseKind(k);
    for (int i = 0; i < 3; ++i)
        if (barsRect(i).contains(d))
            return chooseBars(kBarChoices[static_cast<size_t>(i)]);
    if (d.getDistanceFrom(kGenerate) <= 36.0f)
        return pressGenerate();
    if (kPlay.contains(d))
        return pressPlay();
    if (kShowFile.contains(d))
        return pressShowFile();
    if (kWave.contains(d))
    {
        const bool phrase = phraseRecipe(values.kind, values.recipe) && sourceLength > 0;
        if (phrase && e.mods.isPopupMenu())
        {
            // R16 (owner fix): a right-click asks what it is - the selection, else the mark, else the hit under the mouse
            const int sample = xToSample(d.x, sourceLength);
            if (!selection.isEmpty() && selection.contains(sample))
            {
                int current = -1;
                for (const auto& m : values.marks)
                    if (m.start == selection.getStart() && m.end == selection.getEnd())
                        current = m.role;
                return showRoleMenu(selection, current);
            }
            if (const int index = markAt(sample); index >= 0)
            {
                const auto m = values.marks[static_cast<size_t>(index)];
                return showRoleMenu({ m.start, m.end }, m.role);
            }
            const auto hit = hitAt(sample);
            selectRegion(hit.getStart(), hit.getEnd());
            if (!selection.isEmpty())
                showRoleMenu(selection, -1);
            return;
        }
        if (phrase && d.y >= kWave.getY() + kExportStrip && !e.mods.isAltDown())
        {
            marking = true; // R16: a left-click selects a hit, a left-drag a region; a right-click labels it
            markFrom = markTo = d.x;
            return;
        }
        waveDown = true; // a drag from here exports the WAV (PHRASE FILL: from the top line, or alt + drag)
        return;
    }
    for (int k = 0; k < 4; ++k)
        if (d.getDistanceFrom(kKnobs[static_cast<size_t>(k)]) <= 62.0f * kKnobScale)
        {
            dragKnob = k;
            dragStart = values.knobs[static_cast<size_t>(k)];
            dragStartY = e.position.y;
            return;
        }
}

void HamloRevolutionTransitionPage::mouseDrag(const juce::MouseEvent& e)
{
    if (marking)
    {
        markTo = juce::jlimit(kWave.getX(), kWave.getRight(), toDesign(e.position).x);
        repaint();
        return;
    }
    if (waveDown && !dragged && e.getDistanceFromDragStart() > 6 && fileToDrag)
    {
        dragged = true;
        const auto file = fileToDrag();
        if (file.existsAsFile())
            juce::DragAndDropContainer::performExternalDragDropOfFiles({ file.getFullPathName() }, false, this);
        return;
    }
    if (dragKnob >= 0)
        setKnob(dragKnob, dragStart + static_cast<double>((dragStartY - e.position.y) / (220.0f * scale)), false);
}

void HamloRevolutionTransitionPage::mouseUp(const juce::MouseEvent&)
{
    if (marking)
    {
        // R16: the left button selects (a click: the hit / mark under it; a drag: the region, snapped to GRID)
        marking = false;
        juce::Range<int> region;
        if (std::abs(markTo - markFrom) < 4.0f)
        {
            const int sample = xToSample(markFrom, sourceLength), index = markAt(sample);
            if (index >= 0)
                region = { values.marks[static_cast<size_t>(index)].start, values.marks[static_cast<size_t>(index)].end };
            else
                region = hitAt(sample);
        }
        else
        {
            int a = snapToGrid(xToSample(std::min(markFrom, markTo), sourceLength)), b = snapToGrid(xToSample(std::max(markFrom, markTo), sourceLength));
            if (b <= a) // both edges snapped to one line: keep the drag as it was
            {
                a = xToSample(std::min(markFrom, markTo), sourceLength);
                b = xToSample(std::max(markFrom, markTo), sourceLength);
            }
            region = { a, b };
        }
        selectRegion(region.getStart(), region.getEnd());
        return;
    }
    if (dragKnob >= 0 && onKnobsDone)
        onKnobsDone();
    dragKnob = -1;
    waveDown = false;
}

void HamloRevolutionTransitionPage::mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel)
{
    const auto d = toDesign(e.position);
    if (!kWave.contains(d) || (sourceLength <= 0 && clipBuffer.getNumSamples() <= 0))
        return juce::Component::mouseWheelMove(e, wheel);
    const float vertical = wheel.isReversed ? -wheel.deltaY : wheel.deltaY;
    if (e.mods.isShiftDown() || std::abs(wheel.deltaX) > std::abs(wheel.deltaY))
    {
        // shift + wheel (or a sideways swipe): move along the zoomed wave
        const float along = std::abs(wheel.deltaX) > std::abs(wheel.deltaY) ? wheel.deltaX : vertical;
        const double width = viewEnd - viewStart;
        return setView(viewStart - along * width * 0.5, viewEnd - along * width * 0.5);
    }
    if (vertical == 0.0f)
        return;
    // the wheel zooms in / out around the mouse; one notch is about x1.25
    zoomAround(xToFraction(d.x), std::pow(0.8, juce::jlimit(-4.0, 4.0, static_cast<double>(vertical) / 0.12)));
}

void HamloRevolutionTransitionPage::mouseDoubleClick(const juce::MouseEvent& e)
{
    if (kWave.contains(toDesign(e.position)))
        setView(0.0, 1.0); // the whole wave again
}
} // namespace hpeq::ui

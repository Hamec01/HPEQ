#include "UI/HamloRevolutionTransitionPage.h"

#include <cmath>

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
} // namespace

HamloRevolutionTransitionPage::HamloRevolutionTransitionPage() { setOpaque(true); }

void HamloRevolutionTransitionPage::setValues(const Values& v)
{
    values = v;
    repaint();
}

void HamloRevolutionTransitionPage::setClip(const juce::AudioBuffer<float>& clip)
{
    const int columns = static_cast<int>(kWave.getWidth()) - 8, n = clip.getNumSamples();
    peaks.assign(static_cast<size_t>(columns), { 0.0f, 0.0f });
    if (n == 0 || clip.getNumChannels() == 0)
        return repaint();
    for (int c = 0; c < columns; ++c)
    {
        const int from = static_cast<int>(static_cast<long long>(n) * c / columns), to = std::max(from + 1, static_cast<int>(static_cast<long long>(n) * (c + 1) / columns));
        float lo = 0.0f, hi = 0.0f;
        for (int i = from; i < to && i < n; ++i)
            for (int ch = 0; ch < clip.getNumChannels(); ++ch)
            {
                lo = std::min(lo, clip.getSample(ch, i));
                hi = std::max(hi, clip.getSample(ch, i));
            }
        peaks[static_cast<size_t>(c)] = { lo, hi };
    }
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
    crtButton(g, kindRect(1), "FILL", values.kind == tr::Fill);
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
    const float mid = kWave.getCentreY(), half = kWave.getHeight() * 0.45f;
    float peak = 1.0e-6f;
    for (const auto& [lo, hi] : peaks)
        peak = std::max({ peak, -lo, hi });
    const auto ink = juce::Colour(0xff6dff9a);
    for (size_t c = 0; c < peaks.size(); ++c)
    {
        const float x = kWave.getX() + 4.0f + static_cast<float>(c);
        g.setColour(ink.withAlpha(0.75f));
        g.drawVerticalLine(juce::roundToInt(x), mid - half * peaks[c].second / peak, mid - half * peaks[c].first / peak + 1.0f);
    }
    for (int b = 1; b < values.bars * 4; ++b) // beat lines
    {
        const float x = kWave.getX() + kWave.getWidth() * static_cast<float>(b) / static_cast<float>(values.bars * 4);
        g.setColour(ink.withAlpha(b % 4 == 0 ? 0.35f : 0.12f));
        g.drawVerticalLine(juce::roundToInt(x), kWave.getY() + 2.0f, kWave.getBottom() - 2.0f);
    }
    if (values.playhead >= 0.0)
    {
        g.setColour(juce::Colours::white.withAlpha(0.8f));
        g.drawVerticalLine(juce::roundToInt(kWave.getX() + kWave.getWidth() * static_cast<float>(values.playhead)), kWave.getY(), kWave.getBottom());
    }
    if (values.valid)
        crtText(g, values.fileName, { kWave.getX() + 8.0f, kWave.getY() + 6.0f, 600.0f, 16.0f }, 11.0f, juce::Justification::centredLeft, 0.7f);
    crtEnd(g, kWave);
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
        waveDown = true; // a drag from here exports the WAV
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
    if (dragKnob >= 0 && onKnobsDone)
        onKnobsDone();
    dragKnob = -1;
    waveDown = false;
}
} // namespace hpeq::ui

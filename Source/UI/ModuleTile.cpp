#include "UI/ModuleTile.h"

#include "UI/BlackSteel.h"
#include "UI/HamloCompPanel.h"
#include "UI/HamloEchoPanel.h"
#include "UI/HamloFaderPanel.h"
#include "UI/HamloMagPanel.h"
#include "UI/HamloRedWallPanel.h"
#include "UI/HamloRevolutionPanel.h"
#include "UI/HamloShirotaPanel.h"
#include "UI/HamloUdarnikPanel.h"
#include "UI/HamloVerbPanel.h"
#include "UI/HamloZastavaPanel.h"
#include "UI/Theme.h"

namespace hpeq::ui
{
namespace
{
using namespace steel;

template <typename Panel>
juce::Image renderPage(int width, int height)
{
    Panel page;
    page.setSize(width, height);
    return page.createComponentSnapshot(page.getLocalBounds(), true, 0.32f);
}

juce::Image renderThumbnail(FxType type)
{
    switch (type)
    {
        case FxType::Compressor: return renderPage<HamloCompPanel>(1180, 700);
        case FxType::Reverb: return renderPage<HamloVerbPanel>(1180, 760);
        case FxType::Delay: return renderPage<HamloEchoPanel>(1180, 760);
        case FxType::Saturator: return renderPage<HamloMagPanel>(1180, 820);
        case FxType::Limiter: return renderPage<HamloRedWallPanel>(1180, 700);
        case FxType::TransientShaper: return renderPage<HamloUdarnikPanel>(1180, 700);
        case FxType::GateExpander: return renderPage<HamloZastavaPanel>(1180, 780);
        case FxType::StereoProcessor: return renderPage<HamloShirotaPanel>(1180, 780);
        case FxType::MonstaFx: return renderPage<HamloRevolutionPanel>(1180, 700);
        case FxType::Gain: return renderPage<HamloFaderPanel>(1180, 700);
    }
    return {};
}
} // namespace

juce::Image moduleThumbnail(FxType type)
{
    juce::SharedResourcePointer<ModuleThumbnails> cache;
    auto& image = cache->images[type];
    if (!image.isValid())
        image = renderThumbnail(type);
    return image;
}

std::unique_ptr<juce::Drawable> moduleIcon(FxType type)
{
    auto drawable = std::make_unique<juce::DrawableImage>();
    drawable->setImage(moduleThumbnail(type));
    drawable->setBoundingBox(juce::Rectangle<float>(0.0f, 0.0f, 46.0f, 28.0f));
    return drawable;
}

ModuleTile::ModuleTile(const juce::String& text, int n, std::optional<FxType> t, bool on)
    : juce::TextButton(text), number(n), type(t), enabled(on)
{
}

void ModuleTile::paintButton(juce::Graphics& g, bool over, bool down)
{
    const auto r = getLocalBounds().toFloat().reduced(1.0f);
    getLookAndFeel().drawButtonBackground(g, *this, colours::panelRaised, over, down);
    if (!type)
    {
        // an empty place: an engraved "+"
        const auto c = r.getCentre();
        const float s = juce::jmin(r.getHeight() * 0.3f, 14.0f);
        g.setColour(juce::Colours::black.withAlpha(0.6f));
        g.fillRect(c.x - s, c.y + 0.25f, 2.0f * s, 2.5f);
        g.fillRect(c.x - 1.25f, c.y - s + 1.0f, 2.5f, 2.0f * s);
        g.setColour(stencil.withAlpha(over ? 0.95f : 0.6f));
        g.fillRect(c.x - s, c.y - 1.25f, 2.0f * s, 2.5f);
        g.fillRect(c.x - 1.25f, c.y - s, 2.5f, 2.0f * s);
        return;
    }

    // the page in miniature, filling the tile, darkened so the name reads
    const auto picture = r.reduced(2.0f);
    {
        juce::Graphics::ScopedSaveState state(g);
        juce::Path clip;
        clip.addRoundedRectangle(picture, 3.0f);
        g.reduceClipRegion(clip);
        const auto image = moduleThumbnail(*type);
        if (image.isValid())
            g.drawImage(image, picture, juce::RectanglePlacement::fillDestination | juce::RectanglePlacement::xMid | juce::RectanglePlacement::yMid);
        g.setColour(juce::Colours::black.withAlpha(enabled ? (over ? 0.32f : 0.45f) : 0.68f));
        g.fillRect(picture);
    }

    // the name, clear over the picture; the function under it when there is room; the number
    const bool roomy = picture.getHeight() >= 46.0f;
    const auto nameArea = roomy ? picture.withTrimmedBottom(picture.getHeight() * 0.3f) : picture;
    const float size = juce::jlimit(13.0f, 28.0f, nameArea.getHeight() * 0.5f);
    g.setFont(stencilFont(size));
    const auto name = fxName(*type);
    for (const auto offset : { juce::Point<float>(0.0f, 1.5f), juce::Point<float>(1.0f, 1.0f) })
    {
        g.setColour(juce::Colours::black.withAlpha(0.85f));
        g.drawFittedText(name, nameArea.translated(offset.x, offset.y).toNearestInt(), juce::Justification::centred, 1, 0.8f);
    }
    g.setColour(enabled ? juce::Colour(0xfff4ead0) : stencilDim);
    g.drawFittedText(name, nameArea.toNearestInt(), juce::Justification::centred, 1, 0.8f);
    if (roomy)
    {
        g.setFont(stencilFont(juce::jlimit(9.0f, 13.0f, picture.getHeight() * 0.16f), false));
        const auto line = picture.withTop(nameArea.getBottom() - 4.0f).withTrimmedBottom(4.0f);
        const auto function = fxFunction(*type).toUpperCase() + (enabled ? "" : "  (OFF)");
        g.setColour(juce::Colours::black.withAlpha(0.8f));
        g.drawFittedText(function, line.translated(0.0f, 1.0f).toNearestInt(), juce::Justification::centredTop, 1);
        g.setColour(enabled ? stencil : stencilDim);
        g.drawFittedText(function, line.toNearestInt(), juce::Justification::centredTop, 1);
    }
    g.setFont(juce::FontOptions("Consolas", 11.0f, juce::Font::bold));
    const auto badge = juce::Rectangle<float>(picture.getX() + 3.0f, picture.getY() + 3.0f, 16.0f, 14.0f);
    g.setColour(juce::Colour(0xe0050605));
    g.fillRoundedRectangle(badge, 2.0f);
    g.setColour(colours::accent);
    g.drawText(juce::String(number), badge, juce::Justification::centred);

    // running: an amber lamp stripe along the bottom, as the lit keys
    if (enabled)
    {
        const auto stripe = juce::Rectangle<float>(r.getX() + 6.0f, r.getBottom() - 4.0f, r.getWidth() - 12.0f, 2.5f);
        g.setColour(colours::accent.withAlpha(0.35f));
        g.fillRoundedRectangle(stripe.expanded(1.5f, 1.5f), 2.0f);
        g.setColour(colours::accent);
        g.fillRoundedRectangle(stripe, 1.2f);
    }
}
} // namespace hpeq::ui

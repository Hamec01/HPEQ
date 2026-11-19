// HamloRevolution_Snapshot <out.png> [program] [scale] [generate]: the page (the editor with the preset screen) at its opening size x scale
// (R3: the page opens at 75 %; scale 1.3333 = the design size 1180 x 1280).

#include <cmath>
#include <memory>

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "Standalone/ModulePluginEditor.h"
#include "Standalone/ModulePluginProcessor.h"

int main(int argc, char* argv[])
{
    juce::ScopedJuceInitialiser_GUI gui;
    if (argc < 2)
        return 2;
    auto owned = std::make_unique<hpeq::standalone::ModulePluginProcessor>(static_cast<hpeq::standalone::ModuleKind>(HPEQ_MODULE_KIND)); // large: heap
    auto& p = *owned;
    p.setCurrentProgram(argc > 2 ? juce::String(argv[2]).getIntValue() : 1);
#if HPEQ_MODULE_KIND == 9
    // R4: "generate" = the SIMPLE page with a generated pattern
    if (argc > 4 && juce::String(argv[4]).contains("generate"))
        p.generateSimple(false);
    if (argc > 4 && juce::String(argv[4]).contains("advanced")) // R6: the ADVANCED page
        p.setPage(hpeq::standalone::ModulePluginProcessor::Page::Advanced);
    if (argc > 4 && juce::String(argv[4]).contains("transition")) // RISE / FILL: 4 bars of a beat heard, a riser made
    {
        p.prepareToPlay(48000.0, 512);
        juce::AudioBuffer<float> block(2, 512);
        juce::MidiBuffer midi;
        juce::Random rng(3);
        for (int start = 0; start < 4 * 96000; start += 512)
        {
            for (int i = 0; i < 512; ++i)
            {
                const int n = start + i, inBeat = n % 24000;
                const double t = inBeat / 48000.0, v = 0.7 * std::exp(-t / 0.08) * std::sin(2.0 * 3.14159265 * (50.0 + 90.0 * std::exp(-t / 0.012)) * t)
                                                       + 0.15 * std::exp(-(n % 12000) / 600.0) * (2.0 * rng.nextDouble() - 1.0);
                block.setSample(0, i, static_cast<float>(v));
                block.setSample(1, i, static_cast<float>(v));
            }
            p.processBlock(block, midi);
        }
        p.setRendersFolder(juce::File::getSpecialLocation(juce::File::tempDirectory).getChildFile("HamloRevolutionSnapshot"));
        const bool fill=juce::String(argv[4]).contains("fill");
        const bool r17=juce::String(argv[4]).contains("r17"); // R17: "transition fill r17" = FILL AUTO with its knobs
        const int recipe=juce::String(argv[4]).contains("classic")?0:r17?hpeq::dsp::transition::kR17Recipe:fill?21:1;
        p.makeTransition(fill?1:0,fill?1:2,hpeq::dsp::transition::defaultKnobs(fill?1:0,recipe),true,true,recipe);
        p.setPage(hpeq::standalone::ModulePluginProcessor::Page::Transition);
    }
#endif
    if(argc>4&&juce::String(argv[4]).contains("sample"))
    {
        p.prepareToPlay(48000,256);
        auto& workspace=*p.sampleWorkspace();
        workspace.load(juce::File::getCurrentWorkingDirectory().getChildFile("build/r9_visual_source.wav"));
        const auto start=juce::Time::getMillisecondCounter();
        while(workspace.status(p.sampleRevision()).busy&&juce::Time::getMillisecondCounter()-start<15000)juce::Thread::sleep(2);
    }
    std::unique_ptr<juce::AudioProcessorEditor> e(p.createEditor());
    e->setSize(p.getSpec().editorWidth, p.getSpec().editorHeight);
#if HPEQ_MODULE_KIND == 9
    // R15: "transition fill phrase [whole]": PHRASE FILL with user marks on the source (kick, hat, snare, perc, tom)
    if (argc > 4 && juce::String(argv[4]).contains("phrase"))
        if (auto* editor = dynamic_cast<hpeq::standalone::ModulePluginEditor*>(e.get()))
        {
            auto* page = editor->getTransitionPage();
            page->chooseKind(hpeq::dsp::transition::Fill);
            if (juce::String(argv[4]).contains("whole"))
                page->chooseFillBeats(hpeq::dsp::transition::kWholeLoop);
            const auto hits = page->getValues().hits;
            const int roles[] { 0, 2, 1, 3, 4, 2 };
            for (size_t i = 0; i < hits.size() && i < 6; ++i)
                page->markRegion(hits[i].getStart(), hits[i].getEnd(), roles[i]);
            if (juce::String(argv[4]).contains("r16") && hits.size() > 7) // R16: a SAMPLE mark, a pending selection, GRID 1/16, zoom
            {
                page->markRegion(hits[5].getStart(), hits[5].getEnd(), hpeq::dsp::transition::kSampleMark);
                page->chooseGrid(3);
                page->selectRegion(hits[7].getStart(), hits[7].getEnd());
                page->zoomAround(0.0, 0.6);
            }
        }
#endif
    if(argc>4&&juce::String(argv[4]).contains("minimum"))e->setSize(p.getSpec().editorWidth/2,p.getSpec().editorHeight/2);
    const float scale = argc > 3 ? static_cast<float>(juce::String(argv[3]).getDoubleValue()) : 1.0f;
    const auto image = e->createComponentSnapshot(e->getLocalBounds(), true, scale);
    juce::File out = juce::File::getCurrentWorkingDirectory().getChildFile(argv[1]);
    out.deleteFile();
    juce::FileOutputStream stream(out);
    juce::PNGImageFormat png;
    return stream.openedOk() && png.writeImageToStream(image, stream) ? 0 : 1;
}

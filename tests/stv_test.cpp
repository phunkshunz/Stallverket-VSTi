// Kör processorn med låtsad transport och ritar panelerna till PNG.
// cmake -B build -DSTV_TEST=ON && cmake --build build --target StallverketTest
#include "../src/PluginProcessor.h"
#include "../src/PluginEditor.h"
#include <juce_events/juce_events.h>

struct Spelhuvud : juce::AudioPlayHead
{
    double ppq = 0, bpm = 130; bool play = true;
    juce::Optional<PositionInfo> getPosition() const override
    {
        PositionInfo p; p.setIsPlaying (play); p.setBpm (bpm); p.setPpqPosition (ppq);
        p.setTimeSignature (TimeSignature { 4, 4 }); return p;
    }
};

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI gui;
    const juce::String ut = argc > 1 ? argv[1] : "/tmp";
    StallverketProcessor a, b, c;
    Spelhuvud ph;
    for (auto* p : { &a, &b, &c }) { p->setPlayHead (&ph); p->prepareToPlay (48000, 512); }
    std::printf ("a central=%d kanal=%d | b central=%d kanal=%d | c kanal=%d\n", a.arCentral(), a.kanal(), b.arCentral(), b.kanal(), c.kanal());
    b.state.getParameter ("roll")->setValueNotifyingHost (b.state.getParameter ("roll")->convertTo0to1 (1));
    c.state.getParameter ("roll")->setValueNotifyingHost (c.state.getParameter ("roll")->convertTo0to1 (5));
    a.laddaFabrik (1);
    juce::AudioBuffer<float> buf (2, 512);
    juce::MidiBuffer m;
    juce::Random rnd (1);
    double maxv = 0; int64_t nan = 0;
    for (int blk = 0; blk < 48000 * 60 * 3 / 512; ++blk)   // 3 minuter
    {
        for (auto* p : { &a, &b, &c })
        {
            for (int ch = 0; ch < 2; ++ch) for (int i = 0; i < 512; ++i) buf.setSample (ch, i, (rnd.nextFloat() - 0.5f) * 0.5f);
            p->processBlock (buf, m);
            for (int ch = 0; ch < 2; ++ch) for (int i = 0; i < 512; ++i) { float v = buf.getSample (ch, i); if (! std::isfinite (v)) ++nan; maxv = std::max (maxv, (double) std::abs (v)); }
        }
        ph.ppq += 512.0 * 130.0 / 60.0 / 48000.0;
    }
    std::printf ("NaN: %lld  max: %.3f  följer b mall=%d\n", (long long) nan, maxv, b.installning().mall);
    a.taOver (motor::BYGG);

    for (auto* p : { &a, &b })
    {
        std::unique_ptr<juce::AudioProcessorEditor> ed (p->createEditor());
        auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.0f);
        juce::File f (ut + (p == &a ? "/central.png" : "/kanal.png"));
        f.deleteFile();
        juce::FileOutputStream os (f);
        juce::PNGImageFormat().writeImageToStream (img, os);
        std::printf ("%s %dx%d\n", f.getFullPathName().toRawUTF8(), img.getWidth(), img.getHeight());
    }
    return nan == 0 ? 0 : 1;
}

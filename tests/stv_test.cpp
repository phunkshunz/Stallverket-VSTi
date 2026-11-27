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
    // b står kvar på AUTO och får en trumloop (kick + hats)
    c.state.getParameter ("roll")->setValueNotifyingHost (c.state.getParameter ("roll")->convertTo0to1 (5));
    int prv = 41;   // "Mörkrets tryck" om det finns (mall MÖRKER), annars ett äldre preset
    for (int i = 0; i < a.antalFabrik(); ++i) if (a.getProgramName (i) == juce::String::fromUTF8 ("Mörkrets tryck")) prv = i;
    a.laddaFabrik (prv);
    // d: en tom kanal med BETONG-KASSETT
    StallverketProcessor d;
    d.setPlayHead (&ph); d.prepareToPlay (48000, 512);
    d.state.getParameter ("kassett")->setValueNotifyingHost (1.0f);
    d.state.getParameter ("k_kit")->setValueNotifyingHost (d.state.getParameter ("k_kit")->convertTo0to1 (32));       // Varvet
    d.state.getParameter ("k_monster")->setValueNotifyingHost (d.state.getParameter ("k_monster")->convertTo0to1 (2)); // Lagerlokal
    d.state.getParameter ("roll")->setValueNotifyingHost (d.state.getParameter ("roll")->convertTo0to1 (7));
    for (int i = 0; i < 40; ++i) juce::MessageManager::getInstance()->runDispatchLoopUntil (20);   // ladda ljuden
    std::printf ("kassett: %s\n", d.kassettStatus().toRawUTF8());
    // ljudprov: kassetten genom hela motorn, sparas som WAV
    std::unique_ptr<juce::AudioFormatWriter> prov;
    {
        juce::File pf ("/tmp/stallverket_ljudprov.wav"); pf.deleteFile();
        std::unique_ptr<juce::OutputStream> os (pf.createOutputStream());
        juce::WavAudioFormat wf;
        prov.reset (wf.createWriterFor (os.get(), 48000.0, 2, 24, {}, 0));
        if (prov != nullptr) os.release();
    }
    double kassettMax = 0, kassettE = 0;
    a.vaxlaRec();        // REC från start
    juce::AudioBuffer<float> buf (2, 512);
    juce::MidiBuffer m;
    juce::Random rnd (1);
    double maxv = 0; int64_t nan = 0, midiN = 0;
    for (int blk = 0; blk < 48000 * 60 * 3 / 512; ++blk)   // 3 minuter
    {
        for (auto* p : { &a, &b, &c })
        {
            for (int ch = 0; ch < 2; ++ch) for (int i = 0; i < 512; ++i)
            {
                float x = (rnd.nextFloat() - 0.5f) * 0.5f;
                if (p == &b)
                {
                    const double sl = ph.ppq + i * 130.0 / 60.0 / 48000.0, fas = sl - std::floor (sl), t = fas * 60.0 / 130.0;
                    const double f8 = std::fmod (sl * 4.0, 1.0) * 60.0 / 130.0 / 4.0;
                    x = (float) (std::sin (6.283 * 55.0 * t) * std::exp (-t * 10.0) * 0.7 + (rnd.nextFloat() - 0.5f) * std::exp (-f8 * 90.0) * 0.15);
                }
                buf.setSample (ch, i, x);
            }
            m.clear();
            p->processBlock (buf, m);
            if (p == &a) midiN += m.getNumEvents();
            for (int ch = 0; ch < 2; ++ch) for (int i = 0; i < 512; ++i) { float v = buf.getSample (ch, i); if (! std::isfinite (v)) ++nan; maxv = std::max (maxv, (double) std::abs (v)); }
        }
        {
            buf.clear(); m.clear();
            d.processBlock (buf, m);
            if (prov != nullptr) prov->writeFromAudioSampleBuffer (buf, 0, 512);
            for (int i = 0; i < 512; ++i) { const float v = buf.getSample (0, i); if (! std::isfinite (v)) ++nan; kassettMax = std::max (kassettMax, (double) std::abs (v)); kassettE += v * v; }
        }
        ph.ppq += 512.0 * 130.0 / 60.0 / 48000.0;
        if (blk % 20 == 0) juce::MessageManager::getInstance()->runDispatchLoopUntil (1);
        if (blk == 2000) a.vaxlaRec();   // stoppa efter ~21 s
        if (ph.ppq >= 16.0) ph.ppq -= 16.0;   // DAW:en loopar 4 takter
    }
    std::printf ("takt efter 3 min med 4-taktsloop: %d (ska vara ~97)  roll c: %d  roll b (AUTO, trumloop): %s  roll a (AUTO, brus): %s\n", (int) (a.slagNu() / 4), c.roll(), motor::rollNamn (b.roll()), motor::rollNamn (a.roll()));
    prov.reset();
    std::printf ("kassett: max %.3f  rms %.4f\n", kassettMax, std::sqrt (kassettE / (48000.0 * 180)));
    std::printf ("NaN: %lld  max: %.3f  följer b mall=%d  MIDI-händelser: %lld\n", (long long) nan, maxv, b.installning().mall, (long long) midiN);
    juce::MessageManager::getInstance()->runDispatchLoopUntil (300);
    {
        auto mapp = juce::File::getSpecialLocation (juce::File::userDesktopDirectory).getChildFile (juce::String::fromUTF8 ("STÄLLVERKET inspelningar"));
        for (auto& f : mapp.findChildFiles (juce::File::findFiles, true, "*.wav"))
            std::printf ("REC: %s  %lld byte\n", f.getFileName().toRawUTF8(), (long long) f.getSize());
    }
    // REFERENSLÄSAREN: en syntad låt (intro, groove, break, bygg, drop, outro) i 130 BPM som WAV
    {
        const double rsr = 22050.0, bpm = 130.0, sps = bpm / 60.0 / rsr;
        const int form[][2] = { { 0, 16 }, { 1, 32 }, { 2, 16 }, { 3, 8 }, { 4, 32 }, { 5, 16 } };
        juce::File wavFil ("/tmp/stv_referens.wav"); wavFil.deleteFile();
        std::unique_ptr<juce::OutputStream> os (wavFil.createOutputStream());
        juce::WavAudioFormat wf;
        std::unique_ptr<juce::AudioFormatWriter> w (wf.createWriterFor (os.get(), rsr, 1, 16, {}, 0));
        os.release();
        juce::Random rr (3);
        double sl = 0;
        juce::AudioBuffer<float> rb (1, 4096);
        for (auto& d : form)
        {
            const int n = (int) (d[1] * 4 / sps);
            for (int i0 = 0; i0 < n; i0 += 4096)
            {
                const int m2 = juce::jmin (4096, n - i0);
                for (int i = 0; i < m2; ++i, sl += sps)
                {
                    const double t = (sl - std::floor (sl)) * 60.0 / bpm, s8 = std::fmod (sl * 2, 1.0) * 30.0 / bpm;
                    const float kick = (float) (std::sin (6.2832 * 50 * t) * std::exp (-t * 8));
                    const float bas = (float) (std::sin (6.2832 * 55 * std::fmod (sl + 0.5, 1.0) * 60 / bpm) * 0.5);
                    const float hat = (rr.nextFloat() * 2 - 1) * (float) std::exp (-s8 * 60) * 0.15f;
                    const float pad = (float) (std::sin (6.2832 * 440 * sl * 60 / bpm) * 0.12);
                    const float upp = (float) (i0 + i) / n;
                    float x = d[0] == 0 ? hat : d[0] == 1 ? kick * 0.8f + bas + hat : d[0] == 2 ? pad : d[0] == 3 ? pad + hat * 2 * upp
                            : d[0] == 4 ? kick + bas * 1.2f + hat * 1.5f + pad : hat * 0.8f;
                    rb.setSample (0, i, x * 0.5f);
                }
                w->writeFromAudioSampleBuffer (rb, 0, m2);
            }
        }
        w.reset();
        a.lasReferens (wavFil);
        for (int i = 0; i < 200 && ! a.referensStatus().contains ("BPM"); ++i) juce::MessageManager::getInstance()->runDispatchLoopUntil (30);
        juce::MessageManager::getInstance()->runDispatchLoopUntil (200);
        std::printf ("referens: %s\n  mall nu: %s  skrift: %s\n", a.referensStatus().toRawUTF8(),
                     motor::kMallar[a.installning().mall].namn, a.skrift().toRawUTF8());
    }
    a.taOver (motor::BYGG);
    const auto mid = a.exporteraMidi();
    std::printf ("MIDI-export: %s (%lld byte)\n", mid.getFileName().toRawUTF8(), (long long) mid.getSize());
    {
        juce::FileInputStream is (mid); juce::MidiFile mf; mf.readFrom (is);
        std::printf ("  spår: %d, händelser i spår 2: %d\n", mf.getNumTracks(), mf.getNumTracks() > 1 ? mf.getTrack (1)->getNumEvents() : 0);
    }

    {
        std::unique_ptr<juce::AudioProcessorEditor> ed (d.createEditor());
        auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.0f);
        juce::File f (ut + "/kassett.png"); f.deleteFile();
        juce::FileOutputStream os (f); juce::PNGImageFormat().writeImageToStream (img, os);
    }
    for (auto* p : { &a, &b })
    {
        std::unique_ptr<juce::AudioProcessorEditor> ed (p->createEditor());
        auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.0f);
        juce::File f (ut + (p == &a ? "/central.png" : "/kanal.png"));
        f.deleteFile();
        juce::FileOutputStream os (f);
        juce::PNGImageFormat().writeImageToStream (img, os);
        std::printf ("%s %dx%d\n", f.getFullPathName().toRawUTF8(), img.getWidth(), img.getHeight());
        if (p == &a)
            for (auto* namn : { "CURVE", "TEMPLATE", "TRICKS" })
                for (auto* c : ed->getChildren())
                    if (auto* b = dynamic_cast<juce::TextButton*> (c); b != nullptr && b->getButtonText() == namn && b->onClick)
                    {
                        b->onClick();
                        auto im = ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.0f);
                        juce::File vf (ut + "/vy_" + juce::String (namn) + ".png"); vf.deleteFile();
                        juce::FileOutputStream vo (vf); juce::PNGImageFormat().writeImageToStream (im, vo);
                    }
    }
    return nan == 0 ? 0 : 1;
}

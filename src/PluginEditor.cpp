#include "PluginEditor.h"
#include "Presets.h"

namespace
{
constexpr int kCentralB = 900, kCentralH = 560;
constexpr int kKanalB = 540, kKanalH = 262;

juce::Colour delFarg (int d)
{
    using namespace ArmColours;
    switch (d)
    {
        case motor::INTRO:     return juce::Colour (0xff4a4d50);
        case motor::GROOVE:    return sodiumDk.withMultipliedBrightness (0.75f);
        case motor::LYFT:      return sodiumDk;
        case motor::BREAK:     return coldDk.withMultipliedBrightness (0.8f);
        case motor::BYGG:      return juce::Colour (0xffe0a020);
        case motor::DROP:      return sodium;
        case motor::KLIMAX:    return juce::Colour (0xffffb15a);
        case motor::UTANDNING: return juce::Colour (0xff8a5a2a);
        case motor::OUTRO:     return juce::Colour (0xff3a3c3f);
        default:               return line2;
    }
}

juce::String rollText (int r)
{
    if (r >= 0 && r < motor::NUM_ROLL) return motor::rollNamn (r);
    if (r == motor::NUM_ROLL) return sprak::trU ("ORÖRD");
    return "-";
}
} // namespace

StallverketEditor::StallverketEditor (StallverketProcessor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    sprak::laddaOm();
    setLookAndFeel (&lnf);

    hallKnapp.setButtonText (utf8 ("HÅLL"));
    slappKnapp.setButtonText (utf8 ("SLÄPP"));

    centralKnapp.setClickingTogglesState (true);
    centralKnapp.setTooltip (utf8 ("CENTRAL håller motorn och styr alla andra STÄLLVERKET. En per projekt."));
    centralAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.state, "central", centralKnapp);

    for (int i = 1; i <= ledning::kKanaler; ++i) kanalVal.addItem (utf8 ("KANAL ") + juce::String (i), i);
    for (int i = 0; i <= motor::NUM_ROLL; ++i) rollVal.addItem (rollText (i), i + 1);
    for (int i = 0; i < motor::kAntalMallar; ++i) mallVal.addItem (juce::String::fromUTF8 (motor::kMallar[i].namn), i + 1);
    for (int i = 0; i < motor::kAntalFormer; ++i) formVal.addItem (motor::kFormNamn[i], i + 1);
    kanalAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (proc.state, "kanal", kanalVal);
    rollAtt  = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (proc.state, "roll", rollVal);
    mallAtt  = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (proc.state, "mall", mallVal);
    formAtt  = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (proc.state, "form", formVal);
    kanalVal.setTooltip (utf8 ("Vilken kanal i ställverket den här instansen är (1-8)."));
    rollVal.setTooltip (utf8 ("Vad som spelar på kanalen. Motorn behandlar en KICK annorlunda än en ATMO. ORÖRD = går rakt igenom."));
    mallVal.setTooltip (utf8 ("MALL: hur låten byggs. Varje mall har egna delar, längder och övergångar."));
    formVal.setTooltip (utf8 ("EVIG = låten byggs vidare i timmar utan att upprepa sig. Annars börjar den om efter OUTRO."));

    fyllPresets();
    presetVal.onChange = [this]
    {
        const int id = presetVal.getSelectedId();
        if (id <= 0) return;
        if (id <= proc.antalFabrik()) proc.laddaFabrik (id - 1);
        else if (juce::isPositiveAndBelow (id - 1000, egna.size())) proc.laddaPresetFil (egna[id - 1000]);
    };
    presetFore.onClick  = [this] { const int i = presetVal.getSelectedItemIndex(); presetVal.setSelectedItemIndex (juce::jmax (0, i - 1)); };
    presetNasta.onClick = [this] { const int i = presetVal.getSelectedItemIndex(); presetVal.setSelectedItemIndex (juce::jmin (presetVal.getNumItems() - 1, i + 1)); };
    sparaKnapp.onClick  = [this] { sparaDialog(); };
    sparaKnapp.setTooltip (utf8 ("Spara motorinställningarna som eget preset (.stallverket)."));

    slumpaFro.onClick  = [this] { proc.slumpaFro(); };
    slumpaAllt.onClick = [this] { proc.slumpaAllt(); presetVal.setText (utf8 ("Slumpad"), juce::dontSendNotification); };
    slumpaFro.setTooltip (utf8 ("Nytt frö: samma mall och känsla, en helt ny låt."));
    slumpaAllt.setTooltip (utf8 ("Slumpar mall, frö och alla rattar."));

    breakNu.onClick = [this] { proc.taOver (motor::BREAK); };
    byggNu.onClick  = [this] { proc.taOver (motor::BYGG); };
    dropNu.onClick  = [this] { proc.taOver (motor::DROP); };
    hallKnapp.setClickingTogglesState (false);
    hallKnapp.onClick  = [this] { proc.vaxlaHall(); };
    slappKnapp.onClick = [this] { proc.slappTaOver(); if (proc.haller()) proc.vaxlaHall(); };
    breakNu.setTooltip (utf8 ("Break från nästa takt."));
    byggNu.setTooltip (utf8 ("Uppbyggnad från nästa takt, med drop efter."));
    dropNu.setTooltip (utf8 ("Drop från nästa takt."));
    hallKnapp.setTooltip (utf8 ("Håller kvar delen som spelar nu, om och om igen. Tryck igen för att släppa."));
    slappKnapp.setTooltip (utf8 ("Släpp allt du tagit över, motorn kör vidare själv."));

    sprakKnapp.onClick = [this] { sprak::sattEngelska (! sprak::engelska()); byggLayout(); };

    fro.attach (proc.state, "fro", utf8 ("FRÖ"));
    energi.attach (proc.state, "energi", "ENERGI");
    variation.attach (proc.state, "variation", "VARIATION");
    brytdjup.attach (proc.state, "brytdjup", "BRYTDJUP");
    bygglangd.attach (proc.state, "bygglangd", utf8 ("BYGGLÄNGD"));
    stok.attach (proc.state, "stok", utf8 ("STÖK"));
    filter.attach (proc.state, "filter", "FILTER");
    andas.attach (proc.state, "andas", "ANDAS");
    dub.attach (proc.state, "dub", "DUBKAST");
    djup.attach (proc.state, "djup", "DJUP");
    pump.attach (proc.state, "pump", "PUMP");
    pumptid.attach (proc.state, "pumptid", "PUMPTID");
    pump.slider.setTooltip (utf8 ("Kicken pumpar de andra kanalerna, utan routing. Följer motorn: ingen kick, ingen pump."));
    pumptid.slider.setTooltip (utf8 ("Hur länge pumpen håller i sig efter varje slag."));
    midiKnapp.setClickingTogglesState (true);
    midiKnapp.setTooltip (utf8 ("Skickar MIDI på kanal 16 när en ny del börjar: not C1 + del, CC 20 = del, CC 21 = energi. Byt scener i Bitwig eller mönster i andra maskiner."));
    midiAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.state, "midiut", midiKnapp);
    for (int i = 0; i < ledning::kKanaler; ++i)
    {
        kanalDjup[i].setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        kanalDjup[i].setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        kanalDjup[i].setRotaryParameters (juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
        kanalDjupAtt[i] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.state, "djup" + juce::String (i + 1), kanalDjup[i]);
        kanalDjup[i].setTooltip (utf8 ("Hur mycket motorn får styra kanal ") + juce::String (i + 1));
    }
    energi.slider.setTooltip (utf8 ("Hög energi: färre breaks, fler klimax, fler element i groovet."));
    variation.slider.setTooltip (utf8 ("Hur ofta element kommer och går, och hur mycket delarnas längd varierar."));
    brytdjup.slider.setTooltip (utf8 ("Hur mycket som försvinner i breaken. Högt = kick och bas ut helt."));
    bygglangd.slider.setTooltip (utf8 ("Hur långa uppbyggnaderna är."));
    stok.slider.setTooltip (utf8 ("Stutter, trumrullar och fills i frasslut."));
    filter.slider.setTooltip (utf8 ("Hur mycket filtren rör sig i övergångar och breaks."));
    andas.slider.setTooltip (utf8 ("Chans att allt tystnar sista slaget innan droppet."));
    dub.slider.setTooltip (utf8 ("Dubkast: kanaler skickas till ett eko som ringer vidare."));
    djup.slider.setTooltip (utf8 ("Hur mycket motorn styr totalt. 0 = allt går rakt igenom."));

    for (auto* c : std::initializer_list<juce::Component*> { &centralKnapp, &kanalVal, &rollVal, &mallVal, &formVal, &presetVal,
                                                             &presetFore, &presetNasta, &sparaKnapp, &slumpaFro, &slumpaAllt,
                                                             &breakNu, &byggNu, &dropNu, &hallKnapp, &slappKnapp, &sprakKnapp,
                                                             &fro, &energi, &variation, &brytdjup, &bygglangd, &stok, &filter, &andas, &dub, &djup, &pump, &pumptid, &midiKnapp })
        addChildComponent (c);
    for (auto& k : kanalDjup) addChildComponent (k);
    addChildComponent (om);

    centralVy = proc.arCentral();
    byggLayout();
    startTimerHz (30);
}

StallverketEditor::~StallverketEditor() { setLookAndFeel (nullptr); }

void StallverketEditor::fyllPresets()
{
    presetVal.clear (juce::dontSendNotification);
    presetVal.addSectionHeading (utf8 ("FABRIK"));
    for (int i = 0; i < presets::kAntal; ++i) presetVal.addItem (juce::String::fromUTF8 (presets::kFabrik[i].namn), i + 1);
    egna = proc.egnaPresets();
    if (! egna.isEmpty())
    {
        presetVal.addSectionHeading (utf8 ("EGNA"));
        for (int i = 0; i < egna.size(); ++i) presetVal.addItem (egna[i].getFileNameWithoutExtension(), 1000 + i);
    }
    presetVal.setTextWhenNothingSelected (proc.presetNamn().isNotEmpty() ? proc.presetNamn() : utf8 ("PRESET"));
}

void StallverketEditor::sparaDialog()
{
    auto* w = new juce::AlertWindow (utf8 ("Spara preset"), utf8 ("Namn på presetet:"), juce::MessageBoxIconType::NoIcon, this);
    w->addTextEditor ("namn", proc.presetNamn().isNotEmpty() ? proc.presetNamn() : utf8 ("Mitt ställverk"));
    w->addButton (utf8 ("SPARA"), 1, juce::KeyPress (juce::KeyPress::returnKey));
    w->addButton (utf8 ("AVBRYT"), 0, juce::KeyPress (juce::KeyPress::escapeKey));
    juce::Component::SafePointer<StallverketEditor> sp (this);
    w->enterModalState (true, juce::ModalCallbackFunction::create ([sp, w] (int r)
    {
        if (sp == nullptr || r != 1) return;
        const auto namn = w->getTextEditorContents ("namn").trim();
        if (namn.isNotEmpty() && sp->proc.sparaPreset (namn)) { sp->fyllPresets(); sp->presetVal.setText (namn, juce::dontSendNotification); }
    }), true);
}

void StallverketEditor::byggLayout()
{
    centralVy = proc.arCentral();
    setSize (centralVy ? kCentralB : kKanalB, centralVy ? kCentralH : kKanalH);

    for (auto* c : std::initializer_list<juce::Component*> { &mallVal, &formVal, &presetVal, &presetFore, &presetNasta, &sparaKnapp,
                                                             &slumpaFro, &slumpaAllt, &breakNu, &byggNu, &dropNu, &hallKnapp, &slappKnapp,
                                                             &fro, &energi, &variation, &brytdjup, &bygglangd, &stok, &filter, &andas, &dub, &djup, &pump, &pumptid, &midiKnapp })
        c->setVisible (centralVy);
    for (auto& k : kanalDjup) k.setVisible (centralVy);
    for (auto* c : std::initializer_list<juce::Component*> { &centralKnapp, &kanalVal, &rollVal, &sprakKnapp }) c->setVisible (true);

    // översätt texterna (rollnamn m.m.)
    for (int i = 0; i <= motor::NUM_ROLL; ++i) rollVal.changeItemText (i + 1, rollText (i));
    for (int i = 1; i <= ledning::kKanaler; ++i) kanalVal.changeItemText (i, sprak::trU ("KANAL ") + juce::String (i));
    centralKnapp.setButtonText ("CENTRAL");
    for (auto* b : { &breakNu, &byggNu, &dropNu, &sparaKnapp, &slumpaFro, &slumpaAllt })
        b->setButtonText (sprak::tr (b->getButtonText()));
    hallKnapp.setButtonText (sprak::trU ("HÅLL"));
    slappKnapp.setButtonText (sprak::trU ("SLÄPP"));
    if (sprak::engelska())
    {
        breakNu.setButtonText ("BREAK NOW"); byggNu.setButtonText ("BUILD NOW"); dropNu.setButtonText ("DROP NOW");
        sparaKnapp.setButtonText ("SAVE"); slumpaFro.setButtonText ("DICE"); slumpaAllt.setButtonText ("DICE ALL");
    }
    else
    {
        breakNu.setButtonText ("BREAK NU"); byggNu.setButtonText ("BYGG NU"); dropNu.setButtonText ("DROP NU");
        sparaKnapp.setButtonText ("SPARA"); slumpaFro.setButtonText ("SLUMPA"); slumpaAllt.setButtonText ("SLUMPA ALLT");
    }
    sprak::oversattAllt (*this);
    resized();
    repaint();
}

void StallverketEditor::resized()
{
    auto r = getLocalBounds().reduced (10, 8);
    om.setBounds (getLocalBounds());

    // ---- översta raden
    auto top = r.removeFromTop (30);
    logoYta = top.removeFromLeft (centralVy ? 200 : 150);
    sprakKnapp.setBounds (top.removeFromRight (52).reduced (0, 4));
    top.removeFromRight (8);
    if (centralVy)
    {
        sparaKnapp.setBounds (top.removeFromRight (60).reduced (0, 2));
        top.removeFromRight (4);
        presetNasta.setBounds (top.removeFromRight (24).reduced (0, 2));
        presetVal.setBounds (top.removeFromRight (190).reduced (0, 2));
        presetFore.setBounds (top.removeFromRight (24).reduced (0, 2));
        top.removeFromRight (12);
    }
    centralKnapp.setBounds (top.removeFromLeft (80).reduced (0, 2));
    top.removeFromLeft (6);
    kanalVal.setBounds (top.removeFromLeft (100).reduced (0, 2));
    top.removeFromLeft (6);
    rollVal.setBounds (top.removeFromLeft (96).reduced (0, 2));

    r.removeFromTop (8);

    if (! centralVy)
    {
        kanalYta = r;
        tidYta = {};
        return;
    }

    // ---- tidslinjen med kanalerna
    tidYta = r.removeFromTop (300);
    {
        auto rader = tidYta.reduced (8).withTrimmedTop (44);
        const int radH = rader.getHeight() / ledning::kKanaler;
        for (int i = 0; i < ledning::kKanaler; ++i)
        {
            auto rad = rader.removeFromTop (radH);
            kanalDjup[i].setBounds (rad.removeFromLeft (110).removeFromRight (30).withSizeKeepingCentre (juce::jmin (26, radH), juce::jmin (26, radH)));
        }
    }

    r.removeFromTop (8);

    // ---- rattar och knappar
    auto botten = r;
    auto knappar = botten.removeFromRight (190);
    auto valRad = botten.removeFromTop (26);
    mallVal.setBounds (valRad.removeFromLeft (150));
    valRad.removeFromLeft (6);
    formVal.setBounds (valRad.removeFromLeft (130));
    valRad.removeFromLeft (6);
    slumpaFro.setBounds (valRad.removeFromLeft (80));
    valRad.removeFromLeft (6);
    slumpaAllt.setBounds (valRad.removeFromLeft (110));
    valRad.removeFromLeft (6);
    midiKnapp.setBounds (valRad.removeFromLeft (80));

    botten.removeFromTop (8);
    Knob* rattar[] = { &fro, &energi, &variation, &brytdjup, &bygglangd, &stok, &filter, &andas, &dub, &pump, &pumptid, &djup };
    const int kb = botten.getWidth() / 6;
    auto rad1 = botten.removeFromTop (botten.getHeight() / 2);
    for (int i = 0; i < 6; ++i) rattar[i]->setBounds (rad1.removeFromLeft (kb).reduced (4, 0));
    for (int i = 6; i < 12; ++i) rattar[i]->setBounds (botten.removeFromLeft (kb).reduced (4, 0));

    knappar.removeFromLeft (10);
    const int bh = (knappar.getHeight() - 4 * 6) / 5;
    for (auto* b : { &breakNu, &byggNu, &dropNu, &hallKnapp, &slappKnapp })
    {
        b->setBounds (knappar.removeFromTop (bh));
        knappar.removeFromTop (6);
    }
}

void StallverketEditor::paint (juce::Graphics& g)
{
    using namespace ArmColours;
    g.fillAll (ground);

    // logga
    g.setColour (sodium);
    g.setFont (juce::FontOptions (centralVy ? 24.0f : 20.0f, juce::Font::bold).withHorizontalScale (0.82f));
    g.drawText (utf8 ("STÄLLVERKET"), logoYta.withTrimmedBottom (8), juce::Justification::centredLeft);
    g.setColour (muted);
    g.setFont (juce::FontOptions (8.5f));
    g.drawText (sprak::trU ("MASKINHALLEN · MOTTAGARE + MOTOR"), logoYta.withTrimmedTop (22), juce::Justification::centredLeft);

    if (centralVy)
    {
        ritaTidslinje (g, tidYta);
        auto b = getLocalBounds().reduced (10, 8);
        b.removeFromTop (30 + 8 + 300 + 8);
        auto knappYta = b.removeFromRight (190);
        drawPanel (g, b.withTrimmedTop (34).expanded (0, 2), "");
        juce::ignoreUnused (knappYta);
    }
    else
        ritaKanalvy (g, kanalYta);
}

void StallverketEditor::ritaTidslinje (juce::Graphics& g, juce::Rectangle<int> yta)
{
    using namespace ArmColours;
    drawPanel (g, yta, sprak::trU ("LÅTEN"), {});
    auto r = yta.reduced (8);
    auto rubrik = r.removeFromTop (16);

    const auto in = proc.installning();
    const auto ov = proc.overstyr();
    const auto plan = motor::byggPlan (in);
    const int spt = proc.slagPerTakt();
    const double slag = proc.slagNu();
    const int64_t taktNu = (int64_t) std::floor (slag / spt);
    int64_t tid = 0;
    const auto s = motor::aktuell (plan, ov, taktNu, tid);

    // status uppe till höger
    {
        juce::String txt;
        auto& nav = ledning::nav();
        int anslutna = 0;
        for (auto& k : nav.kanal) if (ledning::levande (k.senast.load(), StallverketProcessor::nuMs())) ++anslutna;
        txt << sprak::trU (motor::delNamn (s.typ)) << "  " << juce::String (tid + 1) << "/" << juce::String (s.langd)
            << "   ·   " << juce::String (anslutna) << sprak::trU (" kanaler anslutna");
        if (proc.haller()) txt << "   ·   " << sprak::trU ("HÅLLER");
        g.setColour (proc.spelarNu() ? text : muted);
        g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
        g.drawText (txt, rubrik.withTrimmedLeft (60), juce::Justification::centredRight);
        if (proc.extraCentral())
        {
            g.setColour (sodium);
            g.drawText (sprak::trU ("EN ANNAN CENTRAL STYR REDAN"), rubrik.withTrimmedLeft (60), juce::Justification::centred);
        }
    }

    r.removeFromTop (4);
    const int vansterB = 110;
    auto delRad = r.removeFromTop (24);
    auto delYta = delRad.withTrimmedLeft (vansterB);

    // fönster: 8 takter bakåt, 56 framåt
    const int fonster = 64;
    const int64_t forsta = juce::jmax ((int64_t) 0, taktNu - 8);
    const float tb = (float) delYta.getWidth() / (float) fonster;
    auto xFor = [&] (double takt) { return (float) delYta.getX() + (float) (takt - (double) forsta) * tb; };

    // delarna
    {
        int64_t t = forsta;
        while (t < forsta + fonster)
        {
            int64_t ti = 0;
            const auto d = motor::aktuell (plan, ov, t, ti);
            const int64_t slut = juce::jmin (forsta + fonster, t + (d.langd - ti));
            auto box = juce::Rectangle<float> (xFor ((double) t), (float) delYta.getY(), (float) (slut - t) * tb, (float) delYta.getHeight()).reduced (0.5f, 0.0f);
            g.setColour (delFarg (d.typ));
            g.fillRoundedRectangle (box, 3.0f);
            if (box.getWidth() > 30)
            {
                g.setColour (ground.withAlpha (0.85f));
                g.setFont (juce::FontOptions (9.5f, juce::Font::bold));
                g.drawText (sprak::trU (motor::delNamn (d.typ)), box.reduced (4, 0), juce::Justification::centredLeft, true);
            }
            t = slut;
        }
    }
    g.setColour (muted);
    g.setFont (juce::FontOptions (9.0f));
    g.drawText (sprak::trU ("TAKT ") + juce::String (taktNu + 1), delRad.withWidth (vansterB - 8), juce::Justification::centredLeft);

    r.removeFromTop (4);

    // kanalerna
    auto& nav = ledning::nav();
    const auto nu = StallverketProcessor::nuMs();
    const int radH = r.getHeight() / ledning::kKanaler;
    for (int k = 0; k < ledning::kKanaler; ++k)
    {
        auto rad = r.removeFromTop (radH).reduced (0, 1);
        const bool lever = ledning::levande (nav.kanal[k].senast.load(), nu) && nav.kanal[k].roll.load() >= 0;
        const int rl = lever ? nav.kanal[k].roll.load() : -1;
        auto namn = rad.withWidth (vansterB - 36);

        g.setColour (lever ? text : muted.withAlpha (0.5f));
        g.setFont (juce::FontOptions (10.0f, juce::Font::bold));
        g.drawText (juce::String (k + 1) + "  " + (lever ? rollText (rl) : juce::String ("-")), namn.withTrimmedLeft (10), juce::Justification::centredLeft);

        // nivålampa
        const float niva = lever ? juce::jlimit (0.0f, 1.0f, nav.kanal[k].niva.load()) : 0.0f;
        g.setColour (niva > 0.01f ? sodium.withAlpha (0.3f + 0.7f * niva) : line2);
        g.fillEllipse (juce::Rectangle<float> (6, 6).withCentre ({ (float) namn.getX() + 3.0f, (float) namn.getCentreY() }));

        auto bana = rad.withTrimmedLeft (vansterB);
        g.setColour (raised.withAlpha (0.6f));
        g.fillRect (bana);
        if (rl >= 0 && rl < motor::NUM_ROLL)
            for (int i = 0; i < fonster; ++i)
            {
                const float v = motor::visning (plan, in, ov, k, rl, forsta + i, spt);
                if (v < 0.02f) continue;
                auto cell = juce::Rectangle<float> (xFor ((double) (forsta + i)), (float) bana.getY(), tb, (float) bana.getHeight()).reduced (0.4f, 2.0f);
                const float h = cell.getHeight() * (0.25f + 0.75f * v);
                g.setColour (sodium.withAlpha (0.25f + 0.65f * v));
                g.fillRect (cell.withSizeKeepingCentre (cell.getWidth(), h));
            }
    }

    // spelhuvud
    if (slag > 0.0 || proc.spelarNu())
    {
        const float x = xFor (slag / spt);
        g.setColour (text);
        g.drawLine (x, (float) delYta.getY() - 2, x, (float) yta.getBottom() - 8, 1.2f);
    }
}

void StallverketEditor::ritaKanalvy (juce::Graphics& g, juce::Rectangle<int> yta)
{
    using namespace ArmColours;
    drawPanel (g, yta, "", {});
    auto r = yta.reduced (12, 10);

    const bool lever = proc.centralLever();
    const bool arC = proc.arCentral();
    auto status = r.removeFromTop (16);
    g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
    if (lever && ! arC) { g.setColour (cold); g.drawText (sprak::trU ("FÖLJER CENTRAL"), status, juce::Justification::centredLeft); }
    else { g.setColour (sodium); g.drawText (sprak::trU ("INGEN CENTRAL HITTAD: KÖR PÅ SPARADE INSTÄLLNINGAR"), status, juce::Justification::centredLeft); }

    const auto in = proc.installning();
    const auto ov = proc.overstyr();
    const auto plan = motor::byggPlan (in);
    const int spt = proc.slagPerTakt();
    const double slag = proc.slagNu();
    const int64_t taktNu = (int64_t) std::floor (slag / spt);
    int64_t tid = 0;
    const auto s = motor::aktuell (plan, ov, taktNu, tid);
    const int rl = proc.roll();
    const int k = proc.kanal();

    r.removeFromTop (8);
    auto mitt = r.removeFromTop (70);
    auto delText = mitt.removeFromLeft (190);
    g.setColour (proc.spelarNu() ? delFarg (s.typ).brighter (0.3f) : muted);
    g.setFont (juce::FontOptions (30.0f, juce::Font::bold).withHorizontalScale (0.85f));
    g.drawText (proc.spelarNu() ? sprak::trU (motor::delNamn (s.typ)) : sprak::trU ("STOPP"), delText.removeFromTop (40), juce::Justification::centredLeft);
    g.setColour (muted);
    g.setFont (juce::FontOptions (11.0f));
    g.drawText (sprak::trU ("TAKT ") + juce::String (tid + 1) + sprak::trU (" AV ") + juce::String (s.langd) + "   ·   " + rollText (rl),
                delText, juce::Justification::centredLeft);

    // mätare: nivå, lågpass, högpass
    auto matare = [&] (juce::Rectangle<int> m, const juce::String& namn, float v, juce::Colour c)
    {
        g.setColour (muted);
        g.setFont (juce::FontOptions (9.0f));
        g.drawText (namn, m.removeFromTop (12), juce::Justification::centredLeft);
        auto bar = m.removeFromTop (10).toFloat();
        g.setColour (line);
        g.fillRoundedRectangle (bar, 3.0f);
        g.setColour (c);
        g.fillRoundedRectangle (bar.withWidth (bar.getWidth() * juce::jlimit (0.0f, 1.0f, v)), 3.0f);
    };
    mitt.removeFromLeft (10);
    auto m1 = mitt.removeFromTop (23), m2 = mitt.removeFromTop (23), m3 = mitt.removeFromTop (23);
    matare (m1, sprak::trU ("NIVÅ"), proc.gainNu(), sodium);
    matare (m2, sprak::trU ("LÅGPASS"), proc.lpNu(), sodiumDk);
    matare (m3, sprak::trU ("HÖGPASS"), proc.hpNu(), cold);

    // de kommande 32 takterna för den här kanalen
    r.removeFromTop (12);
    g.setColour (muted);
    g.setFont (juce::FontOptions (9.0f));
    g.drawText (sprak::trU ("KOMMANDE 32 TAKTER"), r.removeFromTop (12), juce::Justification::centredLeft);
    auto bana = r.removeFromTop (juce::jmin (54, r.getHeight()));
    const int n = 32;
    const float tb = (float) bana.getWidth() / (float) n;
    for (int i = 0; i < n; ++i)
    {
        int64_t ti = 0;
        const auto d = motor::aktuell (plan, ov, taktNu + i, ti);
        auto cell = juce::Rectangle<float> ((float) bana.getX() + (float) i * tb, (float) bana.getY(), tb, (float) bana.getHeight()).reduced (0.5f, 0.0f);
        g.setColour (delFarg (d.typ).withAlpha (0.35f));
        g.fillRect (cell.removeFromTop (6));
        if (rl < motor::NUM_ROLL)
        {
            const float v = motor::visning (plan, in, ov, k, rl, taktNu + i, spt);
            g.setColour (sodium.withAlpha (0.2f + 0.7f * v));
            const float h = cell.getHeight() * (0.15f + 0.85f * v);
            g.fillRect (cell.withTop (cell.getBottom() - h));
        }
    }
}

void StallverketEditor::mouseDown (const juce::MouseEvent& e)
{
    if (logoYta.contains (e.getPosition()))
    {
        om.setVisible (true);
        om.toFront (false);
    }
}

void StallverketEditor::timerCallback()
{
    if (proc.arCentral() != centralVy) byggLayout();
    hallKnapp.setToggleState (proc.haller(), juce::dontSendNotification);
    const auto ov = ledning::nav().ovTyp.load();
    breakNu.setToggleState (ov == motor::BREAK, juce::dontSendNotification);
    byggNu.setToggleState (ov == motor::BYGG, juce::dontSendNotification);
    dropNu.setToggleState (ov == motor::DROP, juce::dontSendNotification);
    if (proc.presetNamn().isNotEmpty() && presetVal.getSelectedId() == 0)
        presetVal.setText (proc.presetNamn(), juce::dontSendNotification);
    repaint (centralVy ? tidYta : kanalYta);
}

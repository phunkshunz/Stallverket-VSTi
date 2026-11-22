#include "PluginEditor.h"
#include "Presets.h"

namespace
{
constexpr int kCentralB = 920, kCentralH = 640;
constexpr int kTidH = 380;
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
    if (r == motor::NUM_ROLL + 1) return "AUTO";
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
    for (int i = 0; i <= motor::NUM_ROLL + 1; ++i) rollVal.addItem (rollText (i), i + 1);
    for (int i = 0; i < motor::kAntalMallar; ++i) mallVal.addItem (juce::String::fromUTF8 (motor::kMallar[i].namn), i + 1);
    mallVal.addItem ("EGEN", motor::kAntalMallar + 1);
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

    recKnapp.onClick = [this] { proc.vaxlaRec(); };
    recKnapp.setTooltip (utf8 ("Spelar in varje kanal som en egen WAV på skrivbordet (mappen STÄLLVERKET inspelningar). Börjar och slutar på hel takt. Lägg CENTRAL på mastern med ROLL = ORÖRD för hela mixen."));
    recKnapp.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xffe0412f));
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

    sprakKnapp.onClick = [this] { sprak::sattEngelska (! sprak::engelska()); licensRuta.texter(); byggLayout(); };

    // ---- flikar
    flikLat.setButtonText (utf8 ("LÅTEN")); flikKurva.setButtonText ("KURVA"); flikMall.setButtonText ("MALL"); flikTrick.setButtonText ("TRICK");
    flikTrick.setTooltip (utf8 ("Roliga effekter i breaks, övergångar och groove. Slå av och på, och välj hur ofta."));
    int fi = 0;
    for (auto* b : { &flikLat, &flikKurva, &flikMall, &flikTrick })
    {
        b->setClickingTogglesState (true);
        b->setRadioGroupId (77);
        const int v = fi++;
        b->onClick = [this, v] { visaVy (v); };
    }
    flikLat.setToggleState (true, juce::dontSendNotification);
    flikKurva.setTooltip (utf8 ("Rita energin över hela setet. Ersätter ENERGI-ratten när KURVA PÅ är tänd."));
    flikMall.setTooltip (utf8 ("Bygg en egen mall: längder, chanser och karaktär. Välj EGEN i mallmenyn."));
    exportKnapp.setButtonText (utf8 ("SPARA MIDI"));
    exportKnapp.setTooltip (utf8 ("Sparar låten som MIDI-fil: delarna som markörer och noter, och ett spår per kanal med nivå (CC 11), lågpass (CC 74) och högpass (CC 75)."));
    exportKnapp.onClick = [this]
    {
        const auto f = proc.exporteraMidi();
        if (f.existsAsFile()) f.revealToUser();
    };

    // ---- kurva
    kurvaPa.setButtonText (utf8 ("KURVA PÅ"));
    kurvaPa.setClickingTogglesState (true);
    kurvaPaAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.state, "kurva", kurvaPa);
    for (auto* t : { "128", "256", "512", "1024", "2048" }) kurvLangd.addItem (juce::String (t) + utf8 (" TAKTER"), kurvLangd.getNumItems() + 1);
    kurvLangdAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (proc.state, "kurvalangd", kurvLangd);
    kurvLangd.setTooltip (utf8 ("Hur lång kurvan är. 512 takter är ungefär en timme i 130 BPM."));

    // ---- mallredigerare
    const char* langdId[] = { "e_in", "e_gr", "e_ly", "e_br", "e_by", "e_dr", "e_kl", "e_ut", "e_ou" };
    for (int d = 0; d < motor::NUM_DEL; ++d)
    {
        for (auto* t : { "4", "8", "16", "32", "64" }) mallLangd[d].addItem (t, mallLangd[d].getNumItems() + 1);
        mallLangdAtt[d] = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (proc.state, langdId[d], mallLangd[d]);
        mallLangd[d].setTooltip (utf8 ("Längd i takter: ") + sprak::trU (motor::delNamn (d)));
    }
    const char* rattId[] = { "e_plyft", "e_pbreak", "e_pbygg", "e_pdrop", "e_pklimax", "e_putand", "e_hard", "e_stok", "e_dub", "e_filter" };
    for (int i = 0; i < 10; ++i)
    {
        mallRatt[i].setSliderStyle (juce::Slider::LinearBar);
        mallRatt[i].setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        mallRattAtt[i] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.state, rattId[i], mallRatt[i]);
        mallRatt[i].setColour (juce::Slider::textBoxTextColourId, juce::Colours::transparentBlack);
        mallRatt[i].setColour (juce::Slider::trackColourId, ArmColours::sodiumDk);
        mallRatt[i].onValueChange = [this] { repaint (tidYta); };
    }
    atmoForst.setButtonText (utf8 ("ATMO FÖRST"));
    atmoForst.setClickingTogglesState (true);
    atmoAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.state, "e_atmo", atmoForst);
    atmoForst.setTooltip (utf8 ("I introt kommer atmo och synt in först, kicken sist."));
    kopieraMall.setButtonText (utf8 ("KOPIERA MALL"));
    kopieraMall.setTooltip (utf8 ("Kopierar mallen som spelar nu till EGEN, så du kan börja därifrån."));
    kopieraMall.onClick = [this] { proc.kopieraMallTillEgen(); };
    sparaMall.setButtonText (utf8 ("SPARA MALL"));
    sparaMall.onClick = [this] { namnDialog (utf8 ("Spara mall"), [this] (const juce::String& n) { if (proc.sparaMall (n)) fyllMallar(); }); };
    laddaMall.onChange = [this]
    {
        const int i = laddaMall.getSelectedId() - 1;
        if (juce::isPositiveAndBelow (i, mallFiler.size())) proc.laddaMall (mallFiler[i]);
    };
    fyllMallar();
    licensRuta.onUpplast = [this] { repaint(); };

    // ---- trick
    const char* trickId[] = { "t_frys", "t_halvtid", "t_bak", "t_vinyl", "t_radio", "t_eko", "t_tomrum",
                              "t_stam", "t_glapp", "t_byt", "t_spoke", "t_stopp",
                              "t_fas", "t_dubkedja", "t_bage", "t_bassvep", "t_kickpaus" };
    const char* trickText[] = { "FRYS", "HALVTID", "BAKLÄNGES", "VINYLBROMS", "RADIOBYTE", "KORT EKO", "TOMRUM",
                                "STAMNING", "BANDGLAPP", "KANALBYTE", "SPÖKEKO", "STOPP-SLAG",
                                "FASFÖRSKJUTNING", "DUBKEDJA", "FILTERBÅGE", "BASSVEP", "KICKPAUS" };
    const char* trickTips[] = {
        "Sista biten innan breaket fryses och ringer vidare medan allt annat tystnar.",
        "Breaket spelar i halv hastighet, tungt och segt.",
        "Hela breaket spelas baklänges ur det som just spelats.",
        "Sista takten innan droppet bromsar in som en skiva, sedan smäller droppet in.",
        "Kanalerna hoppar mellan filtrerat och öppet, som när man vrider på en radio.",
        "Sista slaget upprepas allt snabbare och högre in i droppet.",
        "Allt tystnar utom en slumpvald kanal, ensam i rummet.",
        "En 1/8-del hackar som en platta som hoppar.",
        "Tonhöjden dippar ett slag, som ett band som slirar.",
        "Vänster och höger byter plats en takt.",
        "En kanal försvinner men dess eko hörs vidare.",
        "Allt tystnar ett halvt slag mitt i takten.",
        "En kanal glider en 1/16 var åttonde takt mot de andra, som hos Steve Reich. Groovet ändras utan att något läggs till.",
        "I breaket matar ekot sig självt allt mer, nästan rundgång, och klipps tvärt i droppet.",
        "En kanal får en långsam filterbåge genom delen: stängd, öppen i mitten, stängd igen.",
        "Basen börjar stängd och sveper upp under droppets första takt.",
        "Kicken andas ut de två sista slagen i en 16-taktsfras." };
    for (int i = 0; i < motor::kAntalTrick; ++i)
    {
        trickKnapp[i].setButtonText (utf8 (trickText[i]));
        trickKnapp[i].setClickingTogglesState (true);
        trickKnapp[i].setTooltip (utf8 (trickTips[i]));
        trickAtt[i] = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.state, trickId[i], trickKnapp[i]);
        addChildComponent (trickKnapp[i]);
    }
    trickMangd.setSliderStyle (juce::Slider::LinearBar);
    trickMangd.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    trickMangd.setColour (juce::Slider::textBoxTextColourId, juce::Colours::transparentBlack);
    trickMangd.setColour (juce::Slider::trackColourId, ArmColours::sodiumDk);
    trickMangdAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.state, "trickmangd", trickMangd);
    trickMangd.onValueChange = [this] { repaint (tidYta); };
    trickMangd.setTooltip (utf8 ("Hur ofta trick får hända. 0 = aldrig, 100 % = så ofta det går."));
    addChildComponent (trickMangd);
    skriftFalt.setText (proc.skrift(), false);
    skriftFalt.setFont (juce::FontOptions (14.0f, juce::Font::bold));
    skriftFalt.setColour (juce::TextEditor::backgroundColourId, ArmColours::raised);
    skriftFalt.setColour (juce::TextEditor::textColourId, ArmColours::text);
    skriftFalt.setColour (juce::TextEditor::outlineColourId, ArmColours::line2);
    skriftFalt.setColour (juce::TextEditor::focusedOutlineColourId, ArmColours::sodium);
    skriftFalt.onTextChange = [this] { proc.sattSkrift (skriftFalt.getText()); repaint (tidYta); };
    skriftFalt.setTooltip (utf8 ("Mallen SKRIFTEN följer den här texten. I=INTRO G=GROOVE L=LYFT B=BREAK U=BYGG D=DROP K=KLIMAX A=UTANDNING O=OUTRO, siffra = takter."));
    addChildComponent (skriftFalt);

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
    overgang.attach (proc.state, "overgang", utf8 ("ÖVERGÅNG"));
    overgang.slider.setTooltip (utf8 ("Övergångseffekter: tape stop innan breaken, stutter som stiger i tonhöjd och rum som sväller in i droppet."));
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
                                                             &breakNu, &byggNu, &dropNu, &hallKnapp, &slappKnapp, &recKnapp, &sprakKnapp,
                                                             &fro, &energi, &variation, &brytdjup, &bygglangd, &stok, &filter, &andas, &dub, &djup, &pump, &pumptid, &overgang, &midiKnapp })
        addChildComponent (c);
    for (auto& k : kanalDjup) addChildComponent (k);
    for (auto* c : std::initializer_list<juce::Component*> { &flikLat, &flikKurva, &flikMall, &flikTrick, &exportKnapp, &kurvaPa, &kurvLangd,
                                                             &atmoForst, &kopieraMall, &sparaMall, &laddaMall })
        addChildComponent (c);
    for (auto& c : mallLangd) addChildComponent (c);
    for (auto& c : mallRatt) addChildComponent (c);
    addChildComponent (om);
    addChildComponent (licensRuta);
    licensRuta.setVisible (! licens::upplast().load());

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
                                                             &slumpaFro, &slumpaAllt, &breakNu, &byggNu, &dropNu, &hallKnapp, &slappKnapp, &recKnapp,
                                                             &fro, &energi, &variation, &brytdjup, &bygglangd, &stok, &filter, &andas, &dub, &djup, &pump, &pumptid, &overgang, &midiKnapp })
        c->setVisible (centralVy);
    for (auto* c : std::initializer_list<juce::Component*> { &centralKnapp, &kanalVal, &rollVal, &sprakKnapp }) c->setVisible (true);
    for (auto* c : { &flikLat, &flikKurva, &flikMall, &flikTrick, &exportKnapp }) c->setVisible (centralVy);
    visaVy (vy);

    // översätt texterna (rollnamn m.m.)
    for (int i = 0; i <= motor::NUM_ROLL + 1; ++i) rollVal.changeItemText (i + 1, rollText (i));
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
    licensRuta.toFront (false);
    repaint();
}

void StallverketEditor::resized()
{
    auto r = getLocalBounds().reduced (10, 8);
    om.setBounds (getLocalBounds());
    licensRuta.setBounds (getLocalBounds());

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
    tidYta = r.removeFromTop (kTidH);
    {
        auto f = tidYta.reduced (8).removeFromTop (18);
        for (auto* b : { &flikLat, &flikKurva, &flikMall, &flikTrick }) { b->setBounds (f.removeFromLeft (64)); f.removeFromLeft (4); }

        // trick: tre spalter med knappar, hur ofta, och skriften längst ner
        auto tv = tidYta.reduced (12).withTrimmedTop (30);
        auto skr = tv.removeFromBottom (54);
        skriftYta = skr.removeFromTop (16);
        skriftFalt.setBounds (skr.removeFromTop (30));
        tv.removeFromBottom (8);
        auto ofta = tv.removeFromRight (190);
        trickRub[3] = ofta.removeFromTop (16);
        trickMangd.setBounds (ofta.removeFromTop (30).reduced (0, 4));
        tv.removeFromRight (16);
        const int sp = (tv.getWidth() - 24) / 3;
        auto c1 = tv.removeFromLeft (sp); tv.removeFromLeft (12);
        auto c2 = tv.removeFromLeft (sp); tv.removeFromLeft (12);
        auto c3 = tv;
        trickRub[0] = c1.removeFromTop (16); trickRub[1] = c2.removeFromTop (16); trickRub[2] = c3.removeFromTop (16);
        const int bh = juce::jmin (30, c3.getHeight() / 9);
        // BREAK: frys, halvtid, baklänges, tomrum, radio, dubkedja
        for (int i : { 0, 1, 2, 6, 4, 13 }) { trickKnapp[i].setBounds (c1.removeFromTop (bh).reduced (0, 2)); }
        // ÖVERGÅNG: vinyl, eko, bassvep
        for (int i : { 3, 5, 15 }) { trickKnapp[i].setBounds (c2.removeFromTop (bh).reduced (0, 2)); }
        // GROOVE
        for (int i : { 7, 8, 9, 10, 11, 12, 14, 16 }) { trickKnapp[i].setBounds (c3.removeFromTop (bh).reduced (0, 2)); }

        // kurva
        auto kv = tidYta.reduced (8).withTrimmedTop (26);
        auto kRad = kv.removeFromTop (22);
        kurvaPa.setBounds (kRad.removeFromLeft (90));
        kRad.removeFromLeft (6);
        kurvLangd.setBounds (kRad.removeFromLeft (130));
        kv.removeFromTop (8);
        kurvYta = kv.reduced (20, 10);

        // mall: tre spalter
        auto mv = tidYta.reduced (12).withTrimmedTop (30);
        auto knappRad = mv.removeFromBottom (24);
        kopieraMall.setBounds (knappRad.removeFromLeft (120)); knappRad.removeFromLeft (6);
        sparaMall.setBounds (knappRad.removeFromLeft (110)); knappRad.removeFromLeft (6);
        laddaMall.setBounds (knappRad.removeFromLeft (200));
        mv.removeFromBottom (10);
        const int sb = (mv.getWidth() - 40) / 3;
        auto s1 = mv.removeFromLeft (sb); mv.removeFromLeft (20);
        auto s2 = mv.removeFromLeft (sb); mv.removeFromLeft (20);
        auto s3 = mv;
        mallRubYta[0] = s1.removeFromTop (16); mallRubYta[1] = s2.removeFromTop (16); mallRubYta[2] = s3.removeFromTop (16);
        const int radH = 28;
        for (int d = 0; d < motor::NUM_DEL; ++d)
        {
            auto rad = s1.removeFromTop (radH);
            mallLangd[d].setBounds (rad.removeFromRight (70).reduced (0, 3));
        }
        for (int i = 0; i < 6; ++i) mallRatt[i].setBounds (s2.removeFromTop (radH + 8).removeFromRight (sb - 110).reduced (0, 6));
        for (int i = 6; i < 10; ++i) mallRatt[i].setBounds (s3.removeFromTop (radH + 8).removeFromRight (sb - 110).reduced (0, 6));
        atmoForst.setBounds (s3.removeFromTop (radH + 8).removeFromRight (sb - 110).reduced (0, 5));
    }
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
    valRad.removeFromLeft (6);
    exportKnapp.setBounds (valRad.removeFromLeft (110));

    botten.removeFromTop (8);
    Knob* rattar[] = { &fro, &energi, &variation, &brytdjup, &bygglangd, &stok, &filter, &andas, &dub, &overgang, &pump, &pumptid, &djup };
    const int kb = botten.getWidth() / 7;
    auto rad1 = botten.removeFromTop (botten.getHeight() / 2);
    for (int i = 0; i < 7; ++i) rattar[i]->setBounds (rad1.removeFromLeft (kb).reduced (2, 0));
    for (int i = 7; i < 13; ++i) rattar[i]->setBounds (botten.removeFromLeft (kb).reduced (2, 0));

    knappar.removeFromLeft (10);
    const int bh = (knappar.getHeight() - 5 * 6) / 6;
    for (auto* b : { &recKnapp, &breakNu, &byggNu, &dropNu, &hallKnapp, &slappKnapp })
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
        if (vy == 0) ritaTidslinje (g, tidYta);
        else if (vy == 1) ritaKurva (g, tidYta);
        else if (vy == 2) { drawPanel (g, tidYta, "", {}); ritaMallRubriker (g); }
        else { drawPanel (g, tidYta, "", {}); ritaTrick (g); }
        auto b = getLocalBounds().reduced (10, 8);
        b.removeFromTop (30 + 8 + kTidH + 8);
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
    drawPanel (g, yta, "", {});
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
        g.drawText (txt, rubrik.withTrimmedLeft (220), juce::Justification::centredRight);
        if (proc.extraCentral())
        {
            g.setColour (sodium);
            g.drawText (sprak::trU ("EN ANNAN CENTRAL STYR REDAN"), rubrik.withTrimmedLeft (220), juce::Justification::centredLeft);
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
        const bool tyst = lever && ! ledning::levande (nav.kanal[k].ljud.load(), nu, 3000);
        const int rl = lever ? nav.kanal[k].roll.load() : -1;
        auto namn = rad.withWidth (vansterB - 36);

        g.setColour (lever ? (tyst ? muted : text) : muted.withAlpha (0.5f));
        g.setFont (juce::FontOptions (juce::jmin (10.0f, (float) radH * 0.6f), juce::Font::bold));
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
    if (proc.spelarIn())
    {
        g.setColour (juce::Colour (0xffe0412f));
        g.drawText (proc.skriverNu() ? sprak::trU ("● SPELAR IN") : sprak::trU ("REC: VÄNTAR PÅ TAKT"), status, juce::Justification::centredRight);
    }
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
    const juce::String rollStr = proc.rollVal() == motor::NUM_ROLL + 1
        ? juce::String ("AUTO: ") + (proc.autoGissning() >= 0 ? rollText (proc.autoGissning()) : sprak::trU ("LYSSNAR ..."))
        : rollText (rl);
    g.drawText (sprak::trU ("TAKT ") + juce::String (tid + 1) + sprak::trU (" AV ") + juce::String (s.langd) + "   ·   " + rollStr,
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
    if (centralVy && vy == 1 && kurvYta.expanded (10).contains (e.getPosition())) { dragPunkt = -1; dragKurva (e.getPosition()); return; }
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
    {
        const bool rec = proc.spelarIn();
        recKnapp.setToggleState (rec, juce::dontSendNotification);
        juce::String t = "REC";
        if (rec)
        {
            const int sek = (int) proc.recSekunder();
            t = proc.skriverNu() ? juce::String::fromUTF8 ("● REC ") + juce::String (sek / 60) + ":" + juce::String (sek % 60).paddedLeft ('0', 2)
                                 : sprak::trU ("REC: VÄNTAR PÅ TAKT");
        }
        if (recKnapp.getButtonText() != t) recKnapp.setButtonText (t);
    }
    const auto ov = ledning::nav().ovTyp.load();
    breakNu.setToggleState (ov == motor::BREAK, juce::dontSendNotification);
    byggNu.setToggleState (ov == motor::BYGG, juce::dontSendNotification);
    dropNu.setToggleState (ov == motor::DROP, juce::dontSendNotification);
    if (proc.presetNamn().isNotEmpty() && presetVal.getSelectedId() == 0)
        presetVal.setText (proc.presetNamn(), juce::dontSendNotification);
    repaint (centralVy ? tidYta : kanalYta);
}

void StallverketEditor::visaVy (int v)
{
    vy = v;
    const bool c = centralVy;
    for (auto& k : kanalDjup) k.setVisible (c && vy == 0);
    for (auto* b : { &kurvaPa }) b->setVisible (c && vy == 1);
    kurvLangd.setVisible (c && vy == 1);
    for (auto& m : mallLangd) m.setVisible (c && vy == 2);
    for (auto& m : mallRatt) m.setVisible (c && vy == 2);
    for (auto* b : std::initializer_list<juce::Component*> { &atmoForst, &kopieraMall, &sparaMall, &laddaMall }) b->setVisible (c && vy == 2);
    for (auto& b : trickKnapp) b.setVisible (c && vy == 3);
    trickMangd.setVisible (c && vy == 3);
    skriftFalt.setVisible (c && vy == 3);
    (vy == 0 ? flikLat : vy == 1 ? flikKurva : vy == 2 ? flikMall : flikTrick).setToggleState (true, juce::dontSendNotification);
    repaint();
}

void StallverketEditor::fyllMallar()
{
    mallFiler = proc.egnaMallar();
    laddaMall.clear (juce::dontSendNotification);
    for (int i = 0; i < mallFiler.size(); ++i) laddaMall.addItem (mallFiler[i].getFileNameWithoutExtension(), i + 1);
    laddaMall.setTextWhenNothingSelected (sprak::trU (mallFiler.isEmpty() ? "INGA EGNA MALLAR" : "LADDA MALL"));
}

void StallverketEditor::namnDialog (const juce::String& rubrik, std::function<void (const juce::String&)> klar)
{
    auto* w = new juce::AlertWindow (rubrik, utf8 ("Namn:"), juce::MessageBoxIconType::NoIcon, this);
    w->addTextEditor ("namn", utf8 ("Min mall"));
    w->addButton (sprak::trU ("SPARA"), 1, juce::KeyPress (juce::KeyPress::returnKey));
    w->addButton (sprak::trU ("AVBRYT"), 0, juce::KeyPress (juce::KeyPress::escapeKey));
    juce::Component::SafePointer<StallverketEditor> sp (this);
    w->enterModalState (true, juce::ModalCallbackFunction::create ([sp, w, klar] (int r)
    {
        if (sp == nullptr || r != 1) return;
        const auto n = w->getTextEditorContents ("namn").trim();
        if (n.isNotEmpty()) klar (n);
    }), true);
}

void StallverketEditor::ritaMallRubriker (juce::Graphics& g)
{
    using namespace ArmColours;
    const juce::String rub[] = { sprak::trU ("LÄNGDER (TAKTER)"), sprak::trU ("CHANSER"), sprak::trU ("KARAKTÄR") };
    g.setFont (juce::FontOptions (10.0f, juce::Font::bold));
    for (int i = 0; i < 3; ++i) { g.setColour (sodium); g.drawText (rub[i], mallRubYta[i], juce::Justification::centredLeft); }

    g.setFont (juce::FontOptions (10.0f, juce::Font::bold));
    g.setColour (text);
    for (int d = 0; d < motor::NUM_DEL; ++d)
    {
        auto b = mallLangd[d].getBounds();
        g.setColour (delFarg (d));
        g.fillRoundedRectangle (juce::Rectangle<float> (8, 8).withCentre ({ (float) mallRubYta[0].getX() + 5.0f, (float) b.getCentreY() }), 2.0f);
        g.setColour (text);
        g.drawText (sprak::trU (motor::delNamn (d)), mallRubYta[0].getX() + 14, b.getY(), 120, b.getHeight(), juce::Justification::centredLeft);
    }
    const char* namn[] = { "LYFT EFTER GROOVE", "BREAK EFTER GROOVE", "BYGG DIREKT", "DROP DIREKT", "KLIMAX", "UTANDNING",
                           "HÅRDA KLIPP", "STÖK", "DUBKAST", "FILTER" };
    for (int i = 0; i < 10; ++i)
    {
        auto b = mallRatt[i].getBounds();
        const int x = mallRubYta[i < 6 ? 1 : 2].getX();
        g.setColour (text);
        g.drawText (sprak::trU (namn[i]), x, b.getY() - 9, 108, b.getHeight(), juce::Justification::centredLeft);
        g.setColour (sodium);
        g.drawText (juce::String (juce::roundToInt (mallRatt[i].getValue() * 100.0)) + " %", x, b.getY() + 7, 108, b.getHeight(), juce::Justification::centredLeft);
    }
    if (proc.installning().mall != motor::kAntalMallar)
    {
        g.setColour (sodium);
        g.drawText (sprak::trU ("VÄLJ EGEN I MALLMENYN FÖR ATT HÖRA DIN MALL"), tidYta.reduced (12).removeFromTop (18).withTrimmedLeft (220), juce::Justification::centredRight);
    }
}

void StallverketEditor::ritaTrick (juce::Graphics& g)
{
    using namespace ArmColours;
    const juce::String rub[] = { sprak::trU ("I BREAKET"), sprak::trU ("IN I DROPPET"), sprak::trU ("I GROOVET"), sprak::trU ("HUR OFTA") };
    g.setFont (juce::FontOptions (10.0f, juce::Font::bold));
    for (int i = 0; i < 4; ++i) { g.setColour (sodium); g.drawText (rub[i], trickRub[i], juce::Justification::centredLeft); }
    g.setColour (sodium);
    g.drawText (juce::String (juce::roundToInt (trickMangd.getValue() * 100.0)) + " %", trickRub[3], juce::Justification::centredRight);
    g.setColour (muted);
    g.setFont (juce::FontOptions (9.5f));
    auto info = trickRub[3].translated (0, 52).withHeight (60);
    g.drawFittedText (sprak::trU ("Ett trick per break och per uppbyggnad, och ibland ett i groovet. Tänd = får hända."), info, juce::Justification::topLeft, 4);

    g.setFont (juce::FontOptions (10.0f, juce::Font::bold));
    g.setColour (sodium);
    g.drawText (sprak::trU ("SKRIFTEN"), skriftYta, juce::Justification::centredLeft);
    g.setColour (muted);
    g.setFont (juce::FontOptions (9.5f));
    juce::String hjalp = sprak::trU ("I INTRO · G GROOVE · L LYFT · B BREAK · U BYGG · D DROP · K KLIMAX · A UTANDNING · O OUTRO · siffra = takter");
    const auto in = proc.installning();
    if (in.mall != 14) hjalp = sprak::trU ("VÄLJ SKRIFTEN I MALLMENYN") + "   ·   " + hjalp;
    g.drawText (hjalp, skriftYta.withTrimmedLeft (80), juce::Justification::centredRight);
}

void StallverketEditor::ritaKurva (juce::Graphics& g, juce::Rectangle<int> yta)
{
    using namespace ArmColours;
    drawPanel (g, yta, "", {});
    const auto in = proc.installning();
    auto r = kurvYta.toFloat();
    g.setColour (raised.withAlpha (0.6f));
    g.fillRoundedRectangle (r.expanded (8), 6.0f);
    g.setColour (line2);
    for (int i = 0; i <= 4; ++i) { const float y = r.getY() + r.getHeight() * (float) i / 4.0f; g.drawHorizontalLine ((int) y, r.getX(), r.getRight()); }

    // delarna under kurvan, så man ser vad kurvan gör med låten
    const auto plan = motor::byggPlan (in);
    const motor::Overstyr ov;
    const int n = in.kurvTakter;
    for (int64_t t = 0; t < n;)
    {
        int64_t ti = 0;
        const auto d = motor::aktuell (plan, ov, t, ti);
        const int64_t slut = juce::jmin ((int64_t) n, t + (d.langd - ti));
        const float x0 = r.getX() + r.getWidth() * (float) t / (float) n, x1 = r.getX() + r.getWidth() * (float) slut / (float) n;
        g.setColour (delFarg (d.typ).withAlpha (in.kurvaPa ? 0.55f : 0.2f));
        g.fillRect (juce::Rectangle<float> (x0, r.getBottom() - 10.0f, juce::jmax (1.0f, x1 - x0 - 0.5f), 10.0f));
        t = slut;
    }

    juce::Path p;
    for (int i = 0; i <= 200; ++i)
    {
        auto kopia = in; kopia.kurvaPa = true;
        const float v = motor::energiVid (kopia, (int64_t) ((double) i / 200.0 * (n - 1)));
        const float x = r.getX() + r.getWidth() * (float) i / 200.0f, y = r.getBottom() - 14.0f - (r.getHeight() - 14.0f) * v;
        if (i == 0) p.startNewSubPath (x, y); else p.lineTo (x, y);
    }
    g.setColour (in.kurvaPa ? sodium : muted);
    g.strokePath (p, juce::PathStrokeType (2.2f));
    for (int i = 0; i < 8; ++i)
    {
        const float x = r.getX() + r.getWidth() * (float) i / 7.0f, y = r.getBottom() - 14.0f - (r.getHeight() - 14.0f) * in.kurva[i];
        g.setColour (i == dragPunkt ? text : (in.kurvaPa ? sodium : muted));
        g.fillEllipse (juce::Rectangle<float> (12, 12).withCentre ({ x, y }));
    }
    // spelhuvud
    const double takt = proc.slagNu() / juce::jmax (1, proc.slagPerTakt());
    const float xs = r.getX() + r.getWidth() * (float) std::fmod (takt, (double) n) / (float) n;
    g.setColour (text.withAlpha (0.7f));
    g.drawVerticalLine ((int) xs, r.getY(), r.getBottom());
    g.setColour (muted);
    g.setFont (juce::FontOptions (10.0f));
    g.drawText (sprak::trU ("DRA I PUNKTERNA. ENERGIN STYR BREAKS, KLIMAX OCH HUR MYCKET SOM SPELAR."), yta.reduced (12).removeFromTop (18).withTrimmedLeft (220),
                juce::Justification::centredRight);
}

void StallverketEditor::dragKurva (juce::Point<int> pos)
{
    auto r = kurvYta.toFloat();
    if (dragPunkt < 0)
    {
        const int i = juce::jlimit (0, 7, juce::roundToInt ((pos.x - r.getX()) / r.getWidth() * 7.0f));
        dragPunkt = i;
        if (auto* prm = proc.state.getParameter ("k" + juce::String (i + 1))) prm->beginChangeGesture();
    }
    const float v = juce::jlimit (0.0f, 1.0f, (r.getBottom() - 14.0f - (float) pos.y) / (r.getHeight() - 14.0f));
    if (auto* prm = proc.state.getParameter ("k" + juce::String (dragPunkt + 1))) prm->setValueNotifyingHost (v);
    repaint (tidYta);
}

void StallverketEditor::mouseDrag (const juce::MouseEvent& e)
{
    if (centralVy && vy == 1 && dragPunkt >= 0) dragKurva (e.getPosition());
}

void StallverketEditor::mouseUp (const juce::MouseEvent&)
{
    if (dragPunkt >= 0)
        if (auto* prm = proc.state.getParameter ("k" + juce::String (dragPunkt + 1))) prm->endChangeGesture();
    dragPunkt = -1;
}

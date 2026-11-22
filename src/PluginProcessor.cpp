#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Presets.h"
#include "Licens.h"

namespace
{
// ordningen här = platsen i ledningens inst[]-lista
const char* const kInstId[] = { "mall", "fro", "form", "energi", "variation", "brytdjup", "bygglangd",
                                "stok", "filter", "andas", "dub", "djup",
                                "djup1", "djup2", "djup3", "djup4", "djup5", "djup6", "djup7", "djup8",
                                "djup9", "djup10", "djup11", "djup12", "djup13", "djup14", "djup15", "djup16",
                                "pump", "pumptid", "overgang",
                                "kurva", "kurvalangd", "k1", "k2", "k3", "k4", "k5", "k6", "k7", "k8",
                                "e_in", "e_gr", "e_ly", "e_br", "e_by", "e_dr", "e_kl", "e_ut", "e_ou",
                                "e_plyft", "e_pbreak", "e_pbygg", "e_pdrop", "e_pklimax", "e_putand",
                                "e_hard", "e_stok", "e_dub", "e_filter", "e_atmo",
                                "t_frys", "t_halvtid", "t_bak", "t_vinyl", "t_radio", "t_eko", "t_tomrum",
                                "t_stam", "t_glapp", "t_byt", "t_spoke", "t_stopp", "trickmangd" };
constexpr int kAntalInst = 74;
constexpr int kITrick = 61, kITrickMangd = 73;
constexpr int kIKurva = 31, kIEgen = 41;
constexpr int kKurvTakter[] = { 128, 256, 512, 1024, 2048 };
constexpr int kIPump = 28, kIPumpTid = 29, kIOvergang = 30;
constexpr int kRollOrord = motor::NUM_ROLL, kRollAuto = motor::NUM_ROLL + 1;
static_assert (kAntalInst <= ledning::kInst, "för många inställningar");

juce::StringArray rollLista()
{
    juce::StringArray s;
    for (int i = 0; i < motor::NUM_ROLL; ++i) s.add (motor::rollNamn (i));
    s.add (juce::String::fromUTF8 ("ORÖRD"));
    s.add ("AUTO");
    return s;
}
} // namespace

int64_t StallverketProcessor::nuMs() { return (int64_t) juce::Time::getMillisecondCounter(); }

juce::AudioProcessorValueTreeState::ParameterLayout StallverketProcessor::skapaParametrar()
{
    using namespace juce;
    std::vector<std::unique_ptr<RangedAudioParameter>> p;
    auto proc = [] (float v, int) { return String (juce::roundToInt (v * 100.0f)) + " %"; };
    auto knapp = [&] (const char* id, const char* namn, float def)
    {
        p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, String::fromUTF8 (namn),
                                                             NormalisableRange<float> (0.0f, 1.0f), def,
                                                             AudioParameterFloatAttributes().withStringFromValueFunction (proc)));
    };

    p.push_back (std::make_unique<AudioParameterBool> (ParameterID { "central", 1 }, "Central", false));
    StringArray kanaler; for (int i = 1; i <= ledning::kKanaler; ++i) kanaler.add (String (i));
    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { "kanal", 1 }, "Kanal", kanaler, 0));
    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { "roll", 1 }, "Roll", rollLista(), kRollAuto));

    StringArray mallar; for (const auto& m : motor::kMallar) mallar.add (String::fromUTF8 (m.namn));
    mallar.add ("EGEN");
    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { "mall", 1 }, "Mall", mallar, 0));
    p.push_back (std::make_unique<AudioParameterInt> (ParameterID { "fro", 1 }, String::fromUTF8 ("Frö"), 1, 9999, 1));
    StringArray former; for (auto* f : motor::kFormNamn) former.add (f);
    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { "form", 1 }, "Form", former, 0));

    knapp ("energi",    "Energi",    0.55f);
    knapp ("variation", "Variation", 0.5f);
    knapp ("brytdjup",  "Brytdjup",  0.6f);
    knapp ("bygglangd", "Bygglängd", 0.5f);
    knapp ("stok",      "Stök",      0.35f);
    knapp ("filter",    "Filter",    0.6f);
    knapp ("andas",     "Andas",     0.4f);
    knapp ("dub",       "Dubkast",   0.2f);
    knapp ("djup",      "Djup",      1.0f);
    knapp ("pump",      "Pump",      0.35f);
    knapp ("pumptid",   "Pumptid",   0.4f);
    knapp ("overgang",  "Övergång",  0.5f);

    // energikurva
    p.push_back (std::make_unique<AudioParameterBool> (ParameterID { "kurva", 1 }, "Kurva", false));
    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { "kurvalangd", 1 }, String::fromUTF8 ("Kurvlängd"),
                                                          StringArray { "128", "256", "512", "1024", "2048" }, 2));
    const float kurvDef[] = { 0.3f, 0.45f, 0.6f, 0.7f, 0.85f, 1.0f, 0.7f, 0.4f };
    for (int i = 0; i < 8; ++i)
        knapp (("k" + String (i + 1)).toRawUTF8(), ("Kurva " + String (i + 1)).toRawUTF8(), kurvDef[i]);

    // EGEN mall (börjar som KLASSISK)
    const auto& kl = motor::kMallar[0];
    const char* langdId[] = { "e_in", "e_gr", "e_ly", "e_br", "e_by", "e_dr", "e_kl", "e_ut", "e_ou" };
    for (int d = 0; d < motor::NUM_DEL; ++d)
    {
        int idx = 0; while (idx < 4 && (4 << idx) < kl.langd[d]) ++idx;
        p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { langdId[d], 1 }, String ("Egen ") + motor::delNamn (d),
                                                              StringArray { "4", "8", "16", "32", "64" }, idx));
    }
    knapp ("e_plyft", "Egen lyft", kl.pLyft);     knapp ("e_pbreak", "Egen break", kl.pBreak);
    knapp ("e_pbygg", "Egen bygg", kl.pByggDirekt); knapp ("e_pdrop", "Egen drop", kl.pDropDirekt);
    knapp ("e_pklimax", "Egen klimax", kl.pKlimax); knapp ("e_putand", "Egen utandning", kl.pUtandning);
    knapp ("e_hard", "Egen hårdhet", kl.hard);     knapp ("e_stok", "Egen stök", kl.stok);
    knapp ("e_dub", "Egen dub", kl.dub);            knapp ("e_filter", "Egen filter", kl.filter);
    p.push_back (std::make_unique<AudioParameterBool> (ParameterID { "e_atmo", 1 }, "Egen atmo först", false));

    // TRICK: på/av per effekt + hur ofta
    const char* trickId[] = { "t_frys", "t_halvtid", "t_bak", "t_vinyl", "t_radio", "t_eko", "t_tomrum",
                              "t_stam", "t_glapp", "t_byt", "t_spoke", "t_stopp" };
    const char* trickNamn[] = { "Frys", "Halvtid", "Baklänges-break", "Vinylbroms", "Radiobyte", "Kort eko", "Tomrum",
                                "Stamning", "Bandglapp", "Kanalbyte", "Spökeko", "Stopp-slag" };
    for (int i = 0; i < 12; ++i)
        p.push_back (std::make_unique<AudioParameterBool> (ParameterID { trickId[i], 1 }, String::fromUTF8 (trickNamn[i]), true));
    knapp ("trickmangd", "Hur ofta", 0.5f);
    // knappar som går att mappa till en controller (reagerar när de slås på)
    p.push_back (std::make_unique<AudioParameterBool> (ParameterID { "breaknu", 1 }, "Break nu", false));
    p.push_back (std::make_unique<AudioParameterBool> (ParameterID { "byggnu", 1 },  "Bygg nu", false));
    p.push_back (std::make_unique<AudioParameterBool> (ParameterID { "dropnu", 1 },  "Drop nu", false));
    p.push_back (std::make_unique<AudioParameterBool> (ParameterID { "hall", 1 },    String::fromUTF8 ("Håll"), false));
    p.push_back (std::make_unique<AudioParameterBool> (ParameterID { "slapp", 1 },   String::fromUTF8 ("Släpp"), false));
    p.push_back (std::make_unique<AudioParameterBool> (ParameterID { "midiut", 1 },  "MIDI ut", true));
    for (int i = 1; i <= ledning::kKanaler; ++i)
    {
        const auto id = "djup" + String (i);
        p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, "Djup " + String (i),
                                                             NormalisableRange<float> (0.0f, 1.0f), 1.0f,
                                                             AudioParameterFloatAttributes().withStringFromValueFunction (proc)));
    }
    return { p.begin(), p.end() };
}

StallverketProcessor::StallverketProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      state (*this, nullptr, "STALLVERKET", skapaParametrar())
{
    minId = (uint32_t) juce::Random::getSystemRandom().nextInt (0x7fffffff) | 1u;
    sattSkrift ("I16 G32 L8 B16 U8 D16 K32 A16 B8 U8 D16 G32 O16");
    lasInstallning (cache);
   #if STALLVERKET_TEST_UNLOCKED
    licens::upplast().store (true);   // bara testprogrammet, aldrig det som säljs
   #else
    licens::kontrollera();
   #endif
}

StallverketProcessor::~StallverketProcessor()
{
    auto& n = ledning::nav();
    uint32_t id = minId;
    n.centralId.compare_exchange_strong (id, 0u);
}

bool StallverketProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::stereo() && out != juce::AudioChannelSet::mono()) return false;
    return layouts.getMainInputChannelSet() == out;
}

float StallverketProcessor::las (const char* id) const
{
    if (auto* v = state.getRawParameterValue (id)) return v->load();
    return 0.0f;
}

void StallverketProcessor::satt (const juce::String& id, float verkligt)
{
    if (auto* p = state.getParameter (id))
        p->setValueNotifyingHost (p->convertTo0to1 (verkligt));
}

void StallverketProcessor::lasInstallning (float* ut) const
{
    for (int i = 0; i < kAntalInst; ++i) ut[i] = las (kInstId[i]);
}

motor::Installning StallverketProcessor::fran (const float* v) const
{
    motor::Installning in;
    in.mall      = juce::jlimit (0, motor::kAntalMallar, (int) std::lround (v[0]));
    in.fro       = (uint32_t) juce::jmax (1, (int) std::lround (v[1]));
    in.form      = juce::jlimit (0, motor::kAntalFormer - 1, (int) std::lround (v[2]));
    in.energi    = v[3]; in.variation = v[4]; in.brytDjup = v[5]; in.byggLangd = v[6];
    in.stok      = v[7]; in.filter = v[8]; in.andas = v[9]; in.dub = v[10]; in.overgang = v[kIOvergang];

    in.kurvaPa = v[kIKurva] > 0.5f;
    in.kurvTakter = kKurvTakter[juce::jlimit (0, 4, (int) std::lround (v[kIKurva + 1]))];
    for (int i = 0; i < 8; ++i) in.kurva[i] = juce::jlimit (0.0f, 1.0f, v[kIKurva + 2 + i]);

    auto& e = in.egen;
    e.namn = "EGEN";
    for (int d = 0; d < motor::NUM_DEL; ++d) e.langd[d] = (uint8_t) (4 << juce::jlimit (0, 4, (int) std::lround (v[kIEgen + d])));
    const float* q = v + kIEgen + motor::NUM_DEL;
    e.pLyft = q[0]; e.pBreak = q[1]; e.pByggDirekt = q[2]; e.pDropDirekt = q[3]; e.pKlimax = q[4]; e.pUtandning = q[5];
    e.hard = q[6]; e.stok = q[7]; e.dub = q[8]; e.filter = q[9]; e.atmoForst = q[10] > 0.5f;

    in.trick = 0;
    for (int i = 0; i < motor::kAntalTrick; ++i) if (v[kITrick + i] > 0.5f) in.trick |= 1u << i;
    in.trickMangd = v[kITrickMangd];

    // SKRIFTEN: CENTRAL läser sin egen text, kanalerna läser den CENTRAL skickat
    auto& nav = ledning::nav();
    const bool egen = arCentral() || ! centralLever();
    in.skriftN = egen ? skriftAntal.load() : nav.skriftN.load();
    in.skriftN = juce::jlimit (0, 32, in.skriftN);
    for (int i = 0; i < in.skriftN; ++i) in.skrift[i] = egen ? skriftKod[i].load() : nav.skrift[i].load();

    uint32_t h = 0x5eed;
    for (int i = 0; i < in.skriftN; ++i) h = motor::blanda (h, (uint32_t) in.skrift[i]);
    for (int i = kIKurva; i < kAntalInst; ++i) { uint32_t b; std::memcpy (&b, &v[i], 4); h = motor::blanda (h, b); }
    in.extra = h;
    return in;
}

bool StallverketProcessor::arCentral() const { return las ("central") > 0.5f; }
int  StallverketProcessor::kanal() const     { return juce::jlimit (0, ledning::kKanaler - 1, (int) las ("kanal")); }
int  StallverketProcessor::rollVal() const   { return juce::jlimit (0, kRollAuto, (int) las ("roll")); }
int  StallverketProcessor::roll() const
{
    const int r = rollVal();
    if (r != kRollAuto) return r;
    const int g = autoRoll.load();
    return g >= 0 ? g : motor::LOOP;   // har inte hört nog än: behandlas som LOOP så länge
}

bool StallverketProcessor::centralLever() const
{
    return ledning::levande (ledning::nav().centralSenast.load(), nuMs());
}

motor::Installning StallverketProcessor::installning() const
{
    float v[ledning::kInst] {};
    if (arCentral() || ! centralLever())
    {
        if (arCentral() || ! harCache) lasInstallning (v);
        else std::copy (std::begin (cache), std::end (cache), v);
    }
    else
        for (int i = 0; i < kAntalInst; ++i) v[i] = ledning::nav().inst[i].load();
    return fran (v);
}

motor::Overstyr StallverketProcessor::overstyr() const
{
    auto& n = ledning::nav();
    motor::Overstyr o;
    if (! arCentral() && ! centralLever()) return o;
    o.typ = n.ovTyp.load(); o.start = n.ovStart.load(); o.langd = n.ovLangd.load();
    if (o.typ == motor::BYGG) { o.typ2 = motor::DROP; o.langd2 = 16; }
    o.hall = n.hall.load(); o.hallStart = n.hallStart.load(); o.hallLangd = n.hallLangd.load();
    o.hallTyp = n.hallTyp.load(); o.hallH = n.hallH.load();
    return o;
}

//==============================================================================
void StallverketProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sr = sampleRate;
    monoBuf.assign ((size_t) juce::jmax (4096, samplesPerBlock * 2), 0.0f);
    fack.forbered (sampleRate);
    lyssnare.forbered (sampleRate);

    // Ny instans (inte laddad från ett projekt): välj själv plats.
    // Finns ingen CENTRAL blir den här CENTRAL, annars tar den första lediga kanalen.
    if (! tillstandLaddat && ! autoGjord)
    {
        autoGjord = true;
        auto& n = ledning::nav();
        const auto nu = nuMs();
        if (! ledning::levande (n.centralSenast.load(), nu, 4000) && n.centralId.load() == 0)
        {
            satt ("central", 1.0f);
            n.centralId.store (minId);
            n.centralSenast.store (nu);
        }
        for (int k = 0; k < ledning::kKanaler; ++k)
            if (! ledning::levande (n.kanal[k].senast.load(), nu, 4000))
            {
                satt ("kanal", (float) k);
                n.kanal[k].senast.store (nu);   // reservera
                break;
            }
    }
}

void StallverketProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    midi.clear();
    const int n = buffer.getNumSamples();
    for (int c = getTotalNumInputChannels(); c < getTotalNumOutputChannels(); ++c) buffer.clear (c, 0, n);
    if (n == 0) return;

    // utan giltig nyckel: ljudet går rakt igenom och inget styrs
    if (! licens::upplast().load (std::memory_order_relaxed)) return;

    auto& nav = ledning::nav();
    const auto nu = nuMs();
    const bool central = arCentral();

    // ---- CENTRAL skickar ut inställningarna
    if (central)
    {
        uint32_t id = nav.centralId.load();
        const bool fri = id == 0 || id == minId || ! ledning::levande (nav.centralSenast.load(), nu);
        if (fri)
        {
            nav.centralId.store (minId);
            float v[ledning::kInst] {};
            lasInstallning (v);
            for (int i = 0; i < kAntalInst; ++i) nav.inst[i].store (v[i]);
            const int sn = skriftAntal.load();
            for (int i = 0; i < sn; ++i) nav.skrift[i].store (skriftKod[i].load());
            nav.skriftN.store (sn);
            nav.centralSenast.store (nu);
            annanCentral.store (false);
        }
        else annanCentral.store (true);
    }
    else
    {
        annanCentral.store (false);
        if (ledning::levande (nav.centralSenast.load(), nu))
        {
            for (int i = 0; i < kAntalInst; ++i) cache[i] = nav.inst[i].load();
            harCache = true;
        }
    }

    // ---- tid från DAW:en
    double ppq = 0.0, bpm = 120.0;
    bool play = false;
    int taktTaljare = 4, taktNamnare = 4;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            play = pos->getIsPlaying();
            if (auto b = pos->getBpm()) bpm = *b;
            if (auto q = pos->getPpqPosition()) ppq = *q;
            if (auto ts = pos->getTimeSignature()) { taktTaljare = ts->numerator; taktNamnare = ts->denominator; }
        }
    const int slagPerTakt = juce::jmax (1, (int) std::lround (taktTaljare * 4.0 / juce::jmax (1, taktNamnare)));
    const double slagPerSampel = bpm / 60.0 / sr;

    // ---- löpande klocka: när DAW:en loopar (hoppar bakåt) räknar motorn vidare,
    // annars skulle låten fastna i samma fyra takter. Alla instanser får samma
    // hopp i samma block, så alla räknar exakt lika utan att prata med varandra.
    if (play)
    {
        if (! spelade) klocka = ppq;                                   // play: börja där DAW:en står
        else if (ppq > forvantat + 0.5) klocka += ppq - forvantat;     // hopp framåt: följ med
        // hopp bakåt (loop): ignorera, fortsätt räkna
        forvantat = ppq + n * slagPerSampel;
        ppq = klocka;
        klocka += n * slagPerSampel;
    }
    spelade = play;

    slag.store (ppq); spelar.store (play); spt.store (slagPerTakt);
    if (central) nav.slag.store (ppq);

    // ---- inställningar och plan
    float v[ledning::kInst] {};
    if (central || ! ledning::levande (nav.centralSenast.load(), nu))
    {
        if (central || ! harCache) lasInstallning (v);
        else std::copy (std::begin (cache), std::end (cache), v);
    }
    else std::copy (std::begin (cache), std::end (cache), v);

    // ---- knappar från controller (bara CENTRAL): reagerar när de slås på
    if (central)
    {
        const char* ids[] = { "breaknu", "byggnu", "dropnu", "hall", "slapp" };
        for (int i = 0; i < 5; ++i)
        {
            const bool pa = las (ids[i]) > 0.5f;
            if (pa && ! knappFore[i])
            {
                if (i == 0) taOver (motor::BREAK);
                else if (i == 1) taOver (motor::BYGG);
                else if (i == 2) taOver (motor::DROP);
                else if (i == 3) vaxlaHall();
                else { slappTaOver(); if (haller()) vaxlaHall(); }
            }
            knappFore[i] = pa;
        }
    }

    auto in = fran (v);
    for (int i = 0; i < ledning::kKanaler; ++i)
        if (ledning::levande (nav.kanal[i].senast.load(), nu) && ledning::levande (nav.kanal[i].ljud.load(), nu, 3000)) in.levande |= 1u << i;
    if (! harPlan || in != planFor) { plan = motor::byggPlan (in); planFor = in; harPlan = true; }
    const auto ov = overstyr();

    // ---- MIDI ut från CENTRAL när en ny del börjar:
    //  not 36 + del (C1 INTRO, C#1 GROOVE, D1 LYFT, D#1 BREAK, E1 BYGG, F1 DROP, F#1 KLIMAX, G1 UTANDNING, G#1 OUTRO)
    //  CC 20 = delens nummer, CC 21 = energi. Kanal 16.
    if (central && las ("midiut") > 0.5f)
    {
        if (play)
        {
            const double slutSlag = ppq + n * slagPerSampel;
            for (int64_t t = (int64_t) std::ceil (ppq / slagPerTakt - 1e-9); (double) t * slagPerTakt < slutSlag; ++t)
            {
                int64_t ti = 0;
                const auto d = motor::aktuell (plan, ov, t, ti);
                const int id = (int) (d.start * 16 + d.typ);
                if (ti == 0 || ! midiStartat)
                {
                    if (id != senasteDelId || ! midiStartat)
                    {
                        const int pos = juce::jlimit (0, n - 1, (int) (((double) t * slagPerTakt - ppq) / slagPerSampel));
                        if (midiNot >= 0) midi.addEvent (juce::MidiMessage::noteOff (16, midiNot), pos);
                        midiNot = 36 + d.typ;
                        midi.addEvent (juce::MidiMessage::noteOn (16, midiNot, (juce::uint8) 100), pos);
                        midi.addEvent (juce::MidiMessage::controllerEvent (16, 20, d.typ), pos);
                        midi.addEvent (juce::MidiMessage::controllerEvent (16, 21, juce::jlimit (0, 127, (int) (in.energi * 127.0f))), pos);
                        senasteDelId = id;
                        midiStartat = true;
                        notAv = (int64_t) (sr * 0.1);
                    }
                }
            }
            if (midiNot >= 0 && notAv > 0)
            {
                notAv -= n;
                if (notAv <= 0) { midi.addEvent (juce::MidiMessage::noteOff (16, midiNot), n - 1); midiNot = -1; }
            }
        }
        else if (midiStartat)
        {
            if (midiNot >= 0) midi.addEvent (juce::MidiMessage::noteOff (16, midiNot), 0);
            midiNot = -1; midiStartat = false; senasteDelId = -1;
        }
    }

    const int k = kanal();
    const int r = roll();
    const float djup = juce::jlimit (0.0f, 1.0f, v[11] * v[12 + k]);

    auto* L = buffer.getWritePointer (0);
    auto* R = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : buffer.getWritePointer (0);
    const bool mono = buffer.getNumChannels() < 2;
    if (mono)
    {
        if ((int) monoBuf.size() < n) return;   // ska inte hända: blocket är större än DAW:en lovade
        std::copy (L, L + n, monoBuf.data());
        R = monoBuf.data();
    }

    // ---- lyssna: finns det ljud, och vilken roll låter det som?
    float inTopp = 0.0f;
    for (int i = 0; i < n; ++i) inTopp = juce::jmax (inTopp, std::abs (L[i]), std::abs (R[i]));
    if (inTopp > 0.0005f) nav.kanal[k].ljud.store (nu);
    if (play && rollVal() == kRollAuto)
    {
        for (int i = 0; i < n; ++i) lyssnare.sampla (0.5f * (L[i] + R[i]), ppq + i * slagPerSampel);
        autoRoll.store (lyssnare.roll());
    }

    const bool styr = play && r < motor::NUM_ROLL;
    // ---- pump: hur mycket varje roll trycks ner av kicken
    static const float pumpRoll[] = { 0.0f, 1.0f, 0.25f, 0.35f, 0.8f, 0.7f, 0.5f, 0.55f, 0.3f, 0.2f, 0.6f };
    static_assert (sizeof (pumpRoll) / sizeof (float) == motor::NUM_ROLL, "pump per roll");
    int kickKanal = -1;
    for (int i = 0; i < ledning::kKanaler; ++i)
        if (nav.kanal[i].roll.load() == motor::KICK && ledning::levande (nav.kanal[i].senast.load(), nu)
            && ledning::levande (nav.kanal[i].ljud.load(), nu, 3000)) { kickKanal = i; break; }
    if (r == motor::KICK) kickKanal = k;
    const float pump = r < motor::NUM_ROLL ? v[kIPump] * pumpRoll[r] : 0.0f;
    const float pumpTid = 0.12f + v[kIPumpTid] * 0.7f;
    const float kickDjup = kickKanal >= 0 ? juce::jlimit (0.0f, 1.0f, v[11] * v[12 + kickKanal]) : 0.0f;

    fack.kor (L, R, n, ppq, slagPerSampel, styr, djup, 1.0f,
              [&] (double s)
              {
                  auto a = motor::atgard (plan, in, ov, k, r, s, slagPerTakt);
                  if (pump > 0.0f)
                  {
                      // pumpar bara när kicken faktiskt spelar enligt motorn
                      float kickPa = 1.0f;
                      if (kickKanal >= 0)
                      {
                          const auto ka = motor::atgard (plan, in, ov, kickKanal, motor::KICK, std::floor (s) + 0.01, slagPerTakt);
                          const int steg = (int) std::fmod (std::floor (std::floor (s) * 4.0), 16.0);
                          const float pa = ka.niva * (((ka.grind >> steg) & 1u) ? 1.0f : 0.0f) * (1.0f - ka.hp);
                          kickPa = 1.0f + (pa - 1.0f) * kickDjup;
                      }
                      a.pump = pump * kickPa;
                      a.pumpTid = pumpTid;
                  }
                  return a;
              });

    // ---- rapportera till CENTRAL
    auto& kn = nav.kanal[k];
    kn.roll.store (r);
    kn.niva.store (fack.nivaMatare());
    kn.senast.store (nu);
    int64_t tid = 0;
    kn.del.store (play ? motor::aktuell (plan, ov, (int64_t) std::floor (ppq / slagPerTakt), tid).typ : -1);

    matarNiva.store (fack.nivaMatare());
    gainVisning.store (fack.aktuellGain());
    lpVisning.store (fack.aktuellLp());
    hpVisning.store (fack.aktuellHp());
}

//==============================================================================
void StallverketProcessor::taOver (int typ)
{
    auto& n = ledning::nav();
    const double s = slag.load();
    const int spt_ = spt.load();
    const int64_t nastaTakt = (int64_t) std::floor (s / spt_) + 1;
    const auto in = installning();
    int langd = 16;
    if (typ == motor::BYGG)
        langd = motor::tvapotens (8.0f * std::pow (2.0f, (in.byggLangd - 0.5f) * 2.0f), 4, 32);
    if (typ == motor::BREAK) langd = in.mall == 2 || in.mall == 5 ? 8 : 16;
    n.ovStart.store (nastaTakt);
    n.ovLangd.store (langd);
    n.ovTyp.store (typ);
}

void StallverketProcessor::slappTaOver() { ledning::nav().ovTyp.store (-1); }

bool StallverketProcessor::haller() const { return ledning::nav().hall.load(); }

void StallverketProcessor::vaxlaHall()
{
    auto& n = ledning::nav();
    if (n.hall.load()) { n.hall.store (false); return; }
    const auto in = installning();
    const auto p = motor::byggPlan (in);
    const auto ov = overstyr();
    int64_t tid = 0;
    const int64_t takt = (int64_t) std::floor (slag.load() / spt.load());
    const auto s = motor::aktuell (p, ov, takt, tid);
    n.hallStart.store (takt - tid);
    n.hallLangd.store (s.langd);
    n.hallTyp.store (s.typ);
    n.hallH.store (s.h);
    n.hall.store (true);
}

void StallverketProcessor::slumpaFro()
{
    satt ("fro", (float) juce::Random::getSystemRandom().nextInt ({ 1, 10000 }));
}

void StallverketProcessor::slumpaAllt()
{
    auto& rnd = juce::Random::getSystemRandom();
    satt ("mall", (float) rnd.nextInt (motor::kAntalMallar));
    slumpaFro();
    for (auto* id : { "energi", "variation", "brytdjup", "bygglangd", "stok", "filter", "andas", "dub" })
        satt (id, 0.1f + 0.8f * rnd.nextFloat());
    aktivtPresetNamn = juce::String::fromUTF8 ("Slumpad");
}

//==============================================================================
int StallverketProcessor::antalFabrik() const { return presets::kAntal; }
int StallverketProcessor::getNumPrograms() { return presets::kAntal; }
const juce::String StallverketProcessor::getProgramName (int i)
{
    return juce::isPositiveAndBelow (i, presets::kAntal) ? juce::String::fromUTF8 (presets::kFabrik[i].namn) : juce::String();
}
void StallverketProcessor::setCurrentProgram (int i) { laddaFabrik (i); }

void StallverketProcessor::laddaFabrik (int i)
{
    if (! juce::isPositiveAndBelow (i, presets::kAntal)) return;
    const auto& f = presets::kFabrik[i];
    satt ("mall", (float) f.mall); satt ("form", (float) f.form); satt ("fro", (float) f.fro);
    satt ("energi", f.energi); satt ("variation", f.variation); satt ("brytdjup", f.brytDjup);
    satt ("bygglangd", f.byggLangd); satt ("stok", f.stok); satt ("filter", f.filter);
    satt ("andas", f.andas); satt ("dub", f.dub);
    currentPreset = i;
    aktivtPresetNamn = juce::String::fromUTF8 (f.namn);
}

juce::File StallverketProcessor::presetMapp() const
{
    return juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
        .getChildFile ("Phunkshunz").getChildFile ("STALLVERKET").getChildFile ("Presets");
}

juce::Array<juce::File> StallverketProcessor::egnaPresets() const
{
    auto filer = presetMapp().findChildFiles (juce::File::findFiles, true, "*.stallverket");
    filer.sort();
    return filer;
}

bool StallverketProcessor::sparaPreset (const juce::String& namn)
{
    auto mapp = presetMapp();
    mapp.createDirectory();
    juce::XmlElement x ("STALLVERKET_PRESET");
    x.setAttribute ("namn", namn);
    x.setAttribute ("version", 1);
    for (int i = 0; i < 11; ++i) x.setAttribute (kInstId[i], las (kInstId[i]));
    for (int i = kIOvergang; i < kAntalInst; ++i) x.setAttribute (kInstId[i], las (kInstId[i]));
    x.setAttribute ("skrift", skriftText);
    const auto fil = mapp.getChildFile (juce::File::createLegalFileName (namn) + ".stallverket");
    aktivtPresetNamn = namn;
    return x.writeTo (fil);
}

bool StallverketProcessor::laddaPresetFil (const juce::File& f)
{
    auto x = juce::XmlDocument::parse (f);
    if (x == nullptr || ! x->hasTagName ("STALLVERKET_PRESET")) return false;
    for (int i = 0; i < 11; ++i)
        if (x->hasAttribute (kInstId[i])) satt (kInstId[i], (float) x->getDoubleAttribute (kInstId[i]));
    for (int i = kIOvergang; i < kAntalInst; ++i)
        if (x->hasAttribute (kInstId[i])) satt (kInstId[i], (float) x->getDoubleAttribute (kInstId[i]));
    if (x->hasAttribute ("skrift")) sattSkrift (x->getStringAttribute ("skrift"));
    aktivtPresetNamn = x->getStringAttribute ("namn", f.getFileNameWithoutExtension());
    return true;
}

//==============================================================================
void StallverketProcessor::sattSkrift (const juce::String& txt)
{
    skriftText = txt;
    int kod[32] {};
    const int n = motor::lasSkrift (txt.toRawUTF8(), kod, 32);
    for (int i = 0; i < n; ++i) skriftKod[i].store (kod[i]);
    skriftAntal.store (n);
}

//==============================================================================
// Mallar: kopiera en inbyggd mall till EGEN, spara och ladda .stallmall
void StallverketProcessor::kopieraMallTillEgen()
{
    const auto in = installning();
    const auto& m = motor::mallFor (in);
    for (int d = 0; d < motor::NUM_DEL; ++d)
    {
        int idx = 0; while (idx < 4 && (4 << idx) < m.langd[d]) ++idx;
        satt (kInstId[kIEgen + d], (float) idx);
    }
    const float q[] = { m.pLyft, m.pBreak, m.pByggDirekt, m.pDropDirekt, m.pKlimax, m.pUtandning, m.hard, m.stok, m.dub, m.filter, m.atmoForst ? 1.0f : 0.0f };
    for (int i = 0; i < 11; ++i) satt (kInstId[kIEgen + motor::NUM_DEL + i], q[i]);
    satt ("mall", (float) motor::kAntalMallar);
}

juce::File StallverketProcessor::mallMapp() const { return presetMapp().getParentDirectory().getChildFile ("Mallar"); }

juce::Array<juce::File> StallverketProcessor::egnaMallar() const
{
    auto f = mallMapp().findChildFiles (juce::File::findFiles, true, "*.stallmall");
    f.sort();
    return f;
}

bool StallverketProcessor::sparaMall (const juce::String& namn)
{
    auto mapp = mallMapp();
    mapp.createDirectory();
    juce::XmlElement x ("STALLVERKET_MALL");
    x.setAttribute ("namn", namn);
    x.setAttribute ("version", 1);
    for (int i = kIEgen; i < kAntalInst; ++i) x.setAttribute (kInstId[i], las (kInstId[i]));
    return x.writeTo (mapp.getChildFile (juce::File::createLegalFileName (namn) + ".stallmall"));
}

bool StallverketProcessor::laddaMall (const juce::File& f)
{
    auto x = juce::XmlDocument::parse (f);
    if (x == nullptr || ! x->hasTagName ("STALLVERKET_MALL")) return false;
    for (int i = kIEgen; i < kAntalInst; ++i)
        if (x->hasAttribute (kInstId[i])) satt (kInstId[i], (float) x->getDoubleAttribute (kInstId[i]));
    satt ("mall", (float) motor::kAntalMallar);
    return true;
}

//==============================================================================
// Spara låten som MIDI-fil: ett spår med delarna (markörer + noter) och ett spår per kanal
// med noter när kanalen spelar och automation: CC 11 nivå, CC 74 lågpass, CC 75 högpass.
juce::File StallverketProcessor::exporteraMidi()
{
    const auto in = installning();
    const auto plan = motor::byggPlan (in);
    const motor::Overstyr ov;   // exporten följer planen, inte knapptryck
    const int spt = juce::jmax (1, slagPerTakt());
    const int takter = in.form > 0 ? (int) plan.takter : 192;
    constexpr int tpq = 960;

    juce::MidiFile fil;
    fil.setTicksPerQuarterNote (tpq);

    // tempo och takt
    juce::MidiMessageSequence delar;
    delar.addEvent (juce::MidiMessage::textMetaEvent (3, "STALLVERKET delar"), 0);
    delar.addEvent (juce::MidiMessage::timeSignatureMetaEvent (spt, 4), 0);
    for (int64_t t = 0; t < takter;)
    {
        int64_t ti = 0;
        const auto d = motor::aktuell (plan, ov, t, ti);
        const int64_t slut = juce::jmin ((int64_t) takter, t + (d.langd - ti));
        const double start = (double) t * spt * tpq, stopp = (double) slut * spt * tpq;
        delar.addEvent (juce::MidiMessage::textMetaEvent (6, motor::delNamn (d.typ)), start);
        delar.addEvent (juce::MidiMessage::noteOn (16, 36 + d.typ, (juce::uint8) 100), start);
        delar.addEvent (juce::MidiMessage::noteOff (16, 36 + d.typ), stopp - 1);
        t = slut;
    }
    delar.updateMatchedPairs();
    fil.addTrack (delar);

    // vilka kanaler: de som är inkopplade, annars en per roll
    struct Spar { int kanal, roll; };
    juce::Array<Spar> spar;
    auto& nav = ledning::nav();
    const auto nu = nuMs();
    for (int k = 0; k < ledning::kKanaler; ++k)
        if (ledning::levande (nav.kanal[k].senast.load(), nu) && nav.kanal[k].roll.load() >= 0 && nav.kanal[k].roll.load() < motor::NUM_ROLL)
            spar.add ({ k, nav.kanal[k].roll.load() });
    if (spar.isEmpty())
        for (int r = 0; r < motor::NUM_ROLL; ++r) spar.add ({ r, r });

    for (const auto& s : spar)
    {
        juce::MidiMessageSequence seq;
        const int ch = (s.kanal % 15) + 1;
        seq.addEvent (juce::MidiMessage::textMetaEvent (3, juce::String (s.kanal + 1) + " " + motor::rollNamn (s.roll)), 0);
        int fNiva = -1, fLp = -1, fHp = -1;
        bool notPa = false;
        const int steg = takter * spt * 4;   // 16-delar
        for (int i = 0; i <= steg; ++i)
        {
            const double slag = i * 0.25;
            const double tick = slag * tpq;
            motor::Atgard a;
            if (i < steg) a = motor::atgard (plan, in, ov, s.kanal, s.roll, slag + 0.01, spt);
            const bool grind = ((a.grind >> (i % 16)) & 1u) != 0;
            const float niva = i < steg ? a.niva * (grind ? 1.0f : 0.0f) : 0.0f;
            const int cN = juce::roundToInt (niva * 127.0f), cL = juce::roundToInt (a.lp * 127.0f), cH = juce::roundToInt (a.hp * 127.0f);
            if (cN != fNiva) { seq.addEvent (juce::MidiMessage::controllerEvent (ch, 11, cN), tick); fNiva = cN; }
            if (cL != fLp)   { seq.addEvent (juce::MidiMessage::controllerEvent (ch, 74, cL), tick); fLp = cL; }
            if (cH != fHp)   { seq.addEvent (juce::MidiMessage::controllerEvent (ch, 75, cH), tick); fHp = cH; }
            const bool pa = niva > 0.3f;
            if (pa != notPa)
            {
                if (pa) seq.addEvent (juce::MidiMessage::noteOn (ch, 60, (juce::uint8) juce::jlimit (1, 127, cN)), tick);
                else    seq.addEvent (juce::MidiMessage::noteOff (ch, 60), tick);
                notPa = pa;
            }
        }
        seq.updateMatchedPairs();
        fil.addTrack (seq);
    }

    auto mapp = presetMapp().getParentDirectory().getChildFile ("Export");
    mapp.createDirectory();
    const auto mallNamn = juce::String::fromUTF8 (motor::mallFor (in).namn).replace (juce::String::fromUTF8 ("Ö"), "O");
    auto ut = mapp.getNonexistentChildFile ("STALLVERKET-" + mallNamn + "-" + juce::String (in.fro), ".mid", false);
    juce::FileOutputStream os (ut);
    if (! os.openedOk() || ! fil.writeTo (os, 1)) return {};
    return ut;
}

//==============================================================================
void StallverketProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto s = state.copyState();
    auto c = s.getOrCreateChildWithName ("CACHE", nullptr);
    for (int i = 0; i < kAntalInst; ++i) c.setProperty (kInstId[i], cache[i], nullptr);
    c.setProperty ("har", harCache, nullptr);
    s.setProperty ("preset", aktivtPresetNamn, nullptr);
    s.setProperty ("skrift", skriftText, nullptr);
    if (auto xml = s.createXml()) copyXmlToBinary (*xml, destData);
}

void StallverketProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary (data, sizeInBytes);
    if (xml == nullptr || ! xml->hasTagName (state.state.getType())) return;
    auto s = juce::ValueTree::fromXml (*xml);
    if (auto c = s.getChildWithName ("CACHE"); c.isValid())
    {
        for (int i = 0; i < kAntalInst; ++i) cache[i] = (float) c.getProperty (kInstId[i], cache[i]);
        harCache = (bool) c.getProperty ("har", false);
        s.removeChild (c, nullptr);
    }
    aktivtPresetNamn = s.getProperty ("preset", "").toString();
    sattSkrift (s.getProperty ("skrift", skriftText).toString());
    s.removeProperty ("skrift", nullptr);
    state.replaceState (s);
    tillstandLaddat = true;
}

juce::AudioProcessorEditor* StallverketProcessor::createEditor() { return new StallverketEditor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new StallverketProcessor(); }

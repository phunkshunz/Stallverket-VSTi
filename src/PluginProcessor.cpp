#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Presets.h"

namespace
{
// ordningen här = platsen i ledningens inst[]-lista
const char* const kInstId[] = { "mall", "fro", "form", "energi", "variation", "brytdjup", "bygglangd",
                                "stok", "filter", "andas", "dub", "djup",
                                "djup1", "djup2", "djup3", "djup4", "djup5", "djup6", "djup7", "djup8",
                                "djup9", "djup10", "djup11", "djup12", "djup13", "djup14", "djup15", "djup16",
                                "pump", "pumptid", "overgang" };
constexpr int kAntalInst = 31;
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
    lasInstallning (cache);
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
    in.mall      = juce::jlimit (0, motor::kAntalMallar - 1, (int) std::lround (v[0]));
    in.fro       = (uint32_t) juce::jmax (1, (int) std::lround (v[1]));
    in.form      = juce::jlimit (0, motor::kAntalFormer - 1, (int) std::lround (v[2]));
    in.energi    = v[3]; in.variation = v[4]; in.brytDjup = v[5]; in.byggLangd = v[6];
    in.stok      = v[7]; in.filter = v[8]; in.andas = v[9]; in.dub = v[10]; in.overgang = v[kIOvergang];
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
    return g >= 0 ? g : kRollOrord;   // har inte hört nog än: går rakt igenom
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

    const auto in = fran (v);
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
    x.setAttribute ("overgang", las ("overgang"));
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
    if (x->hasAttribute ("overgang")) satt ("overgang", (float) x->getDoubleAttribute ("overgang"));
    aktivtPresetNamn = x->getStringAttribute ("namn", f.getFileNameWithoutExtension());
    return true;
}

//==============================================================================
void StallverketProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto s = state.copyState();
    auto c = s.getOrCreateChildWithName ("CACHE", nullptr);
    for (int i = 0; i < kAntalInst; ++i) c.setProperty (kInstId[i], cache[i], nullptr);
    c.setProperty ("har", harCache, nullptr);
    s.setProperty ("preset", aktivtPresetNamn, nullptr);
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
    state.replaceState (s);
    tillstandLaddat = true;
}

juce::AudioProcessorEditor* StallverketProcessor::createEditor() { return new StallverketEditor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new StallverketProcessor(); }

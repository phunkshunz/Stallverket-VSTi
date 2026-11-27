#pragma once

// MASKINHALLEN · STYRET (Launch Control utan mappning)
//
// Samma fil i alla maskiner. Maskinen öppnar själv din Novation Launch Control
// (eller Launch Control XL) direkt via USB. Ingen MIDI-routing i DAW:en, ingen
// MIDI learn, inget att ställa in.
//
//  - Den maskin du senast öppnade eller klickade i styrs. Öppna SYRA: rattarna
//    styr SYRA. Klicka i STÄLLVERKET: rattarna styr STÄLLVERKET. Det gäller mellan
//    olika plugins också (de skriver vem som har styret i en liten gemensam fil).
//  - Övre rattraden = maskinens 8 viktigaste rattar (A), nedre raden = 8 till (B).
//    XL: tredje raden och faders = 8 till (C).
//  - Padsen = maskinens knappar. Pads med en funktion lyser.
//  - Rattarna "plockar upp": en ratt gör inget förrän den passerar värdet som står
//    i maskinen, så inget hoppar när du byter maskin.
//
// Launch Control, fabriksmall 1: rattar CC 21-28 och 41-48, pads not 9-12 och 25-28.
// Launch Control XL, fabriksmall 1: rattar CC 13-20, 29-36, 49-56, faders CC 77-84,
// knappar not 41-44, 57-60, 73-76, 89-92. Alla MIDI-kanaler (alla mallar) tas emot.
// Okända noter blir pads i den ordning du trycker på dem första gången.

#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <functional>

namespace styret
{
struct Upplagg
{
    juce::String maskin;                         // t.ex. "STALLVERKET", "SYRA"
    std::array<juce::String, 24> rattar;         // parameter-ID för A1-8, B1-8, C1-8 ("" = ingen)
    std::array<juce::String, 8> padNamn;         // visas i panelen
    std::array<std::function<void()>, 8> pads;   // körs på meddelandetråden
    std::function<bool (int)> padTand;           // lyser paden starkt just nu? (t.ex. HÅLL på)
};

class Styret : private juce::MidiInputCallback, private juce::Timer
{
public:
    static Styret& instans() { static Styret s; return s; }

    // En maskin anmäler sig (från processorn) och avanmäler sig i sin destruktor.
    void anmal (juce::AudioProcessor* p, Upplagg u)
    {
        const juce::ScopedLock l (las);
        for (auto& m : maskiner) if (m.proc == p) { m.upp = std::move (u); return; }
        maskiner.push_back ({ p, std::move (u) });
    }
    void avanmal (juce::AudioProcessor* p)
    {
        const juce::ScopedLock l (las);
        maskiner.erase (std::remove_if (maskiner.begin(), maskiner.end(), [p] (auto& m) { return m.proc == p; }), maskiner.end());
        if (agare == p) agare = nullptr;
    }

    // Panelen tar styret när den öppnas och när man klickar i den.
    void taStyret (juce::AudioProcessor* p)
    {
        {
            const juce::ScopedLock l (las);
            agare = p;
            for (auto& u : upplockad) u = false;
            for (auto& f : forraVarde) f = -1.0f;
        }
        minId = juce::String::toHexString ((juce::pointer_sized_int) p) + juce::String (juce::Time::getMillisecondCounter());
        skrivAgare();
        tandPads();
    }
    // Ta styret åt maskinen som anmält sig med det här namnet (t.ex. en KANAL som lämnar det till CENTRAL)
    void taStyret (const juce::String& maskin)
    {
        juce::AudioProcessor* p = nullptr;
        { const juce::ScopedLock l (las); for (auto& m : maskiner) if (m.upp.maskin == maskin) p = m.proc; }
        if (p != nullptr) taStyret (p);
    }
    bool harStyret (const juce::String& maskin) const
    {
        const juce::ScopedLock l (las);
        for (auto& m : maskiner) if (m.proc == agare && m.upp.maskin == maskin) return villAga();
        return false;
    }
    bool harStyret (const juce::AudioProcessor* p) const { const juce::ScopedLock l (las); return agare == p && villAga(); }
    bool ansluten() const { return in.size() > 0; }

    // För testprogrammet: låtsas att Launch Control skickade det här
    void testaMidi (const juce::MidiMessage& m) { handleIncomingMidiMessage (nullptr, m); timerCallback(); }

    // Senast vridna ratten (för panelens lilla visning)
    juce::String senast() const { const juce::ScopedLock l (las); return senastText; }
    double senastTid() const { return senastMs; }

private:
    Styret()
    {
        sokEnheter();
        startTimerHz (30);
    }
    ~Styret() override { stopTimer(); for (auto& i : in) i->stop(); }

    struct Maskin { juce::AudioProcessor* proc; Upplagg upp; };

    static juce::File agarFil()
    {
        return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
           #if JUCE_MAC
            .getChildFile ("Application Support")
           #endif
            .getChildFile ("Phunkshunz").getChildFile ("STYRET.txt");
    }
    void skrivAgare()
    {
        auto f = agarFil();
        f.getParentDirectory().createDirectory();
        f.replaceWithText (minId);
        senastLastAgare = minId;
    }
    // Har någon annan maskin (eller ett annat plugin) tagit styret efter oss?
    bool villAga() const { return senastLastAgare == minId && minId.isNotEmpty(); }

    void sokEnheter()
    {
        for (auto& d : juce::MidiInput::getAvailableDevices())
        {
            if (! d.name.containsIgnoreCase ("Launch Control")) continue;
            bool finns = false;
            for (auto& i : in) if (i->getIdentifier() == d.identifier) finns = true;
            if (finns) continue;
            if (auto i = juce::MidiInput::openDevice (d.identifier, this)) { i->start(); in.push_back (std::move (i)); }
        }
        for (auto& d : juce::MidiOutput::getAvailableDevices())
            if (d.name.containsIgnoreCase ("Launch Control") && ut.size() < 4)
            {
                bool finns = false;
                for (auto& o : ut) if (o->getIdentifier() == d.identifier) finns = true;
                if (! finns) if (auto o = juce::MidiOutput::openDevice (d.identifier)) ut.push_back (std::move (o));
            }
    }

    // ---- MIDI-tråden: lägg bara i kön
    void handleIncomingMidiMessage (juce::MidiInput*, const juce::MidiMessage& m) override
    {
        int typ = -1, nr = 0, varde = 0;
        if (m.isController()) { typ = 0; nr = m.getControllerNumber(); varde = m.getControllerValue(); }
        else if (m.isNoteOn() && m.getVelocity() > 0) { typ = 1; nr = m.getNoteNumber(); varde = 127; }
        else return;
        kanal.store (m.getChannel());
        int s1, n1, s2, n2;
        fifo.prepareToWrite (1, s1, n1, s2, n2);
        if (n1 > 0) { kö[(size_t) s1] = { typ, nr, varde }; fifo.finishedWrite (1); }
    }

    static int rattFor (int cc)
    {
        if (cc >= 21 && cc <= 28) return cc - 21;        // Launch Control övre
        if (cc >= 41 && cc <= 48) return 8 + cc - 41;    // Launch Control nedre
        if (cc >= 13 && cc <= 20) return cc - 13;        // XL rad 1
        if (cc >= 29 && cc <= 36) return 8 + cc - 29;    // XL rad 2
        if (cc >= 49 && cc <= 56) return 16 + cc - 49;   // XL rad 3
        if (cc >= 77 && cc <= 84) return 16 + cc - 77;   // XL faders
        return -1;
    }
    int padFor (int not_)
    {
        static const int lc[] = { 9, 10, 11, 12, 25, 26, 27, 28 };
        static const int xl[] = { 73, 74, 75, 76, 89, 90, 91, 92 };
        for (int i = 0; i < 8; ++i) if (lc[i] == not_ || xl[i] == not_) return i;
        for (int i = 0; i < 8; ++i) if (larda[i] == not_) return i;
        for (int i = 0; i < 8; ++i) if (larda[i] < 0) { larda[i] = not_; return i; }
        return -1;
    }

    // ---- meddelandetråden
    void timerCallback() override
    {
        if (++raknare % 60 == 0)   // varannan sekund: ny enhet inkopplad? har någon annan tagit styret?
        {
            sokEnheter();
            senastLastAgare = agarFil().loadFileAsString().trim();
        }
        int s1, n1, s2, n2;
        const int antal = fifo.getNumReady();
        fifo.prepareToRead (antal, s1, n1, s2, n2);
        for (int k = 0; k < n1 + n2; ++k)
        {
            const auto h = kö[(size_t) (k < n1 ? s1 + k : s2 + k - n1)];
            hantera (h.typ, h.nr, h.varde);
        }
        fifo.finishedRead (n1 + n2);
    }

    void hantera (int typ, int nr, int varde)
    {
        const juce::ScopedLock l (las);
        if (agare == nullptr || ! villAga()) return;
        Maskin* m = nullptr;
        for (auto& x : maskiner) if (x.proc == agare) m = &x;
        if (m == nullptr) return;
        if (typ == 0)
        {
            const int r = rattFor (nr);
            if (r < 0 || m->upp.rattar[(size_t) r].isEmpty()) return;
            auto* prm = dynamic_cast<juce::RangedAudioParameter*> (findParam (*m->proc, m->upp.rattar[(size_t) r]));
            if (prm == nullptr) return;
            const float ny = (float) varde / 127.0f, nu = prm->getValue();
            // plocka upp: ratten tar över först när den passerar maskinens värde
            if (! upplockad[(size_t) r])
            {
                const float forra = forraVarde[(size_t) r];
                if (std::abs (ny - nu) < 0.03f || (forra >= 0 && (forra - nu) * (ny - nu) <= 0.0f)) upplockad[(size_t) r] = true;
                forraVarde[(size_t) r] = ny;
                if (! upplockad[(size_t) r]) { visa (prm->getName (24) + "  " + juce::String::fromUTF8 ("→ vrid till ") + prm->getCurrentValueAsText()); return; }
            }
            prm->beginChangeGesture();
            prm->setValueNotifyingHost (ny);
            prm->endChangeGesture();
            visa (prm->getName (24) + "  " + prm->getCurrentValueAsText());
        }
        else
        {
            const int p = padFor (nr);
            if (p < 0) return;
            if (auto& f = m->upp.pads[(size_t) p]) { f(); visa (m->upp.padNamn[(size_t) p]); }
            tandPadsLast (*m);
        }
    }

    static juce::AudioProcessorParameter* findParam (juce::AudioProcessor& p, const juce::String& id)
    {
        for (auto* prm : p.getParameters())
            if (auto* w = dynamic_cast<juce::AudioProcessorParameterWithID*> (prm)) if (w->paramID == id) return prm;
        return nullptr;
    }

    void visa (const juce::String& t) { senastText = t; senastMs = juce::Time::getMillisecondCounterHiRes(); }

    void tandPads()
    {
        const juce::ScopedLock l (las);
        for (auto& x : maskiner) if (x.proc == agare) tandPadsLast (x);
    }
    // Pads med en funktion lyser gult, tända lägen (t.ex. HÅLL) grönt, resten släckta.
    void tandPadsLast (Maskin& m)
    {
        if (ut.empty()) return;
        static const int lc[] = { 9, 10, 11, 12, 25, 26, 27, 28 };
        const int ch = juce::jlimit (1, 16, kanal.load());
        for (int i = 0; i < 8; ++i)
        {
            const bool har = m.upp.pads[(size_t) i] != nullptr;
            const bool tand = har && m.upp.padTand && m.upp.padTand (i);
            const int farg = ! har ? 12 : (tand ? 60 : 29);   // av, grön full, gul låg
            for (auto& o : ut) o->sendMessageNow (juce::MidiMessage::noteOn (ch, lc[i], (juce::uint8) farg));
        }
    }

    struct Handelse { int typ = 0, nr = 0, varde = 0; };
    juce::CriticalSection las;
    std::vector<Maskin> maskiner;
    juce::AudioProcessor* agare = nullptr;
    juce::String minId, senastLastAgare, senastText;
    double senastMs = 0;
    std::vector<std::unique_ptr<juce::MidiInput>> in;
    std::vector<std::unique_ptr<juce::MidiOutput>> ut;
    juce::AbstractFifo fifo { 256 };
    std::array<Handelse, 256> kö {};
    std::array<bool, 24> upplockad {};
    std::array<float, 24> forraVarde { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 };
    std::array<int, 8> larda { -1, -1, -1, -1, -1, -1, -1, -1 };
    std::atomic<int> kanal { 9 };
    int raknare = 0;
};
} // namespace styret

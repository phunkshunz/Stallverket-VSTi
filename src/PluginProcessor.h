#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <atomic>
#include "Motor.h"
#include "Ledning.h"
#include "Fack.h"
#include "Lyssna.h"
#include "Kassett.h"
#include "Referens.h"

// STÄLLVERKET: maskinen som alla kanaler går igenom.
//
// Läggs på varje kanal som ska vara med i låten. En av dem är CENTRAL: den
// håller motorn och inställningarna och skickar ut dem till de andra. Varje
// instans (även CENTRAL) behandlar ljudet på sin egen kanal efter sin ROLL.
class StallverketProcessor : public juce::AudioProcessor, private juce::Timer
{
public:
    StallverketProcessor();
    ~StallverketProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "STALLVERKET"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return true; }
    double getTailLengthSeconds() const override { return 4.0; }

    int getNumPrograms() override;
    int getCurrentProgram() override { return currentPreset; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout skapaParametrar();

    juce::AudioProcessorValueTreeState state;

    // ---- för panelen
    bool arCentral() const;
    int  kanal() const;     // 0..7
    int  roll() const;      // roll som gäller (AUTO omräknad), NUM_ROLL = ORÖRD
    int  rollVal() const;   // valet i menyn, NUM_ROLL + 1 = AUTO
    int  autoGissning() const { return autoAktiv.load() >= 0 ? autoAktiv.load() : autoRoll.load(); }
    motor::Installning installning() const;   // det som gäller just nu (från CENTRAL eller sparat)
    motor::Overstyr overstyr() const;
    bool centralLever() const;
    bool extraCentral() const { return annanCentral.load(); }
    double slagNu() const { return slag.load(); }
    bool spelarNu() const { return spelar.load(); }
    int slagPerTakt() const { return spt.load(); }
    float matare() const { return matarNiva.load(); }
    float gainNu() const { return gainVisning.load(); }
    float lpNu() const { return lpVisning.load(); }
    float hpNu() const { return hpVisning.load(); }
    // LOOPDELAREN: hur mycket varje del hörs och om den slår just nu (för panelen)
    bool  delarPa() const { const int r = roll(); return (r == motor::LOOP || r == motor::PERC) && las ("dela") > 0.5f; }
    float delAktivitet (int i) const { return fack.delarna().aktivitet (i); }
    float delNiva (int i) const { return fack.delarna().nivaNu (i); }
    static int64_t nuMs();

    // ---- knapparna på CENTRAL
    void taOver (int typ);          // motor::BREAK / BYGG / DROP
    void slappTaOver();
    void vaxlaHall();
    bool haller() const;
    void slumpaFro();
    void slumpaAllt();

    // ---- REC: varje kanal spelar in sin egen WAV på skrivbordet
    void vaxlaRec();
    bool spelarIn() const { return ledning::nav().recId.load() != 0; }
    bool skriverNu() const { return skriver.load(); }
    double recSekunder() const;
    juce::File recMapp (int64_t id) const;
    juce::String recStatus() const { const juce::ScopedLock l (kassettLas); return recText; }

    // ---- presets
    int  antalFabrik() const;
    void laddaFabrik (int i);
    juce::File presetMapp() const;
    juce::Array<juce::File> egnaPresets() const;
    bool sparaPreset (const juce::String& namn);
    bool laddaPresetFil (const juce::File& f);
    juce::String presetNamn() const { return aktivtPresetNamn; }

    // ---- mallar och export
    void kopieraMallTillEgen();
    juce::File mallMapp() const;
    juce::Array<juce::File> egnaMallar() const;
    bool sparaMall (const juce::String& namn);
    bool laddaMall (const juce::File& f);
    juce::File exporteraMidi();

    // ---- BETONG-KASSETT
    bool kassettPa() const { return las ("kassett") > 0.5f; }
    juce::File ljudbank() const;                       // Musik/MASKINHALLEN/Ljudbank
    void valjKassettMapp (const juce::File& mapp);     // egen mapp med ljud (tom = BETONG-kitet)
    juce::String kassettStatus() const { const juce::ScopedLock l (kassettLas); return kassettText; }
    float kassettPad (int pad) const { return kassett.padAktivitet (pad); }

    // ---- REFERENSLÄSAREN: en låt blir kurva + skrift
    void lasReferens (const juce::File& fil);
    juce::String referensStatus() const { const juce::ScopedLock l (refLas); return refText; }

    // ---- SKRIFTEN
    void sattSkrift (const juce::String& txt);
    juce::String skrift() const { return skriftText; }

    float las (const char* id) const;

private:
    void satt (const juce::String& id, float verkligt);
    void lasInstallning (float* ut) const;
    motor::Installning fran (const float* v) const;

    double sr = 48000.0;
    fack::Fack fack;
    kassett::Kassett kassett;
    juce::ThreadPool kassettTrad { 1 };
    juce::CriticalSection kassettLas;
    juce::String kassettText, kassettMapp;
    int kassettKitLaddat = -1;
    juce::String kassettMappLaddad;
    void laddaKassett();
    void mixaHelaLaten (int64_t id);
    juce::String recText;
    juce::CriticalSection refLas;
    juce::String refText;
    std::unique_ptr<referens::Resultat> refKlar;   // läses av timern och läggs in i parametrarna
    std::atomic<double> bpmNu { 130.0 };
    lyssna::Lyssnare lyssnare;
    std::atomic<int> autoRoll { -1 }, autoAktiv { -1 };
    double klocka = 0.0, forvantat = 0.0;
    juce::String skriftText;

    void timerCallback() override;
    void stangRec();
    juce::SpinLock skrivLas;
    std::unique_ptr<juce::AudioFormatWriter::ThreadedWriter> skrivare;
    int64_t oppenRecId = 0;
    std::atomic<bool> skriver { false }, recKlar { false };
    std::atomic<int64_t> skrivnaSampel { 0 };
    std::atomic<int64_t> oppenRecIdAtomic { 0 };
    std::atomic<int> skriftKod[32] {};
    std::atomic<int> skriftAntal { 0 };
    bool spelade = false;
    std::vector<float> monoBuf;
    motor::Plan plan;
    motor::Installning planFor;
    bool harPlan = false;

    float cache[ledning::kInst] {};
    bool  harCache = false;
    std::atomic<bool> annanCentral { false };
    uint32_t minId = 0;
    bool tillstandLaddat = false, autoGjord = false;

    std::atomic<double> slag { 0.0 };
    std::atomic<bool> spelar { false };
    std::atomic<int> spt { 4 };
    std::atomic<float> matarNiva { 0.0f }, gainVisning { 1.0f }, lpVisning { 1.0f }, hpVisning { 0.0f };

    bool knappFore[5] {};
    int midiNot = -1, senasteDelId = -1;
    bool midiStartat = false;
    int64_t notAv = 0;

    int currentPreset = 0;
    juce::String aktivtPresetNamn;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StallverketProcessor)
};

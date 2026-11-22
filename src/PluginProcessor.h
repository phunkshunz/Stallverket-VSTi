#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>
#include "Motor.h"
#include "Ledning.h"
#include "Fack.h"
#include "Lyssna.h"

// STÄLLVERKET: maskinen som alla kanaler går igenom.
//
// Läggs på varje kanal som ska vara med i låten. En av dem är CENTRAL: den
// håller motorn och inställningarna och skickar ut dem till de andra. Varje
// instans (även CENTRAL) behandlar ljudet på sin egen kanal efter sin ROLL.
class StallverketProcessor : public juce::AudioProcessor
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
    int  autoGissning() const { return autoRoll.load(); }
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
    static int64_t nuMs();

    // ---- knapparna på CENTRAL
    void taOver (int typ);          // motor::BREAK / BYGG / DROP
    void slappTaOver();
    void vaxlaHall();
    bool haller() const;
    void slumpaFro();
    void slumpaAllt();

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

private:
    void satt (const juce::String& id, float verkligt);
    float las (const char* id) const;
    void lasInstallning (float* ut) const;
    motor::Installning fran (const float* v) const;

    double sr = 48000.0;
    fack::Fack fack;
    lyssna::Lyssnare lyssnare;
    std::atomic<int> autoRoll { -1 };
    double klocka = 0.0, forvantat = 0.0;
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

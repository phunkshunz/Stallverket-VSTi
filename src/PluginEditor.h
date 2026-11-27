#pragma once

#include "PluginProcessor.h"
#include "Gui.h"
#include "About.h"
#include "Sprak.h"
#include "Licens.h"

// Panelen: stor vy när instansen är CENTRAL, kompakt vy när den är en KANAL.
class StallverketEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit StallverketEditor (StallverketProcessor&);
    ~StallverketEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;

private:
    void timerCallback() override;
    void byggLayout();
    void fyllPresets();
    void ritaTidslinje (juce::Graphics&, juce::Rectangle<int>);
    void ritaKanalvy (juce::Graphics&, juce::Rectangle<int>);
    void sparaDialog();
    void visaVy (int v);
    void ritaKurva (juce::Graphics&, juce::Rectangle<int>);
    void ritaMallRubriker (juce::Graphics&);
    void dragKurva (juce::Point<int>);
    void fyllMallar();
    void namnDialog (const juce::String& rubrik, std::function<void (const juce::String&)> klar);
    juce::Rectangle<int> kurvYta, refYta;
    int vy = 0, dragPunkt = -1;

    StallverketProcessor& proc;
    ArmLnF lnf;
    bool centralVy = false;

    juce::TextButton centralKnapp { "CENTRAL" };
    juce::ComboBox kanalVal, rollVal, mallVal, formVal, presetVal;
    juce::TextButton presetFore { "<" }, presetNasta { ">" }, sparaKnapp { "SPARA" };
    juce::TextButton slumpaFro { "SLUMPA" }, slumpaAllt { "SLUMPA ALLT" };
    juce::TextButton breakNu { "BREAK NU" }, byggNu { "BYGG NU" }, dropNu { "DROP NU" }, hallKnapp, slappKnapp;
    sprak::Knapp sprakKnapp;

    Knob fro, energi, variation, brytdjup, bygglangd, stok, filter, andas, dub, djup, pump, pumptid, overgang, mjuk, tryck;
    juce::TextButton midiKnapp { "MIDI UT" };
    juce::TextButton recKnapp { "REC" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> midiAtt;
    juce::Slider kanalDjup[ledning::kKanaler];
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> kanalDjupAtt[ledning::kKanaler];
    juce::TooltipWindow tips { this, 700 };

    // flikar i tidslinjen
    juce::TextButton flikLat, flikKurva, flikMall, flikTrick, exportKnapp;
    // trick
    juce::TextButton trickKnapp[motor::kAntalTrick];
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> trickAtt[motor::kAntalTrick];
    juce::Slider trickMangd;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> trickMangdAtt;
    juce::TextEditor skriftFalt;
    juce::Rectangle<int> trickRub[5], skriftYta;
    void ritaTrick (juce::Graphics&);
    // kurva
    juce::TextButton kurvaPa { "KURVA PÅ" };
    juce::ComboBox kurvLangd;
    juce::TextButton refKnapp;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> kurvaPaAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> kurvLangdAtt;
    // mallredigerare
    juce::ComboBox mallLangd[motor::NUM_DEL];
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> mallLangdAtt[motor::NUM_DEL];
    juce::Slider mallRatt[10];
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mallRattAtt[10];
    juce::TextButton atmoForst, kopieraMall, sparaMall;
    juce::ComboBox laddaMall;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> atmoAtt;
    juce::Array<juce::File> mallFiler;
    juce::Rectangle<int> mallRubYta[3];
    licens::Ruta licensRuta;
    // LOOPDELAREN i kanalvyn
    juce::TextButton delaKnapp;
    juce::TextButton delVal[4];     // bläddrar AUTO -> PÅ -> AV
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> delaAtt;
    void uppdateraDelar();
    juce::Rectangle<int> delarYta;
    // BETONG-KASSETT i kanalvyn
    juce::TextButton kassettKnapp, kassettMappKnapp;
    juce::ComboBox kassettKit, kassettMonster;
    juce::Slider kassettLek;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> kassettAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> kassettKitAtt, kassettMonsterAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> kassettLekAtt;
    juce::Rectangle<int> kassettYta;
    // GLITCH i kanalvyn
    juce::TextButton glitchKnapp[8];
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> glitchAtt[8];
    juce::Slider glitchMangd, glitchCentral;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> glitchMangdAtt, glitchCentralAtt;
    juce::Rectangle<int> glitchYta;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> centralAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> kanalAtt, rollAtt, mallAtt, formAtt;

    juce::Rectangle<int> logoYta, tidYta, kanalYta;
    std::unique_ptr<juce::FileChooser> valjare;
    AboutOverlay om { juce::String::fromUTF8 ("STÄLLVERKET"), "mottagare + motor" };
    juce::Array<juce::File> egna;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StallverketEditor)
};

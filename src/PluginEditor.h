#pragma once

#include "PluginProcessor.h"
#include "Gui.h"
#include "About.h"
#include "Sprak.h"

// Panelen: stor vy när instansen är CENTRAL, kompakt vy när den är en KANAL.
class StallverketEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit StallverketEditor (StallverketProcessor&);
    ~StallverketEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;

private:
    void timerCallback() override;
    void byggLayout();
    void fyllPresets();
    void ritaTidslinje (juce::Graphics&, juce::Rectangle<int>);
    void ritaKanalvy (juce::Graphics&, juce::Rectangle<int>);
    void sparaDialog();

    StallverketProcessor& proc;
    ArmLnF lnf;
    bool centralVy = false;

    juce::TextButton centralKnapp { "CENTRAL" };
    juce::ComboBox kanalVal, rollVal, mallVal, formVal, presetVal;
    juce::TextButton presetFore { "<" }, presetNasta { ">" }, sparaKnapp { "SPARA" };
    juce::TextButton slumpaFro { "SLUMPA" }, slumpaAllt { "SLUMPA ALLT" };
    juce::TextButton breakNu { "BREAK NU" }, byggNu { "BYGG NU" }, dropNu { "DROP NU" }, hallKnapp, slappKnapp;
    sprak::Knapp sprakKnapp;

    Knob fro, energi, variation, brytdjup, bygglangd, stok, filter, andas, dub, djup;
    juce::Slider kanalDjup[ledning::kKanaler];
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> kanalDjupAtt[ledning::kKanaler];
    juce::TooltipWindow tips { this, 700 };

    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> centralAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> kanalAtt, rollAtt, mallAtt, formAtt;

    juce::Rectangle<int> logoYta, tidYta, kanalYta;
    std::unique_ptr<juce::FileChooser> valjare;
    AboutOverlay om { juce::String::fromUTF8 ("STÄLLVERKET"), "mottagare + motor" };
    juce::Array<juce::File> egna;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StallverketEditor)
};

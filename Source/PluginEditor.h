/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include <vector>
using namespace std;

//==============================================================================
/**
*/
class AdditiveSynthPluginAudioProcessorEditor : public juce::AudioProcessorEditor,
    public Slider::Listener
{
public:
    AdditiveSynthPluginAudioProcessorEditor(AdditiveSynthPluginAudioProcessor&);
    ~AdditiveSynthPluginAudioProcessorEditor() override;

    //==============================================================================
    void paint(juce::Graphics&) override;
    void resized() override;

    void sliderValueChanged(Slider* slider) override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.

    OwnedArray<Slider> gainSliders;
    OwnedArray<Slider> ADSRSliders;
    Slider volumeSlider;
    Slider modSlider;

    Label volumeLabel;
    Label modLabel;
    Label attackLabel, decayLabel, sustainLabel, releaseLabel;
    OwnedArray<Label> harmonicLabels;

    float attack = 0.5f;
    float decay = 0.5f;
    float sustain = 1.f;
    float release = 0.5f;

    AdditiveSynthPluginAudioProcessor& audioProcessor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AdditiveSynthPluginAudioProcessorEditor)
};

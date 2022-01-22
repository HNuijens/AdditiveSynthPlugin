/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <cmath>
#include <vector>
#include "SynthVoice.h"
using namespace std;

//==============================================================================
/**
*/
class AdditiveSynthPluginAudioProcessor : public juce::AudioProcessor
{
public:
    //==============================================================================
    AdditiveSynthPluginAudioProcessor();
    ~AdditiveSynthPluginAudioProcessor() override;

    //==============================================================================
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

#ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
#endif

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    float f0 = 440.f;
    float fs = 44100.f;

    vector<double> gainVector;

    float g = 1.f;
    float volume = 0.5f;
    int numHarmonics = 16;
    int numVoices = 6; 
    float cent = 0.f;

    void setVoiceHarmonics();
    void setVoiceADSR(float att, float dec, float sus, float rel);

private:
    // variables
    float nyquist = fs / 2.f;
    vector<int> currentPlayingNotes; 
    vector<float> currentAngle;
    vector<float> angleChange;

    vector<SynthVoice> synthVoices;
    int currentNoteIndex = 0;

    // methods
    float limit(float min, float max, float n);


    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AdditiveSynthPluginAudioProcessor)
};

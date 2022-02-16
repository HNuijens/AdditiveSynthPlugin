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


#define NOEDITOR // Uncomment this if the plugin is a Unity plugin

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

    float vol = 0.5f; // volume
    int numHarmonics = 16;
    int numVoices = 6; 
    float cent = 0.f;
    
    void setVoiceHarmonics();
    void setVoiceADSR(float att, float dec, float sus, float rel);
    float att = 0.5f, dec = 0.5f, sus = 1.0f, rel = 0.5f;
private:
    // variables
    float nyquist = fs / 2.f;
    
    vector<double> currentPlayingNotes;    
    int currentPreset = 1;

    int currentVoiceIndex = 0;
    vector<SynthVoice> synthVoices;

#ifdef NOEDITOR 
        // Exposed parameters for Unity
        AudioParameterFloat* volume;
        AudioParameterFloat* modulation;
        AudioParameterFloat* fundamentalFreq;
        AudioParameterFloat* attack;
        AudioParameterFloat* decay;
        AudioParameterFloat* sustain;
        AudioParameterFloat* release;
        AudioParameterBool* resetVoices;
        AudioParameterInt* preset;
        vector<AudioParameterBool*> noteOnOff; 
        AudioParameterBool* voiceIsAdded; 

        int activeVoices = 1;
        vector<bool> isPlaying;
#endif

#ifndef NOEDITOR
        // When playing midi 
        int activeVoices = numVoices;
#endif

    // methods
    float limit(float min, float max, float n);

    void ChangePreset();
    bool isOdd(int value) { return value % 2 != 0; };
   

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AdditiveSynthPluginAudioProcessor)
};

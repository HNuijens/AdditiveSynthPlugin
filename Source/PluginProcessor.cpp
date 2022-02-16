/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"



//==============================================================================
AdditiveSynthPluginAudioProcessor::AdditiveSynthPluginAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor(BusesProperties()
#if ! JucePlugin_IsMidiEffect
#if ! JucePlugin_IsSynth
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
#endif
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)
#endif
    )
#endif
{

#ifdef NOEDITOR
    addParameter(volume = new AudioParameterFloat("volume", // parameter ID
        "Volume", // parameter name
        0.0f,   // minimum value
        1.0f,   // maximum value
        1.0f)); // default value
    addParameter(modulation = new AudioParameterFloat("modulation", // parameter ID
        "Modulation", // parameter name
        -12.0f,   // minimum value
        12.0f,   // maximum value
        0.0f)); // default value
    addParameter(fundamentalFreq = new AudioParameterFloat("fundamentalFreq", // parameter ID
        "FundamentalFreq", // parameter name
        20.0f,   // minimum value
        20000.0f,   // maximum value
        440.0f)); // default value
    addParameter(attack = new AudioParameterFloat("attack", // parameter ID
        "Attack", // parameter name
        0.0f,   // minimum value
        25.0f,   // maximum value
        0.5f)); // default value
    addParameter(decay = new AudioParameterFloat("decay", // parameter ID
        "Decay", // parameter name
        0.0f,   // minimum value
        25.0f,   // maximum value
        0.5f)); // default value
    addParameter(sustain = new AudioParameterFloat("sustain", // parameter ID
        "Sustain", // parameter name
        0.0f,   // minimum value
        1.0f,   // maximum value
        1.0f)); // default value
    addParameter(release = new AudioParameterFloat("release", // parameter ID
        "Release", // parameter name
        0.0f,   // minimum value
        25.0f,   // maximum value
        0.5f)); // default value
    addParameter(voiceIsAdded = new AudioParameterBool("voiceIsAdded", // parameter ID
        "Voice is Added", // parameter name
        false   // default value
        )); // default value
    addParameter(resetVoices = new AudioParameterBool("resetVoices", // parameter ID
        "Reset Voices", // parameter name
        false   // default value
    )); // default value
    addParameter(preset = new AudioParameterInt("preset", // parameter ID
        "Preset", // parameter name
        1,   // minimum value
        4,   // maximum value
        1)); // default value
    noteOnOff.reserve(numVoices); 
    for (int h = 0; h < numVoices; h++)
    {
        noteOnOff.push_back(new AudioParameterBool("noteOnOff" + to_string(h), // parameter ID
            "NoteOnOff" + to_string(h), // parameter name
            false));

        addParameter(noteOnOff[h]);
    }
#endif
}

AdditiveSynthPluginAudioProcessor::~AdditiveSynthPluginAudioProcessor()
{
}

//==============================================================================
const juce::String AdditiveSynthPluginAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool AdditiveSynthPluginAudioProcessor::acceptsMidi() const
{
#if JucePlugin_WantsMidiInput
    return true;
#else
    return false;
#endif
}

bool AdditiveSynthPluginAudioProcessor::producesMidi() const
{
#if JucePlugin_ProducesMidiOutput
    return true;
#else
    return false;
#endif
}

bool AdditiveSynthPluginAudioProcessor::isMidiEffect() const
{
#if JucePlugin_IsMidiEffect
    return true;
#else
    return false;
#endif
}

double AdditiveSynthPluginAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int AdditiveSynthPluginAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int AdditiveSynthPluginAudioProcessor::getCurrentProgram()
{
    return 0;
}

void AdditiveSynthPluginAudioProcessor::setCurrentProgram(int index)
{
}

const juce::String AdditiveSynthPluginAudioProcessor::getProgramName(int index)
{
    return {};
}

void AdditiveSynthPluginAudioProcessor::changeProgramName(int index, const juce::String& newName)
{
}

//==============================================================================
void AdditiveSynthPluginAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    fs = sampleRate;
    nyquist = fs / 2.f;
    gainVector.clear();
    currentPlayingNotes.clear();
    synthVoices.clear();

    for (int h = 0; h < numHarmonics; h++)
    {
        gainVector.push_back(0.f);
    }
    gainVector[0] = 1.f;

    synthVoices.assign(numVoices, SynthVoice());
    for (int i = 0; i < numVoices; i++)
    {
        isPlaying.push_back(false);
        currentPlayingNotes.push_back(0);
        synthVoices[i].setup(sampleRate, numHarmonics);
    }
}

void AdditiveSynthPluginAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool AdditiveSynthPluginAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
#if JucePlugin_IsMidiEffect
    juce::ignoreUnused(layouts);
    return true;
#else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
#if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
#endif

    return true;
#endif
}
#endif

void AdditiveSynthPluginAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();


    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());


#ifndef NOEDITOR 
    // MIDI Input
    MidiBuffer::Iterator it(midiMessages);
    MidiMessage currentMessage;
    int samplePos;

    while (it.getNextEvent(currentMessage, samplePos))
    {
        if (currentMessage.isNoteOn())
        {
            
            f0 = currentMessage.getMidiNoteInHertz(currentMessage.getNoteNumber(), 440);
            currentPlayingNotes[currentNoteIndex] = currentMessage.getNoteNumber();
            synthVoices[currentNoteIndex].noteOn(f0);
            currentNoteIndex++;
            if (currentNoteIndex >= numVoices)currentNoteIndex = 0;
        }
        else if (currentMessage.isNoteOff())
        {
            for (int i = 0; i < numVoices; i++)
            {
                if (currentMessage.getNoteNumber() == currentPlayingNotes[i])
                {
                    synthVoices[i].noteOff();
                }
            }
            
        }
    }
#endif
    #ifdef NOEDITOR     
        if (vol != *volume)vol = *volume;
        if (cent != *modulation)
        {
            // Check modulation ocne every buffer to allow smooth frequency changes
            cent = *modulation * 100.0;
            for (int i = 0; i < numVoices; i++)
            {
                synthVoices[i].cent = cent;
                synthVoices[i].setAngleChange();
            }
        }
        if (att != *attack || dec != *decay || sus != *sustain || rel != *release)
        {
            // Envelope changed
            att = *attack;
            dec = *decay;
            sus = *sustain;
            rel = *release;
            for (int i = 0; i < numVoices; i++)  synthVoices[i].setADSRParams({ att,dec,sus,rel });
        }

        if (currentPreset != *preset)
        {
            // Preset is changed
            currentPreset = *preset;
            ChangePreset();
        }

        for (int i = 0; i < numVoices; i++)
        {
            // Check note on and off
            if (*noteOnOff[i] && !isPlaying[i])
            {
                synthVoices[i].noteOn();
                isPlaying[i] = true; 
            }

            if (!*noteOnOff[i] && isPlaying[i])
            {
                synthVoices[i].noteOff();
                isPlaying[i] = false;
            }
        }

        if (*voiceIsAdded)
        {
            // Voice is added. change the frequency of that voice
            if (currentVoiceIndex != 0 && activeVoices <= numVoices)activeVoices++;
            f0 = *fundamentalFreq;                          // change f0 to current frequency
            synthVoices[currentVoiceIndex].setF0(f0);
            currentVoiceIndex++;
            if (currentVoiceIndex >= numVoices)currentVoiceIndex = 0;
            *voiceIsAdded = false; 
        }

        if (*resetVoices)
        {
            // Reset at script start in Unity
            activeVoices = 1;
            currentVoiceIndex = 0; 
            *resetVoices = false;
        }

    #endif

    for (int n = 0; n < buffer.getNumSamples(); ++n)
    {
        auto outL = buffer.getWritePointer(0);
        auto outR = buffer.getWritePointer(1);

        float x = 0.f;

        for (int i = 0; i < numVoices; i++)
        {
            if (synthVoices[i].adsr.isActive())
            {
                x = x + synthVoices[i].getNextSample();
            }
        }
        
        x = vol * (1.f / static_cast<float>(activeVoices)) * x;
        x = limit(-1.f, 1.f, x);
        outL[n] = x;
        outR[n] = x;
    }
}

//==============================================================================
bool AdditiveSynthPluginAudioProcessor::hasEditor() const
{
    #ifdef NOEDITOR
        return false;
    #else
        return true;
    #endif
}

juce::AudioProcessorEditor* AdditiveSynthPluginAudioProcessor::createEditor()
{
    return new AdditiveSynthPluginAudioProcessorEditor(*this);
}

//==============================================================================
void AdditiveSynthPluginAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void AdditiveSynthPluginAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AdditiveSynthPluginAudioProcessor();
}

float AdditiveSynthPluginAudioProcessor::limit(float min, float max, float n)
{
    if (n < -1.f) return -1.f;
    else if (n > 1.f) return 1.f;
    else return n;
}


void AdditiveSynthPluginAudioProcessor::setVoiceHarmonics()
{
    for (int i = 0; i < numVoices; i++)
    synthVoices[i].setHarmonicGain(gainVector);
}

void AdditiveSynthPluginAudioProcessor::setVoiceADSR(float  att, float dec, float sus, float rel)
{
    for (int i = 0; i < numVoices; i++)
    synthVoices[i].setADSRParams({att,dec,sus,rel});
}

void AdditiveSynthPluginAudioProcessor::ChangePreset()
{
    switch (currentPreset)
    {
    // Sine wave
    case 1:
       
        for (int h = 0; h < numHarmonics; h++)
        {
            gainVector[h] = 0;
        }
        gainVector[0] = 1.f;

        for (int i = 0; i < numVoices; i++)
            synthVoices[i].setHarmonicGain(gainVector);
        break;

    // Triangle wave
    case 2:

        for (int h = 0; h < numHarmonics; h++)
        {
            if (isOdd(h))
                gainVector[h] = 1.0 / (h * h);
            else gainVector[h] = 0.f;
        }

        for (int i = 0; i < numVoices; i++)
            synthVoices[i].setHarmonicGain(gainVector);
        break;
        
    // Saw wave
    case 3:
        
        for (int h = 0; h < numHarmonics; h++)
        {
            gainVector[h] = 1.0 / (h + 1.0);
        }

        for (int i = 0; i < numVoices; i++)
            synthVoices[i].setHarmonicGain(gainVector);
        break;

    // Square wave
    case 4:

        for (int h = 0; h < numHarmonics; h++)
        {
            if (isOdd(h))
                gainVector[h] = 1.0 / (h + 1.0);
            else gainVector[h] = 0.f;
        }

        for (int i = 0; i < numVoices; i++)
            synthVoices[i].setHarmonicGain(gainVector);
        break;
    }
}
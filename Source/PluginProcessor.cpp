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

    addParameter(noteOn = new AudioParameterBool("noteOn", // parameter ID
        "NoteOn", // parameter name
        false   // default value
        )); // default value

    addParameter(noteOff = new AudioParameterBool("noteOff", // parameter ID
        "NoteOff", // parameter name
        false   // default value
        )); // default value

    addParameter(harmonicsChanged = new AudioParameterBool("harmonicChanged", // parameter ID
        "harmonic Changed", // parameter name
        false   // default value
    )); // default value

    // create gain parameter for each harmonic
    gains.reserve(numHarmonics); 
    for (int h = 0; h < numHarmonics; h++)
    {
        gains.push_back(new AudioParameterFloat("gain" + to_string(h), // parameter ID
            "Gain" + to_string(h), // parameter name
            0.0f,                  // minimum value
            1.0f,                  // maximum value
            0.1f));

        addParameter(gains[h]);
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
            cent = *modulation * 100.0;
            for (int i = 0; i < numVoices; i++)
            {
                synthVoices[i].cent = cent;
                synthVoices[i].setAngleChange();
            }
        }
        if (f0 != *fundamentalFreq)f0 = *fundamentalFreq;
        if (att != *attack || dec != *decay || sus != *sustain || rel != *release)
        {
            att = *attack;
            dec = *decay;
            sus = *sustain;
            rel = *release;
            for (int i = 0; i < numVoices; i++)  synthVoices[i].setADSRParams({ att,dec,sus,rel });
        }

        if (*harmonicsChanged)
        {
            for (int h = 0; h < numHarmonics; h++) gainVector[h] = *gains[h];
            
            for (int i = 0; i < numVoices; i++) synthVoices[i].setHarmonicGain(gainVector);
            
            *harmonicsChanged = false; 
        }

        // Note on and off
        if (*noteOn)
        {
            // check if one of the voices is already on
            bool noteExists = false; 
            for (int i = 0; i < numVoices; i++)
            {
                if (f0 == synthVoices[i].f0)
                {
                    synthVoices[i].noteOn(f0);
                    noteExists = true; 
                    currentNoteIndex = i; 
                }
            }

            if (!noteExists)
            {
                currentNoteIndex++;
                if (currentNoteIndex >= numVoices)currentNoteIndex = 0;
                synthVoices[currentNoteIndex].noteOn(f0);

            }

           *noteOn = false; 
        }

        if (*noteOff) // noteOff if the frequency of the noteOff mathces one of the currently playing frequencies
        {
            for (int i = 0; i < numVoices; i++)
            {
                if (f0 == synthVoices[i].f0)
                {
                    synthVoices[i].noteOff();
                }
            }
            *noteOff = false;
        }

    #endif


    for (int n = 0; n < buffer.getNumSamples(); ++n)
    {
        auto outL = buffer.getWritePointer(0);
        auto outR = buffer.getWritePointer(1);

        float x = 0.f;

        for (int i = 0; i < numVoices; i++)
        {
            x = x + synthVoices[i].getNextSample();
        }
        
        x = vol * (1.f / numVoices) * x;
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

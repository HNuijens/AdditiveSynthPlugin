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

    // initialize all vectors:
    gain.clear();
    currentAngle.clear();
    angleChange.clear();

    for (int h = 0; h < nHarmonics; h++)
    {
        gain.push_back(0.f);
        currentAngle.push_back(0.f);
        angleChange.push_back(f0 * static_cast<float>(h + 1) * 2.f * double_Pi * (1.f / fs));
    }

    gain[0] = 1.f;
    adsr.setSampleRate(fs);
    adsr.setParameters({ 0.5f,0.5f,1.0f,0.5f });

    averageGain = computeAverageGain(gain, nHarmonics);
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

    // MIDI Input
    MidiBuffer::Iterator it(midiMessages);
    MidiMessage currentMessage;
    int samplePos;

    while (it.getNextEvent(currentMessage, samplePos))
    {
        if (currentMessage.isNoteOn())
        {
            adsr.noteOn();
            f0 = currentMessage.getMidiNoteInHertz(currentMessage.getNoteNumber(), 440);
            for (int h = 0; h < nHarmonics; h++)
            {
                setAngleChange();
            }
        }
        else if (currentMessage.isNoteOff())
        {
            adsr.noteOff();
        }
    }

    for (int n = 0; n < buffer.getNumSamples(); ++n)
    {
        auto outL = buffer.getWritePointer(0);
        auto outR = buffer.getWritePointer(1);

        float x = 0.f;

        for (int h = 0; h < nHarmonics; h++)
        {
            if (f0 * static_cast<float>(h + 1) < nyquist)
            {
                x = x + adsr.getNextSample() * gain[h] * sin(currentAngle[h]);
            }
            currentAngle[h] += angleChange[h];
            if (currentAngle[h] > 2.f * double_Pi)
            {
                currentAngle[h] -= 2.f * double_Pi;
            }

        }

        x = volume * averageGain * x;
        x = limit(-1.f, 1.f, x);
        outL[n] = x;
        outR[n] = x;
    }
}

//==============================================================================
bool AdditiveSynthPluginAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
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

float AdditiveSynthPluginAudioProcessor::computeAverageGain(vector<float> gainList, float numberOfHarmonics)
{
    float totalGain = 0.f;
    for (int h = 0; h < numberOfHarmonics; h++)
    {
        if (f0 * static_cast<float>(h + 1) < nyquist) // only count audible frequencies
        {
            totalGain = totalGain + gainList[h];
        }
    }
    return 1.f / totalGain;
}


void AdditiveSynthPluginAudioProcessor::setAngleChange()
{
    for (int h = 0; h < nHarmonics; h++)
    {
        angleChange[h] = 2.f * double_Pi * f0 * static_cast<float>(h + 1) * powf(2.f, cent / 1200) * (1.f / fs);
    }
}
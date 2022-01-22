/*
  ==============================================================================

    SynthVoice.cpp
    Created: 22 Jan 2022 11:25:15am
    Author:  Helmer Nuijens

  ==============================================================================
*/

#include "SynthVoice.h"

SynthVoice::SynthVoice()
{

}

SynthVoice::~SynthVoice()
{

}

void SynthVoice::setup(double Fs, int numHarmonics)
{
    this->Fs = Fs;
    this->numHarmonics = numHarmonics;

    nyquist = Fs / 2.f;

    // initialize all vectors:
    gainVector.clear();
    currentAngle.clear();
    angleChange.clear();

    for (int h = 0; h < numHarmonics; h++)
    {
        gainVector.push_back(0.f);
        currentAngle.push_back(0.f);
        angleChange.push_back(f0 * (h + 1) * 2.f * double_Pi * (1.f / Fs));
    }

    gainVector[0] = 1.f;
    adsr.setSampleRate(Fs);
    adsr.setParameters({ 0.5f,0.5f,1.0f,0.5f });

    computeAverageGain();
}

double SynthVoice::getNextSample()
{
    double out = 0.f;

    for (int h = 0; h < numHarmonics; h++)
    {
        if (f0 * (h + 1) < nyquist) // filter out harmonics above nyquist
        {
            out = out + adsr.getNextSample() * gainVector[h] * sin(currentAngle[h]);
        }
        currentAngle[h] += angleChange[h];

        if (currentAngle[h] > 2.f * double_Pi)
        {
            currentAngle[h] -= 2.f * double_Pi;
        }
    }

    return out * averagedGain; 
}

void SynthVoice::setHarmonicGain(vector<double>gainVector)
{
    this->gainVector = gainVector;

    computeAverageGain();
}

void SynthVoice::computeAverageGain()
{
    double totalGain = 0.f;
    for (int h = 0; h < numHarmonics; h++)
    {
        if (f0 * (h + 1) < nyquist) // only count audible frequencies
        {
            totalGain = totalGain + gainVector[h];
        }
    }
    averagedGain =  1.f / totalGain;
}

void SynthVoice::setADSRParams(ADSR::Parameters params)
{
    adsr.setParameters(params);
}

void SynthVoice::noteOn(double f0)
{
    this->f0 = f0;
    setAngleChange();
    adsr.noteOn();
}

void SynthVoice::noteOff()
{
    adsr.noteOff();
}

void SynthVoice::setAngleChange()
{
    for (int h = 0; h < numHarmonics; h++)
    {
        angleChange[h] = 2.f * double_Pi * f0 * (h + 1) * powf(2.f, cent / 1200) * (1.f / Fs);
    }
}
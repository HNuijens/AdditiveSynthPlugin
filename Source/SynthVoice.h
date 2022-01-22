/*
  ==============================================================================

    SynthVoice.h
    Created: 22 Jan 2022 11:25:15am
    Author:  Helmer Nuijens

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <cmath>
#include <vector>
using namespace std;

class SynthVoice {

public: 
    SynthVoice(double Fs, int nHarmonics);

    ~SynthVoice();

    void SetGain(vector<double>gainVector);

    double GetNextSample();

   


private:

    void AverageGain();             // changing the gain when harmonics are altered


    vector<double> gainVector;      // list containing gain for each harmonic
    vector<float> currentAngle;     // current angle of all harmonics
    vector<float> angleChange;      // angular speed of all harmonics

    double Fs = 48000;
    float nyquist = Fs / 2.f;

    int numHarmonics;               // number of harmonics

   

    


};
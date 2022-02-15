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
    SynthVoice();

    ~SynthVoice();


    void setup(double Fs, int numHarmonics);
    void setHarmonicGain(vector<double>gainVector);
    
    double getNextSample();

    void setADSRParams(ADSR::Parameters params);
    void noteOn(double f0);
    void noteOff();
    void setAngleChange();          // changing the angular speed
    
    double cent = 0;                
    double f0 = 220; 


private:

    void computeAverageGain();      // changing the gain when harmonics are altered
   

    vector<double> gainVector;      // list containing gain for each harmonic
    vector<double> currentAngle;     // current angle of all harmonics
    vector<double> angleChange;      // angular speed of all harmonics

    ADSR adsr;                      // envelope
    ADSR::Parameters adsrParams;    // envelope parameters

    double Fs = 48000;              // sampling rate
    double nyquist = Fs / 2.f;      // fundam
    double averagedGain;            // average out all sinusoids

    
    int numHarmonics;               // number of harmonics

};
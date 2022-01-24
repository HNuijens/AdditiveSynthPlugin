/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
AdditiveSynthPluginAudioProcessorEditor::AdditiveSynthPluginAudioProcessorEditor(AdditiveSynthPluginAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    for (int h = 0; h < audioProcessor.numHarmonics; h++)
    {
        gainSliders.add(new Slider());
        harmonicLabels.add(new Label());
    }

    for (int i = 0; i < 4; i++)
    {
        ADSRSliders.add(new Slider());
    }

    for (int h = 0; h < audioProcessor.numHarmonics; h++)
    {
        gainSliders[h]->addListener(this);
        gainSliders[h]->setSliderStyle(Slider::SliderStyle::LinearBarVertical);
        gainSliders[h]->setTextBoxStyle(Slider::TextBoxBelow, true, 50, 30);
        gainSliders[h]->setRange(0.f, 1.f, 0.01f);
        gainSliders[h]->setValue(audioProcessor.gainVector[h]);
        //gainSliders[h]->setSliderSnapsToMousePosition(false);
        addAndMakeVisible(gainSliders[h]);

        harmonicLabels[h]->setText("f" + to_string(h), dontSendNotification);
        harmonicLabels[h]->attachToComponent(gainSliders[h], false);
        harmonicLabels[h]->setJustificationType(Justification::centred);
        addAndMakeVisible(harmonicLabels[h]);
    }


    for (int i = 0; i < 4; i++)
    {
        ADSRSliders[i]->addListener(this);
        ADSRSliders[i]->setSliderStyle(Slider::SliderStyle::RotaryHorizontalVerticalDrag);
        ADSRSliders[i]->setTextBoxStyle(Slider::TextBoxBelow, false, 50, 30);
        ADSRSliders[i]->setRange(0.01f, 25.f, 0.01f);
        ADSRSliders[i]->setValue(0.5f);
        addAndMakeVisible(ADSRSliders[i]);
    }

    ADSRSliders[2]->setRange(0.1f, 1.f, 0.01f);
    ADSRSliders[2]->setValue(1.f);

    attackLabel.setText("Attack", dontSendNotification);
    attackLabel.attachToComponent(ADSRSliders[0], false);
    attackLabel.setJustificationType(Justification::centred);
    addAndMakeVisible(attackLabel);

    decayLabel.setText("Decay", dontSendNotification);
    decayLabel.attachToComponent(ADSRSliders[1], false);
    decayLabel.setJustificationType(Justification::centred);
    addAndMakeVisible(decayLabel);

    sustainLabel.setText("Sustain", dontSendNotification);
    sustainLabel.attachToComponent(ADSRSliders[2], false);
    sustainLabel.setJustificationType(Justification::centred);
    addAndMakeVisible(sustainLabel);

    releaseLabel.setText("Release", dontSendNotification);
    releaseLabel.attachToComponent(ADSRSliders[3], false);
    releaseLabel.setJustificationType(Justification::centred);
    addAndMakeVisible(releaseLabel);

    volumeSlider.addListener(this);
    volumeSlider.setSliderStyle(Slider::SliderStyle::RotaryHorizontalVerticalDrag);
    volumeSlider.setTextBoxStyle(Slider::TextBoxBelow, false, 50, 30);
    volumeSlider.setRange(0.f, 1.f, 0.001f);
    volumeSlider.setValue(1.f);
    addAndMakeVisible(volumeSlider);

    volumeLabel.setText("Volume", dontSendNotification);
    volumeLabel.attachToComponent(&volumeSlider, false);
    volumeLabel.setJustificationType(Justification::centred);
    addAndMakeVisible(volumeLabel);

    modSlider.addListener(this);
    modSlider.setSliderStyle(Slider::SliderStyle::LinearVertical);
    modSlider.setTextBoxStyle(Slider::TextBoxBelow, false, 50, 30);
    modSlider.setRange(-6.f, 6.f, 0.01f);
    modSlider.setValue(0.f);
    addAndMakeVisible(modSlider);

    modLabel.setText("Mod", dontSendNotification);
    modLabel.attachToComponent(&modSlider, false);
    modLabel.setJustificationType(Justification::centred);
    addAndMakeVisible(modLabel);



    setSize(600, 400);
}

AdditiveSynthPluginAudioProcessorEditor::~AdditiveSynthPluginAudioProcessorEditor()
{
}

//==============================================================================
void AdditiveSynthPluginAudioProcessorEditor::paint(juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));

    g.setColour(juce::Colours::white);


    auto area = getLocalBounds();
    auto labelHeight = 30;
    auto header = area.removeFromTop(getHeight() / 15);
    //auto footer = area.removeFromBottom(header.getHeight());
    auto topArea = area.removeFromTop(3.f * getHeight() / 5.f);
    // g.drawRoundedRectangle(topArea.toFloat(), 10.f, 0.5f);
     //auto modArea = topArea.removeFromTop(getHeight() / 2.f);
    auto topLabelArea = topArea.removeFromBottom(labelHeight);
    auto gainSliderArea = topArea.removeFromRight(5.f * getWidth() / 6.f);
    auto sliderArea = gainSliderArea.getWidth() / audioProcessor.numHarmonics;
    // g.drawRoundedRectangle(area.toFloat(), 10.f, 0.5f);
    modSlider.setBounds(topArea);

    g.drawRoundedRectangle(area.toFloat(), 10.f, 0.5f);
    auto bottomLabelArea = area.removeFromTop(labelHeight);
    auto volumeArea = area.removeFromLeft(2.f * getWidth() / 7.f);
    volumeSlider.setBounds(volumeArea);

    //auto ADSRArea = area.removeFromRight(5.f * getWidth() / 7.f);

    auto ADSRSliderArea = area.getWidth() / 4.f;

    for (int h = 0; h < audioProcessor.numHarmonics; h++)
    {
        gainSliders[h]->setBounds(gainSliderArea.removeFromLeft(sliderArea));
    }

    for (int i = 0; i < 4; i++)
    {
        ADSRSliders[i]->setBounds(area.removeFromLeft(ADSRSliderArea));
    }




}

void AdditiveSynthPluginAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
}

void AdditiveSynthPluginAudioProcessorEditor::sliderValueChanged(Slider* slider)
{
    for (int h = 0; h < audioProcessor.numHarmonics; h++)
    {
        if (slider == gainSliders[h])
        {
            audioProcessor.gainVector[h] = gainSliders[h]->getValue();
            audioProcessor.setVoiceHarmonics();
         
        }
    }



    if (slider == ADSRSliders[0])
    {
        attack = ADSRSliders[0]->getValue();
        audioProcessor.att = attack; 
    }

    if (slider == ADSRSliders[1])
    {
        decay = ADSRSliders[1]->getValue();
        audioProcessor.dec = decay;
    }

    if (slider == ADSRSliders[2])
    {
        sustain = ADSRSliders[2]->getValue();
        audioProcessor.sus = sustain;
    }

    if (slider == ADSRSliders[3])
    {
        release = ADSRSliders[3]->getValue();
        audioProcessor.rel = release;
    }

    if (slider == ADSRSliders[0] || slider == ADSRSliders[1] || slider == ADSRSliders[2] || slider == ADSRSliders[3])
    {
        audioProcessor.setVoiceADSR(attack,decay,sustain,release);
    }

    if (slider == &volumeSlider)
    {
        audioProcessor.vol = volumeSlider.getValue();
    }

    if (slider == &modSlider)
    {
        audioProcessor.cent = modSlider.getValue() * 100.f;
       // audioProcessor.setAngleChange();
    }

}
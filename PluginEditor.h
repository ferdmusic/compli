#pragma once

// #include "PluginProcessor.h" // Forward declaration will be used instead for this header
#include <juce_gui_basics/juce_gui_basics.h> // Ensure this is included

// Forward declaration
class AudioPluginAudioProcessor;
namespace ParameterID // Forward declare to use ParameterID constants if needed, though usually in .cpp
{
    static const juce::String inputDevice; // Actual definition in PluginProcessor.cpp
    static const juce::String outputDevice;
}

//==============================================================================
class AudioPluginAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                              public juce::ComboBox::Listener,
                                              public juce::AudioProcessorValueTreeState::Listener,
                                              public juce::Timer // Add Timer
{
public:
    explicit AudioPluginAudioProcessorEditor (AudioPluginAudioProcessor&);
    ~AudioPluginAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

    // ComboBox Listener Override
    void comboBoxChanged (juce::ComboBox* comboBoxThatHasChanged) override;

    // APVTS Listener Override
    void parameterChanged (const juce::String& parameterID, float newValue) override;

    // Timer Callback
    void timerCallback() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    AudioPluginAudioProcessor& processorRef;

    void refreshDeviceComboBoxes();
    juce::AudioParameterInt* getInputDeviceParameter() const;
    juce::AudioParameterInt* getOutputDeviceParameter() const;

    juce::ComboBox microphoneInputSelector;
    juce::Label microphoneInputLabel;

    juce::ComboBox audioOutputSelector;
    juce::Label audioOutputLabel;

    // Compressor Controls
    juce::Slider compThresholdSlider, compRatioSlider, compAttackSlider, compReleaseSlider, compMakeupGainSlider;
    juce::Label compThresholdLabel, compRatioLabel, compAttackLabel, compReleaseLabel, compMakeupGainLabel;
    juce::TextButton compBypassButton; // Or juce::ToggleButton

    // Limiter Controls
    juce::Slider limThresholdSlider, limReleaseSlider;
    juce::Label limThresholdLabel, limReleaseLabel;
    juce::TextButton limBypassButton; // Or juce::ToggleButton

    // APVTS Attachments
    // Compressor Attachments
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> compThresholdAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> compRatioAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> compAttackAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> compReleaseAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> compMakeupGainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> compBypassAttachment;

    // Limiter Attachments
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> limThresholdAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> limReleaseAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> limBypassAttachment;

    // Meter Controls
    juce::Slider inputLevelMeter, outputLevelMeter, gainReductionMeter;
    juce::Label inputLevelLabel, outputLevelLabel, gainReductionLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAudioProcessorEditor)
};

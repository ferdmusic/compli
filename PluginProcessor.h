#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "juce_audio_devices/juce_audio_devices.h"
#include <juce_dsp/juce_dsp.h>
#include <atomic> // Ensure atomic is included

//==============================================================================
class AudioPluginAudioProcessor final : public juce::AudioProcessor,
                                        public juce::AudioProcessorValueTreeState::Listener
{
public:
    //==============================================================================
    // Public for easy access from PluginEditor, or use getter methods
    std::atomic<float> inputLevelDb { -100.0f };
    std::atomic<float> outputLevelDb { -100.0f };
    std::atomic<float> gainReductionDb { 0.0f }; // Typically 0 or negative

    juce::AudioDeviceManager deviceManager;
    juce::String currentInputDeviceName;
    juce::String currentOutputDeviceName;

    juce::AudioProcessorValueTreeState parameters;
    juce::UndoManager undoManager;

    AudioPluginAudioProcessor();
    ~AudioPluginAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

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
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==============================================================================
    // APVTS Listener callback
    void parameterChanged (const juce::String& parameterID, float newValue) override;

private:
    //==============================================================================
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    
    juce::StringArray getAvailableInputDeviceNames();
    juce::StringArray getAvailableOutputDeviceNames();
    void updateDeviceSettingsFromParameters(); 

    // DSP Objects
    juce::dsp::Compressor<float> compressor;
    juce::dsp::Limiter<float> limiter;
    juce::dsp::ProcessSpec spec;

    // Atomic parameter pointers for DSP
    std::atomic<float>* compThresholdParam = nullptr;
    std::atomic<float>* compRatioParam = nullptr;
    std::atomic<float>* compAttackParam = nullptr;
    std::atomic<float>* compReleaseParam = nullptr;
    std::atomic<float>* compMakeupGainParam = nullptr;
    std::atomic<float>* compBypassParam = nullptr;

    std::atomic<float>* limThresholdParam = nullptr;
    std::atomic<float>* limReleaseParam = nullptr;
    std::atomic<float>* limBypassParam = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAudioProcessor)
};

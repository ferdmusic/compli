#include "PluginProcessor.h" // This should include ParameterID if it's defined there as a namespace
#include "PluginEditor.h"


// Helper implementations
juce::AudioParameterInt* AudioPluginAudioProcessorEditor::getInputDeviceParameter() const {
    // It's safer to use the string IDs directly if ParameterID namespace isn't cleanly available/defined in a shared header
    return dynamic_cast<juce::AudioParameterInt*>(processorRef.parameters.getParameter("inputDevice"));
}

juce::AudioParameterInt* AudioPluginAudioProcessorEditor::getOutputDeviceParameter() const {
    return dynamic_cast<juce::AudioParameterInt*>(processorRef.parameters.getParameter("outputDevice"));
}


//==============================================================================
AudioPluginAudioProcessorEditor::AudioPluginAudioProcessorEditor (AudioPluginAudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p)
{
    juce::ignoreUnused (processorRef);

    // Microphone Input Label
    microphoneInputLabel.setText ("Microphone:", juce::dontSendNotification);
    microphoneInputLabel.attachToComponent (&microphoneInputSelector, true);
    addAndMakeVisible (microphoneInputLabel);

    // Microphone Input Selector ComboBox
    addAndMakeVisible (microphoneInputSelector);
    microphoneInputSelector.addListener (this);

    // Audio Output Label
    audioOutputLabel.setText ("Audio Output:", juce::dontSendNotification);
    audioOutputLabel.attachToComponent (&audioOutputSelector, true);
    addAndMakeVisible (audioOutputLabel);

    // Audio Output Selector ComboBox
    addAndMakeVisible (audioOutputSelector);
    audioOutputSelector.addListener (this);
    
    // Initial population and selection from APVTS
    refreshDeviceComboBoxes();

    // Register as APVTS listener
    processorRef.parameters.addParameterListener ("inputDevice", this);
    processorRef.parameters.addParameterListener ("outputDevice", this);

    // Compressor Controls & Attachments
    // Comp Threshold
    compThresholdLabel.setText("Comp Threshold", juce::dontSendNotification);
    compThresholdLabel.attachToComponent(&compThresholdSlider, true);
    addAndMakeVisible(compThresholdLabel);
    compThresholdSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    compThresholdSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 80, 20);
    addAndMakeVisible(compThresholdSlider);
    compThresholdAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processorRef.parameters, "compThreshold", compThresholdSlider);

    // Comp Ratio
    compRatioLabel.setText("Comp Ratio", juce::dontSendNotification);
    compRatioLabel.attachToComponent(&compRatioSlider, true);
    addAndMakeVisible(compRatioLabel);
    compRatioSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    compRatioSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 80, 20);
    addAndMakeVisible(compRatioSlider);
    compRatioAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processorRef.parameters, "compRatio", compRatioSlider);

    // Comp Attack
    compAttackLabel.setText("Comp Attack", juce::dontSendNotification);
    compAttackLabel.attachToComponent(&compAttackSlider, true);
    addAndMakeVisible(compAttackLabel);
    compAttackSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    compAttackSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 80, 20);
    addAndMakeVisible(compAttackSlider);
    compAttackAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processorRef.parameters, "compAttack", compAttackSlider);

    // Comp Release
    compReleaseLabel.setText("Comp Release", juce::dontSendNotification);
    compReleaseLabel.attachToComponent(&compReleaseSlider, true);
    addAndMakeVisible(compReleaseLabel);
    compReleaseSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    compReleaseSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 80, 20);
    addAndMakeVisible(compReleaseSlider);
    compReleaseAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processorRef.parameters, "compRelease", compReleaseSlider);

    // Comp Makeup Gain
    compMakeupGainLabel.setText("Comp Makeup", juce::dontSendNotification);
    compMakeupGainLabel.attachToComponent(&compMakeupGainSlider, true);
    addAndMakeVisible(compMakeupGainLabel);
    compMakeupGainSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    compMakeupGainSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 80, 20);
    addAndMakeVisible(compMakeupGainSlider);
    compMakeupGainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processorRef.parameters, "compMakeupGain", compMakeupGainSlider);
    
    // Comp Bypass
    compBypassButton.setButtonText("Bypass Comp");
    addAndMakeVisible(compBypassButton);
    compBypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        processorRef.parameters, "compBypass", compBypassButton);

    // Limiter Controls & Attachments
    // Limiter Threshold
    limThresholdLabel.setText("Limiter Threshold", juce::dontSendNotification);
    limThresholdLabel.attachToComponent(&limThresholdSlider, true);
    addAndMakeVisible(limThresholdLabel);
    limThresholdSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    limThresholdSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 80, 20);
    addAndMakeVisible(limThresholdSlider);
    limThresholdAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processorRef.parameters, "limThreshold", limThresholdSlider);

    // Limiter Release
    limReleaseLabel.setText("Limiter Release", juce::dontSendNotification);
    limReleaseLabel.attachToComponent(&limReleaseSlider, true);
    addAndMakeVisible(limReleaseLabel);
    limReleaseSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    limReleaseSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 80, 20);
    addAndMakeVisible(limReleaseSlider);
    limReleaseAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processorRef.parameters, "limRelease", limReleaseSlider);

    // Limiter Bypass
    limBypassButton.setButtonText("Bypass Limiter");
    addAndMakeVisible(limBypassButton);
    limBypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        processorRef.parameters, "limBypass", limBypassButton);

    // Meter Controls
    // Input Level Meter
    inputLevelLabel.setText("Input", juce::dontSendNotification);
    addAndMakeVisible(inputLevelLabel);
    inputLevelMeter.setSliderStyle(juce::Slider::LinearBarVertical);
    inputLevelMeter.setRange(-60.0, 6.0, 0.1);
    inputLevelMeter.setValue(-60.0);
    inputLevelMeter.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    inputLevelMeter.setScrollWheelEnabled(false);
    inputLevelMeter.setMouseDragEnabled(false);
    inputLevelMeter.setDoubleClickReturnValue(false, 0.0);
    addAndMakeVisible(inputLevelMeter);

    // Output Level Meter
    outputLevelLabel.setText("Output", juce::dontSendNotification);
    addAndMakeVisible(outputLevelLabel);
    outputLevelMeter.setSliderStyle(juce::Slider::LinearBarVertical);
    outputLevelMeter.setRange(-60.0, 6.0, 0.1);
    outputLevelMeter.setValue(-60.0);
    outputLevelMeter.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    outputLevelMeter.setScrollWheelEnabled(false);
    outputLevelMeter.setMouseDragEnabled(false);
    outputLevelMeter.setDoubleClickReturnValue(false, 0.0);
    addAndMakeVisible(outputLevelMeter);

    // Gain Reduction Meter
    gainReductionLabel.setText("GR", juce::dontSendNotification);
    addAndMakeVisible(gainReductionLabel);
    gainReductionMeter.setSliderStyle(juce::Slider::LinearBarVertical);
    gainReductionMeter.setRange(-24.0, 0.0, 0.1); // GR is 0 or negative
    gainReductionMeter.setValue(0.0);
    gainReductionMeter.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    gainReductionMeter.setScrollWheelEnabled(false);
    gainReductionMeter.setMouseDragEnabled(false);
    gainReductionMeter.setDoubleClickReturnValue(false, 0.0);
    addAndMakeVisible(gainReductionMeter);
    
    setSize (600, 600); // Adjusted initial size for more controls and meters
    startTimerHz(30); // Start timer for meter updates
}

AudioPluginAudioProcessorEditor::~AudioPluginAudioProcessorEditor()
{
    stopTimer(); // Stop timer

    // Unregister APVTS listeners
    processorRef.parameters.removeParameterListener ("inputDevice", this);
    processorRef.parameters.removeParameterListener ("outputDevice", this);

    // ComboBox listener removal
    microphoneInputSelector.removeListener(this);
    audioOutputSelector.removeListener(this);
}

//==============================================================================
void AudioPluginAudioProcessorEditor::timerCallback()
{
    inputLevelMeter.setValue (processorRef.inputLevelDb.load(), juce::dontSendNotification);
    outputLevelMeter.setValue (processorRef.outputLevelDb.load(), juce::dontSendNotification);
    // For GR meter, it's often better to show positive values for reduction amounts
    // So if gainReductionDb.load() is -3.0f, meter shows 3.0f.
    // However, our range is -24 to 0, so direct value is fine.
    gainReductionMeter.setValue (processorRef.gainReductionDb.load(), juce::dontSendNotification);
}

//==============================================================================
void AudioPluginAudioProcessorEditor::refreshDeviceComboBoxes()
{
    // --- Input Device ComboBox ---
    microphoneInputSelector.removeListener(this); // Avoid triggering comboBoxChanged during refresh
    microphoneInputSelector.clear(juce::dontSendNotification);
    juce::StringArray inputDeviceNames = processorRef.getAvailableInputDeviceNames();
    
    if (inputDeviceNames.isEmpty()) {
        microphoneInputSelector.addItem("No input devices found", 1);
        microphoneInputSelector.setEnabled(false);
    } else {
        microphoneInputSelector.addItemList(inputDeviceNames, 1);
        microphoneInputSelector.setEnabled(true);
        if (auto* param = getInputDeviceParameter()) {
            int currentDeviceIndex = static_cast<int>(param->get());
            if (currentDeviceIndex >= 0 && currentDeviceIndex < inputDeviceNames.size()) {
                microphoneInputSelector.setSelectedId(currentDeviceIndex + 1, juce::dontSendNotification);
            } else { // Index out of bounds, select first and update param
                microphoneInputSelector.setSelectedId(1, juce::dontSendNotification);
                 // param->setValueNotifyingHost(param->convertTo0to1(0)); // Let processor handle param update if needed
            }
        } else { // No param found, select first item if any
             if (inputDeviceNames.size() > 0) microphoneInputSelector.setSelectedId(1, juce::dontSendNotification);
        }
    }
    microphoneInputSelector.addListener(this);

    // --- Output Device ComboBox ---
    audioOutputSelector.removeListener(this); // Avoid triggering comboBoxChanged during refresh
    audioOutputSelector.clear(juce::dontSendNotification);
    juce::StringArray outputDeviceNames = processorRef.getAvailableOutputDeviceNames();

    if (outputDeviceNames.isEmpty()) {
        audioOutputSelector.addItem("No output devices found", 1);
        audioOutputSelector.setEnabled(false);
    } else {
        audioOutputSelector.addItemList(outputDeviceNames, 1);
        audioOutputSelector.setEnabled(true);
        if (auto* param = getOutputDeviceParameter()) {
            int currentDeviceIndex = static_cast<int>(param->get());
            if (currentDeviceIndex >= 0 && currentDeviceIndex < outputDeviceNames.size()) {
                audioOutputSelector.setSelectedId(currentDeviceIndex + 1, juce::dontSendNotification);
            } else { // Index out of bounds, select first and update param
                audioOutputSelector.setSelectedId(1, juce::dontSendNotification);
                // param->setValueNotifyingHost(param->convertTo0to1(0)); // Let processor handle param update
            }
        } else { // No param found
            if (outputDeviceNames.size() > 0) audioOutputSelector.setSelectedId(1, juce::dontSendNotification);
        }
    }
    audioOutputSelector.addListener(this);
}

//==============================================================================
void AudioPluginAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    // g.setColour (juce::Colours::white);
    // g.setFont (15.0f);
    // g.drawFittedText ("Hello World!", getLocalBounds(), juce::Justification::centred, 1); // Remove default text
}

void AudioPluginAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
    
    // Define areas
    auto totalBounds = getLocalBounds();
    auto meterAreaWidth = 150;
    auto controlsArea = totalBounds.removeFromLeft(totalBounds.getWidth() - meterAreaWidth);
    auto meterArea = totalBounds; // What's left is the meter area

    // Layout for existing controls in 'controlsArea'
    auto bounds = controlsArea.reduced(10);
    int labelWidth = 100; 
    int sliderHeight = 25;
    int buttonHeight = 25;
    int spacing = 5;
    int sectionSpacing = 15;

    // Device Selectors
    microphoneInputSelector.setBounds (bounds.getX() + labelWidth, bounds.getY(), bounds.getWidth() - labelWidth, sliderHeight);
    bounds.removeFromTop(sliderHeight + spacing);
    audioOutputSelector.setBounds (bounds.getX() + labelWidth, bounds.getY(), bounds.getWidth() - labelWidth, sliderHeight);
    bounds.removeFromTop(sliderHeight + sectionSpacing);

    // Compressor Controls
    compThresholdSlider.setBounds(bounds.getX() + labelWidth, bounds.getY(), bounds.getWidth() - labelWidth, sliderHeight);
    bounds.removeFromTop(sliderHeight + spacing);
    compRatioSlider.setBounds(bounds.getX() + labelWidth, bounds.getY(), bounds.getWidth() - labelWidth, sliderHeight);
    bounds.removeFromTop(sliderHeight + spacing);
    compAttackSlider.setBounds(bounds.getX() + labelWidth, bounds.getY(), bounds.getWidth() - labelWidth, sliderHeight);
    bounds.removeFromTop(sliderHeight + spacing);
    compReleaseSlider.setBounds(bounds.getX() + labelWidth, bounds.getY(), bounds.getWidth() - labelWidth, sliderHeight);
    bounds.removeFromTop(sliderHeight + spacing);
    compMakeupGainSlider.setBounds(bounds.getX() + labelWidth, bounds.getY(), bounds.getWidth() - labelWidth, sliderHeight);
    bounds.removeFromTop(sliderHeight + spacing);
    compBypassButton.setBounds(bounds.getX() + labelWidth, bounds.getY(), bounds.getWidth() - labelWidth, buttonHeight);
    bounds.removeFromTop(buttonHeight + sectionSpacing);

    // Limiter Controls
    limThresholdSlider.setBounds(bounds.getX() + labelWidth, bounds.getY(), bounds.getWidth() - labelWidth, sliderHeight);
    bounds.removeFromTop(sliderHeight + spacing);
    limReleaseSlider.setBounds(bounds.getX() + labelWidth, bounds.getY(), bounds.getWidth() - labelWidth, sliderHeight);
    bounds.removeFromTop(sliderHeight + spacing);
    limBypassButton.setBounds(bounds.getX() + labelWidth, bounds.getY(), bounds.getWidth() - labelWidth, buttonHeight);
    
    // Layout for Meters in 'meterArea'
    auto meterBounds = meterArea.reduced(10);
    auto meterSectionWidth = meterBounds.getWidth() / 3;

    auto inputMeterArea = meterBounds.removeFromLeft(meterSectionWidth).reduced(5, 0);
    inputLevelLabel.setBounds(inputMeterArea.removeFromTop(20));
    inputLevelMeter.setBounds(inputMeterArea);

    auto grMeterArea = meterBounds.removeFromLeft(meterSectionWidth).reduced(5, 0);
    gainReductionLabel.setBounds(grMeterArea.removeFromTop(20));
    gainReductionMeter.setBounds(grMeterArea);
    
    auto outputMeterArea = meterBounds.removeFromLeft(meterSectionWidth).reduced(5, 0);
    outputLevelLabel.setBounds(outputMeterArea.removeFromTop(20));
    outputLevelMeter.setBounds(outputMeterArea);
}

//==============================================================================
void AudioPluginAudioProcessorEditor::comboBoxChanged (juce::ComboBox* comboBoxThatHasChanged)
{
    int selectedIndex = comboBoxThatHasChanged->getSelectedItemIndex();
    if (selectedIndex < 0) return; // Should not happen if items are present and one is selected

    if (comboBoxThatHasChanged == &microphoneInputSelector)
    {
        if (auto* param = getInputDeviceParameter())
        {
            // Check if current param value is different to avoid unnecessary notifications
            if (static_cast<int>(param->get()) != selectedIndex)
            {
                param->setValueNotifyingHost(param->convertTo0to1(selectedIndex));
            }
        }
    }
    else if (comboBoxThatHasChanged == &audioOutputSelector)
    {
        if (auto* param = getOutputDeviceParameter())
        {
            if (static_cast<int>(param->get()) != selectedIndex)
            {
                param->setValueNotifyingHost(param->convertTo0to1(selectedIndex));
            }
        }
    }
}

//==============================================================================
void AudioPluginAudioProcessorEditor::parameterChanged (const juce::String& parameterID, float newValue)
{
    // newValue for AudioParameterInt is the actual integer value.
    if (parameterID == "inputDevice") // Using string literals for safety
    {
        int newDeviceIndex = static_cast<int>(newValue);
        microphoneInputSelector.removeListener(this); // Prevent feedback loop
        if (newDeviceIndex >= 0 && newDeviceIndex < microphoneInputSelector.getNumItems()) {
            microphoneInputSelector.setSelectedItemIndex(newDeviceIndex, juce::dontSendNotification);
        } else if (microphoneInputSelector.getNumItems() > 0) {
             microphoneInputSelector.setSelectedItemIndex(0, juce::dontSendNotification); // Default to first
        } else {
            // No items, might clear or show "No devices" if that item exists
            // For now, if list is empty, refreshDeviceComboBoxes should have handled it.
        }
        microphoneInputSelector.addListener(this);
    }
    else if (parameterID == "outputDevice") // Using string literals
    {
        int newDeviceIndex = static_cast<int>(newValue);
        audioOutputSelector.removeListener(this); // Prevent feedback loop
        if (newDeviceIndex >= 0 && newDeviceIndex < audioOutputSelector.getNumItems()) {
            audioOutputSelector.setSelectedItemIndex(newDeviceIndex, juce::dontSendNotification);
        } else if (audioOutputSelector.getNumItems() > 0) {
             audioOutputSelector.setSelectedItemIndex(0, juce::dontSendNotification); // Default to first
        } else {
            // No items
        }
        audioOutputSelector.addListener(this);
    }
}

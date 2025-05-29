#include "PluginProcessor.h"
#include "PluginEditor.h"

// Define Parameter IDs
namespace ParameterID
{
    static const juce::String inputDevice { "inputDevice" };
    static const juce::String outputDevice { "outputDevice" };

    // Compressor Params
    static const juce::String compThreshold  { "compThreshold" };
    static const juce::String compRatio      { "compRatio" };
    static const juce::String compAttack     { "compAttack" };
    static const juce::String compRelease    { "compRelease" };
    static const juce::String compMakeupGain { "compMakeupGain" };
    static const juce::String compBypass     { "compBypass" };

    // Limiter Params
    static const juce::String limThreshold   { "limThreshold" };
    static const juce::String limRelease     { "limRelease" };
    static const juce::String limBypass      { "limBypass" };
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout AudioPluginAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // Device Parameters
    params.push_back(std::make_unique<juce::AudioParameterInt>(
        ParameterID::inputDevice, "Input Device", 0, 100, 0,
        juce::AudioParameterIntAttributes().withLabel("Input Device Index")));
    params.push_back(std::make_unique<juce::AudioParameterInt>(
        ParameterID::outputDevice, "Output Device", 0, 100, 0,
        juce::AudioParameterIntAttributes().withLabel("Output Device Index")));
    
    // Compressor Parameters
    params.push_back(std::make_unique<juce::AudioParameterFloat>(ParameterID::compThreshold, "Compressor Threshold", juce::NormalisableRange<float>(-60.0f, 0.0f, 0.1f), -20.0f, juce::AudioParameterFloatAttributes().withLabel("dB")));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(ParameterID::compRatio, "Compressor Ratio", juce::NormalisableRange<float>(1.0f, 20.0f, 0.1f), 3.0f)); // No unit string for ratio
    params.push_back(std::make_unique<juce::AudioParameterFloat>(ParameterID::compAttack, "Compressor Attack", juce::NormalisableRange<float>(0.1f, 100.0f, 0.1f, 0.3f), 5.0f, juce::AudioParameterFloatAttributes().withLabel("ms")));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(ParameterID::compRelease, "Compressor Release", juce::NormalisableRange<float>(10.0f, 1000.0f, 0.1f, 0.3f), 100.0f, juce::AudioParameterFloatAttributes().withLabel("ms")));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(ParameterID::compMakeupGain, "Compressor Makeup Gain", juce::NormalisableRange<float>(0.0f, 24.0f, 0.1f), 0.0f, juce::AudioParameterFloatAttributes().withLabel("dB")));
    params.push_back(std::make_unique<juce::AudioParameterBool>(ParameterID::compBypass, "Compressor Bypass", false, juce::AudioParameterBoolAttributes().withLabel("Bypass")));

    // Limiter Parameters
    params.push_back(std::make_unique<juce::AudioParameterFloat>(ParameterID::limThreshold, "Limiter Threshold", juce::NormalisableRange<float>(-20.0f, 0.0f, 0.1f), -1.0f, juce::AudioParameterFloatAttributes().withLabel("dB")));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(ParameterID::limRelease, "Limiter Release", juce::NormalisableRange<float>(1.0f, 100.0f, 0.1f, 0.3f), 10.0f, juce::AudioParameterFloatAttributes().withLabel("ms")));
    params.push_back(std::make_unique<juce::AudioParameterBool>(ParameterID::limBypass, "Limiter Bypass", false, juce::AudioParameterBoolAttributes().withLabel("Bypass")));

    return { params.begin(), params.end() };
}

//==============================================================================
AudioPluginAudioProcessor::AudioPluginAudioProcessor()
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
       parameters (*this, &undoManager, juce::Identifier (JucePlugin_Name), createParameterLayout())
{
    // Initialise device manager: 0 input channels by default (user must select), 2 output channels (stereo default).
    // No specific XML settings, scan for devices, default output device.
    // Last argument `true` for `useDefaultInputDevice` is ignored if numInputChannels is 0.
    juce::String error = deviceManager.initialise (0, 2, nullptr, true, juce::String(), nullptr);
    if (error.isNotEmpty())
    {
        DBG ("Failed to initialise AudioDeviceManager: " + error);
        // Handle initialisation error, perhaps fall back or log extensively
    }
    
    // These are now primarily driven by APVTS logic in updateDeviceSettingsFromParameters
    // currentInputDeviceName = deviceManager.getDefaultAudioInputDeviceName(); 
    // currentOutputDeviceName = deviceManager.getDefaultAudioOutputDeviceName();

    // Initialize atomic parameter pointers
    compThresholdParam = parameters.getRawParameterValue(ParameterID::compThreshold);
    compRatioParam = parameters.getRawParameterValue(ParameterID::compRatio);
    compAttackParam = parameters.getRawParameterValue(ParameterID::compAttack);
    compReleaseParam = parameters.getRawParameterValue(ParameterID::compRelease);
    compMakeupGainParam = parameters.getRawParameterValue(ParameterID::compMakeupGain);
    compBypassParam = parameters.getRawParameterValue(ParameterID::compBypass);

    limThresholdParam = parameters.getRawParameterValue(ParameterID::limThreshold);
    limReleaseParam = parameters.getRawParameterValue(ParameterID::limRelease);
    limBypassParam = parameters.getRawParameterValue(ParameterID::limBypass);

    // Log available input devices for debugging - can be removed later
    // auto inputDeviceTypes = deviceManager.getAvailableDeviceTypes();
    // for (const auto& type : inputDeviceTypes)
    // {
    //     DBG("Device type: " << type->getTypeName());
    //     type->scanForDevices();
    //     for (const auto& deviceName : type->getDeviceNames())
    //         DBG("  Input Device: " << deviceName);
    // }
    // if (currentInputDeviceName.isNotEmpty())
    //    DBG("Default input device: " << currentInputDeviceName);
    // else
    //    DBG("No default input device found.");
    
    parameters.addParameterListener (ParameterID::inputDevice, this);
    parameters.addParameterListener (ParameterID::outputDevice, this);

    // Initialise device names from APVTS or defaults
    // This needs to be on message thread if it's going to call setAudioDeviceSetup
    if (juce::MessageManager::getInstance()->isThisTheMessageThread())
        updateDeviceSettingsFromParameters();
    else
        juce::MessageManager::callAsync([this] { updateDeviceSettingsFromParameters(); });
}

AudioPluginAudioProcessor::~AudioPluginAudioProcessor()
{
    parameters.removeParameterListener (ParameterID::inputDevice, this);
    parameters.removeParameterListener (ParameterID::outputDevice, this);
}

//==============================================================================
// Helper methods for device names
juce::StringArray AudioPluginAudioProcessor::getAvailableInputDeviceNames()
{
    juce::StringArray names;
    auto availableTypes = deviceManager.getAvailableDeviceTypes();
    for (const auto* type : availableTypes)
    {
        if (type->isInputType())
        {
            juce::StringArray typeDeviceNames;
            type->getDeviceNames(typeDeviceNames);
            for (const auto& name : typeDeviceNames)
                names.addIfNotAlreadyThere(name);
        }
    }
    // names.sortNatural(); // Sorting is good for UI consistency
    std::sort(names.begin(), names.end());
    return names;
}

juce::StringArray AudioPluginAudioProcessor::getAvailableOutputDeviceNames()
{
    juce::StringArray names;
    auto availableTypes = deviceManager.getAvailableDeviceTypes();
    for (const auto* type : availableTypes)
    {
        if (type->isOutputType())
        {
            juce::StringArray typeDeviceNames;
            type->getDeviceNames(typeDeviceNames);
            for (const auto& name : typeDeviceNames)
                names.addIfNotAlreadyThere(name);
        }
    }
    // names.sortNatural();
    std::sort(names.begin(), names.end());
    return names;
}

void AudioPluginAudioProcessor::updateDeviceSettingsFromParameters()
{
    #if JUCE_STANDALONE_APPLICATION
    // This function can be called from parameterChanged or constructor.
    // If not on message thread, defer the actual device setup.
    if (!juce::MessageManager::getInstance()->isThisTheMessageThread())
    {
        juce::MessageManager::callAsync([this] { updateDeviceSettingsFromParameters(); });
        return;
    }

    bool settingsMightHaveChanged = false; // Tracks if currentXDeviceName changed based on param
    bool actualDeviceSetupChanged = false; // Tracks if deviceManager.setAudioDeviceSetup will be called

    juce::AudioDeviceManager::AudioDeviceSetup currentAudioSetup;
    deviceManager.getAudioDeviceSetup(currentAudioSetup); // Get current actual setup

    // --- Input Device ---
    auto* inputDeviceIndexParam = dynamic_cast<juce::AudioParameterInt*>(parameters.getParameter(ParameterID::inputDevice));
    if (inputDeviceIndexParam)
    {
        int inputDeviceIndex = inputDeviceIndexParam->get();
        juce::StringArray inputNames = getAvailableInputDeviceNames();

        if (inputNames.size() > 0)
        {
            if (inputDeviceIndex < 0 || inputDeviceIndex >= inputNames.size())
            {
                // Index out of bounds, select first available and update parameter
                if (currentInputDeviceName != inputNames[0])
                {
                    currentInputDeviceName = inputNames[0];
                    settingsMightHaveChanged = true;
                }
                inputDeviceIndexParam->setValueNotifyingHost(inputDeviceIndexParam->convertTo0to1(0)); // Update APVTS
            }
            else
            {
                // Valid index
                if (currentInputDeviceName != inputNames[inputDeviceIndex])
                {
                    currentInputDeviceName = inputNames[inputDeviceIndex];
                    settingsMightHaveChanged = true;
                }
            }
        }
        else if (!currentInputDeviceName.isEmpty()) // No devices available
        {
            currentInputDeviceName = juce::String();
            settingsMightHaveChanged = true;
            // Optionally reset parameter index if desired, e.g., to -1 or 0 if list becomes non-empty later
            // inputDeviceIndexParam->setValueNotifyingHost(inputDeviceIndexParam->convertTo0to1(0)); // or some invalid index marker
        }
    }

    // --- Output Device ---
    auto* outputDeviceIndexParam = dynamic_cast<juce::AudioParameterInt*>(parameters.getParameter(ParameterID::outputDevice));
    if (outputDeviceIndexParam)
    {
        int outputDeviceIndex = outputDeviceIndexParam->get();
        juce::StringArray outputNames = getAvailableOutputDeviceNames();

        if (outputNames.size() > 0)
        {
            if (outputDeviceIndex < 0 || outputDeviceIndex >= outputNames.size())
            {
                // Index out of bounds
                if (currentOutputDeviceName != outputNames[0])
                {
                    currentOutputDeviceName = outputNames[0];
                    settingsMightHaveChanged = true;
                }
                outputDeviceIndexParam->setValueNotifyingHost(outputDeviceIndexParam->convertTo0to1(0));
            }
            else
            {
                // Valid index
                if (currentOutputDeviceName != outputNames[outputDeviceIndex])
                {
                    currentOutputDeviceName = outputNames[outputDeviceIndex];
                    settingsMightHaveChanged = true;
                }
            }
        }
        else if (!currentOutputDeviceName.isEmpty()) // No devices available
        {
            currentOutputDeviceName = juce::String();
            settingsMightHaveChanged = true;
            // outputDeviceIndexParam->setValueNotifyingHost(outputDeviceIndexParam->convertTo0to1(0));
        }
    }

    // Now, compare with currentAudioSetup and apply if needed
    if (currentAudioSetup.inputDeviceName != currentInputDeviceName)
    {
        currentAudioSetup.inputDeviceName = currentInputDeviceName;
        if (!currentInputDeviceName.isEmpty())
            currentAudioSetup.inputChannels = juce::AudioChannelSet::mono().getChannelTypes();
        else
            currentAudioSetup.inputChannels.clear();
        actualDeviceSetupChanged = true;
    }

    if (currentAudioSetup.outputDeviceName != currentOutputDeviceName)
    {
        currentAudioSetup.outputDeviceName = currentOutputDeviceName;
        if (!currentOutputDeviceName.isEmpty())
            currentAudioSetup.outputChannels = juce::AudioChannelSet::stereo().getChannelTypes();
        else
            currentAudioSetup.outputChannels.clear();
        actualDeviceSetupChanged = true;
    }
    
    // Note: We are not changing sampleRate or bufferSize here, those are typically managed by prepareToPlay
    // or host context. If they were also APVTS params, they'd be handled similarly.

    if (actualDeviceSetupChanged)
    {
        // It's crucial that sampleRate and bufferSize are valid in currentAudioSetup
        // They should be what the device manager is currently using or what prepareToPlay last set.
        // If deviceManager.initialise set them, or prepareToPlay has run, they should be sensible.
        juce::String error = deviceManager.setAudioDeviceSetup(currentAudioSetup, true /*forceRestart*/);
        if (error.isNotEmpty()) {
            DBG("Error updating audio device setup from parameters: " + error);
            // Potentially try to revert to a known good state or re-query device manager's actual setup
        }
    }
    #endif // JUCE_STANDALONE_APPLICATION
}


//==============================================================================
const juce::String AudioPluginAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool AudioPluginAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool AudioPluginAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool AudioPluginAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double AudioPluginAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int AudioPluginAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int AudioPluginAudioProcessor::getCurrentProgram()
{
    return 0;
}

void AudioPluginAudioProcessor::setCurrentProgram (int index)
{
    juce::ignoreUnused (index);
}

const juce::String AudioPluginAudioProcessor::getProgramName (int index)
{
    juce::ignoreUnused (index);
    return {};
}

void AudioPluginAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused (index, newName);
}

//==============================================================================
void AudioPluginAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    #if JUCE_STANDALONE_APPLICATION
        // Ensure device names are up-to-date with APVTS first
        // updateDeviceSettingsFromParameters(); // This might call setAudioDeviceSetup, potentially risky if called too often or at wrong time.
                                            // Let's assume parameterChanged and constructor handle setting currentXDeviceName.

        juce::AudioDeviceManager::AudioDeviceSetup newSetup;
        deviceManager.getAudioDeviceSetup(newSetup); // Get current or last known good setup

        bool setupChanged = false;

        // Apply device names from currentXxxDeviceName members (which should be synced with APVTS)
        if (newSetup.inputDeviceName != currentInputDeviceName) {
            newSetup.inputDeviceName = currentInputDeviceName;
            if (!currentInputDeviceName.isEmpty())
                newSetup.inputChannels = juce::AudioChannelSet::mono().getChannelTypes();
            else
                newSetup.inputChannels.clear();
            setupChanged = true;
        }

        if (newSetup.outputDeviceName != currentOutputDeviceName) {
            newSetup.outputDeviceName = currentOutputDeviceName;
            if (!currentOutputDeviceName.isEmpty())
                newSetup.outputChannels = juce::AudioChannelSet::stereo().getChannelTypes();
            else
                newSetup.outputChannels.clear();
            setupChanged = true;
        }
            
        // Then, ensure sampleRate and bufferSize are applied
        if (newSetup.sampleRate != sampleRate) {
            newSetup.sampleRate = sampleRate;
            setupChanged = true;
        }
        if (newSetup.bufferSize != samplesPerBlock) {
            newSetup.bufferSize = samplesPerBlock;
            setupChanged = true;
        }

        if (setupChanged) {
            juce::String error = deviceManager.setAudioDeviceSetup(newSetup, true /*forceRestart if settings actually changed*/);
            if (error.isNotEmpty()) {
                DBG("Error in prepareToPlay updating audio device setup: " + error);
            }
        }
    #else
        // Plugin-specific prepareToPlay logic (if any beyond what JUCE handles)
        // juce::ignoreUnused (sampleRate, samplesPerBlock); // Will use these now for spec
    #endif
    
    // Prepare DSP objects
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = samplesPerBlock;
    // spec.numChannels = getTotalNumOutputChannels(); // Use main bus layout
    // Determine number of channels based on the main output bus, or default to stereo if that's problematic early on.
    // If this is called before buses are fully set up, getTotalNumOutputChannels() might be 0.
    // It's safer to use the bus layout if available, or a reasonable default.
    const auto mainBusLayout = getBusLayout().getMainOutputChannelSet();
    if (mainBusLayout.size() > 0)
        spec.numChannels = (juce::uint32) mainBusLayout.size();
    else // Fallback if bus layout not ready (e.g. during construction or very early init)
        spec.numChannels = 2; // Default to stereo


    compressor.prepare(spec);
    limiter.prepare(spec);

    compressor.reset();
    limiter.reset();
}

void AudioPluginAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
    #if JUCE_STANDALONE_APPLICATION
        deviceManager.closeAudioDevice();
    #endif
}

bool AudioPluginAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JUCE_STANDALONE_APPLICATION
    // For standalone, the device manager handles I/O.
    // We can be more flexible or rely on the device manager's capabilities.
    // For now, let's assume stereo output is fine and input is handled by deviceManager.
    const auto& mainOutput = layouts.getMainOutputChannelSet();
    if (mainOutput != juce::AudioChannelSet::mono() && mainOutput != juce::AudioChannelSet::stereo())
        return false;
    // Input check is less strict here as deviceManager will handle it.
    // Or, we could check if any input is enabled if that's a requirement.
    return true; // Be more permissive for standalone initially.
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

void AudioPluginAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                              juce::MidiBuffer& midiMessages)
{
    midiMessages.clear(); // Not a MIDI effect

    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // Calculate overall RMS input level across all channels for simplicity
    float totalInputRms = 0.0f;
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        totalInputRms += buffer.getRMSLevel (channel, 0, buffer.getNumSamples());
    }
    float averageInputRms = buffer.getNumChannels() > 0 ? totalInputRms / buffer.getNumChannels() : 0.0f;
    inputLevelDb.store (juce::Decibels::gainToDecibels (averageInputRms, -100.0f));

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // Update DSP parameters from APVTS
    // Compressor
    bool compIsBypassed = compBypassParam->load() > 0.5f;
    if (!compIsBypassed)
    {
        compressor.setThreshold (compThresholdParam->load());
        compressor.setRatio     (compRatioParam->load());
        compressor.setAttack    (compAttackParam->load());
        compressor.setRelease   (compReleaseParam->load());
    }

    // Limiter
    bool limIsBypassed = limBypassParam->load() > 0.5f;
    if (!limIsBypassed)
    {
        limiter.setThreshold (limThresholdParam->load());
        limiter.setRelease   (limReleaseParam->load());
    }
    
    // Create AudioBlock
    juce::dsp::AudioBlock<float> block (buffer);
    // Ensure spec is up-to-date for the block, especially numChannels if it can change dynamically
    // Though typically it's set in prepareToPlay. If dynamic bus changes are supported, spec might need updating here.
    // spec.numChannels = (juce::uint32) block.getNumChannels(); // More robust if channel count can change post-prepareToPlay

    // Process audio
    if (!compIsBypassed)
    {
        compressor.process (juce::dsp::ProcessContextReplacing<float> (block));
        gainReductionDb.store (compressor.getGainReduction()); // Get reduction in dB
        
        float makeupGainDb = compMakeupGainParam->load();
        if (makeupGainDb > 0.0f) // Small optimization: only apply if gain is non-zero
        {
            block.multiplyBy(juce::Decibels::decibelsToGain(makeupGainDb));
        }
    }
    else
    {
        gainReductionDb.store (0.0f); // No gain reduction if bypassed
    }

    if (!limIsBypassed)
    {
        limiter.process (juce::dsp::ProcessContextReplacing<float> (block));
    }

    // Calculate overall RMS output level
    float totalOutputRms = 0.0f;
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        totalOutputRms += buffer.getRMSLevel (channel, 0, buffer.getNumSamples());
    }
    float averageOutputRms = buffer.getNumChannels() > 0 ? totalOutputRms / buffer.getNumChannels() : 0.0f;
    outputLevelDb.store (juce::Decibels::gainToDecibels (averageOutputRms, -100.0f));

    // If your algorithm always overwrites all output channels, this loop might not be needed.
    // However, if processing is conditional (like bypass), it's safer.
    // The initial loop already clears extra output channels.
    // If processing happens in-place on input channels that are fewer than output channels,
    // remaining output channels need clearing or copying from processed input if that's the intent.
    // For now, assume in-place processing on all available channels up to min(inputs, outputs)
    // and the initial loop handles any truly "extra" output channels.
}

//==============================================================================
bool AudioPluginAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* AudioPluginAudioProcessor::createEditor()
{
    return new AudioPluginAudioProcessorEditor (*this);
}

//==============================================================================
void AudioPluginAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
    juce::ignoreUnused (destData);
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void AudioPluginAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
    juce::ignoreUnused (data, sizeInBytes);
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName (parameters.state.getType()))
        {
            parameters.replaceState (juce::ValueTree::fromXml (*xmlState));
            // After restoring state, ensure device settings reflect the loaded parameters
            if (juce::MessageManager::getInstance()->isThisTheMessageThread())
                updateDeviceSettingsFromParameters();
            else
                juce::MessageManager::callAsync([this] { updateDeviceSettingsFromParameters(); });
        }
}

//==============================================================================
void AudioPluginAudioProcessor::parameterChanged (const juce::String& parameterID, float newValue)
{
    // Called when an APVTS parameter is changed.
    // newValue is the normalized 0-1 value for RangedAudioParameter, or the direct value for others (like int for AudioParameterInt choices if used that way)
    // For our AudioParameterInt device indices, newValue will be the actual index.
    if (parameterID == ParameterID::inputDevice || parameterID == ParameterID::outputDevice)
    {
        // The value of newValue for AudioParameterInt is the actual int value, not normalized.
        // updateDeviceSettingsFromParameters will fetch the current value from APVTS.
        updateDeviceSettingsFromParameters();
    }
    // Add handling for other parameters (compressor, limiter) here later
    juce::ignoreUnused(newValue); // newValue is used implicitly by updateDeviceSettingsFromParameters fetching from APVTS
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AudioPluginAudioProcessor();
}

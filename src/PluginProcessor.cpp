#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "CircuitSerializer.h"

CircuitsAudioProcessor::CircuitsAudioProcessor()
    : AudioProcessor(BusesProperties()
                     .withInput("Input", juce::AudioChannelSet::stereo(), true)
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
    voltageHistory.resize(HISTORY_SIZE, 0.0f);
}

CircuitsAudioProcessor::~CircuitsAudioProcessor()
{
}

const juce::String CircuitsAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool CircuitsAudioProcessor::acceptsMidi() const
{
    return false;
}

bool CircuitsAudioProcessor::producesMidi() const
{
    return false;
}

bool CircuitsAudioProcessor::isMidiEffect() const
{
    return false;
}

double CircuitsAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int CircuitsAudioProcessor::getNumPrograms()
{
    return 1;
}

int CircuitsAudioProcessor::getCurrentProgram()
{
    return 0;
}

void CircuitsAudioProcessor::setCurrentProgram(int index)
{
    juce::ignoreUnused(index);
}

const juce::String CircuitsAudioProcessor::getProgramName(int index)
{
    juce::ignoreUnused(index);
    return {};
}

void CircuitsAudioProcessor::changeProgramName(int index, const juce::String& newName)
{
    juce::ignoreUnused(index, newName);
}

void CircuitsAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    circuitEngine.setSampleRate(sampleRate);
    circuitEngine.setCircuit(circuitGraph);
    
    juce::ignoreUnused(samplesPerBlock);
}

void CircuitsAudioProcessor::releaseResources()
{
}

bool CircuitsAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;

    return true;
}

void CircuitsAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                          juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused(midiMessages);

    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // Clear any output channels that don't have input
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    // Process audio through circuit
    auto* inputChannel = buffer.getReadPointer(0);
    auto* outputChannel = buffer.getWritePointer(0);
    
    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        float inputSample = inputChannel[sample];
        float outputSample = 0.0f;
        
        circuitEngine.processSample(inputSample, outputSample);
        outputChannel[sample] = outputSample;
        
        // Record voltage for oscilloscope
        if (probeNodeId >= 0)
        {
            voltageHistory[historyIndex] = static_cast<float>(circuitEngine.getNodeVoltage(probeNodeId));
            historyIndex = (historyIndex + 1) % HISTORY_SIZE;
        }
    }
    
    // Copy mono to stereo if needed
    if (totalNumOutputChannels > 1)
    {
        buffer.copyFrom(1, 0, buffer, 0, 0, buffer.getNumSamples());
    }
}

bool CircuitsAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* CircuitsAudioProcessor::createEditor()
{
    return new CircuitsAudioProcessorEditor(*this);
}

void CircuitsAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto json = CircuitSerializer::serialize(circuitGraph);
    destData.replaceAll(json.toRawUTF8(), json.getNumBytesAsUTF8());
}

void CircuitsAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    juce::String json(static_cast<const char*>(data), static_cast<size_t>(sizeInBytes));
    CircuitSerializer::deserialize(json, circuitGraph);
    circuitEngine.setCircuit(circuitGraph);
}

double CircuitsAudioProcessor::getNodeVoltage(int nodeId) const
{
    return circuitEngine.getNodeVoltage(nodeId);
}

const std::vector<float>& CircuitsAudioProcessor::getVoltageHistory(int nodeId) const
{
    juce::ignoreUnused(nodeId);
    return voltageHistory;
}

void CircuitsAudioProcessor::setProbeNode(int nodeId)
{
    probeNodeId = nodeId;
}

// Plugin instantiation
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new CircuitsAudioProcessor();
}

#pragma once

#include <JuceHeader.h>
#include "Engine/CircuitEngine.h"
#include "Engine/CircuitGraph.h"

class CircuitsAudioProcessor : public juce::AudioProcessor
{
public:
    CircuitsAudioProcessor();
    ~CircuitsAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    // Circuit access
    CircuitGraph& getCircuitGraph() { return circuitGraph; }
    CircuitEngine& getCircuitEngine() { return circuitEngine; }
    
    // Voltage probing for oscilloscope
    double getNodeVoltage(int nodeId) const;
    const std::vector<float>& getVoltageHistory(int nodeId) const;
    void setProbeNode(int nodeId);

private:
    CircuitGraph circuitGraph;
    CircuitEngine circuitEngine;
    
    double currentSampleRate = 44100.0;
    
    // Voltage history for oscilloscope
    int probeNodeId = -1;
    std::vector<float> voltageHistory;
    static constexpr size_t HISTORY_SIZE = 1024;
    size_t historyIndex = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CircuitsAudioProcessor)
};

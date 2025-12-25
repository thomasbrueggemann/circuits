#pragma once

#include "Engine/CircuitEngine.h"
#include "Engine/CircuitGraph.h"
#include <JuceHeader.h>

class CircuitsAudioProcessor : public juce::AudioProcessor {
public:
  CircuitsAudioProcessor();
  ~CircuitsAudioProcessor() override;

  void prepareToPlay(double sampleRate, int samplesPerBlock) override;
  void releaseResources() override;

  bool isBusesLayoutSupported(const BusesLayout &layouts) const override;

  void processBlock(juce::AudioBuffer<float> &, juce::MidiBuffer &) override;

  juce::AudioProcessorEditor *createEditor() override;
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
  void changeProgramName(int index, const juce::String &newName) override;

  void getStateInformation(juce::MemoryBlock &destData) override;
  void setStateInformation(const void *data, int sizeInBytes) override;

  // Circuit access
  CircuitGraph &getCircuitGraph() { return circuitGraph; }
  CircuitEngine &getCircuitEngine() { return circuitEngine; }

  // Voltage probing for oscilloscope
  double getNodeVoltage(int nodeId) const;
  void getLatestSamples(std::vector<float> &dest) const;
  void setProbeNode(int nodeId);
  int getProbeNodeId() const { return probeNodeId; }
  double getCurrentSampleRate() const { return currentSampleRate; }

private:
  CircuitGraph circuitGraph;
  CircuitEngine circuitEngine;

  double currentSampleRate = 44100.0;

  // Voltage history for oscilloscope
  int probeNodeId = -1;
  juce::CriticalSection historyLock;
  std::vector<float> voltageHistory;
  static constexpr size_t HISTORY_SIZE = 2048; // ~46ms at 44.1kHz for visible waveform cycles
  size_t historyIndex = 0;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CircuitsAudioProcessor)
};

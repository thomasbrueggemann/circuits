#pragma once

#include "CircuitGraph.h"
#include "MNASolver.h"

class CircuitEngine {
public:
  CircuitEngine();
  ~CircuitEngine();

  // Setup
  void setCircuit(const CircuitGraph &graph);
  void setSampleRate(double sampleRate);

  // Audio processing
  void processSample(float inputSample, float &outputSample);
  void processBlock(const float *input, float *output, int numSamples);

  // Parameters
  void setComponentValue(int componentId, double value);
  double getNodeVoltage(int nodeId) const;

  // Simulation control
  void setSimulationActive(bool active) { simulationActive = active; }
  bool isSimulationActive() const { return simulationActive; }

  // Oversampling factor for stability

private:
  MNASolver solver;
  const CircuitGraph *circuit = nullptr;

  double sampleRate = 44100.0;
  int oversamplingFactor = 2; // 2x oversampling for stability
  bool simulationActive = false;

  // DC blocking filter for output
  double dcBlockerState = 0.0;
  static constexpr double DC_BLOCKER_COEFF = 0.995;

  juce::CriticalSection renderLock;
};

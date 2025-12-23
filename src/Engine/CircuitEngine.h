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

  // Oversampling factor for stability
  void setOversamplingFactor(int factor) { oversamplingFactor = factor; }
  int getOversamplingFactor() const { return oversamplingFactor; }

private:
  MNASolver solver;
  const CircuitGraph *circuit = nullptr;

  double sampleRate = 44100.0;
  int oversamplingFactor = 2; // 2x oversampling for stability

  // DC blocking filter for output
  double dcBlockerState = 0.0;
  static constexpr double DC_BLOCKER_COEFF = 0.995;
};

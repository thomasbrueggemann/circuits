#pragma once

#include "CircuitGraph.h"
#include "WDF/WDFEngine.h"

/**
 * Circuit simulation engine using Wave Digital Filters.
 * 
 * WDF-based simulation provides:
 * - Guaranteed stability for linear circuits
 * - No matrix inversion (efficient real-time processing)
 * - Natural handling of reactive elements (C, L)
 * - Local nonlinear solving at root elements
 */
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
  bool isSimulationValid() const { return !wdfEngine.isSimulationFailed(); }

private:
  WDF::WDFEngine wdfEngine;
  const CircuitGraph *circuit = nullptr;

  double sampleRate = 44100.0;
  int oversamplingFactor = 2; // 2x oversampling for stability
  bool simulationActive = false;

  // DC blocking filter for output
  double dcBlockerState = 0.0;
  static constexpr double DC_BLOCKER_COEFF = 0.995;

  juce::CriticalSection renderLock;
};

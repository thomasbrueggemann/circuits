#include "CircuitEngine.h"

CircuitEngine::CircuitEngine() {}

CircuitEngine::~CircuitEngine() = default;

void CircuitEngine::setCircuit(const CircuitGraph &graph) {
  const juce::ScopedLock sl(renderLock);
  circuit = &graph;
  solver.setCircuit(graph);
}

void CircuitEngine::setSampleRate(double rate) {
  const juce::ScopedLock sl(renderLock);
  sampleRate = rate;
  // Set MNA solver sample rate with oversampling
  solver.setSampleRate(rate * oversamplingFactor);
}

void CircuitEngine::processSample(float inputSample, float &outputSample) {
  // Note: recursive lock is okay if called from processBlock
  const juce::ScopedLock sl(renderLock);

  if (!circuit || circuit->getComponentCount() == 0) {
    outputSample = inputSample; // Bypass if no circuit
    return;
  }

  double output = 0.0;

  // Oversample for stability
  for (int i = 0; i < oversamplingFactor; ++i) {
    // Scale input to typical voltage levels (±1V -> ±1V, but can be configured)
    double inputVoltage = static_cast<double>(inputSample);

    solver.step(inputVoltage);
    output = solver.getOutputVoltage();
  }

  // Average the oversampled result (simple decimation)
  output /= oversamplingFactor;

  // DC blocking filter
  double dcBlocked = output - dcBlockerState;
  dcBlockerState =
      output * (1.0 - DC_BLOCKER_COEFF) + dcBlockerState * DC_BLOCKER_COEFF;

  // Soft clipping to prevent nasty digital distortion
  if (dcBlocked > 1.0)
    dcBlocked = 1.0 - 1.0 / (dcBlocked + 1.0);
  else if (dcBlocked < -1.0)
    dcBlocked = -1.0 + 1.0 / (-dcBlocked + 1.0);

  outputSample = static_cast<float>(dcBlocked);
}

void CircuitEngine::processBlock(const float *input, float *output,
                                 int numSamples) {
  const juce::ScopedLock sl(renderLock);
  for (int i = 0; i < numSamples; ++i) {
    processSample(input[i], output[i]);
  }
}

void CircuitEngine::setComponentValue(int componentId, double value) {
  const juce::ScopedLock sl(renderLock);
  solver.updateComponentValue(componentId, value);
}

double CircuitEngine::getNodeVoltage(int nodeId) const {
  const juce::ScopedLock sl(renderLock);
  return solver.getNodeVoltage(nodeId);
}

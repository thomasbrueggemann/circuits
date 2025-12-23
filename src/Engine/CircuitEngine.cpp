#include "CircuitEngine.h"

CircuitEngine::CircuitEngine() {}

CircuitEngine::~CircuitEngine() = default;

void CircuitEngine::setCircuit(const CircuitGraph &graph) {
  circuit = &graph;
  solver.setCircuit(graph);
}

void CircuitEngine::setSampleRate(double rate) {
  sampleRate = rate;
  // Set MNA solver sample rate with oversampling
  solver.setSampleRate(rate * oversamplingFactor);
}

void CircuitEngine::processSample(float inputSample, float &outputSample) {
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
  for (int i = 0; i < numSamples; ++i) {
    processSample(input[i], output[i]);
  }
}

void CircuitEngine::setComponentValue(int componentId, double value) {
  solver.updateComponentValue(componentId, value);
}

double CircuitEngine::getNodeVoltage(int nodeId) const {
  return solver.getNodeVoltage(nodeId);
}

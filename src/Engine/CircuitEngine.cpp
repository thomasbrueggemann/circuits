#include "CircuitEngine.h"
#include "Components/AudioInput.h"

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
  const juce::ScopedLock sl(renderLock);

  if (!circuit || !simulationActive) {
    outputSample = 0.0f; // No output if simulation is off
    return;
  }

  // Update internal signal generators
  for (const auto &comp :
       circuit->getComponentsByType(ComponentType::AudioInput)) {
    auto *audioIn = static_cast<AudioInput *>(comp);
    if (audioIn->getSource() == SignalSource::DAV) {
      audioIn->setVoltage(static_cast<double>(inputSample));
    } else {
      audioIn->updateInternalVoltage(sampleRate * oversamplingFactor);
    }
  }

  double output = 0.0;

  // Oversample for stability
  for (int i = 0; i < oversamplingFactor; ++i) {
    // Note: AudioInput voltage is already scaled/set above
    solver.step(0.0); // We pass 0.0 because AudioInput components handle their
                      // own values
    output += solver.getOutputVoltage();
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

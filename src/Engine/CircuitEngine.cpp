#include "CircuitEngine.h"
#include "Components/AudioInput.h"
#include "Components/AudioOutput.h"

CircuitEngine::CircuitEngine() {}

CircuitEngine::~CircuitEngine() = default;

void CircuitEngine::setCircuit(const CircuitGraph &graph) {
  const juce::ScopedLock sl(renderLock);
  circuit = &graph;
  wdfEngine.setCircuit(graph);
}

void CircuitEngine::setSampleRate(double rate) {
  const juce::ScopedLock sl(renderLock);
  sampleRate = rate;
  // Set WDF engine sample rate with oversampling
  wdfEngine.setSampleRate(rate * oversamplingFactor);
}

void CircuitEngine::processSample(float inputSample, float &outputSample) {
  const juce::ScopedLock sl(renderLock);

  if (!circuit || !simulationActive) {
    outputSample = 0.0f; // No output if simulation is off
    return;
  }

  double output = 0.0;

  // Oversample for stability
  for (int i = 0; i < oversamplingFactor; ++i) {
    // Update internal signal generators for each oversample
    for (const auto &comp :
         circuit->getComponentsByType(ComponentType::AudioInput)) {
      auto *audioIn = static_cast<AudioInput *>(comp);
      if (audioIn->getSource() == SignalSource::DAW) {
        audioIn->setVoltage(static_cast<double>(inputSample));
      } else {
        audioIn->updateInternalVoltage(sampleRate * oversamplingFactor);
      }
    }

    // Get input voltage from the audio input component
    double inputVoltage = 0.0;
    for (const auto &comp :
         circuit->getComponentsByType(ComponentType::AudioInput)) {
      auto *audioIn = static_cast<AudioInput *>(comp);
      inputVoltage = audioIn->getScaledVoltage();
      break; // Use first audio input
    }

    // Process through WDF engine
    wdfEngine.step(inputVoltage);
    output += wdfEngine.getOutputVoltage();
  }

  // Average the oversampled result (simple decimation)
  output /= oversamplingFactor;

  // Get output gain from AudioOutput component
  double outputGain = 1.0;
  for (const auto &comp :
       circuit->getComponentsByType(ComponentType::AudioOutput)) {
    if (auto *audioOut = static_cast<AudioOutput *>(comp)) {
      outputGain = audioOut->getGain();
      break;
    }
  }
  output *= outputGain;

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
  wdfEngine.updateComponentValue(componentId, value);
}

double CircuitEngine::getNodeVoltage(int nodeId) const {
  const juce::ScopedLock sl(renderLock);
  return wdfEngine.getNodeVoltage(nodeId);
}

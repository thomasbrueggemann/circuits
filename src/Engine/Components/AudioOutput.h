#pragma once

#include "Component.h"

/**
 * AudioOutput - Voltage probe that sends signal to DAW output.
 *
 * Terminal 1 (positive): Probe point
 * Terminal 2 (negative): Usually ground reference
 *
 * This is a high-impedance probe (doesn't load the circuit).
 */
class AudioOutput : public CircuitComponent {
public:
  AudioOutput(int id, const juce::String &name, int nodePos, int nodeNeg)
      : CircuitComponent(ComponentType::AudioOutput, id, name, nodePos,
                         nodeNeg) {}

  // Get the output node for voltage measurement
  int getOutputNode() const { return getNode1(); }
  int getReferenceNode() const { return getNode2(); }

  // Output gain/scaling
  double getGain() const { return gain; }
  void setGain(double g) { gain = g; }

  juce::String getSymbol() const override { return "OUT"; }

  juce::String getValueString() const override { return "Audio Out"; }

private:
  double gain = 1.0; // Scale factor from voltage to audio
};

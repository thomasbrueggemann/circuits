#pragma once

#include "Component.h"

/**
 * AudioInput - Voltage source driven by DAW audio input.
 *
 * Terminal 1 (positive): Positive terminal
 * Terminal 2 (negative): Usually connected to ground
 *
 * The voltage is set by the audio processor for each sample.
 */
class AudioInput : public CircuitComponent {
public:
  AudioInput(int id, const juce::String &name, int nodePos, int nodeNeg)
      : CircuitComponent(ComponentType::AudioInput, id, name, nodePos,
                         nodeNeg) {}

  // Current voltage value (set by processor)
  double getVoltage() const { return voltage; }
  void setVoltage(double v) { voltage = v; }

  // Input gain/scaling
  double getGain() const { return gain; }
  void setGain(double g) { gain = g; }

  // Scaled voltage
  double getScaledVoltage() const { return voltage * gain; }

  juce::String getSymbol() const override { return "IN"; }

  juce::String getValueString() const override { return "Audio In"; }

private:
  double voltage = 0.0;
  double gain = 1.0; // Scale factor from audio (-1 to 1) to voltage
};

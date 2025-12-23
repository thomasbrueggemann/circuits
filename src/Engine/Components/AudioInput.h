#pragma once

#include "Component.h"
#include <algorithm>
#include <cmath>

/**
 * AudioInput - Voltage source driven by DAW audio input.
 *
 * Terminal 1 (positive): Positive terminal
 * Terminal 2 (negative): Usually connected to ground
 *
 * The voltage is set by the audio processor for each sample.
 */
enum class SignalSource { DAV, Sine, Square, Noise };

class AudioInput : public CircuitComponent {
public:
  AudioInput(int id, const juce::String &name, int nodePos, int nodeNeg)
      : CircuitComponent(ComponentType::AudioInput, id, name, nodePos,
                         nodeNeg) {}

  // Current voltage value (set by processor or internal generator)
  double getVoltage() const { return voltage; }
  void setVoltage(double v) { voltage = v; }

  // Signal Source control
  SignalSource getSource() const { return source; }
  void setSource(SignalSource s) { source = s; }

  // Frequency and Amplitude for internal generator
  double getFrequency() const { return frequency; }
  void setFrequency(double f) { frequency = f; }
  double getAmplitude() const { return amplitude; }
  void setAmplitude(double a) { amplitude = a; }

  // Input gain/scaling for DAW input
  double getGain() const { return gain; }
  void setGain(double g) { gain = g; }

  // Scaled voltage
  double getScaledVoltage() const { return voltage * gain; }

  // Generate next sample for internal generator
  void updateInternalVoltage(double sampleRate) {
    if (source == SignalSource::DAV)
      return;

    if (source == SignalSource::Sine) {
      voltage = amplitude * std::sin(phase);
      phase += 2.0 * juce::MathConstants<double>::pi * frequency / sampleRate;
    } else if (source == SignalSource::Square) {
      voltage = std::sin(phase) >= 0 ? amplitude : -amplitude;
      phase += 2.0 * juce::MathConstants<double>::pi * frequency / sampleRate;
    } else if (source == SignalSource::Noise) {
      voltage = (random.nextFloat() * 2.0f - 1.0f) * amplitude;
    }

    if (phase >= 2.0 * juce::MathConstants<double>::pi)
      phase -= 2.0 * juce::MathConstants<double>::pi;
  }

  juce::String getSymbol() const override { return "IN"; }

  juce::String getValueString() const override { return "Audio In"; }

private:
  double voltage = 0.0;
  double gain = 1.0; // Scale factor from audio (-1 to 1) to voltage

  SignalSource source = SignalSource::Sine; // Default to sine for easy testing
  double frequency = 440.0;
  double amplitude = 1.0;
  double phase = 0.0;
  juce::Random random;
};

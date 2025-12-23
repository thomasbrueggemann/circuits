#pragma once

#include <JuceHeader.h>
#include <vector>

/**
 * OscilloscopeView - Real-time waveform display for voltage probing.
 *
 * Click on a wire in the circuit to probe it and see the voltage waveform.
 */
class OscilloscopeView : public juce::Component, public juce::Timer {
public:
  OscilloscopeView();
  ~OscilloscopeView() override;

  void paint(juce::Graphics &g) override;
  void resized() override;
  void timerCallback() override;

  // Update waveform data
  void updateWaveform(const std::vector<float> &samples);

  // Probe state
  void setProbeActive(bool active);
  bool isProbeActive() const { return probeActive; }

  // Display settings
  void setTimeScale(float msPerDiv) { timeScale = msPerDiv; }
  void setVoltageScale(float vPerDiv) { voltageScale = vPerDiv; }
  void setAutoScale(bool enable) { autoScale = enable; }

private:
  // Waveform data
  std::vector<float> waveformBuffer;
  size_t writeIndex = 0;
  static constexpr size_t BUFFER_SIZE = 2048;

  // Display state
  bool probeActive = false;
  float timeScale = 10.0f;   // ms per division
  float voltageScale = 1.0f; // V per division
  bool autoScale = true;
  float autoScaleMax = 1.0f;

  // Grid settings
  static constexpr int GRID_DIVISIONS_X = 10;
  static constexpr int GRID_DIVISIONS_Y = 8;

  // Drawing
  void drawBackground(juce::Graphics &g, juce::Rectangle<float> bounds);
  void drawGrid(juce::Graphics &g, juce::Rectangle<float> bounds);
  void drawWaveform(juce::Graphics &g, juce::Rectangle<float> bounds);
  void drawLabels(juce::Graphics &g, juce::Rectangle<float> bounds);
  void drawNoSignalMessage(juce::Graphics &g, juce::Rectangle<float> bounds);

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OscilloscopeView)
};

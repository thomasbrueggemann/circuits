#pragma once

#include <JuceHeader.h>
#include <vector>

/**
 * OscilloscopeView - Real-time waveform and spectrum display for voltage probing.
 *
 * Click on a wire in the circuit to probe it and see the voltage waveform
 * or frequency spectrum (FFT).
 */
class OscilloscopeView : public juce::Component, public juce::Timer {
public:
  enum class DisplayMode { Waveform, Spectrum };

  OscilloscopeView();
  ~OscilloscopeView() override;

  void paint(juce::Graphics &g) override;
  void resized() override;
  void timerCallback() override;
  void mouseDown(const juce::MouseEvent &event) override;

  // Update waveform data
  void updateWaveform(const std::vector<float> &samples);

  // Probe state
  void setProbeActive(bool active);
  bool isProbeActive() const { return probeActive; }

  // Display settings
  void setDisplayMode(DisplayMode mode) { displayMode = mode; }
  DisplayMode getDisplayMode() const { return displayMode; }
  void toggleDisplayMode();
  
  void setTimeScale(float msPerDiv) { timeScale = msPerDiv; }
  void setVoltageScale(float vPerDiv) { voltageScale = vPerDiv; }
  void setAutoScale(bool enable) { autoScale = enable; }
  void setSimulationRunning(bool running) { simulationRunning = running; }
  void setSimulationValid(bool valid) { simulationValid = valid; }
  void setNodeInfo(int probeId, int totalNodes) {
    lastProbeNodeId = probeId;
    lastNodeCount = totalNodes;
  }
  void setSampleRate(double rate) { sampleRate = rate; }

private:
  // FFT settings
  static constexpr int FFT_ORDER = 11;  // 2^11 = 2048 points
  static constexpr int FFT_SIZE = 1 << FFT_ORDER;
  
  // Display mode
  DisplayMode displayMode = DisplayMode::Spectrum;  // Default to spectrum for frequency analysis
  
  // Waveform data
  std::vector<float> waveformBuffer;
  size_t writeIndex = 0;
  static constexpr size_t BUFFER_SIZE = 2048;
  
  // FFT data
  juce::dsp::FFT fft;
  juce::dsp::WindowingFunction<float> window;
  std::array<float, FFT_SIZE * 2> fftData;
  std::array<float, FFT_SIZE / 2> spectrumData;
  std::array<float, FFT_SIZE / 2> smoothedSpectrum;
  double sampleRate = 44100.0;

  // Display state
  bool probeActive = false;
  float timeScale = 10.0f;   // ms per division
  float voltageScale = 1.0f; // V per division
  bool autoScale = true;
  float autoScaleMax = 1.0f;
  bool simulationRunning = false;
  bool simulationValid = true;

  // Diagnostics
  uint32_t logicHeartbeat = 0;
  std::vector<float> lastSampleBatch;
  int lastProbeNodeId = -1;
  int lastNodeCount = 0;

  // Grid settings
  static constexpr int GRID_DIVISIONS_X = 10;
  static constexpr int GRID_DIVISIONS_Y = 8;

  // Mode toggle button bounds
  juce::Rectangle<float> modeButtonBounds;

  // Drawing
  void drawBackground(juce::Graphics &g, juce::Rectangle<float> bounds);
  void drawGrid(juce::Graphics &g, juce::Rectangle<float> bounds);
  void drawWaveform(juce::Graphics &g, juce::Rectangle<float> bounds);
  void drawSpectrum(juce::Graphics &g, juce::Rectangle<float> bounds);
  void drawSpectrumGrid(juce::Graphics &g, juce::Rectangle<float> bounds);
  void drawLabels(juce::Graphics &g, juce::Rectangle<float> bounds);
  void drawModeButton(juce::Graphics &g, juce::Rectangle<float> bounds);
  void drawNoSignalMessage(juce::Graphics &g, juce::Rectangle<float> bounds);
  
  // FFT processing
  void computeFFT();
  float frequencyToX(float freq, float width) const;
  float dbToY(float db, float height) const;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OscilloscopeView)
};

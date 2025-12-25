#pragma once

#include "../Engine/CircuitEngine.h"
#include "../Engine/CircuitGraph.h"
#include <JuceHeader.h>

class TopBar : public juce::Component, public juce::Timer {
public:
  TopBar(CircuitGraph &graph, CircuitEngine &engine);
  ~TopBar() override;

  void paint(juce::Graphics &g) override;
  void resized() override;
  void timerCallback() override;

  // Callback when circuit is loaded from file
  std::function<void()> onCircuitLoaded;

private:
  CircuitGraph &circuitGraph;
  CircuitEngine &circuitEngine;

  // File controls
  std::unique_ptr<juce::TextButton> loadButton;
  std::unique_ptr<juce::TextButton> saveButton;

  // Simulation controls
  std::unique_ptr<juce::TextButton> startButton;
  std::unique_ptr<juce::TextButton> stopButton;
  std::unique_ptr<juce::Label> statusLabel;

  // Generator controls
  std::unique_ptr<juce::ComboBox> signalSourceSelector;
  std::unique_ptr<juce::Slider> frequencySlider;
  std::unique_ptr<juce::Slider> amplitudeSlider;
  std::unique_ptr<juce::Label> frequencyLabel;
  std::unique_ptr<juce::Label> amplitudeLabel;
  std::unique_ptr<juce::Label> genHeaderLabel;

  void updateButtonStates();
  void loadCircuit();
  void saveCircuit();
  class AudioInput *getFirstInput();

  std::unique_ptr<juce::FileChooser> fileChooser;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TopBar)
};

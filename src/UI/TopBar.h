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

private:
  CircuitGraph &circuitGraph;
  CircuitEngine &circuitEngine;

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
  class AudioInput *getFirstInput();

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TopBar)
};

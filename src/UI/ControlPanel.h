#pragma once

#include "../Engine/CircuitEngine.h"
#include "../Engine/CircuitGraph.h"
#include <JuceHeader.h>

/**
 * ControlPanel - Auto-generated control panel with knobs and switches.
 *
 * Automatically creates:
 * - Rotary knobs for each Potentiometer in the circuit
 * - Toggle switches for each Switch in the circuit
 */
class ControlPanel : public juce::Component {
public:
  ControlPanel(CircuitGraph &graph, CircuitEngine &engine);
  ~ControlPanel() override;

  void paint(juce::Graphics &g) override;
  void resized() override;

  // Rebuild controls when circuit changes
  void rebuildControls();

private:
  CircuitGraph &circuitGraph;
  CircuitEngine &circuitEngine;

  // Knob for potentiometers
  struct KnobControl {
    std::unique_ptr<juce::Slider> slider;
    std::unique_ptr<juce::Label> label;
    int componentId;
  };

  // Switch control
  struct SwitchControl {
    std::unique_ptr<juce::ToggleButton> toggle;
    int componentId;
  };

  std::vector<KnobControl> knobs;
  std::vector<SwitchControl> switches;

  // Simulation controls
  std::unique_ptr<juce::ToggleButton> powerButton;
  std::unique_ptr<juce::ComboBox> signalSourceSelector;
  std::unique_ptr<juce::Slider> frequencySlider;
  std::unique_ptr<juce::Slider> amplitudeSlider;
  std::unique_ptr<juce::Label> frequencyLabel;
  std::unique_ptr<juce::Label> amplitudeLabel;

  void createKnob(int componentId, const juce::String &name, double minVal,
                  double maxVal, double defaultVal);
  void createSwitch(int componentId, const juce::String &name,
                    bool defaultState);

  // Custom look and feel for vintage knobs
  class VintageLookAndFeel : public juce::LookAndFeel_V4 {
  public:
    void drawRotarySlider(juce::Graphics &g, int x, int y, int width,
                          int height, float sliderPosProportional,
                          float rotaryStartAngle, float rotaryEndAngle,
                          juce::Slider &slider) override;

    void drawToggleButton(juce::Graphics &g, juce::ToggleButton &button,
                          bool shouldDrawButtonAsHighlighted,
                          bool shouldDrawButtonAsDown) override;
  };

  VintageLookAndFeel vintageLook;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ControlPanel)
};

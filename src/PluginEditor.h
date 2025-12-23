#pragma once

#include "PluginProcessor.h"
#include "UI/CircuitDesigner.h"
#include "UI/ComponentPalette.h"
#include "UI/ControlPanel.h"
#include "UI/OscilloscopeView.h"
#include "UI/TopBar.h"
#include <JuceHeader.h>

class CircuitsAudioProcessorEditor : public juce::AudioProcessorEditor,
                                     public juce::DragAndDropContainer,
                                     public juce::Timer {
public:
  explicit CircuitsAudioProcessorEditor(CircuitsAudioProcessor &);
  ~CircuitsAudioProcessorEditor() override;

  void paint(juce::Graphics &) override;
  void resized() override;
  void timerCallback() override;

  void updateControlPanel();
  void autoProbe();

private:
  CircuitsAudioProcessor &audioProcessor;

  // UI Components
  std::unique_ptr<ComponentPalette> componentPalette;
  std::unique_ptr<CircuitDesigner> circuitDesigner;
  std::unique_ptr<ControlPanel> controlPanel;
  std::unique_ptr<OscilloscopeView> oscilloscopeView;
  std::unique_ptr<TopBar> topBar;

  // Layout
  static constexpr int TOP_BAR_HEIGHT = 60;
  static constexpr int PALETTE_WIDTH = 80;
  static constexpr int CONTROL_PANEL_WIDTH = 200;
  static constexpr int OSCILLOSCOPE_HEIGHT = 150;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CircuitsAudioProcessorEditor)
};

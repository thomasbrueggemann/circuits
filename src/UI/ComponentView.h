#pragma once

#include "../Engine/CircuitGraph.h"
#include <JuceHeader.h>

class CircuitComponent;

/**
 * ComponentView - Visual representation of a component on the canvas.
 *
 * Handles drawing, selection, and terminal positions.
 */
class ComponentView {
public:
  ComponentView(CircuitComponent *component, CircuitGraph &graph);
  ~ComponentView();

  // Drawing
  void draw(juce::Graphics &g, const juce::AffineTransform &transform,
            float zoom);

  // Position
  juce::Point<float> getCanvasPosition() const { return canvasPosition; }
  void setCanvasPosition(juce::Point<float> pos) { canvasPosition = pos; }

  // Bounds for hit testing
  juce::Rectangle<float> getBoundsInCanvas() const;

  // Selection
  bool isSelected() const { return selected; }
  void setSelected(bool sel) { selected = sel; }

  // Get terminal positions in canvas coordinates
  std::vector<std::pair<int, juce::Point<float>>> getTerminalPositions() const;

  // Component access
  CircuitComponent *getComponent() { return component; }
  const CircuitComponent *getComponent() const { return component; }

  // Value editing
  void showValueEditor();

private:
  CircuitComponent *component;
  CircuitGraph &circuitGraph;

  juce::Point<float> canvasPosition;
  bool selected = false;

  // Component dimensions
  static constexpr float WIDTH = 60.0f;
  static constexpr float HEIGHT = 40.0f;

  // Drawing helpers
  void drawResistor(juce::Graphics &g, juce::Rectangle<float> bounds);
  void drawCapacitor(juce::Graphics &g, juce::Rectangle<float> bounds);
  void drawInductor(juce::Graphics &g, juce::Rectangle<float> bounds);
  void drawPotentiometer(juce::Graphics &g, juce::Rectangle<float> bounds);
  void drawSwitch(juce::Graphics &g, juce::Rectangle<float> bounds);
  void drawDiode(juce::Graphics &g, juce::Rectangle<float> bounds);
  void drawDiodePair(juce::Graphics &g, juce::Rectangle<float> bounds);
  void drawSoftClipper(juce::Graphics &g, juce::Rectangle<float> bounds);
  void drawVacuumTube(juce::Graphics &g, juce::Rectangle<float> bounds);
  void drawAudioInput(juce::Graphics &g, juce::Rectangle<float> bounds);
  void drawAudioOutput(juce::Graphics &g, juce::Rectangle<float> bounds);
  void drawGround(juce::Graphics &g, juce::Rectangle<float> bounds);
};

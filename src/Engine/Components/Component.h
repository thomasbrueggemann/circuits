#pragma once

#include "../CircuitGraph.h"
#include <JuceHeader.h>

class MNAMatrix;

/**
 * Base class for all circuit components.
 * Named CircuitComponent to avoid collision with juce::Component.
 */
class CircuitComponent {
public:
  CircuitComponent(ComponentType type, int id, const juce::String &name,
                   int node1, int node2)
      : componentType(type), componentId(id), componentName(name),
        terminalNode1(node1), terminalNode2(node2) {}

  virtual ~CircuitComponent() = default;

  // Component properties
  int getId() const { return componentId; }
  void setId(int id) { componentId = id; }

  const juce::String &getName() const { return componentName; }
  void setName(const juce::String &name) { componentName = name; }

  ComponentType getType() const { return componentType; }

  // Terminal nodes
  int getNode1() const { return terminalNode1; }
  int getNode2() const { return terminalNode2; }
  void setNode1(int node) { terminalNode1 = node; }
  void setNode2(int node) { terminalNode2 = node; }

  // Get all nodes connected to this component (for deletion)
  virtual std::vector<int> getAllNodes() const {
    return {terminalNode1, terminalNode2};
  }

  // Position for UI
  juce::Point<float> getPosition() const { return position; }
  void setPosition(juce::Point<float> pos) { position = pos; }

  // Rotation (0, 90, 180, 270 degrees)
  int getRotation() const { return rotation; }
  void setRotation(int rot) { rotation = rot % 360; }

  // Component value (resistance, capacitance, etc.)
  virtual double getValue() const { return value; }
  virtual void setValue(double val) { value = val; }

  // Selection state for UI
  bool isSelected() const { return selected; }
  void setSelected(bool sel) { selected = sel; }

  // Nonlinear components need iterative solving
  virtual bool isNonLinear() const { return false; }

  // Get component symbol for display
  virtual juce::String getSymbol() const = 0;

  // Get formatted value string
  virtual juce::String getValueString() const { return juce::String(value); }

protected:
  ComponentType componentType;
  int componentId = -1;
  juce::String componentName;

  int terminalNode1 = -1;
  int terminalNode2 = -1;

  juce::Point<float> position{0.0f, 0.0f};
  int rotation = 0;

  double value = 0.0;
  bool selected = false;
};

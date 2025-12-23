#pragma once

#include "Component.h"

class Ground : public CircuitComponent {
public:
  Ground(int id, const juce::String &name, int node)
      : CircuitComponent(ComponentType::Ground, id, name, node, -1) {}

  ~Ground() override = default;

  juce::String getSymbol() const override { return "GND"; }

  // Ground has no value to display
  juce::String getValueString() const override { return ""; }
};

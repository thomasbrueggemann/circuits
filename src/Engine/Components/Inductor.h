#pragma once

#include "Component.h"

class Inductor : public CircuitComponent {
public:
  Inductor(int id, const juce::String &name, int node1, int node2,
           double inductance = 100e-3)
      : CircuitComponent(ComponentType::Inductor, id, name, node1, node2) {
    value = inductance;
  }

  double getInductance() const { return value; }
  void setInductance(double l) {
    value = std::max(1e-9, l);
  } // Minimum 1nH

  juce::String getSymbol() const override { return "L"; }

  juce::String getValueString() const override {
    if (value >= 1.0)
      return juce::String(value, 2) + " H";
    else if (value >= 1e-3)
      return juce::String(value * 1e3, 2) + " mH";
    else if (value >= 1e-6)
      return juce::String(value * 1e6, 2) + " µH";
    else
      return juce::String(value * 1e9, 1) + " nH";
  }
};


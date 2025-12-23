#pragma once

#include "Component.h"

class Resistor : public CircuitComponent {
public:
  Resistor(int id, const juce::String &name, int node1, int node2,
           double resistance = 1000.0)
      : CircuitComponent(ComponentType::Resistor, id, name, node1, node2) {
    value = resistance;
  }

  double getResistance() const { return value; }
  void setResistance(double r) {
    value = std::max(0.01, r);
  } // Minimum 0.01 ohm

  juce::String getSymbol() const override { return "R"; }

  juce::String getValueString() const override {
    if (value >= 1e6)
      return juce::String(value / 1e6, 2) + " MΩ";
    else if (value >= 1e3)
      return juce::String(value / 1e3, 2) + " kΩ";
    else
      return juce::String(value, 1) + " Ω";
  }
};

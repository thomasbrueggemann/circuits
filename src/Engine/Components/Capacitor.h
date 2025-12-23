#pragma once

#include "Component.h"

class Capacitor : public CircuitComponent {
public:
  Capacitor(int id, const juce::String &name, int node1, int node2,
            double capacitance = 100e-9)
      : CircuitComponent(ComponentType::Capacitor, id, name, node1, node2) {
    value = capacitance;
  }

  double getCapacitance() const { return value; }
  void setCapacitance(double c) { value = std::max(1e-12, c); } // Minimum 1pF

  juce::String getSymbol() const override { return "C"; }

  juce::String getValueString() const override {
    if (value >= 1e-6)
      return juce::String(value * 1e6, 2) + " µF";
    else if (value >= 1e-9)
      return juce::String(value * 1e9, 2) + " nF";
    else
      return juce::String(value * 1e12, 1) + " pF";
  }

  // State for trapezoidal integration
  double getPreviousVoltage() const { return previousVoltage; }
  void setPreviousVoltage(double v) { previousVoltage = v; }

  double getPreviousCurrent() const { return previousCurrent; }
  void setPreviousCurrent(double i) { previousCurrent = i; }

private:
  double previousVoltage = 0.0;
  double previousCurrent = 0.0;
};

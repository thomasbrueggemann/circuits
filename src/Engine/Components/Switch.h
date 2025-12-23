#pragma once

#include "Component.h"

/**
 * Switch - Simple on/off switch between two nodes.
 *
 * When closed: Very low resistance (0.001 ohm)
 * When open: Very high resistance (1 gigaohm)
 */
class Switch : public CircuitComponent {
public:
  Switch(int id, const juce::String &name, int node1, int node2)
      : CircuitComponent(ComponentType::Switch, id, name, node1, node2) {}

  // State
  bool isClosed() const { return closed; }
  void setClosed(bool c) { closed = c; }
  void toggle() { closed = !closed; }

  // Equivalent resistance
  double getResistance() const {
    return closed ? CLOSED_RESISTANCE : OPEN_RESISTANCE;
  }

  // Switch type (for visual representation)
  enum class Type { SPST, SPDT, Momentary };

  Type getSwitchType() const { return switchType; }
  void setSwitchType(Type t) { switchType = t; }

  juce::String getSymbol() const override { return "SW"; }

  juce::String getValueString() const override { return closed ? "ON" : "OFF"; }

private:
  bool closed = false;
  Type switchType = Type::SPST;

  static constexpr double CLOSED_RESISTANCE = 0.001; // 1 milliohm
  static constexpr double OPEN_RESISTANCE = 1e9;     // 1 gigaohm
};

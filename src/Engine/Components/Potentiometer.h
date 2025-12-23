#pragma once

#include "Component.h"

/**
 * Potentiometer - Three terminal variable resistor.
 *
 * Terminal 1 (node1): One end of the resistor
 * Terminal 2 (node2): Other end of the resistor
 * Terminal 3 (node3): Wiper (tap point)
 *
 * Position 0.0 = wiper at node1 (R between wiper and node2 = totalR)
 * Position 1.0 = wiper at node2 (R between wiper and node1 = totalR)
 */
class Potentiometer : public CircuitComponent {
public:
  Potentiometer(int id, const juce::String &name, int node1, int node2,
                int node3, double totalResistance = 10000.0)
      : CircuitComponent(ComponentType::Potentiometer, id, name, node1, node2),
        wiperNode(node3), totalR(totalResistance) {
    value = totalResistance;
  }

  // Third terminal (wiper)
  int getNode3() const { return wiperNode; }
  void setNode3(int node) { wiperNode = node; }

  // Total resistance end-to-end
  double getTotalResistance() const { return totalR; }
  void setTotalResistance(double r) {
    totalR = std::max(100.0, r);
    value = totalR;
  }

  // Wiper position (0.0 to 1.0)
  double getWiperPosition() const { return wiperPosition; }
  void setWiperPosition(double pos) {
    wiperPosition = juce::jlimit(0.0, 1.0, pos);
  }

  // Resistance from node1 to wiper
  double getResistance1() const {
    return std::max(0.01, totalR * wiperPosition);
  }

  // Resistance from wiper to node2
  double getResistance2() const {
    return std::max(0.01, totalR * (1.0 - wiperPosition));
  }

  // Taper type for audio/linear response
  enum class Taper { Linear, Logarithmic, ReverseLogarithmic };

  Taper getTaper() const { return taper; }
  void setTaper(Taper t) { taper = t; }

  // Get effective position with taper applied
  double getEffectivePosition() const {
    switch (taper) {
    case Taper::Logarithmic:
      // Audio taper: slow at start, fast at end
      return std::pow(wiperPosition, 2.0);
    case Taper::ReverseLogarithmic:
      return std::sqrt(wiperPosition);
    default:
      return wiperPosition;
    }
  }

  juce::String getSymbol() const override { return "POT"; }

  juce::String getValueString() const override {
    juce::String taperStr;
    switch (taper) {
    case Taper::Linear:
      taperStr = "B";
      break;
    case Taper::Logarithmic:
      taperStr = "A";
      break;
    case Taper::ReverseLogarithmic:
      taperStr = "C";
      break;
    }

    if (totalR >= 1e6)
      return juce::String(totalR / 1e6, 1) + "M" + taperStr;
    else if (totalR >= 1e3)
      return juce::String(totalR / 1e3, 0) + "k" + taperStr;
    else
      return juce::String(static_cast<int>(totalR)) + taperStr;
  }

private:
  int wiperNode = -1;
  double totalR = 10000.0;
  double wiperPosition = 0.5; // Default to middle
  Taper taper = Taper::Linear;
};

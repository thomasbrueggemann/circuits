#pragma once

#include "Component.h"
#include <cmath>

/**
 * Diode - Semiconductor diode using Shockley equation.
 *
 * Terminals:
 *   Node 1 (anode): Positive terminal
 *   Node 2 (cathode): Negative terminal
 *
 * Current flows from anode to cathode when forward biased.
 *
 * Common diode types:
 *   1N4148: Small signal diode
 *   1N4001: General purpose rectifier
 *   1N34A: Germanium diode (lower forward voltage)
 *   LED: Light emitting diode (higher forward voltage)
 */
class Diode : public CircuitComponent {
public:
  enum class DiodeType {
    Type1N4148,  // Small signal silicon
    Type1N4001,  // General purpose rectifier
    Type1N34A,   // Germanium
    LED,         // Light emitting diode
    Custom
  };

  Diode(int id, const juce::String &name, int anodeNode, int cathodeNode)
      : CircuitComponent(ComponentType::Diode, id, name, anodeNode, cathodeNode) {
    setDiodeType(DiodeType::Type1N4148);
  }

  void setDiodeType(DiodeType type) {
    diodeType = type;

    switch (type) {
    case DiodeType::Type1N4148:
      saturationCurrent = 2.52e-9;
      emissionCoefficient = 1.752;
      break;
    case DiodeType::Type1N4001:
      saturationCurrent = 14.11e-9;
      emissionCoefficient = 1.984;
      break;
    case DiodeType::Type1N34A:
      saturationCurrent = 200e-9;  // Germanium has higher Is
      emissionCoefficient = 1.3;
      break;
    case DiodeType::LED:
      saturationCurrent = 1e-18;
      emissionCoefficient = 2.0;
      break;
    case DiodeType::Custom:
      // Keep current values
      break;
    }
  }

  DiodeType getDiodeType() const { return diodeType; }

  // Shockley parameters
  double getSaturationCurrent() const { return saturationCurrent; }
  void setSaturationCurrent(double is) {
    saturationCurrent = std::max(1e-20, is);
    diodeType = DiodeType::Custom;
  }

  double getEmissionCoefficient() const { return emissionCoefficient; }
  void setEmissionCoefficient(double n) {
    emissionCoefficient = std::max(1.0, std::min(3.0, n));
    diodeType = DiodeType::Custom;
  }

  /**
   * Calculate diode current using Shockley equation.
   * I = Is * (exp(V / (n * Vt)) - 1)
   */
  double calculateCurrent(double voltage) const {
    const double Vt = 0.02585;  // Thermal voltage at room temp (~26mV)
    double nVt = emissionCoefficient * Vt;
    
    if (voltage > 0.7) {
      // Linear extrapolation for high voltages to prevent overflow
      double Iref = saturationCurrent * (std::exp(0.7 / nVt) - 1.0);
      double gRef = saturationCurrent / nVt * std::exp(0.7 / nVt);
      return Iref + gRef * (voltage - 0.7);
    }
    return saturationCurrent * (std::exp(voltage / nVt) - 1.0);
  }

  bool isNonLinear() const override { return true; }

  juce::String getSymbol() const override { return "D"; }

  juce::String getValueString() const override {
    switch (diodeType) {
    case DiodeType::Type1N4148:
      return "1N4148";
    case DiodeType::Type1N4001:
      return "1N4001";
    case DiodeType::Type1N34A:
      return "1N34A";
    case DiodeType::LED:
      return "LED";
    case DiodeType::Custom:
      return "Custom";
    }
    return "Diode";
  }

private:
  DiodeType diodeType = DiodeType::Type1N4148;
  double saturationCurrent = 2.52e-9;
  double emissionCoefficient = 1.752;
};


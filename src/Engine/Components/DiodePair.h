#pragma once

#include "Component.h"
#include <cmath>

/**
 * DiodePair - Anti-parallel diode pair for symmetric clipping.
 *
 * Two diodes connected in opposite directions between terminals.
 * Used for symmetric soft clipping in overdrive/distortion circuits.
 *
 * Terminals:
 *   Node 1: First terminal
 *   Node 2: Second terminal
 *
 * Types:
 *   Silicon: Standard silicon diodes (1N4148-like)
 *   Germanium: Lower forward voltage, softer clipping
 *   LED: Higher forward voltage, harder clipping
 *   Asymmetric: Different diode types for asymmetric clipping
 */
class DiodePair : public CircuitComponent {
public:
  enum class PairType {
    Silicon,     // Two silicon diodes
    Germanium,   // Two germanium diodes
    LED,         // Two LEDs
    Asymmetric,  // Silicon + Germanium for asymmetric clipping
    Custom
  };

  DiodePair(int id, const juce::String &name, int node1, int node2)
      : CircuitComponent(ComponentType::DiodePair, id, name, node1, node2) {
    setPairType(PairType::Silicon);
  }

  void setPairType(PairType type) {
    pairType = type;

    switch (type) {
    case PairType::Silicon:
      saturationCurrent = 2.52e-9;
      emissionCoefficient = 1.752;
      break;
    case PairType::Germanium:
      saturationCurrent = 200e-9;
      emissionCoefficient = 1.3;
      break;
    case PairType::LED:
      saturationCurrent = 1e-18;
      emissionCoefficient = 2.0;
      break;
    case PairType::Asymmetric:
      // Average of silicon and germanium
      saturationCurrent = 50e-9;
      emissionCoefficient = 1.5;
      break;
    case PairType::Custom:
      break;
    }
  }

  PairType getPairType() const { return pairType; }

  double getSaturationCurrent() const { return saturationCurrent; }
  void setSaturationCurrent(double is) {
    saturationCurrent = std::max(1e-20, is);
    pairType = PairType::Custom;
  }

  double getEmissionCoefficient() const { return emissionCoefficient; }
  void setEmissionCoefficient(double n) {
    emissionCoefficient = std::max(1.0, std::min(3.0, n));
    pairType = PairType::Custom;
  }

  /**
   * Calculate current through anti-parallel diode pair.
   * I = 2 * Is * sinh(V / (n * Vt))
   */
  double calculateCurrent(double voltage) const {
    const double Vt = 0.02585;
    double nVt = emissionCoefficient * Vt;
    double x = voltage / nVt;
    
    if (std::abs(x) > 25.0) {
      // Linear extrapolation to prevent overflow
      double sign = (x > 0) ? 1.0 : -1.0;
      double Iref = 2.0 * saturationCurrent * std::sinh(25.0 * sign);
      double gRef = 2.0 * saturationCurrent / nVt * std::cosh(25.0);
      return Iref + gRef * (voltage - 25.0 * nVt * sign);
    }
    return 2.0 * saturationCurrent * std::sinh(x);
  }

  bool isNonLinear() const override { return true; }

  juce::String getSymbol() const override { return "DP"; }

  juce::String getValueString() const override {
    switch (pairType) {
    case PairType::Silicon:
      return "Silicon";
    case PairType::Germanium:
      return "Germanium";
    case PairType::LED:
      return "LED";
    case PairType::Asymmetric:
      return "Asymmetric";
    case PairType::Custom:
      return "Custom";
    }
    return "Diode Pair";
  }

private:
  PairType pairType = PairType::Silicon;
  double saturationCurrent = 2.52e-9;
  double emissionCoefficient = 1.752;
};


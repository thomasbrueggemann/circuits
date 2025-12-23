#pragma once

#include "Component.h"
#include <cmath>

/**
 * VacuumTube - Triode vacuum tube model using Koren equations.
 *
 * Terminals:
 *   Node 1 (grid): Control grid
 *   Node 2 (cathode): Cathode
 *   Node 3 (plate): Plate/Anode
 *
 * Common tube types:
 *   12AX7: High-gain preamp tube (mu=100)
 *   12AT7: Medium-gain (mu=60)
 *   12AU7: Low-gain (mu=20)
 *   EL34: Power tube (mu=11, higher current)
 */
class VacuumTube : public CircuitComponent {
public:
  VacuumTube(int id, const juce::String &name, int gridNode, int cathodeNode,
             int plateNode)
      : CircuitComponent(ComponentType::VacuumTube, id, name, gridNode,
                         cathodeNode),
        plateNodeId(plateNode) {
    // Default to 12AX7 parameters
    setTubeType(TubeType::Type12AX7);
  }

  // Triode parameters
  enum class TubeType {
    Type12AX7, // High-gain preamp
    Type12AT7, // Medium-gain
    Type12AU7, // Low-gain
    TypeEL34,  // Power tube
    Custom
  };

  void setTubeType(TubeType type) {
    tubeType = type;

    switch (type) {
    case TubeType::Type12AX7:
      mu = 100.0;
      kp = 600.0;
      kvb = 300.0;
      kg1 = 1060.0;
      ex = 1.4;
      break;
    case TubeType::Type12AT7:
      mu = 60.0;
      kp = 300.0;
      kvb = 300.0;
      kg1 = 460.0;
      ex = 1.35;
      break;
    case TubeType::Type12AU7:
      mu = 20.0;
      kp = 84.0;
      kvb = 300.0;
      kg1 = 1180.0;
      ex = 1.3;
      break;
    case TubeType::TypeEL34:
      mu = 11.0;
      kp = 60.0;
      kvb = 24.0;
      kg1 = 650.0;
      ex = 1.35;
      break;
    case TubeType::Custom:
      // Keep current values
      break;
    }
  }

  TubeType getTubeType() const { return tubeType; }

  // Plate node (third terminal)
  int getPlateNode() const { return plateNodeId; }
  void setPlateNode(int node) { plateNodeId = node; }

  std::vector<int> getAllNodes() const override {
    return {terminalNode1, terminalNode2, plateNodeId};
  }

  // Koren parameters
  double getMu() const { return mu; }
  void setMu(double m) {
    mu = m;
    tubeType = TubeType::Custom;
  }

  double getKp() const { return kp; }
  void setKp(double k) {
    kp = k;
    tubeType = TubeType::Custom;
  }

  double getKvb() const { return kvb; }
  void setKvb(double k) {
    kvb = k;
    tubeType = TubeType::Custom;
  }

  double getKg1() const { return kg1; }
  void setKg1(double k) {
    kg1 = k;
    tubeType = TubeType::Custom;
  }

  /**
   * Calculate plate current using Koren equations.
   *
   * @param vgk Grid-to-cathode voltage
   * @param vpk Plate-to-cathode voltage
   * @return Plate current in amperes
   */
  double calculatePlateCurrent(double vgk, double vpk) const {
    if (vpk <= 0.0)
      return 0.0;

    // Koren model E1 calculation
    double sqrtTerm = std::sqrt(kvb + vpk * vpk);
    double logArg = 1.0 + std::exp(kp * (1.0 / mu + vgk / sqrtTerm));
    double E1 = (vpk / kp) * std::log(logArg);

    if (E1 <= 0.0)
      return 0.0;

    // Plate current
    return std::pow(E1, ex) / kg1;
  }

  /**
   * Calculate derivatives for Newton-Raphson linearization.
   *
   * @param vgk Grid-to-cathode voltage
   * @param vpk Plate-to-cathode voltage
   * @param ip Output: plate current
   * @param gm Output: transconductance (dIp/dVgk)
   * @param gp Output: plate conductance (dIp/dVpk)
   */
  void calculateDerivatives(double vgk, double vpk, double &ip, double &gm,
                            double &gp) const {
    const double h = 1e-6; // Small delta for numerical derivatives

    ip = calculatePlateCurrent(vgk, vpk);

    // Transconductance (dIp/dVgk)
    double ip_gm_plus = calculatePlateCurrent(vgk + h, vpk);
    gm = (ip_gm_plus - ip) / h;

    // Plate conductance (dIp/dVpk)
    double ip_gp_plus = calculatePlateCurrent(vgk, vpk + h);
    gp = (ip_gp_plus - ip) / h;

    // Ensure positive conductances for stability
    gm = std::max(gm, 1e-9);
    gp = std::max(gp, 1e-9);
  }

  bool isNonLinear() const override { return true; }

  juce::String getSymbol() const override { return "V"; }

  juce::String getValueString() const override {
    switch (tubeType) {
    case TubeType::Type12AX7:
      return "12AX7";
    case TubeType::Type12AT7:
      return "12AT7";
    case TubeType::Type12AU7:
      return "12AU7";
    case TubeType::TypeEL34:
      return "EL34";
    case TubeType::Custom:
      return "Custom";
    }
    return "Tube";
  }

private:
  int plateNodeId = -1;
  TubeType tubeType = TubeType::Type12AX7;

  // Koren model parameters
  double mu = 100.0;   // Amplification factor
  double kp = 600.0;   // Koren parameter
  double kvb = 300.0;  // Knee voltage parameter
  double kg1 = 1060.0; // Kg1 parameter
  double ex = 1.4;     // Exponent
};

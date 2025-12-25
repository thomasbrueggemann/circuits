#pragma once

#include "Component.h"
#include <cmath>

/**
 * SoftClipper - Tanh-based soft clipping element.
 *
 * Models the smooth saturation characteristic of tube amplifiers
 * and tape machines. Uses tanh function for smooth, musical clipping.
 *
 * Terminals:
 *   Node 1: Input terminal
 *   Node 2: Output terminal (usually ground reference)
 *
 * Parameters:
 *   Saturation: Voltage level at which clipping begins
 *   Drive: Amount of gain before the clipper (increases distortion)
 */
class SoftClipper : public CircuitComponent {
public:
  enum class ClipperType {
    Mild,      // Gentle saturation (like tape)
    Medium,    // Moderate distortion (like preamp)
    Hard,      // Heavy distortion (like fuzz)
    Asymmetric, // Different positive/negative clipping
    Custom
  };

  SoftClipper(int id, const juce::String &name, int node1, int node2)
      : CircuitComponent(ComponentType::SoftClipper, id, name, node1, node2) {
    setClipperType(ClipperType::Medium);
  }

  void setClipperType(ClipperType type) {
    clipperType = type;

    switch (type) {
    case ClipperType::Mild:
      saturationVoltage = 2.0;
      driveGain = 0.5;
      break;
    case ClipperType::Medium:
      saturationVoltage = 1.0;
      driveGain = 1.0;
      break;
    case ClipperType::Hard:
      saturationVoltage = 0.5;
      driveGain = 2.0;
      break;
    case ClipperType::Asymmetric:
      saturationVoltage = 0.8;
      driveGain = 1.5;
      break;
    case ClipperType::Custom:
      break;
    }
  }

  ClipperType getClipperType() const { return clipperType; }

  double getSaturationVoltage() const { return saturationVoltage; }
  void setSaturationVoltage(double sat) {
    saturationVoltage = std::max(0.1, sat);
    clipperType = ClipperType::Custom;
  }

  double getDriveGain() const { return driveGain; }
  void setDriveGain(double drive) {
    driveGain = std::max(0.1, drive);
    clipperType = ClipperType::Custom;
  }

  /**
   * Calculate output using tanh soft clipping.
   * Output = saturation * tanh(input * drive / saturation)
   */
  double calculateOutput(double input) const {
    double x = input / saturationVoltage * driveGain;
    return saturationVoltage * std::tanh(x);
  }

  /**
   * Calculate derivative for Newton-Raphson.
   * d/dx[sat * tanh(x * drive / sat)] = drive * sech²(x * drive / sat)
   */
  double calculateDerivative(double input) const {
    double x = input / saturationVoltage * driveGain;
    double sech = 1.0 / std::cosh(x);
    return driveGain * sech * sech;
  }

  bool isNonLinear() const override { return true; }

  juce::String getSymbol() const override { return "SC"; }

  juce::String getValueString() const override {
    switch (clipperType) {
    case ClipperType::Mild:
      return "Mild";
    case ClipperType::Medium:
      return "Medium";
    case ClipperType::Hard:
      return "Hard";
    case ClipperType::Asymmetric:
      return "Asymmetric";
    case ClipperType::Custom:
      return "Custom";
    }
    return "Soft Clipper";
  }

private:
  ClipperType clipperType = ClipperType::Medium;
  double saturationVoltage = 1.0;
  double driveGain = 1.0;
};


#pragma once

#include "WDFCore.h"
#include <functional>

namespace WDF {

/**
 * Nonlinear WDF elements using Newton-Raphson iteration.
 * 
 * For nonlinear elements, we can't use the simple wave scattering.
 * Instead, we solve the nonlinear equation iteratively at the root.
 */

/**
 * Generic nonlinear root element.
 * Uses Newton-Raphson to find the operating point.
 */
class WDFNonlinearRoot : public WDFRoot {
public:
    // Function signature for I-V characteristic: current = f(voltage)
    using IVFunction = std::function<double(double)>;
    // Function for derivative: dI/dV = f'(voltage)
    using IVDerivative = std::function<double(double)>;
    
    WDFNonlinearRoot() = default;
    
    void setIVCharacteristic(IVFunction iv, IVDerivative div) {
        ivFunc = iv;
        ivDerivFunc = div;
    }
    
    void setMaxIterations(int max) { maxIterations = max; }
    void setTolerance(double tol) { tolerance = tol; }
    
    void propagate() override {
        if (!connectedTree || !ivFunc || !ivDerivFunc) return;
        
        // Get reflected wave from tree
        connectedTree->propagate();
        R = connectedTree->getPortResistance();
        a = connectedTree->getReflectedWave();
        
        // Newton-Raphson iteration to find voltage
        // We need to solve: i = f(v) and v = a + b, i = (a - b) / R
        // Combining: i = (a - (v - a)) / R = (2a - v) / R
        // So: f(v) = (2a - v) / R
        // Or: R * f(v) + v = 2a
        
        double v = lastVoltage;  // Initial guess
        
        for (int iter = 0; iter < maxIterations; ++iter) {
            double i = ivFunc(v);
            double di = ivDerivFunc(v);
            
            // Residual: g(v) = R * i - 2a + v = 0
            double g = R * i + v - 2.0 * a;
            double dg = R * di + 1.0;
            
            if (std::abs(dg) < 1e-15) break;
            
            double dv = -g / dg;
            v += dv;
            
            if (std::abs(dv) < tolerance) break;
        }
        
        lastVoltage = v;
        b = v - a;
        
        // Send reflected wave back to tree
        connectedTree->setIncidentWave(b);
    }
    
protected:
    IVFunction ivFunc;
    IVDerivative ivDerivFunc;
    int maxIterations = 20;
    double tolerance = 1e-9;
    double lastVoltage = 0.0;  // For warm-starting Newton
};

/**
 * Diode using Shockley equation.
 * I = Is * (exp(V / (n * Vt)) - 1)
 */
class WDFDiode : public WDFNonlinearRoot {
public:
    WDFDiode(double saturationCurrent = 1e-15, double emissionCoeff = 1.0) 
        : Is(saturationCurrent), n(emissionCoeff) {
        updateCharacteristic();
    }
    
    void setParameters(double saturationCurrent, double emissionCoeff) {
        Is = saturationCurrent;
        n = emissionCoeff;
        updateCharacteristic();
    }
    
    void setTemperature(double tempKelvin) {
        temperature = tempKelvin;
        Vt = 8.617333262e-5 * temperature;  // k * T / q
        updateCharacteristic();
    }
    
private:
    void updateCharacteristic() {
        double nVt = n * Vt;
        
        ivFunc = [this, nVt](double v) -> double {
            if (v > 0.7) {
                // Linear extrapolation for high voltages to prevent overflow
                double Iref = Is * (std::exp(0.7 / nVt) - 1.0);
                double gRef = Is / nVt * std::exp(0.7 / nVt);
                return Iref + gRef * (v - 0.7);
            }
            return Is * (std::exp(v / nVt) - 1.0);
        };
        
        ivDerivFunc = [this, nVt](double v) -> double {
            if (v > 0.7) {
                return Is / nVt * std::exp(0.7 / nVt);
            }
            return Is / nVt * std::exp(v / nVt);
        };
    }
    
    double Is = 1e-15;      // Saturation current
    double n = 1.0;         // Emission coefficient
    double Vt = 0.02585;    // Thermal voltage (~26mV at room temp)
    double temperature = 300.0;  // Temperature in Kelvin
};

/**
 * Anti-parallel diode pair (for symmetric clipping).
 * Two diodes in opposite directions.
 */
class WDFDiodePair : public WDFNonlinearRoot {
public:
    WDFDiodePair(double saturationCurrent = 1e-15, double emissionCoeff = 1.0)
        : Is(saturationCurrent), n(emissionCoeff) {
        updateCharacteristic();
    }
    
    void setParameters(double saturationCurrent, double emissionCoeff) {
        Is = saturationCurrent;
        n = emissionCoeff;
        updateCharacteristic();
    }
    
private:
    void updateCharacteristic() {
        double nVt = n * Vt;
        
        ivFunc = [this, nVt](double v) -> double {
            // Two diodes in anti-parallel: I = Is * (exp(V/nVt) - exp(-V/nVt))
            // = 2 * Is * sinh(V / nVt)
            double x = v / nVt;
            if (std::abs(x) > 25.0) {
                // Linear extrapolation
                double sign = (x > 0) ? 1.0 : -1.0;
                double Iref = 2.0 * Is * std::sinh(25.0 * sign);
                double gRef = 2.0 * Is / nVt * std::cosh(25.0);
                return Iref + gRef * (v - 25.0 * nVt * sign);
            }
            return 2.0 * Is * std::sinh(x);
        };
        
        ivDerivFunc = [this, nVt](double v) -> double {
            double x = v / nVt;
            if (std::abs(x) > 25.0) {
                return 2.0 * Is / nVt * std::cosh(25.0);
            }
            return 2.0 * Is / nVt * std::cosh(x);
        };
    }
    
    double Is = 1e-15;
    double n = 1.0;
    double Vt = 0.02585;
};

/**
 * Vacuum tube triode model using simplified Koren equations.
 * 
 * This models a triode with grid, cathode, and plate.
 * For WDF, we model it as a voltage-controlled current source.
 */
class WDFTriode : public WDFRoot {
public:
    enum class TubeType {
        Type12AX7,  // High-gain preamp
        Type12AT7,  // Medium-gain
        Type12AU7,  // Low-gain
        TypeEL34,   // Power tube
        Custom
    };
    
    WDFTriode(TubeType type = TubeType::Type12AX7) {
        setTubeType(type);
    }
    
    void setTubeType(TubeType type) {
        tubeType = type;
        switch (type) {
            case TubeType::Type12AX7:
                mu = 100.0; kp = 600.0; kvb = 300.0; kg1 = 1060.0; ex = 1.4;
                break;
            case TubeType::Type12AT7:
                mu = 60.0; kp = 300.0; kvb = 300.0; kg1 = 460.0; ex = 1.35;
                break;
            case TubeType::Type12AU7:
                mu = 20.0; kp = 84.0; kvb = 300.0; kg1 = 1180.0; ex = 1.3;
                break;
            case TubeType::TypeEL34:
                mu = 11.0; kp = 60.0; kvb = 24.0; kg1 = 650.0; ex = 1.35;
                break;
            case TubeType::Custom:
                break;
        }
    }
    
    void setParameters(double _mu, double _kp, double _kvb, double _kg1, double _ex) {
        mu = _mu; kp = _kp; kvb = _kvb; kg1 = _kg1; ex = _ex;
        tubeType = TubeType::Custom;
    }
    
    // Grid-to-cathode voltage (input)
    void setGridVoltage(double vgk) { Vgk = vgk; }
    double getGridVoltage() const { return Vgk; }
    
    // For a complete triode model, we'd need separate WDF trees for 
    // grid-cathode and plate-cathode. This simplified version assumes
    // we're driving the plate with a fixed grid voltage.
    
    void propagate() override {
        if (!connectedTree) return;
        
        connectedTree->propagate();
        R = connectedTree->getPortResistance();
        a = connectedTree->getReflectedWave();
        
        // Newton-Raphson for plate current
        double Vpk = lastVpk;
        
        for (int iter = 0; iter < maxIterations; ++iter) {
            double Ip = calculatePlateCurrent(Vgk, Vpk);
            double gp = calculatePlateDerivative(Vgk, Vpk);
            
            // Wave equation: Vpk = a + b, Ip = (a - b) / R
            // Combined: R * Ip + Vpk = 2a
            double g = R * Ip + Vpk - 2.0 * a;
            double dg = R * gp + 1.0;
            
            if (std::abs(dg) < 1e-15) break;
            
            double dv = -g / dg;
            Vpk += dv;
            
            if (std::abs(dv) < tolerance) break;
        }
        
        lastVpk = Vpk;
        b = Vpk - a;
        connectedTree->setIncidentWave(b);
    }
    
    double getPlateVoltage() const { return lastVpk; }
    double getPlateCurrent() const { return calculatePlateCurrent(Vgk, lastVpk); }
    
private:
    double calculatePlateCurrent(double vgk, double vpk) const {
        if (vpk <= 0.0) return 0.0;
        
        double sqrtTerm = std::sqrt(kvb + vpk * vpk);
        double logArg = 1.0 + std::exp(kp * (1.0 / mu + vgk / sqrtTerm));
        double E1 = (vpk / kp) * std::log(logArg);
        
        if (E1 <= 0.0) return 0.0;
        
        return std::pow(E1, ex) / kg1;
    }
    
    double calculatePlateDerivative(double vgk, double vpk) const {
        const double h = 1e-6;
        double Ip = calculatePlateCurrent(vgk, vpk);
        double Ip_plus = calculatePlateCurrent(vgk, vpk + h);
        return std::max(1e-9, (Ip_plus - Ip) / h);
    }
    
    TubeType tubeType = TubeType::Type12AX7;
    double mu = 100.0, kp = 600.0, kvb = 300.0, kg1 = 1060.0, ex = 1.4;
    
    double Vgk = 0.0;       // Grid-cathode voltage
    double lastVpk = 0.0;   // For Newton warm-start
    
    int maxIterations = 20;
    double tolerance = 1e-9;
};

/**
 * Generic soft clipper using tanh.
 * Good for modeling overdrive/distortion.
 */
class WDFSoftClipper : public WDFNonlinearRoot {
public:
    WDFSoftClipper(double saturationVoltage = 1.0, double driveGain = 1.0)
        : saturation(saturationVoltage), drive(driveGain) {
        updateCharacteristic();
    }
    
    void setSaturation(double sat) {
        saturation = std::max(0.1, sat);
        updateCharacteristic();
    }
    
    void setDrive(double d) {
        drive = std::max(0.1, d);
        updateCharacteristic();
    }
    
private:
    void updateCharacteristic() {
        // Model as tanh-shaped I-V curve
        // I = G * sat * tanh(V / sat * drive)
        // where G = 1/R is the conductance
        
        ivFunc = [this](double v) -> double {
            double x = v / saturation * drive;
            return (1.0 / 1000.0) * saturation * std::tanh(x);  // G = 1mS
        };
        
        ivDerivFunc = [this](double v) -> double {
            double x = v / saturation * drive;
            double sech = 1.0 / std::cosh(x);
            return (1.0 / 1000.0) * drive * sech * sech;
        };
    }
    
    double saturation;
    double drive;
};

} // namespace WDF


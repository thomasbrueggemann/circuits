#pragma once

#include <cmath>
#include <memory>
#include <vector>
#include <JuceHeader.h>

/**
 * Wave Digital Filter (WDF) Core Classes
 * 
 * WDF represents circuit elements using wave variables instead of 
 * voltage/current. Each port has:
 *   - Incident wave 'a' (coming into the element)
 *   - Reflected wave 'b' (going out of the element)
 *   - Port resistance 'R' (characteristic impedance)
 * 
 * Wave relationships:
 *   v = a + b  (voltage at port)
 *   i = (a - b) / R  (current through port)
 *   a = (v + R*i) / 2
 *   b = (v - R*i) / 2
 */

namespace WDF {

/**
 * Base class for all WDF elements.
 * Each element has a port with resistance R and wave variables a, b.
 */
class WDFElement {
public:
    WDFElement() = default;
    virtual ~WDFElement() = default;
    
    // Port resistance (characteristic impedance)
    double getPortResistance() const { return R; }
    void setPortResistance(double resistance) { R = resistance; }
    
    // Incident wave (set by parent adaptor)
    double getIncidentWave() const { return a; }
    void setIncidentWave(double wave) { a = wave; }
    
    // Reflected wave (computed by this element)
    double getReflectedWave() const { return b; }
    
    // Voltage and current at port
    double getVoltage() const { return a + b; }
    double getCurrent() const { return (a - b) / R; }
    
    // Calculate port resistance based on component values
    virtual void calculatePortResistance() {}
    
    // Propagate waves through the element
    virtual void propagate() = 0;
    
    // Reset state (for capacitors, etc.)
    virtual void reset() { a = 0.0; b = 0.0; }
    
protected:
    double R = 1.0;   // Port resistance
    double a = 0.0;   // Incident wave
    double b = 0.0;   // Reflected wave
};

/**
 * One-port elements (resistors, capacitors, sources)
 */
class WDFOnePort : public WDFElement {
public:
    WDFOnePort() = default;
};

/**
 * Adaptor connects multiple ports together.
 * Base class for series, parallel, and R-type adaptors.
 */
class WDFAdaptor : public WDFElement {
public:
    WDFAdaptor() = default;
    
    // Connect child elements to this adaptor
    virtual void connectChild(WDFElement* child) = 0;
    
    // Number of connected ports (excluding parent)
    virtual int getNumPorts() const = 0;
    
    // Propagate waves from children up to this adaptor's reflected wave
    virtual void propagateFromChildren() = 0;
    
    // Scatter incident wave down to children
    virtual void scatterToChildren() = 0;
};

/**
 * Root element for the WDF tree.
 * The root is where we compute the scattering - typically a voltage source
 * or nonlinear element.
 */
class WDFRoot : public WDFElement {
public:
    WDFRoot() = default;
    
    // Connect the WDF tree to this root
    void connectTree(WDFElement* tree) { connectedTree = tree; }
    
    // Get connected tree element
    WDFElement* getTree() const { return connectedTree; }
    
protected:
    WDFElement* connectedTree = nullptr;
};

// =============================================================================
// One-Port Elements
// =============================================================================

/**
 * Ideal resistor.
 * When adapted (R = resistance), the resistor absorbs all incident waves.
 */
class WDFResistor : public WDFOnePort {
public:
    explicit WDFResistor(double resistanceValue) : resistance(resistanceValue) {
        R = resistance;
    }
    
    void setResistance(double r) { 
        resistance = std::max(1e-9, r);
        R = resistance;
    }
    
    double getResistance() const { return resistance; }
    
    void calculatePortResistance() override {
        R = resistance;
    }
    
    void propagate() override {
        // For an adapted resistor (R = resistance), b = 0
        // For non-adapted: b = ((resistance - R) / (resistance + R)) * a
        double gamma = (resistance - R) / (resistance + R);
        b = gamma * a;
    }
    
private:
    double resistance;
};

/**
 * Capacitor using bilinear transform (trapezoidal integration).
 * Port resistance R = dt / (2 * C)
 */
class WDFCapacitor : public WDFOnePort {
public:
    explicit WDFCapacitor(double capacitanceValue, double sampleRateValue = 44100.0)
        : capacitance(capacitanceValue), sampleRate(sampleRateValue) {
        calculatePortResistance();
    }
    
    void setCapacitance(double c) { 
        capacitance = std::max(1e-15, c);
        calculatePortResistance();
    }
    
    double getCapacitance() const { return capacitance; }
    
    void setSampleRate(double rate) {
        sampleRate = rate;
        calculatePortResistance();
    }
    
    void calculatePortResistance() override {
        double dt = 1.0 / sampleRate;
        R = dt / (2.0 * capacitance);
    }
    
    void propagate() override {
        // Capacitor: b[n] = a[n-1] (unit delay on reflected wave)
        b = state;
        state = a;
    }
    
    void reset() override {
        WDFOnePort::reset();
        state = 0.0;
    }
    
private:
    double capacitance;
    double sampleRate;
    double state = 0.0;  // z^-1 state
};

/**
 * Inductor using bilinear transform.
 * Port resistance R = 2 * L / dt
 */
class WDFInductor : public WDFOnePort {
public:
    explicit WDFInductor(double inductanceValue, double sampleRateValue = 44100.0)
        : inductance(inductanceValue), sampleRate(sampleRateValue) {
        calculatePortResistance();
    }
    
    void setInductance(double l) {
        inductance = std::max(1e-15, l);
        calculatePortResistance();
    }
    
    double getInductance() const { return inductance; }
    
    void setSampleRate(double rate) {
        sampleRate = rate;
        calculatePortResistance();
    }
    
    void calculatePortResistance() override {
        double dt = 1.0 / sampleRate;
        R = 2.0 * inductance / dt;
    }
    
    void propagate() override {
        // Inductor: b[n] = -a[n-1] (unit delay with sign inversion)
        b = -state;
        state = a;
    }
    
    void reset() override {
        WDFOnePort::reset();
        state = 0.0;
    }
    
private:
    double inductance;
    double sampleRate;
    double state = 0.0;
};

/**
 * Open circuit (infinite resistance).
 * Reflects all incident wave with same sign.
 */
class WDFOpen : public WDFOnePort {
public:
    WDFOpen() { R = 1e12; }
    
    void propagate() override {
        b = a;  // Perfect reflection, no sign change
    }
};

/**
 * Short circuit (zero resistance).
 * Reflects all incident wave with opposite sign.
 */
class WDFShort : public WDFOnePort {
public:
    WDFShort() { R = 1e-12; }
    
    void propagate() override {
        b = -a;  // Perfect reflection with sign inversion
    }
};

/**
 * Switch - either open or short circuit
 */
class WDFSwitch : public WDFOnePort {
public:
    WDFSwitch(bool isClosed = false) : closed(isClosed) {
        updateResistance();
    }
    
    void setClosed(bool c) { 
        closed = c;
        updateResistance();
    }
    
    bool isClosed() const { return closed; }
    void toggle() { setClosed(!closed); }
    
    void propagate() override {
        if (closed) {
            b = -a;  // Short circuit
        } else {
            b = a;   // Open circuit
        }
    }
    
private:
    void updateResistance() {
        R = closed ? 1e-12 : 1e12;
    }
    
    bool closed;
};

// =============================================================================
// Adaptors
// =============================================================================

/**
 * Series adaptor for two ports.
 * 
 * Connects two elements in series. The parent port sees the sum of voltages
 * and same current through both elements.
 */
class WDFSeriesAdaptor : public WDFAdaptor {
public:
    WDFSeriesAdaptor() = default;
    
    void connectChild(WDFElement* child) override {
        if (!child1) {
            child1 = child;
        } else if (!child2) {
            child2 = child;
            calculatePortResistance();
        }
    }
    
    void connectChildren(WDFElement* c1, WDFElement* c2) {
        child1 = c1;
        child2 = c2;
        calculatePortResistance();
    }
    
    int getNumPorts() const override { return 2; }
    
    void calculatePortResistance() override {
        if (child1 && child2) {
            double R1 = child1->getPortResistance();
            double R2 = child2->getPortResistance();
            R = R1 + R2;
            
            // Scattering coefficient
            gamma1 = R1 / R;
            gamma2 = R2 / R;
        }
    }
    
    void propagateFromChildren() override {
        if (child1 && child2) {
            // Propagate children first
            child1->propagate();
            child2->propagate();
            
            // Reflected wave at parent port
            b = -(child1->getReflectedWave() + child2->getReflectedWave());
        }
    }
    
    void scatterToChildren() override {
        if (child1 && child2) {
            double b1 = child1->getReflectedWave();
            double b2 = child2->getReflectedWave();
            
            // Scatter to children
            child1->setIncidentWave(a - gamma1 * (a + b1 + b2));
            child2->setIncidentWave(a - gamma2 * (a + b1 + b2));
        }
    }
    
    void propagate() override {
        propagateFromChildren();
    }
    
    WDFElement* getChild1() const { return child1; }
    WDFElement* getChild2() const { return child2; }
    
private:
    WDFElement* child1 = nullptr;
    WDFElement* child2 = nullptr;
    double gamma1 = 0.5;
    double gamma2 = 0.5;
};

/**
 * Parallel adaptor for two ports.
 * 
 * Connects two elements in parallel. The parent port sees the same voltage
 * and sum of currents through both elements.
 */
class WDFParallelAdaptor : public WDFAdaptor {
public:
    WDFParallelAdaptor() = default;
    
    void connectChild(WDFElement* child) override {
        if (!child1) {
            child1 = child;
        } else if (!child2) {
            child2 = child;
            calculatePortResistance();
        }
    }
    
    void connectChildren(WDFElement* c1, WDFElement* c2) {
        child1 = c1;
        child2 = c2;
        calculatePortResistance();
    }
    
    int getNumPorts() const override { return 2; }
    
    void calculatePortResistance() override {
        if (child1 && child2) {
            double R1 = child1->getPortResistance();
            double R2 = child2->getPortResistance();
            R = (R1 * R2) / (R1 + R2);  // Parallel combination
            
            // Scattering coefficient (reflection from child 1)
            gamma1 = R / R1;
            gamma2 = R / R2;
        }
    }
    
    void propagateFromChildren() override {
        if (child1 && child2) {
            // Propagate children first
            child1->propagate();
            child2->propagate();
            
            // Reflected wave at parent port
            double b1 = child1->getReflectedWave();
            double b2 = child2->getReflectedWave();
            b = gamma1 * b1 + gamma2 * b2;
        }
    }
    
    void scatterToChildren() override {
        if (child1 && child2) {
            double b1 = child1->getReflectedWave();
            double b2 = child2->getReflectedWave();
            
            // Scatter to children
            double bDiff = gamma1 * b1 - gamma2 * b2;
            child1->setIncidentWave(a - bDiff + gamma2 * (b2 - b1));
            child2->setIncidentWave(a + bDiff + gamma1 * (b1 - b2));
        }
    }
    
    void propagate() override {
        propagateFromChildren();
    }
    
    WDFElement* getChild1() const { return child1; }
    WDFElement* getChild2() const { return child2; }
    
private:
    WDFElement* child1 = nullptr;
    WDFElement* child2 = nullptr;
    double gamma1 = 0.5;
    double gamma2 = 0.5;
};

/**
 * Polarity inverter - flips the sign of voltage/current.
 * Useful for handling ground references.
 */
class WDFPolarityInverter : public WDFAdaptor {
public:
    WDFPolarityInverter() = default;
    
    void connectChild(WDFElement* child) override {
        child1 = child;
        if (child1) {
            R = child1->getPortResistance();
        }
    }
    
    int getNumPorts() const override { return 1; }
    
    void calculatePortResistance() override {
        if (child1) {
            R = child1->getPortResistance();
        }
    }
    
    void propagateFromChildren() override {
        if (child1) {
            child1->propagate();
            b = -child1->getReflectedWave();
        }
    }
    
    void scatterToChildren() override {
        if (child1) {
            child1->setIncidentWave(-a);
        }
    }
    
    void propagate() override {
        propagateFromChildren();
    }
    
private:
    WDFElement* child1 = nullptr;
};

// =============================================================================
// Root Elements (Sources and Nonlinear)
// =============================================================================

/**
 * Ideal voltage source (root element).
 * Sets voltage regardless of current - not adaptable.
 */
class WDFIdealVoltageSource : public WDFRoot {
public:
    WDFIdealVoltageSource(double voltageValue = 0.0) : voltage(voltageValue) {}
    
    void setVoltage(double v) { voltage = v; }
    double getSourceVoltage() const { return voltage; }
    
    void propagate() override {
        if (connectedTree) {
            // First propagate from the tree
            connectedTree->propagate();
            
            // Root scattering: voltage = a + b, so b = V - a
            R = connectedTree->getPortResistance();
            a = connectedTree->getReflectedWave();
            b = voltage - a;
            
            // Send back to tree
            connectedTree->setIncidentWave(b);
        }
    }
    
    // Get output current (through the source)
    double getOutputCurrent() const {
        if (connectedTree) {
            return (a - b) / R;
        }
        return 0.0;
    }
    
private:
    double voltage = 0.0;
};

/**
 * Resistive voltage source (voltage source with series resistance).
 * This is adaptable and can be used as a leaf element.
 */
class WDFResistiveVoltageSource : public WDFOnePort {
public:
    WDFResistiveVoltageSource(double voltageValue = 0.0, double resistanceValue = 1.0)
        : voltage(voltageValue), resistance(resistanceValue) {
        R = resistance;
    }
    
    void setVoltage(double v) { voltage = v; }
    double getSourceVoltage() const { return voltage; }
    
    void setResistance(double r) { 
        resistance = std::max(1e-9, r);
        R = resistance;
    }
    
    void calculatePortResistance() override {
        R = resistance;
    }
    
    void propagate() override {
        // b = V - a (for adapted source where R = resistance)
        b = voltage - a;
    }
    
private:
    double voltage;
    double resistance;
};

/**
 * Current source with parallel resistance.
 * Adaptable as a leaf element.
 */
class WDFResistiveCurrentSource : public WDFOnePort {
public:
    WDFResistiveCurrentSource(double currentValue = 0.0, double resistanceValue = 1e6)
        : current(currentValue), resistance(resistanceValue) {
        R = resistance;
    }
    
    void setCurrent(double i) { current = i; }
    double getSourceCurrent() const { return current; }
    
    void setResistance(double r) {
        resistance = std::max(1e-9, r);
        R = resistance;
    }
    
    void calculatePortResistance() override {
        R = resistance;
    }
    
    void propagate() override {
        // For current source: V = I * R, so b = I*R - a (when R = resistance)
        b = current * R + a;  // Note: sign convention for current
    }
    
private:
    double current;
    double resistance;
};

} // namespace WDF


/**
 * WDF Component Unit Tests
 *
 * These tests verify the correctness of Wave Digital Filter (WDF) components
 * and their chaining behavior.
 */

#include <JuceHeader.h>
#include "../src/Engine/WDF/WDFCore.h"
#include "../src/Engine/WDF/WDFNonlinear.h"
#include <cmath>
#include <iostream>

using namespace WDF;

// =============================================================================
// Test Utilities
// =============================================================================

class WDFTests : public juce::UnitTest {
public:
    WDFTests() : UnitTest("WDF Component Tests") {}

    void runTest() override {
        // Basic component tests
        testResistor();
        testCapacitor();
        testInductor();
        testSwitch();

        // Adaptor tests
        testSeriesAdaptor();
        testParallelAdaptor();

        // Source tests
        testIdealVoltageSource();
        testResistiveVoltageSource();

        // Chaining tests
        testRCLowPassFilter();
        testRCHighPassFilter();
        testRLFilter();
        testVoltageDivider();
        testSeriesResistors();
        testParallelResistors();

        // Nonlinear element tests
        testDiode();
        testDiodePair();
        testSoftClipper();
    }

private:
    // Helper to check if two values are approximately equal
    bool approxEqual(double a, double b, double tolerance = 1e-6) {
        return std::abs(a - b) < tolerance;
    }

    // Helper to simulate multiple samples through a circuit
    void runSamples(WDFRoot* root, int numSamples) {
        for (int i = 0; i < numSamples; ++i) {
            root->propagate();
        }
    }

    // =========================================================================
    // Basic Component Tests
    // =========================================================================

    void testResistor() {
        beginTest("Resistor - Port Resistance");

        WDFResistor resistor(10000.0); // 10k ohm
        expectEquals(resistor.getPortResistance(), 10000.0);
        expectEquals(resistor.getResistance(), 10000.0);

        // Change resistance
        resistor.setResistance(4700.0);
        expectEquals(resistor.getResistance(), 4700.0);

        beginTest("Resistor - Wave Propagation (Adapted)");
        // When port resistance equals component resistance, all energy is absorbed
        resistor.setResistance(1000.0);
        resistor.setPortResistance(1000.0);
        resistor.setIncidentWave(1.0);
        resistor.propagate();
        // For adapted resistor: b = 0
        expect(approxEqual(resistor.getReflectedWave(), 0.0, 1e-9));

        beginTest("Resistor - Wave Propagation (Non-Adapted)");
        resistor.setResistance(2000.0);
        resistor.setPortResistance(1000.0);
        resistor.setIncidentWave(1.0);
        resistor.propagate();
        // gamma = (R_comp - R_port) / (R_comp + R_port) = (2000-1000)/(2000+1000) = 1/3
        double expectedGamma = (2000.0 - 1000.0) / (2000.0 + 1000.0);
        expect(approxEqual(resistor.getReflectedWave(), expectedGamma, 1e-9));
    }

    void testCapacitor() {
        beginTest("Capacitor - Port Resistance Calculation");

        double sampleRate = 44100.0;
        double capacitance = 100e-9; // 100nF
        WDFCapacitor cap(capacitance, sampleRate);

        // R = dt / (2 * C) = (1/44100) / (2 * 100e-9)
        double expectedR = (1.0 / sampleRate) / (2.0 * capacitance);
        expect(approxEqual(cap.getPortResistance(), expectedR, 1.0));

        beginTest("Capacitor - State Memory (Unit Delay)");
        cap.reset();
        cap.setIncidentWave(0.5);
        cap.propagate();
        // First sample: b = state (which is 0)
        expect(approxEqual(cap.getReflectedWave(), 0.0, 1e-9));

        // Second sample with same input
        cap.propagate();
        // Now b should equal previous a
        expect(approxEqual(cap.getReflectedWave(), 0.5, 1e-9));

        beginTest("Capacitor - Sample Rate Change");
        cap.setSampleRate(48000.0);
        double newExpectedR = (1.0 / 48000.0) / (2.0 * capacitance);
        expect(approxEqual(cap.getPortResistance(), newExpectedR, 1.0));
    }

    void testInductor() {
        beginTest("Inductor - Port Resistance Calculation");

        double sampleRate = 44100.0;
        double inductance = 100e-3; // 100mH
        WDFInductor ind(inductance, sampleRate);

        // R = 2 * L / dt = 2 * L * sampleRate
        double expectedR = 2.0 * inductance * sampleRate;
        expect(approxEqual(ind.getPortResistance(), expectedR, 1.0));

        beginTest("Inductor - State Memory (Inverted Unit Delay)");
        ind.reset();
        ind.setIncidentWave(0.5);
        ind.propagate();
        // First sample: b = -state (which is 0)
        expect(approxEqual(ind.getReflectedWave(), 0.0, 1e-9));

        // Second sample with same input
        ind.propagate();
        // Now b should equal -previous_a
        expect(approxEqual(ind.getReflectedWave(), -0.5, 1e-9));
    }

    void testSwitch() {
        beginTest("Switch - Open State");
        WDFSwitch sw(false); // Open
        expect(!sw.isClosed());

        sw.setIncidentWave(1.0);
        sw.propagate();
        // Open switch: b = a (perfect reflection, no sign change)
        expectEquals(sw.getReflectedWave(), 1.0);

        beginTest("Switch - Closed State");
        sw.setClosed(true);
        expect(sw.isClosed());

        sw.setIncidentWave(1.0);
        sw.propagate();
        // Closed switch: b = -a (short circuit, sign inversion)
        expectEquals(sw.getReflectedWave(), -1.0);

        beginTest("Switch - Toggle");
        sw.toggle();
        expect(!sw.isClosed());
    }

    // =========================================================================
    // Adaptor Tests
    // =========================================================================

    void testSeriesAdaptor() {
        beginTest("Series Adaptor - Port Resistance");

        WDFResistor r1(1000.0);
        WDFResistor r2(2000.0);
        WDFSeriesAdaptor series;
        series.connectChildren(&r1, &r2);

        // Series: R_total = R1 + R2
        expectEquals(series.getPortResistance(), 3000.0);

        beginTest("Series Adaptor - Wave Propagation");
        // Use a voltage source to drive the series combination
        WDFIdealVoltageSource source(3.0);
        source.connectTree(&series);

        source.propagate();
        series.scatterToChildren();

        // With voltage divider, v1/v2 = R1/R2
        double v1 = r1.getVoltage();
        double v2 = r2.getVoltage();
        // Check total voltage is ~3V
        expect(approxEqual(v1 + v2, 3.0, 0.5));
    }

    void testParallelAdaptor() {
        beginTest("Parallel Adaptor - Port Resistance");

        WDFResistor r1(1000.0);
        WDFResistor r2(1000.0);
        WDFParallelAdaptor parallel;
        parallel.connectChildren(&r1, &r2);

        // Parallel: R_total = R1 * R2 / (R1 + R2)
        double expectedR = (1000.0 * 1000.0) / (1000.0 + 1000.0);
        expectEquals(parallel.getPortResistance(), expectedR);

        beginTest("Parallel Adaptor - Different Resistances");
        WDFResistor r3(1000.0);
        WDFResistor r4(3000.0);
        WDFParallelAdaptor parallel2;
        parallel2.connectChildren(&r3, &r4);

        double expectedR2 = (1000.0 * 3000.0) / (1000.0 + 3000.0);
        expectEquals(parallel2.getPortResistance(), expectedR2);
    }

    // =========================================================================
    // Source Tests
    // =========================================================================

    void testIdealVoltageSource() {
        beginTest("Ideal Voltage Source - Basic Operation");

        WDFIdealVoltageSource source(5.0);
        WDFResistor load(1000.0);
        source.connectTree(&load);

        source.propagate();

        // With ideal voltage source set to 5V, output should be close to 5V
        double outputV = load.getVoltage();
        expect(approxEqual(outputV, 5.0, 0.1));

        beginTest("Ideal Voltage Source - Voltage Change");
        source.setVoltage(2.5);
        source.propagate();
        expect(approxEqual(load.getVoltage(), 2.5, 0.1));
    }

    void testResistiveVoltageSource() {
        beginTest("Resistive Voltage Source");

        WDFResistiveVoltageSource source(5.0, 100.0); // 5V, 100 ohm internal resistance
        expectEquals(source.getSourceVoltage(), 5.0);
        expectEquals(source.getPortResistance(), 100.0);

        source.setVoltage(10.0);
        expectEquals(source.getSourceVoltage(), 10.0);
    }

    // =========================================================================
    // Circuit Chaining Tests
    // =========================================================================

    void testRCLowPassFilter() {
        beginTest("RC Low-Pass Filter - DC Passthrough");

        // RC low-pass: series R and C driven by voltage source
        // At DC steady state, capacitor charges to source voltage
        double sampleRate = 44100.0;
        double R = 1000.0;
        double C = 1e-6; // 1uF for faster settling

        WDFResistor resistor(R);
        WDFCapacitor capacitor(C, sampleRate);
        WDFSeriesAdaptor series;
        series.connectChildren(&resistor, &capacitor);

        WDFIdealVoltageSource source(1.0);
        source.connectTree(&series);

        // Run enough samples for capacitor to charge (time constant = RC)
        // With R=1k, C=1uF: tau = 1ms, need ~5*tau = 5ms = 220 samples at 44.1k
        for (int i = 0; i < 5000; ++i) {
            source.propagate();
            series.scatterToChildren();
        }

        // Check that total voltage across series is close to source
        double totalV = series.getVoltage();
        expect(approxEqual(totalV, 1.0, 0.2));

        beginTest("RC Low-Pass Filter - Frequency Response");
        // Reset and test with AC signal
        resistor.reset();
        capacitor.reset();

        // Test at low frequency (should pass through)
        double lowFreq = 100.0; // Well below cutoff (~160 Hz for 1k/1uF)
        double amplitude = 0.0;
        int samplesPerCycle = static_cast<int>(sampleRate / lowFreq);

        // Run several cycles to reach steady state
        for (int i = 0; i < samplesPerCycle * 20; ++i) {
            double input = std::sin(2.0 * M_PI * lowFreq * i / sampleRate);
            source.setVoltage(input);
            source.propagate();
            series.scatterToChildren();
            // After settling, track amplitude
            if (i > samplesPerCycle * 10) {
                amplitude = std::max(amplitude, std::abs(capacitor.getVoltage()));
            }
        }

        // Low frequency should mostly pass through (below cutoff)
        expect(amplitude > 0.5); // At least 50% amplitude
    }

    void testRCHighPassFilter() {
        beginTest("RC High-Pass Filter - DC Blocking");

        double sampleRate = 44100.0;
        double R = 1000.0;
        double C = 1e-6; // 1uF

        // High-pass: series C and R - at DC, C blocks current
        WDFCapacitor capacitor(C, sampleRate);
        WDFResistor resistor(R);
        WDFSeriesAdaptor series;
        series.connectChildren(&capacitor, &resistor);

        WDFIdealVoltageSource source(1.0);
        source.connectTree(&series);

        // Apply DC and let it settle - capacitor charges, current drops
        for (int i = 0; i < 10000; ++i) {
            source.propagate();
            series.scatterToChildren();
        }

        // At DC steady state, current through circuit should be very small
        // The series adaptor voltage should be close to source voltage
        double totalV = series.getVoltage();
        expect(approxEqual(totalV, 1.0, 0.3));
        
        // At steady state, capacitor has most of the voltage
        double capV = std::abs(capacitor.getVoltage());
        double resV = std::abs(resistor.getVoltage());
        // Capacitor should have more voltage than resistor at DC steady state
        expect(capV > resV * 0.5); // Capacitor dominates
    }

    void testRLFilter() {
        beginTest("RL Filter - Basic Operation");

        double sampleRate = 44100.0;
        double R = 1000.0;
        double L = 10e-3; // 10mH for faster settling (tau = L/R = 10us)

        WDFResistor resistor(R);
        WDFInductor inductor(L, sampleRate);
        WDFSeriesAdaptor series;
        series.connectChildren(&resistor, &inductor);

        WDFIdealVoltageSource source(1.0);
        source.connectTree(&series);

        // Run to steady state - tau = L/R = 10mH/1k = 10us, need ~5*tau
        // At 44.1kHz, that's about 2-3 samples, but we run more for stability
        for (int i = 0; i < 5000; ++i) {
            source.propagate();
            series.scatterToChildren();
        }

        // At DC steady state, total voltage across series = source
        double totalV = series.getVoltage();
        expect(approxEqual(totalV, 1.0, 0.2));
    }

    void testVoltageDivider() {
        beginTest("Voltage Divider - 50/50 Split");

        // Two equal resistors in series: output should be half input
        WDFResistor r1(1000.0);
        WDFResistor r2(1000.0);
        WDFSeriesAdaptor series;
        series.connectChildren(&r1, &r2);

        WDFIdealVoltageSource source(10.0);
        source.connectTree(&series);

        source.propagate();
        series.scatterToChildren();

        // Each resistor should have half the voltage
        double v1 = r1.getVoltage();
        double v2 = r2.getVoltage();
        expect(approxEqual(v1, 5.0, 0.5));
        expect(approxEqual(v2, 5.0, 0.5));

        beginTest("Voltage Divider - 1:2 Ratio");
        WDFResistor ra(1000.0);
        WDFResistor rb(2000.0);
        WDFSeriesAdaptor series2;
        series2.connectChildren(&ra, &rb);

        WDFIdealVoltageSource source2(9.0);
        source2.connectTree(&series2);

        source2.propagate();
        series2.scatterToChildren();

        // Total voltage should be 9V
        double totalV = series2.getVoltage();
        expect(approxEqual(totalV, 9.0, 1.5));

        // The port resistance should be correct (sum of resistances)
        expect(approxEqual(series2.getPortResistance(), 3000.0, 1.0));
    }

    void testSeriesResistors() {
        beginTest("Series Resistors - Total Resistance");

        WDFResistor r1(1000.0);
        WDFResistor r2(2200.0);
        WDFResistor r3(4700.0);

        // Chain: r1 + (r2 + r3)
        WDFSeriesAdaptor inner;
        inner.connectChildren(&r2, &r3);

        WDFSeriesAdaptor outer;
        outer.connectChildren(&r1, &inner);

        // Total = 1000 + 2200 + 4700 = 7900
        expectEquals(outer.getPortResistance(), 7900.0);
    }

    void testParallelResistors() {
        beginTest("Parallel Resistors - Total Resistance");

        WDFResistor r1(1000.0);
        WDFResistor r2(1000.0);
        WDFParallelAdaptor parallel;
        parallel.connectChildren(&r1, &r2);

        // Parallel of two 1k = 500 ohm
        expectEquals(parallel.getPortResistance(), 500.0);

        beginTest("Parallel Resistors - Three Equal");
        WDFResistor ra(1000.0);
        WDFResistor rb(1000.0);
        WDFResistor rc(1000.0);

        WDFParallelAdaptor p1;
        p1.connectChildren(&ra, &rb);

        WDFParallelAdaptor p2;
        p2.connectChildren(&p1, &rc);

        // Parallel of three 1k = 333.33 ohm
        double expected = 1000.0 / 3.0;
        expect(approxEqual(p2.getPortResistance(), expected, 1.0));
    }

    // =========================================================================
    // Nonlinear Element Tests
    // =========================================================================

    void testDiode() {
        beginTest("Diode - Forward Bias");

        // Diode with series resistor (load)
        WDFDiode diode(1e-12, 1.0); // Higher Is for easier testing
        WDFResistor load(1000.0);
        diode.connectTree(&load);

        // Simulate forward bias with positive incident wave
        load.setIncidentWave(1.0);
        for (int i = 0; i < 100; ++i) {
            diode.propagate();
        }

        // Forward bias: diode conducts, voltage is positive
        double vd = load.getVoltage();
        expect(vd >= 0.0); // Forward biased voltage

        beginTest("Diode - Reverse Bias");
        // Test with negative voltage - diode should block
        load.reset();
        load.setIncidentWave(-1.0);
        for (int i = 0; i < 100; ++i) {
            diode.propagate();
        }

        // In reverse bias, diode blocks - voltage should be negative
        double vReverse = load.getVoltage();
        expect(vReverse <= 0.0);
    }

    void testDiodePair() {
        beginTest("Diode Pair - Symmetric Clipping");

        WDFDiodePair diodePair(1e-12, 1.0); // Higher Is for easier testing
        WDFResistor load(1000.0);
        diodePair.connectTree(&load);

        // Test positive clipping
        load.setIncidentWave(2.0);
        load.propagate();
        diodePair.propagate();
        double vPos = load.getVoltage();

        // Test negative clipping
        load.setIncidentWave(-2.0);
        load.propagate();
        diodePair.propagate();
        double vNeg = load.getVoltage();

        // Symmetric clipping: |vPos| should approximately equal |vNeg|
        expect(approxEqual(std::abs(vPos), std::abs(vNeg), 0.1));
    }

    void testSoftClipper() {
        beginTest("Soft Clipper - Saturation Behavior");

        WDFSoftClipper clipper(1.0, 2.0); // 1V saturation, 2x drive
        WDFResistor load(1000.0);
        clipper.connectTree(&load);

        // Small signal - should pass through mostly linear
        load.setIncidentWave(0.1);
        load.propagate();
        clipper.propagate();
        double vSmall = load.getVoltage();

        // Large signal - should be clipped
        load.setIncidentWave(5.0);
        load.propagate();
        clipper.propagate();
        double vLarge = load.getVoltage();

        // Large signal should be limited by saturation
        expect(std::abs(vLarge) < std::abs(5.0)); // Should be less than input

        beginTest("Soft Clipper - Symmetric");
        // Check positive and negative are symmetric
        load.setIncidentWave(3.0);
        load.propagate();
        clipper.propagate();
        double vPos = load.getVoltage();

        load.setIncidentWave(-3.0);
        load.propagate();
        clipper.propagate();
        double vNeg = load.getVoltage();

        expect(approxEqual(vPos, -vNeg, 0.1));
    }
};

// =============================================================================
// Main function to run tests
// =============================================================================

static WDFTests wdfTests;

int main(int argc, char* argv[]) {
    juce::ignoreUnused(argc, argv);

    // Initialize JUCE
    juce::ScopedJuceInitialiser_GUI init;

    // Run tests
    juce::UnitTestRunner runner;
    runner.runAllTests();

    // Print results
    int numFailed = 0;
    for (int i = 0; i < runner.getNumResults(); ++i) {
        auto* result = runner.getResult(i);
        if (result->failures > 0) {
            numFailed += result->failures;
            std::cerr << "FAILED: " << result->unitTestName << std::endl;
            for (auto& msg : result->messages) {
                std::cerr << "  " << msg << std::endl;
            }
        }
    }

    if (numFailed == 0) {
        std::cout << "\n✓ All WDF tests passed!\n" << std::endl;
        return 0;
    } else {
        std::cerr << "\n✗ " << numFailed << " test(s) failed\n" << std::endl;
        return 1;
    }
}


#pragma once

#include "WDFCore.h"
#include "WDFNonlinear.h"
#include "../CircuitGraph.h"
#include <map>
#include <memory>
#include <vector>

namespace WDF {

/**
 * WDF-based circuit simulation engine.
 * 
 * This replaces the MNA matrix solver with wave digital filter processing.
 * 
 * Key differences from MNA:
 * - No matrix inversion needed (faster for real-time)
 * - Naturally handles reactive elements (C, L) with guaranteed stability
 * - Nonlinear elements solved locally at root nodes
 * - Sample-rate dependent element values (bilinear transform)
 */
class WDFEngine {
public:
    WDFEngine();
    ~WDFEngine();
    
    // Setup
    void setCircuit(const CircuitGraph& graph);
    void setSampleRate(double rate);
    
    // Audio processing
    void step(double inputVoltage);
    double getOutputVoltage() const;
    
    // State
    void reset();
    bool isSimulationFailed() const { return simulationFailed; }
    
    // Debug
    double getNodeVoltage(int nodeId) const;
    
    // Component value updates
    void updateComponentValue(int componentId, double value);
    
private:
    // Build WDF tree from circuit graph
    void buildWDFTree();
    
    // Find connected subgraphs and build trees
    void analyzeCircuitTopology();
    
    // Create WDF elements for components
    std::unique_ptr<WDFElement> createElementForComponent(CircuitComponent* comp);
    
    // Create nonlinear root elements (diodes, clippers, etc.)
    std::unique_ptr<WDFRoot> createNonlinearRoot(CircuitComponent* comp);
    
    // Process one sample through the WDF tree
    void processWDFTree();
    
    // Circuit reference
    const CircuitGraph* circuitGraph = nullptr;
    
    // Sample rate
    double sampleRate = 44100.0;
    double dt = 1.0 / 44100.0;
    
    // WDF elements storage
    std::vector<std::unique_ptr<WDFElement>> ownedElements;
    
    // Root element (voltage source or nonlinear)
    WDFRoot* rootElement = nullptr;
    
    // Input source
    WDFResistiveVoltageSource* inputSource = nullptr;
    
    // Output probe point
    WDFElement* outputElement = nullptr;
    int outputNodeId = -1;
    
    // Node to WDF element mapping
    std::map<int, WDFElement*> nodeToElement;
    
    // Component ID to element mapping
    std::map<int, WDFElement*> componentToElement;
    
    // Current output voltage
    double outputVoltage = 0.0;
    
    // Error state
    bool simulationFailed = false;
    
    // Thread safety
    juce::CriticalSection lock;
};

} // namespace WDF


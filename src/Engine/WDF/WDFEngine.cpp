#include "WDFEngine.h"
#include "../Components/AudioInput.h"
#include "../Components/AudioOutput.h"
#include "../Components/Capacitor.h"
#include "../Components/Component.h"
#include "../Components/Diode.h"
#include "../Components/DiodePair.h"
#include "../Components/Inductor.h"
#include "../Components/Potentiometer.h"
#include "../Components/Resistor.h"
#include "../Components/SoftClipper.h"
#include "../Components/Switch.h"
#include "../Components/VacuumTube.h"
#include "WDFNonlinear.h"

#include <algorithm>
#include <set>
#include <queue>
#include <iostream>

#define WDF_DEBUG(msg) std::cerr << "[WDF] " << msg << std::endl

namespace WDF {

WDFEngine::WDFEngine() {}

WDFEngine::~WDFEngine() = default;

void WDFEngine::setCircuit(const CircuitGraph& graph) {
    const juce::ScopedLock sl(lock);
    circuitGraph = &graph;
    buildWDFTree();
}

void WDFEngine::setSampleRate(double rate) {
    const juce::ScopedLock sl(lock);
    sampleRate = rate;
    dt = 1.0 / rate;
    
    // Update all sample-rate dependent elements
    for (auto& elem : ownedElements) {
        if (auto* cap = dynamic_cast<WDFCapacitor*>(elem.get())) {
            cap->setSampleRate(rate);
        } else if (auto* ind = dynamic_cast<WDFInductor*>(elem.get())) {
            ind->setSampleRate(rate);
        }
    }
    
    if (circuitGraph) {
        buildWDFTree();
    }
}

void WDFEngine::reset() {
    const juce::ScopedLock sl(lock);
    for (auto& elem : ownedElements) {
        elem->reset();
    }
    outputVoltage = 0.0;
    simulationFailed = false;
}

void WDFEngine::buildWDFTree() {
    if (!circuitGraph) return;
    
    const juce::ScopedLock sl(circuitGraph->getLock());
    
    // Clear existing elements
    ownedElements.clear();
    nodeToElement.clear();
    componentToElement.clear();
    rootElement = nullptr;
    inputSource = nullptr;
    outputElement = nullptr;
    simulationFailed = false;
    
    // Analyze circuit and create WDF structure
    analyzeCircuitTopology();
}

void WDFEngine::analyzeCircuitTopology() {
    // This method builds a WDF tree from the circuit graph.
    // 
    // Strategy:
    // 1. Find the audio input (becomes root voltage source)
    // 2. Find the audio output (observation point)
    // 3. Build WDF tree from components between input and output
    //
    // For simple RC/RL circuits, we use series/parallel adaptors.
    // For complex topologies, we'd use R-type adaptors.
    
    const auto& components = circuitGraph->getComponents();
    const auto& wires = circuitGraph->getWires();
    
    WDF_DEBUG("analyzeCircuitTopology: " << components.size() << " components, " << wires.size() << " wires");
    
    if (components.empty()) {
        WDF_DEBUG("No components, returning");
        return;
    }
    
    // Find audio input and output
    CircuitComponent* audioIn = nullptr;
    CircuitComponent* audioOut = nullptr;
    std::vector<CircuitComponent*> passiveComponents;
    std::vector<CircuitComponent*> nonlinearComponents;
    
    for (const auto& comp : components) {
        switch (comp->getType()) {
            case ComponentType::AudioInput:
                audioIn = comp.get();
                break;
            case ComponentType::AudioOutput:
                audioOut = comp.get();
                break;
            case ComponentType::VacuumTube:
            case ComponentType::Diode:
            case ComponentType::DiodePair:
            case ComponentType::SoftClipper:
                nonlinearComponents.push_back(comp.get());
                break;
            case ComponentType::Ground:
                // Skip ground markers
                break;
            default:
                passiveComponents.push_back(comp.get());
                break;
        }
    }
    
    WDF_DEBUG("audioIn=" << (audioIn != nullptr) << " audioOut=" << (audioOut != nullptr));
    
    if (!audioIn) {
        // No input, can't process
        WDF_DEBUG("No audio input, simulation failed");
        simulationFailed = true;
        return;
    }
    
    // Track the output node for probing
    int outputNodeId = -1;
    for (const auto& comp : components) {
        if (comp->getType() == ComponentType::AudioOutput) {
            outputNodeId = comp->getNode1();
            break;
        }
    }
    
    // Create WDF elements for all passive components
    std::vector<WDFElement*> wdfElements;
    
    for (auto* comp : passiveComponents) {
        auto elem = createElementForComponent(comp);
        if (elem) {
            WDFElement* ptr = elem.get();
            componentToElement[comp->getId()] = ptr;
            wdfElements.push_back(ptr);
            
            // Map nodes to elements for voltage probing
            int node1 = comp->getNode1();
            int node2 = comp->getNode2();
            if (node1 >= 0) nodeToElement[node1] = ptr;
            if (node2 >= 0) nodeToElement[node2] = ptr;
            
            ownedElements.push_back(std::move(elem));
        }
    }
    
    // Store output node ID for special handling
    this->outputNodeId = outputNodeId;
    
    // Check if there's a valid path from audio input to audio output
    bool hasValidPath = false;
    if (audioIn && audioOut) {
        int inputNode = audioIn->getNode1();
        int outputNode = audioOut->getNode1();
        
        WDF_DEBUG("Path check: inputNode=" << inputNode << " outputNode=" << outputNode);
        
        // Build reachable set using BFS through wires AND components
        std::set<int> reachableNodes;
        std::queue<int> toVisit;
        toVisit.push(inputNode);
        reachableNodes.insert(inputNode);
        
        // Get wires from the circuit graph
        const auto& wires = circuitGraph->getWires();
        WDF_DEBUG("Wires for path check: " << wires.size());
        for (const auto& wire : wires) {
            WDF_DEBUG("  Wire: " << wire.nodeA << " <-> " << wire.nodeB);
        }
        
        while (!toVisit.empty()) {
            int currentNode = toVisit.front();
            toVisit.pop();
            
            // Traverse through wires
            for (const auto& wire : wires) {
                if (wire.nodeA == currentNode && reachableNodes.find(wire.nodeB) == reachableNodes.end()) {
                    reachableNodes.insert(wire.nodeB);
                    toVisit.push(wire.nodeB);
                }
                if (wire.nodeB == currentNode && reachableNodes.find(wire.nodeA) == reachableNodes.end()) {
                    reachableNodes.insert(wire.nodeA);
                    toVisit.push(wire.nodeA);
                }
            }
            
            // Traverse through passive components
            for (auto* comp : passiveComponents) {
                int n1 = comp->getNode1();
                int n2 = comp->getNode2();
                
                if (n1 == currentNode && reachableNodes.find(n2) == reachableNodes.end()) {
                    reachableNodes.insert(n2);
                    toVisit.push(n2);
                }
                if (n2 == currentNode && reachableNodes.find(n1) == reachableNodes.end()) {
                    reachableNodes.insert(n1);
                    toVisit.push(n1);
                }
            }
        }
        
        WDF_DEBUG("Reachable nodes: " << reachableNodes.size());
        for (int n : reachableNodes) {
            WDF_DEBUG("  Reachable: " << n);
        }
        
        hasValidPath = (reachableNodes.find(outputNode) != reachableNodes.end());
        WDF_DEBUG("hasValidPath=" << hasValidPath);
    }
    
    if (!hasValidPath) {
        // No valid path from input to output - output silence
        WDF_DEBUG("No valid path from input to output");
        simulationFailed = false;  // Not a failure, just no signal path
        rootElement = nullptr;
        outputElement = nullptr;
        return;
    }
    
    WDF_DEBUG("Valid path found, wdfElements count: " << wdfElements.size());
    
    if (wdfElements.empty()) {
        // Direct connection from input to output (no passive elements)
        // Create a simple passthrough: voltage source with small load resistor
        WDF_DEBUG("Creating passthrough circuit");
        auto source = std::make_unique<WDFIdealVoltageSource>(0.0);
        auto load = std::make_unique<WDFResistor>(10000.0);  // Load resistor
        
        source->connectTree(load.get());
        rootElement = source.get();
        outputElement = load.get();
        
        // Map ALL reachable nodes for probing (they're all at the same potential)
        // This includes input node, output node, and any junction nodes connected by wires
        if (audioIn) {
            int inputNode = audioIn->getNode1();
            std::set<int> connectedNodes;
            std::queue<int> toVisit;
            toVisit.push(inputNode);
            connectedNodes.insert(inputNode);
            
            const auto& wires = circuitGraph->getWires();
            WDF_DEBUG("Starting node mapping from node " << inputNode << ", wires: " << wires.size());
            
            while (!toVisit.empty()) {
                int current = toVisit.front();
                toVisit.pop();
                nodeToElement[current] = load.get();
                WDF_DEBUG("Mapped node " << current);
                
                for (const auto& wire : wires) {
                    if (wire.nodeA == current && connectedNodes.find(wire.nodeB) == connectedNodes.end()) {
                        connectedNodes.insert(wire.nodeB);
                        toVisit.push(wire.nodeB);
                    }
                    if (wire.nodeB == current && connectedNodes.find(wire.nodeA) == connectedNodes.end()) {
                        connectedNodes.insert(wire.nodeA);
                        toVisit.push(wire.nodeA);
                    }
                }
            }
            WDF_DEBUG("Total mapped nodes: " << nodeToElement.size());
        }
        
        ownedElements.push_back(std::move(load));
        ownedElements.push_back(std::move(source));
        WDF_DEBUG("Passthrough circuit created successfully");
        return;
    }
    
    // Build tree structure based on circuit topology
    // For now, we use a simplified approach:
    // - Put all elements in series from input to output (for series circuits)
    // - Or in parallel to ground (for parallel circuits)
    
    // Determine if this is primarily a series or parallel topology
    // by looking at node connections
    
    int groundId = circuitGraph->getGroundNodeId();
    
    // Count how many elements connect to ground
    int groundConnections = 0;
    for (auto* comp : passiveComponents) {
        if (comp->getNode1() == groundId || comp->getNode2() == groundId) {
            groundConnections++;
        }
    }
    
    bool isParallelDominant = (groundConnections > passiveComponents.size() / 2);
    
    // Build the WDF tree
    WDFElement* treeRoot = nullptr;
    
    if (wdfElements.size() == 1) {
        // Single element - connect directly
        treeRoot = wdfElements[0];
        outputElement = treeRoot;
    }
    else if (wdfElements.size() == 2) {
        // Two elements - series or parallel
        if (isParallelDominant) {
            auto adaptor = std::make_unique<WDFParallelAdaptor>();
            adaptor->connectChildren(wdfElements[0], wdfElements[1]);
            treeRoot = adaptor.get();
            outputElement = wdfElements[0];  // First element as output
            ownedElements.push_back(std::move(adaptor));
        } else {
            auto adaptor = std::make_unique<WDFSeriesAdaptor>();
            adaptor->connectChildren(wdfElements[0], wdfElements[1]);
            treeRoot = adaptor.get();
            outputElement = adaptor.get();
            ownedElements.push_back(std::move(adaptor));
        }
    }
    else {
        // Multiple elements - build tree recursively
        // Use a combination of series and parallel adaptors
        
        // Group elements by their connection pattern
        std::vector<WDFElement*> seriesGroup;
        std::vector<WDFElement*> parallelGroup;
        
        for (size_t i = 0; i < passiveComponents.size(); ++i) {
            auto* comp = passiveComponents[i];
            auto* elem = wdfElements[i];
            
            // Elements with one terminal at ground go in parallel
            if (comp->getNode1() == groundId || comp->getNode2() == groundId) {
                parallelGroup.push_back(elem);
            } else {
                seriesGroup.push_back(elem);
            }
        }
        
        // Build parallel subtree
        WDFElement* parallelTree = nullptr;
        if (parallelGroup.size() == 1) {
            parallelTree = parallelGroup[0];
        } else if (parallelGroup.size() >= 2) {
            // Build binary tree of parallel adaptors
            while (parallelGroup.size() > 1) {
                auto adaptor = std::make_unique<WDFParallelAdaptor>();
                adaptor->connectChildren(parallelGroup[0], parallelGroup[1]);
                parallelGroup.erase(parallelGroup.begin());
                parallelGroup[0] = adaptor.get();
                ownedElements.push_back(std::move(adaptor));
            }
            parallelTree = parallelGroup[0];
        }
        
        // Build series subtree
        WDFElement* seriesTree = nullptr;
        if (seriesGroup.size() == 1) {
            seriesTree = seriesGroup[0];
        } else if (seriesGroup.size() >= 2) {
            while (seriesGroup.size() > 1) {
                auto adaptor = std::make_unique<WDFSeriesAdaptor>();
                adaptor->connectChildren(seriesGroup[0], seriesGroup[1]);
                seriesGroup.erase(seriesGroup.begin());
                seriesGroup[0] = adaptor.get();
                ownedElements.push_back(std::move(adaptor));
            }
            seriesTree = seriesGroup[0];
        }
        
        // Combine series and parallel trees
        if (seriesTree && parallelTree) {
            auto adaptor = std::make_unique<WDFSeriesAdaptor>();
            adaptor->connectChildren(seriesTree, parallelTree);
            treeRoot = adaptor.get();
            ownedElements.push_back(std::move(adaptor));
        } else if (seriesTree) {
            treeRoot = seriesTree;
        } else if (parallelTree) {
            treeRoot = parallelTree;
        }
        
        outputElement = treeRoot;
    }
    
    // Handle nonlinear elements (vacuum tubes, diodes, clippers)
    if (!nonlinearComponents.empty()) {
        auto* nlComp = nonlinearComponents[0];
        
        if (nlComp->getType() == ComponentType::VacuumTube) {
            // Vacuum tube handling
            auto* tube = static_cast<VacuumTube*>(nlComp);
            
            auto triode = std::make_unique<WDFTriode>();
            switch (tube->getTubeType()) {
                case VacuumTube::TubeType::Type12AX7:
                    triode->setTubeType(WDFTriode::TubeType::Type12AX7);
                    break;
                case VacuumTube::TubeType::Type12AT7:
                    triode->setTubeType(WDFTriode::TubeType::Type12AT7);
                    break;
                case VacuumTube::TubeType::Type12AU7:
                    triode->setTubeType(WDFTriode::TubeType::Type12AU7);
                    break;
                case VacuumTube::TubeType::TypeEL34:
                    triode->setTubeType(WDFTriode::TubeType::TypeEL34);
                    break;
                default:
                    break;
            }
            
            if (treeRoot) {
                triode->connectTree(treeRoot);
            }
            
            rootElement = triode.get();
            componentToElement[tube->getId()] = triode.get();
            ownedElements.push_back(std::move(triode));
        }
        else {
            // Other nonlinear elements (Diode, DiodePair, SoftClipper)
            auto nlRoot = createNonlinearRoot(nlComp);
            if (nlRoot) {
                if (treeRoot) {
                    nlRoot->connectTree(treeRoot);
                }
                
                rootElement = nlRoot.get();
                componentToElement[nlComp->getId()] = nlRoot.get();
                ownedElements.push_back(std::move(nlRoot));
            }
        }
    }
    else {
        // Create ideal voltage source as root
        auto source = std::make_unique<WDFIdealVoltageSource>(0.0);
        if (treeRoot) {
            source->connectTree(treeRoot);
        }
        rootElement = source.get();
        ownedElements.push_back(std::move(source));
    }
}

std::unique_ptr<WDFElement> WDFEngine::createElementForComponent(CircuitComponent* comp) {
    switch (comp->getType()) {
        case ComponentType::Resistor: {
            auto* r = static_cast<Resistor*>(comp);
            return std::make_unique<WDFResistor>(r->getResistance());
        }
        
        case ComponentType::Capacitor: {
            auto* c = static_cast<Capacitor*>(comp);
            return std::make_unique<WDFCapacitor>(c->getCapacitance(), sampleRate);
        }
        
        case ComponentType::Inductor: {
            auto* l = static_cast<Inductor*>(comp);
            return std::make_unique<WDFInductor>(l->getInductance(), sampleRate);
        }
        
        case ComponentType::Potentiometer: {
            // Potentiometer is two resistors in series with a tap
            // For simplicity, we model it as a single variable resistor
            auto* p = static_cast<Potentiometer*>(comp);
            double effectiveR = p->getTotalResistance() * p->getWiperPosition();
            return std::make_unique<WDFResistor>(std::max(1.0, effectiveR));
        }
        
        case ComponentType::Switch: {
            auto* s = static_cast<Switch*>(comp);
            return std::make_unique<WDFSwitch>(s->isClosed());
        }
        
        default:
            return nullptr;
    }
}

std::unique_ptr<WDFRoot> WDFEngine::createNonlinearRoot(CircuitComponent* comp) {
    switch (comp->getType()) {
        case ComponentType::Diode: {
            auto* d = static_cast<Diode*>(comp);
            auto diode = std::make_unique<WDFDiode>(
                d->getSaturationCurrent(),
                d->getEmissionCoefficient()
            );
            return diode;
        }
        
        case ComponentType::DiodePair: {
            auto* dp = static_cast<DiodePair*>(comp);
            auto diodePair = std::make_unique<WDFDiodePair>(
                dp->getSaturationCurrent(),
                dp->getEmissionCoefficient()
            );
            return diodePair;
        }
        
        case ComponentType::SoftClipper: {
            auto* sc = static_cast<SoftClipper*>(comp);
            auto clipper = std::make_unique<WDFSoftClipper>(
                sc->getSaturationVoltage(),
                sc->getDriveGain()
            );
            return clipper;
        }
        
        default:
            return nullptr;
    }
}

void WDFEngine::step(double inputVoltage) {
    const juce::ScopedLock sl(lock);
    
    if (!rootElement || simulationFailed) {
        outputVoltage = 0.0;
        // Debug: uncomment to see why no output
        // DBG("WDF step: no root=<<(rootElement == nullptr) + " failed=<<(simulationFailed));
        return;
    }
    
    // Set input voltage
    if (auto* voltageRoot = dynamic_cast<WDFIdealVoltageSource*>(rootElement)) {
        voltageRoot->setVoltage(inputVoltage);
    } else if (auto* triode = dynamic_cast<WDFTriode*>(rootElement)) {
        // For triode, the input goes to the grid
        triode->setGridVoltage(inputVoltage);
    }
    
    // Process the WDF tree
    processWDFTree();
}

void WDFEngine::processWDFTree() {
    if (!rootElement) return;
    
    // WDF processing:
    // 1. Propagate waves from leaves to root
    // 2. Compute scattering at root
    // 3. Propagate waves from root to leaves
    
    // The propagate() method handles steps 1 and 2
    rootElement->propagate();
    
    // Scatter back down the tree
    WDFElement* tree = nullptr;
    if (auto* vs = dynamic_cast<WDFIdealVoltageSource*>(rootElement)) {
        tree = vs->getTree();
    } else if (auto* triode = dynamic_cast<WDFTriode*>(rootElement)) {
        tree = triode->getTree();
    }
    
    if (tree) {
        // Recursively scatter down through adaptors
        std::function<void(WDFElement*)> scatter;
        scatter = [&scatter](WDFElement* elem) {
            if (auto* adaptor = dynamic_cast<WDFAdaptor*>(elem)) {
                adaptor->scatterToChildren();
                
                // Recurse into children
                if (auto* series = dynamic_cast<WDFSeriesAdaptor*>(adaptor)) {
                    if (series->getChild1()) scatter(series->getChild1());
                    if (series->getChild2()) scatter(series->getChild2());
                } else if (auto* parallel = dynamic_cast<WDFParallelAdaptor*>(adaptor)) {
                    if (parallel->getChild1()) scatter(parallel->getChild1());
                    if (parallel->getChild2()) scatter(parallel->getChild2());
                }
            }
        };
        scatter(tree);
    }
    
    // Get output voltage
    if (outputElement) {
        outputVoltage = outputElement->getVoltage();
    } else if (tree) {
        outputVoltage = tree->getVoltage();
    } else {
        outputVoltage = 0.0;
    }
    
    // Check for NaN/Inf
    if (!std::isfinite(outputVoltage)) {
        outputVoltage = 0.0;
        // Don't mark as failed - just clamp the value
    }
}

double WDFEngine::getOutputVoltage() const {
    return outputVoltage;
}

double WDFEngine::getNodeVoltage(int nodeId) const {
    // Only return voltage for valid nodes
    if (nodeId < 0) {
        return 0.0;
    }
    
    // If probing the output node, return the output voltage
    if (nodeId == outputNodeId && outputElement) {
        return outputVoltage;
    }
    
    // Try to find an element associated with this node
    auto it = nodeToElement.find(nodeId);
    if (it != nodeToElement.end() && it->second) {
        return it->second->getVoltage();
    }
    
    // Fallback: if we have an output voltage, return that for any valid node
    // This ensures the oscilloscope shows SOMETHING when probing
    if (outputElement) {
        return outputVoltage;
    }
    
    return 0.0;
}

void WDFEngine::updateComponentValue(int componentId, double value) {
    const juce::ScopedLock sl(lock);
    
    auto it = componentToElement.find(componentId);
    if (it == componentToElement.end()) return;
    
    WDFElement* elem = it->second;
    
    // Update the element based on its type
    if (auto* res = dynamic_cast<WDFResistor*>(elem)) {
        res->setResistance(value);
    } else if (auto* cap = dynamic_cast<WDFCapacitor*>(elem)) {
        cap->setCapacitance(value);
    } else if (auto* sw = dynamic_cast<WDFSwitch*>(elem)) {
        sw->setClosed(value > 0.5);
    }
    
    // Recalculate port resistances up the tree
    // (In a more sophisticated implementation, we'd only update affected adaptors)
    if (circuitGraph) {
        buildWDFTree();
    }
}

} // namespace WDF


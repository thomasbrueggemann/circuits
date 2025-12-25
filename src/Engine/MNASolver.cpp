#include "MNASolver.h"
#include "CircuitGraph.h"
#include "Components/AudioInput.h"
#include "Components/AudioOutput.h"
#include "Components/Capacitor.h"
#include "Components/Component.h"
#include "Components/Potentiometer.h"
#include "Components/Resistor.h"
#include "Components/Switch.h"
#include "Components/VacuumTube.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <set>

// Debug logging to file
static void logDebug(const juce::String &msg) {
  static std::ofstream logFile("/tmp/mna_debug.log", std::ios::app);
  logFile << msg.toStdString() << std::endl;
  logFile.flush();
  DBG(msg);
}

MNASolver::MNASolver() {}

MNASolver::~MNASolver() = default;

void MNASolver::setCircuit(const CircuitGraph &graph) {
  circuitGraph = &graph;
  buildMatrix();
}

void MNASolver::setSampleRate(double rate) {
  sampleRate = rate;
  dt = 1.0 / rate;

  if (circuitGraph)
    buildMatrix();
}

void MNASolver::buildNodeEquivalenceClasses() {
  // Initialize Union-Find: each node is its own parent
  nodeParent.clear();
  nodeRank.clear();

  for (const auto &node : circuitGraph->getNodes()) {
    nodeParent[node.id] = node.id;
    nodeRank[node.id] = 0;
  }

  // Also ensure all wire endpoints are in the map (handles edge cases)
  for (const auto &wire : circuitGraph->getWires()) {
    if (nodeParent.find(wire.nodeA) == nodeParent.end()) {
      nodeParent[wire.nodeA] = wire.nodeA;
      nodeRank[wire.nodeA] = 0;
    }
    if (nodeParent.find(wire.nodeB) == nodeParent.end()) {
      nodeParent[wire.nodeB] = wire.nodeB;
      nodeRank[wire.nodeB] = 0;
    }
  }

  // Also ensure all component nodes are in the map
  for (const auto &comp : circuitGraph->getComponents()) {
    for (int nodeId : comp->getAllNodes()) {
      if (nodeParent.find(nodeId) == nodeParent.end()) {
        nodeParent[nodeId] = nodeId;
        nodeRank[nodeId] = 0;
      }
    }
  }

  // Union all nodes connected by wires
  for (const auto &wire : circuitGraph->getWires()) {
    unionNodes(wire.nodeA, wire.nodeB);
  }
}

int MNASolver::findNodeRepresentative(int nodeId) {
  // Path compression: make every node point directly to root
  if (nodeParent.find(nodeId) == nodeParent.end())
    return nodeId;

  if (nodeParent[nodeId] != nodeId) {
    nodeParent[nodeId] = findNodeRepresentative(nodeParent[nodeId]);
  }
  return nodeParent[nodeId];
}

void MNASolver::unionNodes(int nodeA, int nodeB) {
  int rootA = findNodeRepresentative(nodeA);
  int rootB = findNodeRepresentative(nodeB);

  if (rootA == rootB)
    return;

  // Union by rank: attach smaller tree under root of larger tree
  if (nodeRank[rootA] < nodeRank[rootB]) {
    nodeParent[rootA] = rootB;
  } else if (nodeRank[rootA] > nodeRank[rootB]) {
    nodeParent[rootB] = rootA;
  } else {
    nodeParent[rootB] = rootA;
    nodeRank[rootA]++;
  }
}

void MNASolver::buildMatrix() {
  if (!circuitGraph)
    return;

  const juce::ScopedLock sl(circuitGraph->getLock());

  // Reset simulation failed flag when rebuilding matrix
  simulationFailed = false;

  // Build node equivalence classes from wires (merge connected nodes)
  buildNodeEquivalenceClasses();

  // Map node representatives to matrix indices (ground is not included)
  nodeToIndex.clear();
  std::map<int, int> representativeToIndex; // Map unique representatives to indices
  int index = 0;

  // First, find the ground representative
  int groundNodeId = circuitGraph->getGroundNodeId();
  int groundRepresentative = findNodeRepresentative(groundNodeId);
  groundIndex = groundNodeId;

  // Collect all unique node IDs (from nodes list, components, and wires)
  std::set<int> allNodeIds;
  for (const auto &node : circuitGraph->getNodes()) {
    allNodeIds.insert(node.id);
  }
  for (const auto &comp : circuitGraph->getComponents()) {
    allNodeIds.insert(comp->getNode1());
    allNodeIds.insert(comp->getNode2());
    // Handle components with more than 2 terminals
    for (int nodeId : comp->getAllNodes()) {
      allNodeIds.insert(nodeId);
    }
  }
  for (const auto &wire : circuitGraph->getWires()) {
    allNodeIds.insert(wire.nodeA);
    allNodeIds.insert(wire.nodeB);
  }

  // Assign matrix indices to unique node representatives
  for (int nodeId : allNodeIds) {
    // Ensure node is in nodeParent map
    if (nodeParent.find(nodeId) == nodeParent.end()) {
      nodeParent[nodeId] = nodeId;
      nodeRank[nodeId] = 0;
    }

    int rep = findNodeRepresentative(nodeId);

    // Check if this representative is connected to ground
    if (rep == groundRepresentative) {
      nodeToIndex[nodeId] = -1; // Ground has no index
    } else if (representativeToIndex.find(rep) == representativeToIndex.end()) {
      // New representative, assign an index
      representativeToIndex[rep] = index;
      nodeToIndex[nodeId] = index;
      index++;
    } else {
      // Representative already has an index, use it
      nodeToIndex[nodeId] = representativeToIndex[rep];
    }
  }

  numNodes = index;
  outputNodeIndex = -1;

  // Count voltage sources (only those that aren't shorted by wire merging)
  numVSources = 0;
  for (const auto &comp : circuitGraph->getComponents()) {
    if (comp->getType() == ComponentType::AudioInput) {
      int n1 = nodeToIndex.count(comp->getNode1())
                   ? nodeToIndex[comp->getNode1()]
                   : -1;
      int n2 = nodeToIndex.count(comp->getNode2())
                   ? nodeToIndex[comp->getNode2()]
                   : -1;

      if (n1 != n2) {
        numVSources++;
      }
    }
  }

  matrixSize = numNodes + numVSources;

  if (matrixSize == 0)
    return;

  // Initialize matrices
  G.assign(matrixSize, std::vector<double>(matrixSize, 0.0));
  z.assign(matrixSize, 0.0);
  G_static.assign(matrixSize, std::vector<double>(matrixSize, 0.0));
  z_static.assign(matrixSize, 0.0);
  x.assign(matrixSize, 0.0);
  xPrev.assign(matrixSize, 0.0);
  LU.assign(matrixSize, std::vector<double>(matrixSize, 0.0));
  pivot.assign(matrixSize, 0);

  // Initialize capacitor state if necessary
  if (capVoltages.size() !=
      static_cast<size_t>(
          numVSources)) { // This is wrong, should be based on capacitors
    // We'll reset state if the number of capacitors changed
  }

  // Clear cached lists
  cachedCapacitors.clear();
  cachedNonLinear.clear();
  cachedAudioInputs.clear();

  clearMatrix();

  // Stamp all components
  int vsourceIndex = numNodes;

  for (const auto &comp : circuitGraph->getComponents()) {
    int n1 = nodeToIndex.count(comp->getNode1()) ? nodeToIndex[comp->getNode1()] : -1;
    int n2 = nodeToIndex.count(comp->getNode2()) ? nodeToIndex[comp->getNode2()] : -1;

    switch (comp->getType()) {
    case ComponentType::Resistor: {
      auto *r = static_cast<Resistor *>(comp.get());
      // Skip if both terminals merged to same node (shorted by wires)
      if (n1 != n2) {
        stampResistorStatic(n1, n2, r->getResistance());
      }
      break;
    }
    case ComponentType::Capacitor: {
      auto *c = static_cast<Capacitor *>(comp.get());
      // Skip if both terminals merged to same node (shorted by wires)
      if (n1 != n2) {
        double geq = 2.0 * c->getCapacitance() / dt;
        stampResistorStatic(n1, n2, 1.0 / geq);

        // Cache for audio thread
        cachedCapacitors.push_back({n1, n2, c->getCapacitance(),
                                    static_cast<int>(cachedCapacitors.size()),
                                    c});
      }
      break;
    }
    case ComponentType::Potentiometer: {
      auto *p = static_cast<Potentiometer *>(comp.get());
      double pos = p->getWiperPosition();
      double totalR = p->getTotalResistance();
      double r1 = totalR * pos;
      double r2 = totalR * (1.0 - pos);

      int n3 = nodeToIndex.count(p->getNode3()) ? nodeToIndex[p->getNode3()] : -1;

      r1 = std::max(r1, 0.01);
      r2 = std::max(r2, 0.01);

      // Only stamp if nodes are different
      if (n1 != n3) stampResistorStatic(n1, n3, r1);
      if (n3 != n2) stampResistorStatic(n3, n2, r2);
      break;
    }
    case ComponentType::Switch: {
      auto *s = static_cast<Switch *>(comp.get());
      double resistance = s->isClosed() ? 0.001 : 1e9;
      stampResistorStatic(n1, n2, resistance);
      break;
    }
    case ComponentType::AudioInput: {
      auto *audioIn = static_cast<AudioInput *>(comp.get());
      // Only stamp voltage source if terminals are different
      // (not shorted together by wires)
      if (n1 != n2) {
        if (vsourceIndex >= matrixSize) {
          logDebug("ERROR: vsourceIndex " + juce::String(vsourceIndex) +
                   " >= matrixSize " + juce::String(matrixSize));
        } else {
          stampVoltageSourceStatic(n1, n2, vsourceIndex, 0.0);
          cachedAudioInputs.push_back({audioIn, vsourceIndex});
          vsourceIndex++;
        }
      }
      break;
    }
    case ComponentType::AudioOutput: {
      if (outputNodeIndex < 0)
        outputNodeIndex = n1 >= 0 ? n1 : n2;
      break;
    }
    case ComponentType::VacuumTube: {
      auto *tube = static_cast<VacuumTube *>(comp.get());
      int grid = n1;
      int cathode = n2;
      int plate = nodeToIndex.count(tube->getPlateNode()) ? nodeToIndex[tube->getPlateNode()] : -1;
      cachedNonLinear.push_back({tube, grid, cathode, plate});
      break;
    }
    default:
      break;
    }
  }

  // Wires are now handled via node merging (Union-Find) - nodes connected by
  // wires share the same matrix index, making them true short circuits.
  // No resistance stamping needed for wires.

  // Add shunt conductance to ground for all nodes to prevent singular matrices
  // and stabilize floating nodes. This acts as a very high resistance (1 GΩ) to ground.
  constexpr double SHUNT_CONDUCTANCE = 1e-9;
  for (int i = 0; i < numNodes; ++i) {
    G_static[i][i] += SHUNT_CONDUCTANCE;
  }

  // Also add a small diagonal perturbation to voltage source rows if present
  // to help with numerical stability
  for (int i = numNodes; i < matrixSize; ++i) {
    G_static[i][i] += 1e-12;
  }

  // Initial copy to active matrices
  G = G_static;
  z = z_static;

  // Update capacitor state vectors size
  if (capVoltages.size() != cachedCapacitors.size()) {
    capVoltages.assign(cachedCapacitors.size(), 0.0);
    capCurrents.assign(cachedCapacitors.size(), 0.0);
  }
}

void MNASolver::clearMatrix() {
  for (auto &row : G)
    std::fill(row.begin(), row.end(), 0.0);
  std::fill(z.begin(), z.end(), 0.0);
}

void MNASolver::stampResistor(int node1, int node2, double resistance) {
  if (resistance <= 0.0)
    return;

  double g = 1.0 / resistance;

  if (node1 >= 0)
    G[node1][node1] += g;
  if (node2 >= 0)
    G[node2][node2] += g;
  if (node1 >= 0 && node2 >= 0) {
    G[node1][node2] -= g;
    G[node2][node1] -= g;
  }
}

void MNASolver::stampCapacitor(int node1, int node2, double capacitance) {
  // Trapezoidal integration companion model
  double geq = 2.0 * capacitance / dt;
  stampResistor(node1, node2, 1.0 / geq);
}

void MNASolver::stampVoltageSource(int node1, int node2, int branchIndex,
                                   double voltage) {
  // Voltage source stamping in MNA
  // G(n1, branch) = 1
  // G(n2, branch) = -1
  // G(branch, n1) = 1
  // G(branch, n2) = -1
  // z(branch) = voltage

  if (node1 >= 0) {
    G[node1][branchIndex] += 1.0;
    G[branchIndex][node1] += 1.0;
  }
  if (node2 >= 0) {
    G[node2][branchIndex] -= 1.0;
    G[branchIndex][node2] -= 1.0;
  }

  z[branchIndex] = voltage;
}

void MNASolver::stampCurrentSource(int node1, int node2, double current) {
  if (node1 >= 0)
    z[node1] -= current;
  if (node2 >= 0)
    z[node2] += current;
}

void MNASolver::stampVCCS(int n1, int n2, int nc1, int nc2, double gm) {
  // Voltage-controlled current source
  // Current flows from n1 to n2, controlled by voltage nc1 to nc2
  if (n1 >= 0 && nc1 >= 0)
    G[n1][nc1] += gm;
  if (n1 >= 0 && nc2 >= 0)
    G[n1][nc2] -= gm;
  if (n2 >= 0 && nc1 >= 0)
    G[n2][nc1] -= gm;
  if (n2 >= 0 && nc2 >= 0)
    G[n2][nc2] += gm;
}

bool MNASolver::solve() {
  if (matrixSize == 0 || !circuitGraph)
    return false;

  simulationFailed = false;

  const juce::ScopedLock sl(circuitGraph->getLock());

  // Check for nonlinear components
  bool hasNonlinear = false;
  for (const auto &comp : circuitGraph->getComponents()) {
    if (comp->isNonLinear()) {
      hasNonlinear = true;
      break;
    }
  }

  if (hasNonlinear)
    return solveNonlinear();

  // LU decomposition and solve
  if (!luDecompose())
    return false;

  luSolve();
  return true;
}

bool MNASolver::solveNonlinear(int maxIterations, double tolerance) {
  simulationFailed = false;

  // Newton-Raphson iteration
  for (int iter = 0; iter < maxIterations; ++iter) {
    // Rebuild matrix from static base + nonlinear stubs
    G = G_static;
    z = z_static;

    // Add capacitor-like sources if needed (already in z from step())
    // Actually, step() should have set z before calling solve()

    updateNonlinearStamps();

    if (!luDecompose())
      return false;

    luSolve();

    // Check convergence
    double maxDiff = 0.0;
    for (int i = 0; i < matrixSize; ++i) {
      maxDiff = std::max(maxDiff, std::abs(x[i] - xPrev[i]));
    }

    xPrev = x;

    if (maxDiff < tolerance)
      return true;
  }

  return true; // Return result even if not fully converged
}

void MNASolver::updateNonlinearStamps() {
  // Update vacuum tube stamps based on current operating point
  for (const auto &cached : cachedNonLinear) {
    auto *tube = cached.tube;

    int gridNode = cached.gridNode;
    int cathodeNode = cached.cathodeNode;
    int plateNode = cached.plateNode;

    // Get current voltages
    double vg = gridNode >= 0 ? x[gridNode] : 0.0;
    double vk = cathodeNode >= 0 ? x[cathodeNode] : 0.0;
    double vp = plateNode >= 0 ? x[plateNode] : 0.0;

    double vgk = vg - vk;
    double vpk = vp - vk;

    // Calculate plate current and transconductance
    double ip, gm, gp;
    tube->calculateDerivatives(vgk, vpk, ip, gm, gp);

    // Stamp linearized model (Norton equivalent)
    // Plate current source
    double ieq = ip - gm * vgk - gp * vpk;
    stampCurrentSource(plateNode, cathodeNode, ieq);

    // Transconductance (VCCS from grid to plate)
    stampVCCS(plateNode, cathodeNode, gridNode, cathodeNode, gm);

    // Plate conductance
    if (plateNode >= 0)
      G[plateNode][plateNode] += gp;
    if (cathodeNode >= 0)
      G[cathodeNode][cathodeNode] += gp;
    if (plateNode >= 0 && cathodeNode >= 0) {
      G[plateNode][cathodeNode] -= gp;
      G[cathodeNode][plateNode] -= gp;
    }
  }
}

bool MNASolver::luDecompose() {
  // Copy G to LU
  LU = G;

  // Check for obviously problematic matrices
  for (int i = 0; i < matrixSize; ++i) {
    bool hasNonZero = false;
    for (int j = 0; j < matrixSize; ++j) {
      if (std::abs(LU[i][j]) > 1e-20) {
        hasNonZero = true;
        break;
      }
    }
    if (!hasNonZero) {
      logDebug("ERROR: Row " + juce::String(i) + " is all zeros!");
      simulationFailed = true;
      debugPrintMatrix();
      return false;
    }
  }

  for (int i = 0; i < matrixSize; ++i)
    pivot[i] = i;

  for (int k = 0; k < matrixSize; ++k) {
    // Find pivot (partial pivoting for numerical stability)
    double maxVal = 0.0;
    int maxRow = k;

    for (int i = k; i < matrixSize; ++i) {
      if (std::abs(LU[i][k]) > maxVal) {
        maxVal = std::abs(LU[i][k]);
        maxRow = i;
      }
    }

    // Swap rows before checking for singularity
    if (maxRow != k) {
      std::swap(LU[k], LU[maxRow]);
      std::swap(pivot[k], pivot[maxRow]);
    }

    // Check for singular/near-singular matrix after pivoting
    if (std::abs(LU[k][k]) < 1e-15) {
      // Add a small perturbation to make it solvable (regularization)
      LU[k][k] = 1e-12;
      simulationFailed = true;
      logDebug("Singular matrix detected at pivot " + juce::String(k));
      debugPrintMatrix();
    }

    // Elimination
    for (int i = k + 1; i < matrixSize; ++i) {
      LU[i][k] /= LU[k][k];
      for (int j = k + 1; j < matrixSize; ++j) {
        LU[i][j] -= LU[i][k] * LU[k][j];
      }
    }
  }

  return true;
}

void MNASolver::luSolve() {
  // Apply pivot to z
  std::vector<double> b(matrixSize);
  for (int i = 0; i < matrixSize; ++i)
    b[i] = z[pivot[i]];

  // Forward substitution (Ly = b)
  std::vector<double> y(matrixSize);
  for (int i = 0; i < matrixSize; ++i) {
    y[i] = b[i];
    for (int j = 0; j < i; ++j)
      y[i] -= LU[i][j] * y[j];
  }

  // Back substitution (Ux = y)
  for (int i = matrixSize - 1; i >= 0; --i) {
    x[i] = y[i];
    for (int j = i + 1; j < matrixSize; ++j)
      x[i] -= LU[i][j] * x[j];
    x[i] /= LU[i][i];

    // NaN protection: clamp to reasonable voltage range
    if (!std::isfinite(x[i])) {
      x[i] = 0.0;
      simulationFailed = true;
    }
  }
}

void MNASolver::step(double inputVoltage) {
  if (matrixSize == 0 || !circuitGraph)
    return;

  // Reset to static base
  G = G_static;
  z = z_static;

  const juce::ScopedLock sl(circuitGraph->getLock());

  // Update capacitor companion models (add to z)
  for (const auto &cap : cachedCapacitors) {
    int n1 = cap.node1;
    int n2 = cap.node2;

    double v1 = n1 >= 0 ? x[n1] : 0.0;
    double v2 = n2 >= 0 ? x[n2] : 0.0;
    double vCap = v1 - v2;

    double geq = 2.0 * cap.capacitance / dt;
    double ieq =
        geq * capVoltages[cap.stateIndex] + capCurrents[cap.stateIndex];

    // Add equivalent current source
    stampCurrentSource(n1, n2, ieq);

    // Update state for NEXT step
    capVoltages[cap.stateIndex] = vCap;
    capCurrents[cap.stateIndex] = geq * vCap - ieq;
  }

  for (const auto &input : cachedAudioInputs) {
    if (input.branchIndex >= 0 && input.branchIndex < matrixSize) {
      z[input.branchIndex] = input.component->getScaledVoltage();
    }
  }

  // Solve the system
  solve();
}

void MNASolver::stampResistorStatic(int node1, int node2, double resistance) {
  double g = 1.0 / std::max(resistance, 1e-9);
  if (node1 >= 0)
    G_static[node1][node1] += g;
  if (node2 >= 0)
    G_static[node2][node2] += g;
  if (node1 >= 0 && node2 >= 0) {
    G_static[node1][node2] -= g;
    G_static[node2][node1] -= g;
  }
}

void MNASolver::stampVoltageSourceStatic(int node1, int node2, int branchIndex,
                                         double voltage) {
  if (node1 >= 0) {
    G_static[node1][branchIndex] += 1.0;
    G_static[branchIndex][node1] += 1.0;
  }
  if (node2 >= 0) {
    G_static[node2][branchIndex] -= 1.0;
    G_static[branchIndex][node2] -= 1.0;
  }
  z_static[branchIndex] = voltage;
}

double MNASolver::getNodeVoltage(int nodeId) const {
  auto it = nodeToIndex.find(nodeId);
  if (it == nodeToIndex.end() || it->second < 0)
    return 0.0;

  if (it->second >= static_cast<int>(x.size()))
    return 0.0;

  return x[it->second];
}

double MNASolver::getBranchCurrent(int branchId) const {
  int index = numNodes + branchId;
  if (index >= static_cast<int>(x.size()))
    return 0.0;

  return x[index];
}

double MNASolver::getOutputVoltage() const {
  if (outputNodeIndex >= 0 && outputNodeIndex < static_cast<int>(x.size()))
    return x[outputNodeIndex];

  return 0.0;
}

void MNASolver::updateComponentValue(int componentId, double value) {
  if (!circuitGraph)
    return;

  const juce::ScopedLock sl(circuitGraph->getLock());

  // Find component and update
  CircuitComponent *comp =
      const_cast<CircuitComponent *>(circuitGraph->getComponent(componentId));
  if (comp) {
    comp->setValue(value);
    buildMatrix();
  }
}

void MNASolver::debugPrintMatrix() const {
  logDebug("=== MNA Matrix Debug ===");
  logDebug("Matrix size: " + juce::String(matrixSize));
  logDebug("Num nodes: " + juce::String(numNodes));
  logDebug("Num voltage sources: " + juce::String(numVSources));

  if (circuitGraph) {
    logDebug("Circuit Graph Info:");
    logDebug("  Total nodes in graph: " +
             juce::String(circuitGraph->getNodeCount()));
    logDebug("  Total wires: " +
             juce::String((int)circuitGraph->getWires().size()));
    logDebug("  Total junctions: " +
             juce::String((int)circuitGraph->getJunctions().size()));
    logDebug("  Ground node ID: " +
             juce::String(circuitGraph->getGroundNodeId()));

    logDebug("  Wires:");
    for (const auto &wire : circuitGraph->getWires()) {
      logDebug("    Wire " + juce::String(wire.id) + ": " +
               juce::String(wire.nodeA) + " -> " + juce::String(wire.nodeB));
    }

    logDebug("  Components:");
    for (const auto &comp : circuitGraph->getComponents()) {
      logDebug("    " + comp->getName() + " (type " +
               juce::String(static_cast<int>(comp->getType())) + "): nodes " +
               juce::String(comp->getNode1()) + ", " +
               juce::String(comp->getNode2()));
    }

    logDebug("  Junctions:");
    for (const auto &junc : circuitGraph->getJunctions()) {
      logDebug("    Junction node " + juce::String(junc.nodeId));
    }
  }

  logDebug("Node to index mapping:");
  for (const auto &[nodeId, idx] : nodeToIndex) {
    logDebug("  Node " + juce::String(nodeId) + " -> index " +
             juce::String(idx));
  }

  if (matrixSize > 0 && matrixSize <= 10) {
    logDebug("G_static matrix:");
    for (int i = 0; i < matrixSize; ++i) {
      juce::String row;
      for (int j = 0; j < matrixSize; ++j) {
        row += juce::String(G_static[i][j], 6) + " ";
      }
      logDebug("  [" + row + "]");
    }

    logDebug("z_static vector:");
    juce::String zStr;
    for (int i = 0; i < matrixSize; ++i) {
      zStr += juce::String(z_static[i], 6) + " ";
    }
    logDebug("  [" + zStr + "]");
  }

  logDebug("========================");
}

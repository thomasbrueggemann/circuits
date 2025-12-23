#pragma once

#include <map>
#include <vector>

class CircuitGraph;
class Component;

/**
 * Modified Nodal Analysis (MNA) solver for circuit simulation.
 *
 * The MNA formulation creates a system of equations: Gx = z
 * Where:
 *   G = conductance matrix (n+m by n+m)
 *   x = solution vector (node voltages + branch currents)
 *   z = right-hand side vector (current sources + voltage sources)
 *
 * For n nodes and m voltage sources:
 * | G  B | | v |   | i |
 * | C  D | | j | = | e |
 */
class MNASolver {
public:
  MNASolver();
  ~MNASolver();

  // Setup
  void setCircuit(const CircuitGraph &graph);
  void setSampleRate(double sampleRate);

  // Solving
  void buildMatrix();
  bool solve();
  void step(double inputVoltage);

  // Results
  double getNodeVoltage(int nodeId) const;
  double getBranchCurrent(int branchId) const;
  double getOutputVoltage() const;

  // Component value updates (for real-time control)
  void updateComponentValue(int componentId, double value);

private:
  // Cached component info for audio thread safety
  struct CachedCapacitor {
    int node1;
    int node2;
    double capacitance;
    int stateIndex;
    class Capacitor *component;
  };

  struct CachedNonLinear {
    class VacuumTube *tube;
    int gridNode;
    int cathodeNode;
    int plateNode;
  };

  struct CachedAudioInput {
    class AudioInput *component;
    int branchIndex;
  };

  // Build helpers
  void stampResistor(int node1, int node2, double resistance);
  void stampResistorStatic(int node1, int node2, double resistance);
  void stampCapacitor(int node1, int node2, double capacitance);
  void stampVoltageSource(int node1, int node2, int branchIndex,
                          double voltage);
  void stampVoltageSourceStatic(int node1, int node2, int branchIndex,
                                double voltage);
  void stampCurrentSource(int node1, int node2, double current);
  void stampVCCS(int n1, int n2, int nc1, int nc2,
                 double gm); // Voltage-controlled current source

  // Newton-Raphson for nonlinear components
  bool solveNonlinear(int maxIterations = 20, double tolerance = 1e-6);
  void updateNonlinearStamps();

  // Matrix operations
  void clearMatrix();
  bool luDecompose();
  void luSolve();

  // Circuit reference
  const CircuitGraph *circuitGraph = nullptr;

  // Cached lists for audio thread
  std::vector<CachedCapacitor> cachedCapacitors;
  std::vector<CachedNonLinear> cachedNonLinear;
  std::vector<CachedAudioInput> cachedAudioInputs;

  // MNA matrices
  std::vector<std::vector<double>> G; // Conductance matrix
  std::vector<double> z;              // RHS vector
  std::vector<double> x;              // Solution vector
  std::vector<double> xPrev;          // Previous solution (for capacitors)

  // Static matrix parts (for optimization and stability)
  std::vector<std::vector<double>> G_static;
  std::vector<double> z_static;

  // LU decomposition storage
  std::vector<std::vector<double>> LU;
  std::vector<int> pivot;

  // Node mapping (circuit node ID -> matrix index)
  std::map<int, int> nodeToIndex;
  int groundIndex = -1;

  // Dimensions
  int numNodes = 0;    // Number of non-ground nodes
  int numVSources = 0; // Number of voltage sources
  int matrixSize = 0;  // Total matrix size

  // Simulation parameters
  double sampleRate = 44100.0;
  double dt = 1.0 / 44100.0; // Timestep

  // State for capacitors (companion model)
  std::vector<double> capVoltages;
  std::vector<double> capCurrents;

  // Input/output node indices
  int inputNodeIndex = -1;
  int outputNodeIndex = -1;
};

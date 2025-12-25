#pragma once

#include <JuceHeader.h>
#include <memory>
#include <vector>

// Forward declarations
class CircuitComponent;

enum class ComponentType {
  Resistor,
  Capacitor,
  Inductor,
  Potentiometer,
  Switch,
  Diode,
  DiodePair,
  SoftClipper,
  VacuumTube,
  AudioInput,
  AudioOutput,
  Ground
};

struct Node {
  int id = -1;
  juce::String name;
  bool isGround = false;
  juce::Point<float> position;

  Node() = default;
  Node(int id_, const juce::String &name_, bool ground = false)
      : id(id_), name(name_), isGround(ground) {}
};

struct Wire {
  int id = -1;
  int nodeA = -1;
  int nodeB = -1;

  Wire() = default;
  Wire(int id_, int a, int b) : id(id_), nodeA(a), nodeB(b) {}
};

struct Junction {
  int nodeId = -1;
  juce::Point<float> position;

  Junction() = default;
  Junction(int node, juce::Point<float> pos) : nodeId(node), position(pos) {}
};

class CircuitGraph {
public:
  CircuitGraph();
  ~CircuitGraph();

  // Node management
  int createNode(const juce::String &name = "");
  int createGroundNode();
  void addNode(int id, const juce::String &name, bool isGround);
  Node *getNode(int id);
  const Node *getNode(int id) const;
  const std::vector<Node> &getNodes() const { return nodes; }
  int getNodeCount() const { return static_cast<int>(nodes.size()); }
  int getGroundNodeId() const { return groundNodeId; }

  // Component management
  void addComponent(std::unique_ptr<CircuitComponent> component);
  void removeComponent(int componentId);
  CircuitComponent *getComponent(int componentId);
  const CircuitComponent *getComponent(int componentId) const;
  const std::vector<std::unique_ptr<CircuitComponent>> &getComponents() const {
    return components;
  }
  int getComponentCount() const { return static_cast<int>(components.size()); }

  // Wire management
  int connectNodes(int nodeA, int nodeB);
  void removeWire(int wireId);
  const std::vector<Wire> &getWires() const { return wires; }
  Wire *getWireById(int wireId);

  // Junction management (wire-to-wire connections)
  int createJunctionOnWire(int wireId, juce::Point<float> position);
  void addJunction(int nodeId, juce::Point<float> position);
  const std::vector<Junction> &getJunctions() const { return junctions; }
  Junction *getJunctionByNode(int nodeId);
  bool isJunctionNode(int nodeId) const;
  void cleanupOrphanedJunctions();
  int countWiresConnectedToNode(int nodeId) const;

  // Utility
  void clear();
  bool isValid() const;

  // Get components by type
  std::vector<CircuitComponent *> getComponentsByType(ComponentType type) const;

  // Thread safety
  juce::CriticalSection &getLock() const { return graphLock; }

private:
  std::vector<Node> nodes;
  std::vector<std::unique_ptr<CircuitComponent>> components;
  std::vector<Wire> wires;
  std::vector<Junction> junctions;

  int nextNodeId = 0;
  int nextComponentId = 0;
  int nextWireId = 0;
  int groundNodeId = -1;

  mutable juce::CriticalSection graphLock;
};

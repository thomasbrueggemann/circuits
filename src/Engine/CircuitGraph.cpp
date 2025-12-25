#include "CircuitGraph.h"
#include "Components/Component.h"

CircuitGraph::CircuitGraph() {
  // Create ground node by default
  groundNodeId = createGroundNode();
}

CircuitGraph::~CircuitGraph() = default;

int CircuitGraph::createNode(const juce::String &name) {
  const juce::ScopedLock sl(graphLock);
  int id = nextNodeId++;
  juce::String nodeName = name.isEmpty() ? "Node" + juce::String(id) : name;
  nodes.emplace_back(id, nodeName, false);
  return id;
}

int CircuitGraph::createGroundNode() {
  const juce::ScopedLock sl(graphLock);
  if (groundNodeId >= 0)
    return groundNodeId;

  int id = nextNodeId++;
  nodes.emplace_back(id, "GND", true);
  groundNodeId = id;
  return id;
}

void CircuitGraph::addNode(int id, const juce::String &name, bool isGround) {
  const juce::ScopedLock sl(graphLock);
  // Check if node already exists
  for (auto &node : nodes) {
    if (node.id == id) {
      node.name = name;
      node.isGround = isGround;
      if (isGround)
        groundNodeId = id;
      return;
    }
  }

  nodes.emplace_back(id, name, isGround);
  if (isGround)
    groundNodeId = id;

  if (id >= nextNodeId)
    nextNodeId = id + 1;
}

Node *CircuitGraph::getNode(int id) {
  for (auto &node : nodes) {
    if (node.id == id)
      return &node;
  }
  return nullptr;
}

const Node *CircuitGraph::getNode(int id) const {
  for (const auto &node : nodes) {
    if (node.id == id)
      return &node;
  }
  return nullptr;
}

void CircuitGraph::addComponent(std::unique_ptr<CircuitComponent> component) {
  const juce::ScopedLock sl(graphLock);
  if (!component)
    return;

  if (component->getId() < 0)
    component->setId(nextComponentId++);
  else if (component->getId() >= nextComponentId)
    nextComponentId = component->getId() + 1;

  components.push_back(std::move(component));
}

void CircuitGraph::removeComponent(int componentId) {
  const juce::ScopedLock sl(graphLock);

  // Find component first to get its nodes
  CircuitComponent *compToRemove = nullptr;
  for (auto &c : components) {
    if (c->getId() == componentId) {
      compToRemove = c.get();
      break;
    }
  }

  if (!compToRemove)
    return;

  // Get all nodes used by this component
  std::vector<int> componentNodes = compToRemove->getAllNodes();

  // Find all wires connected to these nodes
  std::vector<int> wiresToRemove;
  for (const auto &wire : wires) {
    for (int node : componentNodes) {
      if (wire.nodeA == node || wire.nodeB == node) {
        wiresToRemove.push_back(wire.id);
        break;
      }
    }
  }

  // Remove the connected wires
  for (int wireId : wiresToRemove) {
    wires.erase(
        std::remove_if(wires.begin(), wires.end(),
                       [wireId](const Wire &w) { return w.id == wireId; }),
        wires.end());
  }

  // Remove the component
  components.erase(
      std::remove_if(components.begin(), components.end(),
                     [componentId](const std::unique_ptr<CircuitComponent> &c) {
                       return c->getId() == componentId;
                     }),
      components.end());
}

CircuitComponent *CircuitGraph::getComponent(int componentId) {
  for (auto &comp : components) {
    if (comp->getId() == componentId)
      return comp.get();
  }
  return nullptr;
}

const CircuitComponent *CircuitGraph::getComponent(int componentId) const {
  for (const auto &comp : components) {
    if (comp->getId() == componentId)
      return comp.get();
  }
  return nullptr;
}

int CircuitGraph::connectNodes(int nodeA, int nodeB) {
  const juce::ScopedLock sl(graphLock);
  // Check if wire already exists
  for (const auto &wire : wires) {
    if ((wire.nodeA == nodeA && wire.nodeB == nodeB) ||
        (wire.nodeA == nodeB && wire.nodeB == nodeA)) {
      return wire.id;
    }
  }

  int id = nextWireId++;
  wires.emplace_back(id, nodeA, nodeB);
  return id;
}

void CircuitGraph::removeWire(int wireId) {
  {
    const juce::ScopedLock sl(graphLock);
    wires.erase(
        std::remove_if(wires.begin(), wires.end(),
                       [wireId](const Wire &w) { return w.id == wireId; }),
        wires.end());
  }
  // Clean up any orphaned junctions after removing the wire
  cleanupOrphanedJunctions();
}

Wire *CircuitGraph::getWireById(int wireId) {
  for (auto &wire : wires) {
    if (wire.id == wireId)
      return &wire;
  }
  return nullptr;
}

int CircuitGraph::createJunctionOnWire(int wireId, juce::Point<float> position) {
  const juce::ScopedLock sl(graphLock);

  // Find the wire to split
  Wire *wire = nullptr;
  int wireIndex = -1;
  for (size_t i = 0; i < wires.size(); ++i) {
    if (wires[i].id == wireId) {
      wire = &wires[i];
      wireIndex = static_cast<int>(i);
      break;
    }
  }

  if (!wire)
    return -1;

  // Store original wire endpoints
  int originalNodeA = wire->nodeA;
  int originalNodeB = wire->nodeB;

  // Create a new junction node
  int junctionNodeId = nextNodeId++;
  juce::String nodeName = "Junction" + juce::String(junctionNodeId);
  nodes.emplace_back(junctionNodeId, nodeName, false);

  // Record the junction with its position
  junctions.emplace_back(junctionNodeId, position);

  // Update node position
  if (auto *node = getNode(junctionNodeId)) {
    node->position = position;
  }

  // Remove the original wire
  wires.erase(wires.begin() + wireIndex);

  // Create two new wires: nodeA -> junction and junction -> nodeB
  int wire1Id = nextWireId++;
  int wire2Id = nextWireId++;
  wires.emplace_back(wire1Id, originalNodeA, junctionNodeId);
  wires.emplace_back(wire2Id, junctionNodeId, originalNodeB);

  return junctionNodeId;
}

void CircuitGraph::addJunction(int nodeId, juce::Point<float> position) {
  const juce::ScopedLock sl(graphLock);

  // Check if junction already exists
  for (auto &junction : junctions) {
    if (junction.nodeId == nodeId) {
      junction.position = position;
      return;
    }
  }

  junctions.emplace_back(nodeId, position);
}

Junction *CircuitGraph::getJunctionByNode(int nodeId) {
  for (auto &junction : junctions) {
    if (junction.nodeId == nodeId)
      return &junction;
  }
  return nullptr;
}

bool CircuitGraph::isJunctionNode(int nodeId) const {
  for (const auto &junction : junctions) {
    if (junction.nodeId == nodeId)
      return true;
  }
  return false;
}

int CircuitGraph::countWiresConnectedToNode(int nodeId) const {
  int count = 0;
  for (const auto &wire : wires) {
    if (wire.nodeA == nodeId || wire.nodeB == nodeId)
      count++;
  }
  return count;
}

void CircuitGraph::cleanupOrphanedJunctions() {
  const juce::ScopedLock sl(graphLock);

  bool changed = true;

  // Keep cleaning until no more changes (handles cascading cleanup)
  while (changed) {
    changed = false;

    // Find ONE junction to clean up
    for (auto it = junctions.begin(); it != junctions.end(); ++it) {
      int junctionNodeId = it->nodeId;
      int wireCount = countWiresConnectedToNode(junctionNodeId);

      if (wireCount == 0) {
        // Orphaned junction - remove it
        // Remove from nodes list
        nodes.erase(std::remove_if(nodes.begin(), nodes.end(),
                                   [junctionNodeId](const Node &n) {
                                     return n.id == junctionNodeId;
                                   }),
                    nodes.end());
        // Remove from junctions list
        junctions.erase(it);
        changed = true;
        break; // Restart iteration
      } else if (wireCount == 1) {
        // Junction with only one wire - it's a dead end
        // Find and remove the single wire connected to this junction
        wires.erase(std::remove_if(wires.begin(), wires.end(),
                                   [junctionNodeId](const Wire &w) {
                                     return w.nodeA == junctionNodeId ||
                                            w.nodeB == junctionNodeId;
                                   }),
                    wires.end());
        // Remove from nodes list
        nodes.erase(std::remove_if(nodes.begin(), nodes.end(),
                                   [junctionNodeId](const Node &n) {
                                     return n.id == junctionNodeId;
                                   }),
                    nodes.end());
        // Remove from junctions list
        junctions.erase(it);
        changed = true;
        break; // Restart iteration
      } else if (wireCount == 2) {
        // Junction with exactly 2 wires - merge them back into one wire
        // Find the two wires
        std::vector<std::pair<int, int>> wireInfo; // (wireId, otherEndpoint)
        for (const auto &wire : wires) {
          if (wire.nodeA == junctionNodeId) {
            wireInfo.push_back({wire.id, wire.nodeB});
          } else if (wire.nodeB == junctionNodeId) {
            wireInfo.push_back({wire.id, wire.nodeA});
          }
        }

        if (wireInfo.size() == 2) {
          int wire1Id = wireInfo[0].first;
          int wire2Id = wireInfo[1].first;
          int endpoint1 = wireInfo[0].second;
          int endpoint2 = wireInfo[1].second;

          // Remove the two wires
          wires.erase(std::remove_if(wires.begin(), wires.end(),
                                     [wire1Id, wire2Id](const Wire &w) {
                                       return w.id == wire1Id ||
                                              w.id == wire2Id;
                                     }),
                      wires.end());

          // Create a single wire connecting the two endpoints
          // (only if they're different and wire doesn't already exist)
          if (endpoint1 != endpoint2) {
            bool wireExists = false;
            for (const auto &wire : wires) {
              if ((wire.nodeA == endpoint1 && wire.nodeB == endpoint2) ||
                  (wire.nodeA == endpoint2 && wire.nodeB == endpoint1)) {
                wireExists = true;
                break;
              }
            }
            if (!wireExists) {
              int newWireId = nextWireId++;
              wires.emplace_back(newWireId, endpoint1, endpoint2);
            }
          }

          // Remove from nodes list
          nodes.erase(std::remove_if(nodes.begin(), nodes.end(),
                                     [junctionNodeId](const Node &n) {
                                       return n.id == junctionNodeId;
                                     }),
                      nodes.end());
          // Remove from junctions list
          junctions.erase(it);
          changed = true;
          break; // Restart iteration
        }
      }
      // If wireCount >= 3, the junction is still needed
    }
  }
}

void CircuitGraph::clear() {
  const juce::ScopedLock sl(graphLock);
  nodes.clear();
  components.clear();
  wires.clear();
  junctions.clear();
  nextNodeId = 0;
  nextComponentId = 0;
  nextWireId = 0;

  // Recreate ground node
  groundNodeId = createGroundNode();
}

bool CircuitGraph::isValid() const {
  // Check that we have at least ground node
  if (groundNodeId < 0)
    return false;

  // Check that all components have valid nodes
  for (const auto &comp : components) {
    if (getNode(comp->getNode1()) == nullptr)
      return false;
    if (getNode(comp->getNode2()) == nullptr)
      return false;
  }

  return true;
}

std::vector<CircuitComponent *>
CircuitGraph::getComponentsByType(ComponentType type) const {
  std::vector<CircuitComponent *> result;
  for (const auto &comp : components) {
    if (comp->getType() == type)
      result.push_back(comp.get());
  }
  return result;
}

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
  const juce::ScopedLock sl(graphLock);
  wires.erase(
      std::remove_if(wires.begin(), wires.end(),
                     [wireId](const Wire &w) { return w.id == wireId; }),
      wires.end());
}

Wire *CircuitGraph::getWireById(int wireId) {
  for (auto &wire : wires) {
    if (wire.id == wireId)
      return &wire;
  }
  return nullptr;
}

void CircuitGraph::clear() {
  const juce::ScopedLock sl(graphLock);
  nodes.clear();
  components.clear();
  wires.clear();
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
CircuitGraph::getComponentsByType(ComponentType type) {
  std::vector<CircuitComponent *> result;
  for (auto &comp : components) {
    if (comp->getType() == type)
      result.push_back(comp.get());
  }
  return result;
}

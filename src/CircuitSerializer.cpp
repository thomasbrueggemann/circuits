#include "CircuitSerializer.h"
#include "Engine/Components/AudioInput.h"
#include "Engine/Components/AudioOutput.h"
#include "Engine/Components/Capacitor.h"
#include "Engine/Components/Potentiometer.h"
#include "Engine/Components/Resistor.h"
#include "Engine/Components/Switch.h"
#include "Engine/Components/VacuumTube.h"

juce::String CircuitSerializer::serialize(const CircuitGraph &graph) {
  juce::DynamicObject::Ptr root = new juce::DynamicObject();

  // Serialize nodes
  juce::Array<juce::var> nodesArray;
  for (const auto &node : graph.getNodes()) {
    juce::DynamicObject::Ptr nodeObj = new juce::DynamicObject();
    nodeObj->setProperty("id", node.id);
    nodeObj->setProperty("name", node.name);
    nodeObj->setProperty("isGround", node.isGround);
    nodesArray.add(juce::var(nodeObj.get()));
  }
  root->setProperty("nodes", nodesArray);

  // Serialize components
  juce::Array<juce::var> componentsArray;
  for (const auto &comp : graph.getComponents()) {
    juce::DynamicObject::Ptr compObj = new juce::DynamicObject();
    compObj->setProperty("id", comp->getId());
    compObj->setProperty("type", static_cast<int>(comp->getType()));
    compObj->setProperty("name", comp->getName());
    compObj->setProperty("node1", comp->getNode1());
    compObj->setProperty("node2", comp->getNode2());
    compObj->setProperty("posX", comp->getPosition().x);
    compObj->setProperty("posY", comp->getPosition().y);
    compObj->setProperty("value", comp->getValue());

    // Component-specific properties
    if (auto *pot = dynamic_cast<Potentiometer *>(comp.get())) {
      compObj->setProperty("node3", pot->getNode3());
      compObj->setProperty("position", pot->getWiperPosition());
    } else if (auto *sw = dynamic_cast<Switch *>(comp.get())) {
      compObj->setProperty("closed", sw->isClosed());
    } else if (auto *tube = dynamic_cast<VacuumTube *>(comp.get())) {
      compObj->setProperty("node3", tube->getPlateNode());
      compObj->setProperty("mu", tube->getMu());
    }

    componentsArray.add(juce::var(compObj.get()));
  }
  root->setProperty("components", componentsArray);

  // Serialize wires
  juce::Array<juce::var> wiresArray;
  for (const auto &wire : graph.getWires()) {
    juce::DynamicObject::Ptr wireObj = new juce::DynamicObject();
    wireObj->setProperty("id", wire.id);
    wireObj->setProperty("nodeA", wire.nodeA);
    wireObj->setProperty("nodeB", wire.nodeB);
    wiresArray.add(juce::var(wireObj.get()));
  }
  root->setProperty("wires", wiresArray);

  return juce::JSON::toString(juce::var(root.get()));
}

bool CircuitSerializer::deserialize(const juce::String &json,
                                    CircuitGraph &graph) {
  auto parsed = juce::JSON::parse(json);
  if (parsed.isVoid())
    return false;

  graph.clear();

  auto *root = parsed.getDynamicObject();
  if (!root)
    return false;

  // Deserialize nodes
  if (auto *nodesArray = root->getProperty("nodes").getArray()) {
    for (const auto &nodeVar : *nodesArray) {
      if (auto *nodeObj = nodeVar.getDynamicObject()) {
        int id = nodeObj->getProperty("id");
        juce::String name = nodeObj->getProperty("name").toString();
        bool isGround = nodeObj->getProperty("isGround");
        graph.addNode(id, name, isGround);
      }
    }
  }

  // Deserialize components
  if (auto *componentsArray = root->getProperty("components").getArray()) {
    for (const auto &compVar : *componentsArray) {
      if (auto *compObj = compVar.getDynamicObject()) {
        ComponentType type = static_cast<ComponentType>(
            static_cast<int>(compObj->getProperty("type")));
        int id = compObj->getProperty("id");
        juce::String name = compObj->getProperty("name").toString();
        int node1 = compObj->getProperty("node1");
        int node2 = compObj->getProperty("node2");
        float posX = compObj->getProperty("posX");
        float posY = compObj->getProperty("posY");
        double value = compObj->getProperty("value");

        std::unique_ptr<CircuitComponent> comp;

        switch (type) {
        case ComponentType::Resistor:
          comp = std::make_unique<Resistor>(id, name, node1, node2, value);
          break;
        case ComponentType::Capacitor:
          comp = std::make_unique<Capacitor>(id, name, node1, node2, value);
          break;
        case ComponentType::Potentiometer: {
          int node3 = compObj->getProperty("node3");
          double position = compObj->getProperty("position");
          auto pot = std::make_unique<Potentiometer>(id, name, node1, node2,
                                                     node3, value);
          pot->setWiperPosition(position);
          comp = std::move(pot);
          break;
        }
        case ComponentType::Switch: {
          bool closed = compObj->getProperty("closed");
          auto sw = std::make_unique<Switch>(id, name, node1, node2);
          sw->setClosed(closed);
          comp = std::move(sw);
          break;
        }
        case ComponentType::VacuumTube: {
          int node3 = compObj->getProperty("node3");
          double mu = compObj->getProperty("mu");
          auto tube =
              std::make_unique<VacuumTube>(id, name, node1, node2, node3);
          tube->setMu(mu);
          comp = std::move(tube);
          break;
        }
        case ComponentType::AudioInput:
          comp = std::make_unique<AudioInput>(id, name, node1, node2);
          break;
        case ComponentType::AudioOutput:
          comp = std::make_unique<AudioOutput>(id, name, node1, node2);
          break;
        default:
          continue;
        }

        if (comp) {
          comp->setPosition(juce::Point<float>(posX, posY));
          graph.addComponent(std::move(comp));
        }
      }
    }
  }

  // Deserialize wires
  if (auto *wiresArray = root->getProperty("wires").getArray()) {
    for (const auto &wireVar : *wiresArray) {
      if (auto *wireObj = wireVar.getDynamicObject()) {
        int nodeA = wireObj->getProperty("nodeA");
        int nodeB = wireObj->getProperty("nodeB");
        graph.connectNodes(nodeA, nodeB);
      }
    }
  }

  return true;
}

bool CircuitSerializer::saveToFile(const CircuitGraph &graph,
                                   const juce::File &file) {
  juce::String json = serialize(graph);
  return file.replaceWithText(json);
}

bool CircuitSerializer::loadFromFile(juce::File &file, CircuitGraph &graph) {
  if (!file.existsAsFile())
    return false;

  juce::String json = file.loadFileAsString();
  return deserialize(json, graph);
}

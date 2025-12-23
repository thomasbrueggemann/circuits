#pragma once

#include "Engine/CircuitGraph.h"
#include <JuceHeader.h>

class CircuitSerializer {
public:
  static juce::String serialize(const CircuitGraph &graph);
  static bool deserialize(const juce::String &json, CircuitGraph &graph);

  static bool saveToFile(const CircuitGraph &graph, const juce::File &file);
  static bool loadFromFile(juce::File &file, CircuitGraph &graph);
};

#pragma once

#include <JuceHeader.h>

/**
 * WireView - Visual representation of a wire connection.
 */
class WireView {
public:
  WireView(int wireId, int nodeA, int nodeB);
  ~WireView();

  // Drawing
  void draw(juce::Graphics &g, juce::Point<float> startScreen,
            juce::Point<float> endScreen, bool selected);

  // Positions in canvas coordinates
  juce::Point<float> getStartPosition() const { return startPosition; }
  juce::Point<float> getEndPosition() const { return endPosition; }
  void setPositions(juce::Point<float> start, juce::Point<float> end);

  // Wire identification
  int getId() const { return id; }
  int getNodeA() const { return nodeA; }
  int getNodeB() const { return nodeB; }

  // Hit testing
  bool hitTest(juce::Point<float> canvasPos) const;

  // Signal visualization
  void setSignalLevel(float level) { signalLevel = level; }
  float getSignalLevel() const { return signalLevel; }

private:
  int id;
  int nodeA;
  int nodeB;

  juce::Point<float> startPosition;
  juce::Point<float> endPosition;

  float signalLevel = 0.0f;

  static constexpr float HIT_TOLERANCE = 8.0f;
};

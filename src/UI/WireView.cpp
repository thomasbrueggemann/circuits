#include "WireView.h"

WireView::WireView(int wireId, int a, int b) : id(wireId), nodeA(a), nodeB(b) {}

WireView::~WireView() = default;

void WireView::setPositions(juce::Point<float> start, juce::Point<float> end) {
  startPosition = start;
  endPosition = end;
}

void WireView::draw(juce::Graphics &g, juce::Point<float> startScreen,
                    juce::Point<float> endScreen, bool selected) {
  // Wire color based on selection and signal level
  juce::Colour baseColour =
      selected ? juce::Colour(0xFF00ff88) : juce::Colour(0xFF4a6a8a);

  // Brighten based on signal level
  if (signalLevel > 0.0f) {
    float t = std::min(signalLevel, 1.0f);
    baseColour =
        baseColour.interpolatedWith(juce::Colour(0xFF00ffff), t * 0.5f);
  }

  g.setColour(baseColour);

  // Calculate wire path with orthogonal routing
  juce::Path wirePath;
  wirePath.startNewSubPath(startScreen);

  // Simple orthogonal routing (horizontal then vertical)
  float midX = (startScreen.x + endScreen.x) / 2.0f;

  if (std::abs(startScreen.y - endScreen.y) < 5.0f) {
    // Nearly horizontal - just draw straight line
    wirePath.lineTo(endScreen);
  } else if (std::abs(startScreen.x - endScreen.x) < 5.0f) {
    // Nearly vertical - just draw straight line
    wirePath.lineTo(endScreen);
  } else {
    // Orthogonal routing
    wirePath.lineTo(midX, startScreen.y);
    wirePath.lineTo(midX, endScreen.y);
    wirePath.lineTo(endScreen);
  }

  float strokeWidth = selected ? 3.0f : 2.0f;
  g.strokePath(wirePath,
               juce::PathStrokeType(strokeWidth, juce::PathStrokeType::curved,
                                    juce::PathStrokeType::rounded));

  // Draw connection dots at endpoints
  float dotRadius = selected ? 4.0f : 3.0f;
  g.fillEllipse(startScreen.x - dotRadius, startScreen.y - dotRadius,
                dotRadius * 2, dotRadius * 2);
  g.fillEllipse(endScreen.x - dotRadius, endScreen.y - dotRadius, dotRadius * 2,
                dotRadius * 2);

  // If selected, draw glow effect
  if (selected) {
    g.setColour(baseColour.withAlpha(0.3f));
    g.strokePath(wirePath,
                 juce::PathStrokeType(8.0f, juce::PathStrokeType::curved,
                                      juce::PathStrokeType::rounded));
  }
}

bool WireView::hitTest(juce::Point<float> canvasPos) const {
  // Check distance from point to line segments

  // Calculate midpoint for orthogonal routing
  float midX = (startPosition.x + endPosition.x) / 2.0f;

  // Check each segment
  std::vector<std::pair<juce::Point<float>, juce::Point<float>>> segments;

  if (std::abs(startPosition.y - endPosition.y) < 5.0f ||
      std::abs(startPosition.x - endPosition.x) < 5.0f) {
    // Straight line
    segments.push_back({startPosition, endPosition});
  } else {
    // Orthogonal segments
    segments.push_back({startPosition, {midX, startPosition.y}});
    segments.push_back({{midX, startPosition.y}, {midX, endPosition.y}});
    segments.push_back({{midX, endPosition.y}, endPosition});
  }

  for (const auto &segment : segments) {
    // Calculate distance from point to line segment
    auto &p1 = segment.first;
    auto &p2 = segment.second;

    float dx = p2.x - p1.x;
    float dy = p2.y - p1.y;
    float lengthSq = dx * dx + dy * dy;

    if (lengthSq == 0.0f) {
      // Segment is a point
      if (canvasPos.getDistanceFrom(p1) < HIT_TOLERANCE)
        return true;
      continue;
    }

    // Calculate projection
    float t = std::max(0.0f, std::min(1.0f, ((canvasPos.x - p1.x) * dx +
                                             (canvasPos.y - p1.y) * dy) /
                                                lengthSq));

    juce::Point<float> projection(p1.x + t * dx, p1.y + t * dy);
    float distance = canvasPos.getDistanceFrom(projection);

    if (distance < HIT_TOLERANCE)
      return true;
  }

  return false;
}

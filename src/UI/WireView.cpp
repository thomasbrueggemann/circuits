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
      selected ? juce::Colour(0xFFffaa00) : juce::Colour(0xFF555555);

  if (signalLevel > 0.0f) {
    float t = std::min(signalLevel, 1.0f);
    baseColour =
        baseColour.interpolatedWith(juce::Colour(0xFFffdd00), t * 0.4f);
  }

  // Calculate wire path
  juce::Path wirePath;
  wirePath.startNewSubPath(startScreen);
  float midX = (startScreen.x + endScreen.x) / 2.0f;

  if (std::abs(startScreen.y - endScreen.y) < 5.0f ||
      std::abs(startScreen.x - endScreen.x) < 5.0f) {
    wirePath.lineTo(endScreen);
  } else {
    wirePath.lineTo(midX, startScreen.y);
    wirePath.lineTo(midX, endScreen.y);
    wirePath.lineTo(endScreen);
  }

  float strokeWidth = selected ? 4.0f : 3.0f;

  // 1. Shadow
  g.setColour(juce::Colours::black.withAlpha(0.3f));
  g.strokePath(wirePath,
               juce::PathStrokeType(strokeWidth, juce::PathStrokeType::curved,
                                    juce::PathStrokeType::rounded),
               juce::AffineTransform::translation(1.0f, 1.0f));

  // 2. Main cable
  g.setColour(baseColour);
  g.strokePath(wirePath,
               juce::PathStrokeType(strokeWidth, juce::PathStrokeType::curved,
                                    juce::PathStrokeType::rounded));

  // 3. Highlight (shine)
  g.setColour(juce::Colours::white.withAlpha(0.2f));
  g.strokePath(wirePath, juce::PathStrokeType(strokeWidth * 0.4f,
                                              juce::PathStrokeType::curved,
                                              juce::PathStrokeType::rounded));

  // Draw connection dots at endpoints
  g.setColour(juce::Colour(0xFF888888));
  float dotRadius = 3.5f;
  g.fillEllipse(startScreen.x - dotRadius, startScreen.y - dotRadius,
                dotRadius * 2, dotRadius * 2);
  g.fillEllipse(endScreen.x - dotRadius, endScreen.y - dotRadius, dotRadius * 2,
                dotRadius * 2);

  // Outer ring for dots
  g.setColour(juce::Colours::black.withAlpha(0.5f));
  g.drawEllipse(startScreen.x - dotRadius, startScreen.y - dotRadius,
                dotRadius * 2, dotRadius * 2, 1.0f);
  g.drawEllipse(endScreen.x - dotRadius, endScreen.y - dotRadius, dotRadius * 2,
                dotRadius * 2, 1.0f);
}

bool WireView::hitTest(juce::Point<float> canvasPos, float zoom) const {
  // Check distance from point to line segments
  float tolerance = HIT_TOLERANCE / zoom;

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
      if (canvasPos.getDistanceFrom(p1) < tolerance)
        return true;
      continue;
    }

    // Calculate projection
    float t = std::max(0.0f, std::min(1.0f, ((canvasPos.x - p1.x) * dx +
                                             (canvasPos.y - p1.y) * dy) /
                                                lengthSq));

    juce::Point<float> projection(p1.x + t * dx, p1.y + t * dy);
    float distance = canvasPos.getDistanceFrom(projection);

    if (distance < tolerance)
      return true;
  }

  return false;
}

juce::Point<float>
WireView::getClosestPointOnWire(juce::Point<float> canvasPos) const {
  // Calculate midpoint for orthogonal routing
  float midX = (startPosition.x + endPosition.x) / 2.0f;

  // Build segment list
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

  juce::Point<float> closestPoint = startPosition;
  float minDistance = std::numeric_limits<float>::max();

  for (const auto &segment : segments) {
    auto &p1 = segment.first;
    auto &p2 = segment.second;

    float dx = p2.x - p1.x;
    float dy = p2.y - p1.y;
    float lengthSq = dx * dx + dy * dy;

    juce::Point<float> projection;

    if (lengthSq == 0.0f) {
      // Segment is a point
      projection = p1;
    } else {
      // Calculate projection parameter t, clamped to [0, 1]
      float t = std::max(0.0f, std::min(1.0f, ((canvasPos.x - p1.x) * dx +
                                               (canvasPos.y - p1.y) * dy) /
                                                  lengthSq));
      projection = juce::Point<float>(p1.x + t * dx, p1.y + t * dy);
    }

    float distance = canvasPos.getDistanceFrom(projection);
    if (distance < minDistance) {
      minDistance = distance;
      closestPoint = projection;
    }
  }

  return closestPoint;
}

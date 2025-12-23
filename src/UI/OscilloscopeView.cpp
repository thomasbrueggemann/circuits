#include "OscilloscopeView.h"
#include <algorithm>
#include <cmath>

OscilloscopeView::OscilloscopeView() {
  waveformBuffer.resize(BUFFER_SIZE, 0.0f);
  startTimerHz(60); // 60fps update
}

OscilloscopeView::~OscilloscopeView() { stopTimer(); }

void OscilloscopeView::paint(juce::Graphics &g) {
  auto bounds = getLocalBounds().toFloat().reduced(5);

  drawBackground(g, bounds);
  drawGrid(g, bounds);

  if (probeActive) {
    drawWaveform(g, bounds);
  } else {
    drawNoSignalMessage(g, bounds);
  }

  drawLabels(g, bounds);
}

void OscilloscopeView::resized() {}

void OscilloscopeView::timerCallback() { repaint(); }

void OscilloscopeView::updateWaveform(const std::vector<float> &samples) {
  for (float sample : samples) {
    waveformBuffer[writeIndex] = sample;
    writeIndex = (writeIndex + 1) % BUFFER_SIZE;

    // Update auto-scale
    if (autoScale) {
      float absSample = std::abs(sample);
      if (absSample > autoScaleMax) {
        autoScaleMax = absSample;
      } else {
        // Slowly decay max
        autoScaleMax *= 0.9999f;
        autoScaleMax = std::max(autoScaleMax, 0.01f);
      }
    }
  }
}

void OscilloscopeView::setProbeActive(bool active) {
  probeActive = active;
  if (!active) {
    std::fill(waveformBuffer.begin(), waveformBuffer.end(), 0.0f);
  }
}

void OscilloscopeView::drawBackground(juce::Graphics &g,
                                      juce::Rectangle<float> bounds) {
  // Dark oscilloscope screen
  juce::ColourGradient gradient(juce::Colour(0xFF0a1520), bounds.getCentreX(),
                                bounds.getY(), juce::Colour(0xFF051015),
                                bounds.getCentreX(), bounds.getBottom(), false);
  g.setGradientFill(gradient);
  g.fillRoundedRectangle(bounds, 6.0f);

  // Border
  g.setColour(juce::Colour(0xFF2a4a6a));
  g.drawRoundedRectangle(bounds, 6.0f, 1.5f);

  // Subtle inner glow
  for (int i = 0; i < 3; ++i) {
    g.setColour(
        juce::Colour(0xFF00aa55).withAlpha(0.03f * static_cast<float>(3 - i)));
    g.drawRoundedRectangle(bounds.reduced(static_cast<float>(i * 2)), 6.0f,
                           1.0f);
  }
}

void OscilloscopeView::drawGrid(juce::Graphics &g,
                                juce::Rectangle<float> bounds) {
  auto gridBounds = bounds.reduced(30, 15);

  // Grid lines
  g.setColour(juce::Colour(0xFF1a3a4a).withAlpha(0.5f));

  // Vertical lines
  float xStep = gridBounds.getWidth() / GRID_DIVISIONS_X;
  for (int i = 0; i <= GRID_DIVISIONS_X; ++i) {
    float x = gridBounds.getX() + i * xStep;
    if (i == GRID_DIVISIONS_X / 2) {
      g.setColour(juce::Colour(0xFF2a5a6a).withAlpha(0.7f));
      g.drawVerticalLine(static_cast<int>(x), gridBounds.getY(),
                         gridBounds.getBottom());
      g.setColour(juce::Colour(0xFF1a3a4a).withAlpha(0.5f));
    } else {
      g.drawVerticalLine(static_cast<int>(x), gridBounds.getY(),
                         gridBounds.getBottom());
    }
  }

  // Horizontal lines
  float yStep = gridBounds.getHeight() / GRID_DIVISIONS_Y;
  for (int i = 0; i <= GRID_DIVISIONS_Y; ++i) {
    float y = gridBounds.getY() + i * yStep;
    if (i == GRID_DIVISIONS_Y / 2) {
      // Center line (0V reference)
      g.setColour(juce::Colour(0xFF2a5a6a).withAlpha(0.7f));
      g.drawHorizontalLine(static_cast<int>(y), gridBounds.getX(),
                           gridBounds.getRight());
      g.setColour(juce::Colour(0xFF1a3a4a).withAlpha(0.5f));
    } else {
      g.drawHorizontalLine(static_cast<int>(y), gridBounds.getX(),
                           gridBounds.getRight());
    }
  }
}

void OscilloscopeView::drawWaveform(juce::Graphics &g,
                                    juce::Rectangle<float> bounds) {
  auto gridBounds = bounds.reduced(30, 15);

  float yScale = autoScale ? autoScaleMax : voltageScale;
  float yCenter = gridBounds.getCentreY();
  float yRange = gridBounds.getHeight() / 2.0f;

  // Create waveform path
  juce::Path waveformPath;

  int samplesPerPixel = static_cast<int>(BUFFER_SIZE / gridBounds.getWidth());
  samplesPerPixel = std::max(1, samplesPerPixel);

  bool pathStarted = false;

  for (int x = 0; x < static_cast<int>(gridBounds.getWidth()); ++x) {
    // Get sample at this position
    size_t sampleIndex =
        (writeIndex + BUFFER_SIZE -
         static_cast<size_t>(gridBounds.getWidth() - x) * samplesPerPixel) %
        BUFFER_SIZE;
    float sample = waveformBuffer[sampleIndex];

    // Normalize and scale
    float normalizedSample = sample / yScale;
    normalizedSample = juce::jlimit(-1.0f, 1.0f, normalizedSample);

    float yPos = yCenter - normalizedSample * yRange;
    float xPos = gridBounds.getX() + x;

    if (!pathStarted) {
      waveformPath.startNewSubPath(xPos, yPos);
      pathStarted = true;
    } else {
      waveformPath.lineTo(xPos, yPos);
    }
  }

  // Draw phosphor glow effect
  for (int i = 3; i >= 0; --i) {
    float alpha = 0.1f * static_cast<float>(4 - i);
    float thickness = 1.0f + i * 1.5f;
    g.setColour(juce::Colour(0xFF00ff88).withAlpha(alpha));
    g.strokePath(waveformPath, juce::PathStrokeType(thickness));
  }

  // Draw main trace
  g.setColour(juce::Colour(0xFF00ff88));
  g.strokePath(waveformPath, juce::PathStrokeType(1.5f));
}

void OscilloscopeView::drawLabels(juce::Graphics &g,
                                  juce::Rectangle<float> bounds) {
  g.setColour(juce::Colour(0xFF6a8a9a));
  g.setFont(10.0f);

  // Title
  g.drawText("OSCILLOSCOPE", bounds.removeFromTop(15),
             juce::Justification::centred);

  // Scale info
  if (probeActive) {
    float vScale = autoScale ? autoScaleMax : voltageScale;
    juce::String scaleText = juce::String(vScale, 2) + " V/div";
    g.drawText(scaleText, bounds.removeFromBottom(12),
               juce::Justification::centredLeft);

    g.drawText(juce::String(timeScale, 1) + " ms/div",
               bounds.removeFromBottom(12).removeFromRight(100),
               juce::Justification::centredRight);
  }
}

void OscilloscopeView::drawNoSignalMessage(juce::Graphics &g,
                                           juce::Rectangle<float> bounds) {
  g.setColour(juce::Colour(0xFF4a6a7a).withAlpha(0.7f));
  g.setFont(12.0f);

  juce::String msg = "Click a wire to probe";
  if (!probeActive)
    msg = "Connect AudioOutput or click wire to probe\nEnsure Simulation Power "
          "is ON";

  g.drawText(msg, bounds, juce::Justification::centred);
}

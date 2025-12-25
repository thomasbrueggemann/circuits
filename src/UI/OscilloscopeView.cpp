#include "OscilloscopeView.h"
#include <algorithm>
#include <cmath>
#include <cstddef>

OscilloscopeView::OscilloscopeView()
    : fft(FFT_ORDER),
      window(FFT_SIZE, juce::dsp::WindowingFunction<float>::hann) {
  waveformBuffer.resize(BUFFER_SIZE, 0.0f);
  fftData.fill(0.0f);
  spectrumData.fill(0.0f);
  smoothedSpectrum.fill(-100.0f);
  startTimerHz(60); // 60fps update
}

OscilloscopeView::~OscilloscopeView() { stopTimer(); }

void OscilloscopeView::paint(juce::Graphics &g) {
  auto bounds = getLocalBounds().toFloat().reduced(5);

  drawBackground(g, bounds);

  if (displayMode == DisplayMode::Waveform) {
    drawGrid(g, bounds);
  } else {
    drawSpectrumGrid(g, bounds);
  }

  // Always try to draw waveform/spectrum if we have data
  bool hasData = !waveformBuffer.empty() && waveformBuffer.size() > 0;
  
  if (probeActive && simulationValid && hasData) {
    if (displayMode == DisplayMode::Waveform) {
      drawWaveform(g, bounds);
    } else {
      drawSpectrum(g, bounds);
    }
  } else {
    drawNoSignalMessage(g, bounds);
  }

  drawLabels(g, bounds);
  drawModeButton(g, bounds);
  
  // Always show status in corner for debugging
  g.setColour(juce::Colours::white);
  g.setFont(10.0f);
  juce::String status = "P:" + juce::String(probeActive ? "Y" : "N") +
                        " V:" + juce::String(simulationValid ? "Y" : "N") + 
                        " R:" + juce::String(simulationRunning ? "Y" : "N") +
                        " N:" + juce::String(lastProbeNodeId);
  g.drawText(status, bounds.getRight() - 150, bounds.getY() + 2, 145, 14, juce::Justification::right);
}

void OscilloscopeView::resized() {
  // Calculate mode button bounds in top-right corner
  auto bounds = getLocalBounds().toFloat().reduced(5);
  modeButtonBounds = bounds.removeFromTop(20).removeFromRight(80);
}

void OscilloscopeView::timerCallback() { repaint(); }

void OscilloscopeView::mouseDown(const juce::MouseEvent &event) {
  if (modeButtonBounds.contains(event.position)) {
    toggleDisplayMode();
  }
}

void OscilloscopeView::toggleDisplayMode() {
  displayMode = (displayMode == DisplayMode::Waveform) ? DisplayMode::Spectrum
                                                       : DisplayMode::Waveform;
}

void OscilloscopeView::updateWaveform(const std::vector<float> &samples) {
  if (waveformBuffer.size() != samples.size())
    waveformBuffer.resize(samples.size());

  std::copy(samples.begin(), samples.end(), waveformBuffer.begin());

  // Diagnostics
  lastSampleBatch = samples;
  logicHeartbeat++;

  // Update auto-scale based on the new batch
  if (autoScale) {
    float maxInBatch = 0.05f;
    for (float sample : samples) {
      maxInBatch = std::max(maxInBatch, std::abs(sample));
    }

    // Smoothly transition autoScaleMax
    if (maxInBatch > autoScaleMax) {
      autoScaleMax = maxInBatch;
    } else {
      autoScaleMax = autoScaleMax * 0.9f + maxInBatch * 0.1f;
    }
    autoScaleMax = std::max(autoScaleMax, 0.05f);
  }

  // Compute FFT for spectrum display
  if (displayMode == DisplayMode::Spectrum) {
    computeFFT();
  }
}

void OscilloscopeView::computeFFT() {
  // Copy samples to FFT buffer and apply window
  size_t copySize = std::min(waveformBuffer.size(), static_cast<size_t>(FFT_SIZE));
  
  for (size_t i = 0; i < FFT_SIZE; ++i) {
    if (i < copySize) {
      fftData[i] = waveformBuffer[i];
    } else {
      fftData[i] = 0.0f;
    }
  }
  
  // Apply Hann window
  window.multiplyWithWindowingTable(fftData.data(), FFT_SIZE);
  
  // Perform FFT (in-place, interleaved real/imag)
  fft.performFrequencyOnlyForwardTransform(fftData.data());
  
  // Convert to dB and smooth
  const float smoothingFactor = 0.7f;
  const float minDb = -100.0f;
  const float maxDb = 0.0f;
  
  for (int i = 0; i < FFT_SIZE / 2; ++i) {
    float magnitude = fftData[i];
    
    // Normalize by FFT size
    magnitude /= static_cast<float>(FFT_SIZE);
    
    // Convert to dB
    float db = magnitude > 0.0f ? 20.0f * std::log10(magnitude) : minDb;
    db = juce::jlimit(minDb, maxDb, db);
    
    spectrumData[i] = db;
    
    // Smooth the spectrum for better visualization
    if (db > smoothedSpectrum[i]) {
      smoothedSpectrum[i] = db;  // Fast attack
    } else {
      smoothedSpectrum[i] = smoothedSpectrum[i] * smoothingFactor + db * (1.0f - smoothingFactor);  // Slow decay
    }
  }
}

void OscilloscopeView::setProbeActive(bool active) {
  probeActive = active;
  if (!active) {
    std::fill(waveformBuffer.begin(), waveformBuffer.end(), 0.0f);
    smoothedSpectrum.fill(-100.0f);
  }
}

void OscilloscopeView::drawBackground(juce::Graphics &g,
                                      juce::Rectangle<float> bounds) {
  // Deep greenish-black CRT screen
  juce::ColourGradient gradient(juce::Colour(0xFF001100), bounds.getCentreX(),
                                bounds.getY(), juce::Colour(0xFF000500),
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
      g.setColour(juce::Colour(0x66004400));
      g.drawVerticalLine(static_cast<int>(x), gridBounds.getY(),
                         gridBounds.getBottom());
      g.setColour(juce::Colour(0x33004400));
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
      g.setColour(juce::Colour(0x66004400));
      g.drawHorizontalLine(static_cast<int>(y), gridBounds.getX(),
                           gridBounds.getRight());
      g.setColour(juce::Colour(0x33004400));
    } else {
      g.drawHorizontalLine(static_cast<int>(y), gridBounds.getX(),
                           gridBounds.getRight());
    }
  }
}

void OscilloscopeView::drawSpectrumGrid(juce::Graphics &g,
                                        juce::Rectangle<float> bounds) {
  auto gridBounds = bounds.reduced(30, 25);

  // Draw frequency markers (logarithmic scale)
  g.setColour(juce::Colour(0xFF1a3a4a).withAlpha(0.5f));
  
  // Frequency lines at key points: 100Hz, 1kHz, 10kHz
  std::array<float, 7> freqMarkers = {100.0f, 200.0f, 500.0f, 1000.0f, 2000.0f, 5000.0f, 10000.0f};
  
  for (float freq : freqMarkers) {
    float x = frequencyToX(freq, gridBounds.getWidth());
    if (x >= 0 && x <= gridBounds.getWidth()) {
      float xPos = gridBounds.getX() + x;
      
      // Major frequency lines (100, 1k, 10k)
      if (freq == 100.0f || freq == 1000.0f || freq == 10000.0f) {
        g.setColour(juce::Colour(0x66004400));
      } else {
        g.setColour(juce::Colour(0x33003300));
      }
      g.drawVerticalLine(static_cast<int>(xPos), gridBounds.getY(), gridBounds.getBottom());
    }
  }
  
  // Draw dB markers (-60dB to 0dB in 10dB steps)
  g.setColour(juce::Colour(0xFF1a3a4a).withAlpha(0.5f));
  for (int db = -60; db <= 0; db += 10) {
    float y = dbToY(static_cast<float>(db), gridBounds.getHeight());
    float yPos = gridBounds.getY() + y;
    
    if (db == -30) {
      g.setColour(juce::Colour(0x66004400));
    } else {
      g.setColour(juce::Colour(0x33003300));
    }
    g.drawHorizontalLine(static_cast<int>(yPos), gridBounds.getX(), gridBounds.getRight());
  }
}

float OscilloscopeView::frequencyToX(float freq, float width) const {
  // Logarithmic frequency scale from 20Hz to Nyquist
  const float minFreq = 20.0f;
  const float maxFreq = static_cast<float>(sampleRate / 2.0);
  
  if (freq < minFreq) return 0.0f;
  if (freq > maxFreq) return width;
  
  float logMin = std::log10(minFreq);
  float logMax = std::log10(maxFreq);
  float logFreq = std::log10(freq);
  
  return width * (logFreq - logMin) / (logMax - logMin);
}

float OscilloscopeView::dbToY(float db, float height) const {
  // Map dB range (-80 to 0) to height
  const float minDb = -80.0f;
  const float maxDb = 0.0f;
  
  float normalized = (db - maxDb) / (minDb - maxDb);
  return height * juce::jlimit(0.0f, 1.0f, normalized);
}

void OscilloscopeView::drawWaveform(juce::Graphics &g,
                                    juce::Rectangle<float> bounds) {
  auto gridBounds = bounds.reduced(30, 15);

  float yScale = autoScale ? autoScaleMax : voltageScale;
  float yCenter = gridBounds.getCentreY();
  float yRange = gridBounds.getHeight() / 2.0f;

  // Create waveform path
  juce::Path waveformPath;

  int samplesToDisplay = static_cast<int>(BUFFER_SIZE);
  // Optional: Adjust samplesToDisplay based on timeScale here if needed

  bool pathStarted = false;

  for (int x = 0; x < static_cast<int>(gridBounds.getWidth()); ++x) {
    // Get sample at this position
    float progress = static_cast<float>(x) / gridBounds.getWidth();
    size_t sampleIndex =
        static_cast<size_t>(progress * (waveformBuffer.size() - 1));
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
  for (int i = 4; i >= 0; --i) {
    float alpha = 0.08f * static_cast<float>(5 - i);
    float thickness = 1.0f + static_cast<float>(i) * 2.5f;
    g.setColour(juce::Colour(0xFF00FF41).withAlpha(alpha)); // Phosphor green
    g.strokePath(waveformPath, juce::PathStrokeType(thickness));
  }

  // Draw main trace
  g.setColour(juce::Colour(0xFFBBFFBB)); // Bright core
  g.strokePath(waveformPath, juce::PathStrokeType(1.0f));
}

void OscilloscopeView::drawSpectrum(juce::Graphics &g,
                                    juce::Rectangle<float> bounds) {
  auto gridBounds = bounds.reduced(30, 25);
  
  const float minFreq = 20.0f;
  const float maxFreq = static_cast<float>(sampleRate / 2.0);
  const float binWidth = static_cast<float>(sampleRate) / FFT_SIZE;
  
  // Create spectrum path
  juce::Path spectrumPath;
  juce::Path fillPath;
  bool pathStarted = false;
  
  float startX = gridBounds.getX();
  float startY = gridBounds.getBottom();
  
  // Draw spectrum using logarithmic frequency scale
  for (int x = 0; x < static_cast<int>(gridBounds.getWidth()); ++x) {
    // Convert x position to frequency (logarithmic)
    float progress = static_cast<float>(x) / gridBounds.getWidth();
    float logMin = std::log10(minFreq);
    float logMax = std::log10(maxFreq);
    float freq = std::pow(10.0f, logMin + progress * (logMax - logMin));
    
    // Find corresponding FFT bin
    int bin = static_cast<int>(freq / binWidth);
    bin = juce::jlimit(0, FFT_SIZE / 2 - 1, bin);
    
    // Get dB value (use smoothed spectrum for nicer display)
    float db = smoothedSpectrum[bin];
    
    // Map dB to Y position
    float yPos = gridBounds.getY() + dbToY(db, gridBounds.getHeight());
    float xPos = gridBounds.getX() + x;
    
    if (!pathStarted) {
      spectrumPath.startNewSubPath(xPos, yPos);
      fillPath.startNewSubPath(xPos, gridBounds.getBottom());
      fillPath.lineTo(xPos, yPos);
      pathStarted = true;
    } else {
      spectrumPath.lineTo(xPos, yPos);
      fillPath.lineTo(xPos, yPos);
    }
  }
  
  // Close fill path
  fillPath.lineTo(gridBounds.getRight(), gridBounds.getBottom());
  fillPath.closeSubPath();
  
  // Draw filled gradient
  juce::ColourGradient fillGradient(
      juce::Colour(0x8800FF88), gridBounds.getX(), gridBounds.getY(),
      juce::Colour(0x22004422), gridBounds.getX(), gridBounds.getBottom(), false);
  g.setGradientFill(fillGradient);
  g.fillPath(fillPath);
  
  // Draw glow effect
  for (int i = 3; i >= 0; --i) {
    float alpha = 0.1f * static_cast<float>(4 - i);
    float thickness = 1.0f + static_cast<float>(i) * 2.0f;
    g.setColour(juce::Colour(0xFF00FF88).withAlpha(alpha));
    g.strokePath(spectrumPath, juce::PathStrokeType(thickness));
  }
  
  // Draw main spectrum line
  g.setColour(juce::Colour(0xFFAAFFCC));
  g.strokePath(spectrumPath, juce::PathStrokeType(1.5f));
  
  // Draw frequency labels
  g.setColour(juce::Colour(0xFF6a9a8a));
  g.setFont(9.0f);
  
  std::array<std::pair<float, const char*>, 5> freqLabels = {{
    {100.0f, "100"},
    {500.0f, "500"},
    {1000.0f, "1k"},
    {5000.0f, "5k"},
    {10000.0f, "10k"}
  }};
  
  for (const auto& [freq, label] : freqLabels) {
    float x = gridBounds.getX() + frequencyToX(freq, gridBounds.getWidth());
    g.drawText(label, static_cast<int>(x) - 15, static_cast<int>(gridBounds.getBottom()) + 2, 30, 12,
               juce::Justification::centred);
  }
  
  // Draw dB labels
  g.setColour(juce::Colour(0xFF6a8a9a));
  for (int db = -60; db <= 0; db += 20) {
    float y = gridBounds.getY() + dbToY(static_cast<float>(db), gridBounds.getHeight());
    g.drawText(juce::String(db) + "dB", 
               static_cast<int>(gridBounds.getX()) - 28, static_cast<int>(y) - 6, 
               25, 12, juce::Justification::right);
  }
}

void OscilloscopeView::drawModeButton(juce::Graphics &g,
                                      juce::Rectangle<float> bounds) {
  // Draw mode toggle button
  g.setColour(juce::Colour(0xFF1a3a4a));
  g.fillRoundedRectangle(modeButtonBounds, 3.0f);
  
  g.setColour(juce::Colour(0xFF3a5a7a));
  g.drawRoundedRectangle(modeButtonBounds, 3.0f, 1.0f);
  
  g.setColour(juce::Colour(0xFF8ac8a8));
  g.setFont(10.0f);
  
  juce::String modeText = (displayMode == DisplayMode::Waveform) ? "WAVEFORM" : "SPECTRUM";
  g.drawText(modeText, modeButtonBounds, juce::Justification::centred);
}

void OscilloscopeView::drawLabels(juce::Graphics &g,
                                  juce::Rectangle<float> bounds) {
  g.setColour(juce::Colour(0xFF6a8a9a));
  g.setFont(10.0f);

  // Title
  juce::String title = (displayMode == DisplayMode::Waveform) ? "OSCILLOSCOPE" : "SPECTRUM ANALYZER";
  g.drawText(title, bounds.removeFromTop(15).removeFromLeft(150),
             juce::Justification::centredLeft);

  // Scale info
  if (probeActive) {
    if (displayMode == DisplayMode::Waveform) {
      float vScale = autoScale ? autoScaleMax : voltageScale;
      juce::String scaleText = juce::String(vScale, 2) + " V/div";
      if (simulationValid) {
        g.drawText(scaleText, bounds.removeFromBottom(12),
                   juce::Justification::centredLeft);
      } else {
        g.setColour(juce::Colours::red);
        g.drawText("ERR: SINGULAR MATRIX", bounds.removeFromBottom(12),
                   juce::Justification::centredLeft);
      }

      g.setColour(juce::Colour(0xFF6a8a9a));
      g.drawText(juce::String(timeScale, 1) + " ms/div",
                 bounds.removeFromBottom(12).removeFromRight(100),
                 juce::Justification::centredRight);
    } else {
      // Spectrum mode labels
      g.drawText("Hz", bounds.removeFromBottom(12).removeFromRight(80),
                 juce::Justification::centredRight);
    }

    // Debug info - show probe status and signal stats
    g.setColour(juce::Colours::yellow.withAlpha(0.7f));
    g.setFont(9.0f);
    
    // Calculate signal statistics
    float minVal = 0.0f, maxVal = 0.0f, avgVal = 0.0f;
    if (!lastSampleBatch.empty()) {
      minVal = *std::min_element(lastSampleBatch.begin(), lastSampleBatch.end());
      maxVal = *std::max_element(lastSampleBatch.begin(), lastSampleBatch.end());
      for (float s : lastSampleBatch) avgVal += s;
      avgVal /= lastSampleBatch.size();
    }
    
    juce::String debugInfo = "PROBE NODE: " + juce::String(lastProbeNodeId);
    g.drawText(debugInfo, bounds.removeFromTop(12).removeFromLeft(150), juce::Justification::left);
    
    // Signal presence indicator
    bool hasSignal = (maxVal - minVal) > 0.001f;
    g.setColour(hasSignal ? juce::Colours::lime : juce::Colours::red.withAlpha(0.7f));
    juce::String signalInfo = hasSignal ? 
        "SIGNAL: min=" + juce::String(minVal, 3) + " max=" + juce::String(maxVal, 3) :
        "NO SIGNAL (all zeros)";
    g.drawText(signalInfo, bounds.removeFromTop(12).removeFromLeft(300), juce::Justification::left);
  }

  // Logic Heartbeat dot (blinks top-left)
  if ((logicHeartbeat / 5) % 2 == 0) {
    g.setColour(juce::Colours::yellow);
    g.fillEllipse(5, 5, 4, 4);
  }
  
  // Simulation state overlay
  g.setColour(juce::Colours::white.withAlpha(0.7f));
  g.setFont(12.0f);
  juce::String simStateText;
  if (simulationRunning) {
    simStateText = "SIMULATION RUNNING";
    g.setColour(juce::Colours::green.withAlpha(0.7f));
  } else {
    simStateText = "SIMULATION STOPPED";
    g.setColour(juce::Colours::red.withAlpha(0.7f));
  }
  g.drawText(simStateText,
             bounds.withSizeKeepingCentre(200, 20).removeFromBottom(20),
             juce::Justification::centred);
}

void OscilloscopeView::drawNoSignalMessage(juce::Graphics &g,
                                           juce::Rectangle<float> bounds) {
  g.setColour(juce::Colour(0xFF4a6a7a).withAlpha(0.7f));
  g.setFont(12.0f);

  juce::String msg = "Click a wire to probe";
  if (!simulationValid)
    msg = "NO SIGNAL PATH\n\nAudio In and Out are not connected!\n"
          "Draw a wire from Input to Output";
  else if (!probeActive)
    msg = "No Probe Active\n\nClick a wire or connect AudioOutput\nEnsure "
          "START is pressed";
  else if (!simulationRunning)
    msg = "Simulation Stopped\n\nPress START to begin";

  g.drawText(msg, bounds, juce::Justification::centred);
}

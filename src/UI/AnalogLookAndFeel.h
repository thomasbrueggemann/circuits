#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

class AnalogLookAndFeel : public juce::LookAndFeel_V4 {
public:
  AnalogLookAndFeel() {
    // Warm, vintage color palette
    setColour(juce::ResizableWindow::backgroundColourId,
              juce::Colour(0xFF2a2a2a));
    setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF3d3d3d));
    setColour(juce::TextButton::textColourOffId, juce::Colour(0xFFdcdcdc));
    setColour(juce::Slider::thumbColourId, juce::Colour(0xFFff8800));
    setColour(juce::Slider::trackColourId, juce::Colour(0xFF1a1a1a));
    setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xFFff8800));
    setColour(juce::Slider::rotarySliderOutlineColourId,
              juce::Colour(0xFF1a1a1a));

    // Label colors
    setColour(juce::Label::textColourId, juce::Colour(0xFFb0b0b0));
  }

  void drawRotarySlider(juce::Graphics &g, int x, int y, int width, int height,
                        float sliderPos, const float rotaryStartAngle,
                        const float rotaryEndAngle,
                        juce::Slider &slider) override {
    auto radius = (float)juce::jmin(width / 2, height / 2) - 4.0f;
    auto centreX = (float)x + (float)width * 0.5f;
    auto centreY = (float)y + (float)height * 0.5f;
    auto rx = centreX - radius;
    auto ry = centreY - radius;
    auto rw = radius * 2.0f;
    auto angle =
        rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

    // Fill
    g.setColour(juce::Colour(0xFF333333));
    g.fillEllipse(rx, ry, rw, rw);

    // Outline
    g.setColour(juce::Colour(0xFF1a1a1a));
    g.drawEllipse(rx, ry, rw, rw, 2.0f);

    // Pointer
    juce::Path p;
    auto pointerLength = radius * 0.8f;
    auto pointerThickness = 3.0f;
    p.addRectangle(-pointerThickness * 0.5f, -radius, pointerThickness,
                   radius * 0.5f);
    p.applyTransform(
        juce::AffineTransform::rotation(angle).translated(centreX, centreY));

    g.setColour(juce::Colour(0xFFff8800));
    g.fillPath(p);

    // Cap
    g.setColour(juce::Colour(0xFF444444));
    g.fillEllipse(centreX - radius * 0.2f, centreY - radius * 0.2f,
                  radius * 0.4f, radius * 0.4f);
  }

  void drawLinearSlider(juce::Graphics &g, int x, int y, int width, int height,
                        float sliderPos, float minSliderPos, float maxSliderPos,
                        const juce::Slider::SliderStyle style,
                        juce::Slider &slider) override {
    if (style == juce::Slider::LinearBar ||
        style == juce::Slider::LinearBarVertical) {
      LookAndFeel_V4::drawLinearSlider(g, x, y, width, height, sliderPos,
                                       minSliderPos, maxSliderPos, style,
                                       slider);
      return;
    }

    auto isVertical = slider.isHorizontal() == false;
    auto trackWidth = isVertical ? 4.0f : (float)height * 0.25f;

    // Track
    g.setColour(slider.findColour(juce::Slider::trackColourId));
    if (isVertical)
      g.fillRoundedRectangle((float)x + (float)width * 0.5f - trackWidth * 0.5f,
                             (float)y, trackWidth, (float)height,
                             trackWidth * 0.5f);
    else
      g.fillRoundedRectangle(
          (float)x, (float)y + (float)height * 0.5f - trackWidth * 0.5f,
          (float)width, trackWidth, trackWidth * 0.5f);

    // Thumb
    g.setColour(slider.findColour(juce::Slider::thumbColourId));
    auto thumbSize = 16.0f;
    if (isVertical)
      g.fillRoundedRectangle((float)x + (float)width * 0.5f - thumbSize * 0.5f,
                             sliderPos - thumbSize * 0.5f, thumbSize, thumbSize,
                             2.0f);
    else
      g.fillRoundedRectangle(sliderPos - thumbSize * 0.5f,
                             (float)y + (float)height * 0.5f - thumbSize * 0.5f,
                             thumbSize, thumbSize, 2.0f);
  }
};

#pragma once

#include "../Engine/CircuitGraph.h"
#include <JuceHeader.h>

/**
 * ComponentPalette - Sidebar with draggable component icons.
 *
 * Users drag components from here onto the CircuitDesigner canvas.
 */
class ComponentPalette : public juce::Component {
public:
  ComponentPalette();
  ~ComponentPalette() override;

  void paint(juce::Graphics &g) override;
  void resized() override;

private:
  /**
   * PaletteItem - A single draggable component in the palette.
   */
  class PaletteItem : public juce::Component {
  public:
    PaletteItem(const juce::String &name, const juce::String &dragId,
                ComponentType type);

    void paint(juce::Graphics &g) override;
    void mouseDown(const juce::MouseEvent &e) override;
    void mouseDrag(const juce::MouseEvent &e) override;
    void mouseUp(const juce::MouseEvent &e) override;

  private:
    juce::String itemName;
    juce::String dragIdentifier;
    ComponentType componentType;
    bool isBeingDragged = false;

    void drawSymbol(juce::Graphics &g, juce::Rectangle<float> bounds);
  };

  std::vector<std::unique_ptr<PaletteItem>> items;

  void createItems();

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ComponentPalette)
};

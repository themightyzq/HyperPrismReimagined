# HyperPrism Revived - UI Standardization Guide

This guide outlines the standardized UI layout system implemented for all HyperPrism Revived plugins.

## Overview

The standardized layout system provides:
- Consistent window sizes and proportions
- Standardized spacing and margins
- Grid-based control layout
- Unified header and section styling
- Proper X/Y pad positioning

## Implementation

### 1. Include StandardLayout

Add to all Editor header files:
```cpp
#include "../Shared/StandardLayout.h"
```

### 2. Standard Window Sizes

Use these constants in the editor constructor:
```cpp
setSize(HyperPrismLayout::Constants::standardWidth, HyperPrismLayout::Constants::standardHeight);  // 700x550
// or
setSize(HyperPrismLayout::Constants::standardWidth, HyperPrismLayout::Constants::compactHeight);   // 700x450
// or  
setSize(HyperPrismLayout::Constants::standardWidth, HyperPrismLayout::Constants::largeHeight);     // 700x650
```

### 3. Standardized Paint Method

Replace existing paint methods with:
```cpp
void PluginEditor::paint(juce::Graphics& g)
{
    // Use standardized background
    HyperPrismLayout::StandardPaint::paintBackground(g, getLocalBounds());
    
    // Paint section headers
    auto bounds = getLocalBounds().reduced(HyperPrismLayout::Constants::windowMargin);
    auto headerArea = bounds.removeFromTop(HyperPrismLayout::Constants::headerHeight + 20);
    
    // Skip header for title
    bounds.removeFromTop(20);
    
    // Create sections for headers
    auto [leftSection, rightSection] = HyperPrismLayout::LayoutHelper::createTwoColumnLayout(bounds);
    
    auto leftHeader = leftSection.removeFromTop(25);
    auto rightHeader = rightSection.removeFromTop(25);
    
    HyperPrismLayout::StandardPaint::paintSectionHeader(g, leftHeader, "LEFT SECTION TITLE");
    HyperPrismLayout::StandardPaint::paintSectionHeader(g, rightHeader, "RIGHT SECTION TITLE");
}
```

### 4. Standardized Resized Method - Two Column Layout

For plugins with X/Y pad on left, controls on right:
```cpp
void PluginEditor::resized()
{
    using namespace HyperPrismLayout;
    
    auto bounds = getLocalBounds().reduced(Constants::windowMargin);
    
    // Header with title and buttons
    auto headerArea = bounds.removeFromTop(Constants::headerHeight);
    std::vector<juce::Component*> buttons = { &bypassButton, &additionalButton };
    LayoutHelper::layoutHeader(headerArea, titleLabel, buttons);
    
    bounds.removeFromTop(Constants::sectionSpacing);
    
    // Two-column layout: X/Y pad on left, controls on right
    auto [leftPanel, rightPanel] = LayoutHelper::createTwoColumnLayout(bounds);
    
    // X/Y Pad (left side) with section header space
    leftPanel.removeFromTop(30); // Space for section header
    auto xyPadBounds = LayoutHelper::calculateXYPadBounds(leftPanel);
    xyPad.setBounds(xyPadBounds);
    
    // Controls (right side) with section header space
    rightPanel.removeFromTop(30); // Space for section header
    
    // Layout controls in a grid (adjust maxColumns as needed)
    std::vector<juce::Component*> controls = {
        &control1, &control2, &control3,
        &control4, &control5, &control6
    };
    
    auto gridBounds = LayoutHelper::calculateControlGrid(rightPanel, controls.size(), 3);
    
    // Position controls in the calculated grid
    int col = 0, row = 0;
    for (auto* control : controls)
    {
        int x = gridBounds.getX() + col * (Constants::controlWidth + Constants::controlSpacing);
        int y = gridBounds.getY() + row * (Constants::controlHeight + Constants::rowSpacing);
        
        control->setBounds(x, y, Constants::controlWidth, Constants::controlHeight);
        
        col++;
        if (col >= 3)  // Adjust based on maxColumns
        {
            col = 0;
            row++;
        }
    }
}
```

### 5. Standardized Resized Method - Three Section Layout

For plugins with controls at top, X/Y pad and meter at bottom:
```cpp
void PluginEditor::resized()
{
    using namespace HyperPrismLayout;
    
    auto bounds = getLocalBounds().reduced(Constants::windowMargin);
    
    // Header with title and buttons
    auto headerArea = bounds.removeFromTop(Constants::headerHeight);
    std::vector<juce::Component*> buttons = { &bypassButton };
    LayoutHelper::layoutHeader(headerArea, titleLabel, buttons);
    
    bounds.removeFromTop(Constants::sectionSpacing);
    
    // Three-section layout: controls top, X/Y pad and meter bottom
    auto [topSection, leftSection, rightSection] = 
        LayoutHelper::createThreeSectionLayout(bounds, 140);
    
    // Top section - controls grid
    auto gridBounds = LayoutHelper::calculateControlGrid(topSection, controls.size(), 5);
    
    // Layout controls horizontally
    int col = 0;
    for (auto* control : controls)
    {
        int x = gridBounds.getX() + col * (Constants::controlWidth + Constants::controlSpacing);
        control->setBounds(x, gridBounds.getY(), Constants::controlWidth, Constants::controlHeight);
        col++;
    }
    
    // Bottom left - X/Y pad
    leftSection.removeFromTop(20); // Space for label
    auto xyPadBounds = LayoutHelper::calculateXYPadBounds(leftSection);
    xyPad.setBounds(xyPadBounds);
    
    // Bottom right - meter
    rightSection.removeFromTop(20); // Space for label
    auto meterBounds = LayoutHelper::calculateMeterBounds(rightSection);
    meter.setBounds(meterBounds);
}
```

## Standard Constants

All spacing and sizing should use these constants:

```cpp
// Layout margins and spacing
HyperPrismLayout::Constants::windowMargin     // 20px - outer window margin
HyperPrismLayout::Constants::contentMargin    // 15px - content margins
HyperPrismLayout::Constants::sectionSpacing   // 20px - space between sections

// Header dimensions
HyperPrismLayout::Constants::headerHeight     // 50px - header area height
HyperPrismLayout::Constants::titleHeight      // 40px - title label height
HyperPrismLayout::Constants::buttonWidth      // 90px - standard button width
HyperPrismLayout::Constants::buttonHeight     // 32px - standard button height

// Control grid
HyperPrismLayout::Constants::controlWidth     // 90px - rotary slider width
HyperPrismLayout::Constants::controlHeight    // 120px - rotary slider height (including text box)
HyperPrismLayout::Constants::controlSpacing   // 15px - space between controls
HyperPrismLayout::Constants::rowSpacing       // 20px - space between rows

// Special components
HyperPrismLayout::Constants::xyPadSize        // 140px - standard X/Y pad size
HyperPrismLayout::Constants::meterHeight      // 180px - standard meter height

// Window sizes
HyperPrismLayout::Constants::standardWidth    // 700px
HyperPrismLayout::Constants::standardHeight   // 550px
HyperPrismLayout::Constants::compactHeight    // 450px
HyperPrismLayout::Constants::largeHeight      // 650px
```

## Implementation Status

✅ **Completed:**
- StandardLayout helper classes created
- DelayEditor updated as example
- CMakeLists.txt updated with shared files

🔄 **To be applied to remaining plugins:**
- Apply standardized layout to all 29 remaining plugins
- Update window sizes to use constants
- Standardize section headers and spacing
- Ensure consistent control sizing and positioning

## Benefits

- **Visual Consistency:** All plugins look and feel unified
- **Maintainability:** Changes to layout constants affect all plugins
- **User Experience:** Predictable interface across all effects
- **Development Speed:** New plugins can be quickly styled with standard layouts
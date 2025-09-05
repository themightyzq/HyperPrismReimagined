# HyperPrism UI Layout Guide

## Standard Layout Rules

All HyperPrism plugins follow these layout rules to ensure consistency and proper XY Pad placement:

### Window Size
- **650x550 pixels** (fixed for all plugins)

### Title Section
- Height: 40px
- Title: Cyan color, Arial Bold 24pt, centered
- Bypass button: Top right (bounds.getWidth() - 100, 10, 80, 30)

### XY Pad Position (MUST BE CONSISTENT)
- **Position**: Centered horizontally, 40px from top of control area
- **Size**: 200x180 pixels
- **Label**: Below pad, 200x20 pixels

### Control Layouts by Parameter Count

#### 4 Parameters or Less (e.g., AutoPan)
- Single row above XY Pad
- Centered horizontally
- Knob size: 80x120 pixels
- Spacing: 15px

#### 5-7 Parameters (e.g., Chorus)
- **Left Column**: 2x2 grid (4 controls max)
- **XY Pad**: Center
- **Right Column**: Remaining controls
- This keeps XY Pad in exact same position

#### 8+ Parameters
- **Top Row**: 4 controls max
- **Left/Right Columns**: Additional controls
- **XY Pad**: Center (same position)
- May need to reduce knob size slightly

### Layout Code Template
```cpp
void EditorName::resized()
{
    auto bounds = getLocalBounds();
    
    // Title
    titleLabel.setBounds(bounds.removeFromTop(40));
    
    // Bypass button (top right)
    bypassButton.setBounds(bounds.getWidth() - 100, 10, 80, 30);
    
    bounds.reduce(20, 10);
    
    // Controls area (fixed height to ensure XY pad position)
    auto controlsArea = bounds.removeFromTop(300);
    
    // XY Pad (ALWAYS in same position)
    auto xyPadWidth = 200;
    auto xyPadHeight = 180;
    auto centerX = controlsArea.getCentreX();
    xyPad.setBounds(centerX - xyPadWidth/2, controlsArea.getY() + 40, xyPadWidth, xyPadHeight);
    xyPadLabel.setBounds(xyPad.getX(), xyPad.getBottom(), xyPadWidth, 20);
    
    // ... arrange other controls around XY pad ...
}
```

### Visual Hierarchy
1. **Title** - Most prominent (cyan)
2. **XY Pad** - Central focus with colored crosshairs
3. **Parameter Knobs** - Organized around XY pad
4. **Labels** - Color-coded based on XY assignment

### Color Coding
- **X-axis**: Blue (0, 150, 255)
- **Y-axis**: Yellow (255, 220, 0)
- **Both**: Green (blend)
- **Unassigned**: Light grey

This ensures every plugin has the XY Pad in the exact same position for muscle memory and visual consistency.
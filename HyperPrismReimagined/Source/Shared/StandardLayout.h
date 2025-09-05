//==============================================================================
// HyperPrism Revived - Standard Layout Helper
//==============================================================================

#pragma once

#include <JuceHeader.h>

namespace HyperPrismLayout
{
    // Standard dimensions and spacing
    struct Constants
    {
        // Main layout
        static constexpr int windowMargin = 20;
        static constexpr int contentMargin = 15;
        static constexpr int sectionSpacing = 20;
        
        // Header
        static constexpr int headerHeight = 50;
        static constexpr int titleHeight = 40;
        static constexpr int buttonWidth = 90;
        static constexpr int buttonHeight = 32;
        static constexpr int buttonSpacing = 10;
        
        // Control grid
        static constexpr int controlWidth = 90;
        static constexpr int controlHeight = 120;
        static constexpr int controlSpacing = 15;
        static constexpr int rowSpacing = 20;
        
        // X/Y Pad (consistent 200x180 standard)
        static constexpr int xyPadWidth = 200;
        static constexpr int xyPadHeight = 180;
        static constexpr int xyPadSize = 140; // Legacy - use xyPadWidth/Height instead
        static constexpr int xyPadMargin = 20;
        
        // Meters
        static constexpr int meterHeight = 180;
        static constexpr int meterMargin = 10;
        
        // Standard window sizes
        static constexpr int standardWidth = 700;
        static constexpr int standardHeight = 550;
        static constexpr int compactHeight = 450;
        static constexpr int largeHeight = 650;
    };
    
    // Layout helper functions
    class LayoutHelper
    {
    public:
        // Calculate optimal control grid layout
        static juce::Rectangle<int> calculateControlGrid(
            const juce::Rectangle<int>& area,
            int numControls,
            int maxColumns = 4,
            int controlWidth = Constants::controlWidth,
            int controlHeight = Constants::controlHeight,
            int spacing = Constants::controlSpacing);
        
        // Standard header layout with title and buttons
        static void layoutHeader(
            const juce::Rectangle<int>& headerArea,
            juce::Label& titleLabel,
            std::vector<juce::Component*> buttons);
        
        // Two-column layout (X/Y pad on left, controls on right)
        static std::pair<juce::Rectangle<int>, juce::Rectangle<int>> 
        createTwoColumnLayout(
            const juce::Rectangle<int>& area,
            float leftColumnRatio = 0.4f);
        
        // Three-section layout (controls top, X/Y pad and meter bottom)
        static std::tuple<juce::Rectangle<int>, juce::Rectangle<int>, juce::Rectangle<int>>
        createThreeSectionLayout(
            const juce::Rectangle<int>& area,
            int topSectionHeight = 140);
        
        // Position X/Y pad with consistent sizing (200x180 standard)
        static juce::Rectangle<int> calculateXYPadBounds(
            const juce::Rectangle<int>& area,
            int padWidth = Constants::xyPadWidth,
            int padHeight = Constants::xyPadHeight);
        
        // Position meter component
        static juce::Rectangle<int> calculateMeterBounds(
            const juce::Rectangle<int>& area,
            int meterHeight = Constants::meterHeight);
    };
    
    // Standard paint methods
    class StandardPaint
    {
    public:
        // Standard background with surface and borders
        static void paintBackground(juce::Graphics& g, const juce::Rectangle<int>& bounds);
        
        // Section headers and dividers
        static void paintSectionHeader(juce::Graphics& g, 
                                     const juce::Rectangle<int>& area,
                                     const juce::String& title);
        
        // Draw grid guides (for development/debugging)
        static void paintLayoutGuides(juce::Graphics& g, 
                                    const juce::Rectangle<int>& bounds,
                                    bool enabled = false);
    };
}
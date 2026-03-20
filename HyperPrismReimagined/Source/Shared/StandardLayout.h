//==============================================================================
// HyperPrism Reimagined - Standard Layout Helper
//==============================================================================

#pragma once

#include <JuceHeader.h>

namespace HyperPrismLayout
{
    // Standard dimensions and spacing
    struct Constants
    {
        // Header
        static constexpr int headerHeight = 72;

        // Control columns (vertical layout)
        static constexpr int columnWidth = 100;
        static constexpr int columnMinWidth = 85;
        static constexpr int columnSpacing = 10;
        static constexpr int knobSize = 64;
        static constexpr int knobSizeLarge = 80;
        static constexpr int knobSizeSmall = 54;
        static constexpr int knobVerticalSpacing = 90;
        static constexpr int knobVerticalSpacingTight = 76;

        // XY Pad (right side)
        static constexpr int xyPadMinWidth = 280;
        static constexpr int xyPadMinHeight = 240;

        // Output section (below XY pad)
        static constexpr int outputSectionHeight = 140;

        // Footer
        static constexpr int footerHeight = 20;

        // Overall
        static constexpr int contentMargin = 12;
        static constexpr int sectionHeaderHeight = 16;

        // Standard window sizes
        static constexpr int standardWidth = 700;
        static constexpr int standardHeight = 550;
        static constexpr int compactHeight = 450;
        static constexpr int largeHeight = 650;

        // Resize constraints
        static constexpr int minWidth = 600;
        static constexpr int maxWidth = 900;
        static constexpr int minHeight = 480;
        static constexpr int maxHeight = 750;
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
            int controlWidth = Constants::columnWidth,
            int controlHeight = Constants::knobVerticalSpacing,
            int spacing = Constants::columnSpacing);

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

        // Position X/Y pad
        static juce::Rectangle<int> calculateXYPadBounds(
            const juce::Rectangle<int>& area,
            int padWidth = Constants::xyPadMinWidth,
            int padHeight = Constants::xyPadMinHeight);

        // Position meter component
        static juce::Rectangle<int> calculateMeterBounds(
            const juce::Rectangle<int>& area,
            int meterHeight = Constants::outputSectionHeight);
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
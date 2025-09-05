//==============================================================================
// HyperPrism Revived - Standard Layout Helper Implementation
//==============================================================================

#include "StandardLayout.h"
#include "HyperPrismLookAndFeel.h"

namespace HyperPrismLayout
{
    juce::Rectangle<int> LayoutHelper::calculateControlGrid(
        const juce::Rectangle<int>& area,
        int numControls,
        int maxColumns,
        int controlWidth,
        int controlHeight,
        int spacing)
    {
        // Calculate optimal grid dimensions
        int columns = juce::jmin(numControls, maxColumns);
        int rows = (numControls + columns - 1) / columns; // Ceiling division
        
        // Calculate total grid size
        int totalWidth = (columns * controlWidth) + ((columns - 1) * spacing);
        int totalHeight = (rows * controlHeight) + ((rows - 1) * Constants::rowSpacing);
        
        // Center the grid in the available area
        int startX = area.getX() + (area.getWidth() - totalWidth) / 2;
        int startY = area.getY() + (area.getHeight() - totalHeight) / 2;
        
        return juce::Rectangle<int>(startX, startY, totalWidth, totalHeight);
    }
    
    void LayoutHelper::layoutHeader(
        const juce::Rectangle<int>& headerArea,
        juce::Label& titleLabel,
        std::vector<juce::Component*> buttons)
    {
        auto area = headerArea;
        
        // Reserve space for buttons on the right
        int buttonsWidth = static_cast<int>(buttons.size()) * (Constants::buttonWidth + Constants::buttonSpacing) - Constants::buttonSpacing;
        auto buttonArea = area.removeFromRight(buttonsWidth);
        
        // Title gets remaining space
        titleLabel.setBounds(area.reduced(5));
        
        // Position buttons from right to left
        for (int i = static_cast<int>(buttons.size()) - 1; i >= 0; --i)
        {
            auto bounds = buttonArea.removeFromRight(Constants::buttonWidth);
            bounds = bounds.withSizeKeepingCentre(Constants::buttonWidth, Constants::buttonHeight);
            buttons[i]->setBounds(bounds);
            
            if (i > 0) // Add spacing between buttons
                buttonArea.removeFromRight(Constants::buttonSpacing);
        }
    }
    
    std::pair<juce::Rectangle<int>, juce::Rectangle<int>> 
    LayoutHelper::createTwoColumnLayout(
        const juce::Rectangle<int>& area,
        float leftColumnRatio)
    {
        auto areaCopy = area;
        auto leftColumn = areaCopy.removeFromLeft(static_cast<int>(area.getWidth() * leftColumnRatio));
        auto rightColumn = areaCopy.reduced(Constants::sectionSpacing / 2, 0);
        
        return {leftColumn, rightColumn};
    }
    
    std::tuple<juce::Rectangle<int>, juce::Rectangle<int>, juce::Rectangle<int>>
    LayoutHelper::createThreeSectionLayout(
        const juce::Rectangle<int>& area,
        int topSectionHeight)
    {
        auto areaCopy = area;
        auto topSection = areaCopy.removeFromTop(topSectionHeight);
        auto remainingArea = areaCopy.reduced(0, Constants::sectionSpacing);
        
        auto [leftSection, rightSection] = createTwoColumnLayout(remainingArea);
        
        return {topSection, leftSection, rightSection};
    }
    
    juce::Rectangle<int> LayoutHelper::calculateXYPadBounds(
        const juce::Rectangle<int>& area,
        int padWidth,
        int padHeight)
    {
        // Ensure the pad fits in the available area
 
        int maxWidth = area.getWidth() - Constants::xyPadMargin * 2;
        int maxHeight = area.getHeight() - Constants::xyPadMargin * 2;
        int actualWidth = juce::jmin(padWidth, maxWidth);
        int actualHeight = juce::jmin(padHeight, maxHeight);
        
        return area.withSizeKeepingCentre(actualWidth, actualHeight);
    }
    
    juce::Rectangle<int> LayoutHelper::calculateMeterBounds(
        const juce::Rectangle<int>& area,
        int meterHeight)
    {
        int actualHeight = juce::jmin(meterHeight, area.getHeight() - Constants::meterMargin * 2);
        auto bounds = area.withHeight(actualHeight);
        bounds = bounds.withPosition(area.getX(), area.getY() + Constants::meterMargin);
        
        return bounds.reduced(Constants::meterMargin, 0);
    }
    
    void StandardPaint::paintBackground(juce::Graphics& g, const juce::Rectangle<int>& bounds)
    {
        // Background gradient
        juce::ColourGradient gradient(
            HyperPrismLookAndFeel::Colors::surfaceVariant, 0, 0,
            HyperPrismLookAndFeel::Colors::surface, 0, static_cast<float>(bounds.getHeight()), 
            false);
        g.setGradientFill(gradient);
        g.fillAll();
        
        // Main surface
        auto surfaceArea = bounds.reduced(Constants::windowMargin);
        g.setColour(HyperPrismLookAndFeel::Colors::surface);
        g.fillRoundedRectangle(surfaceArea.toFloat(), 8.0f);
        
        // Border
        g.setColour(HyperPrismLookAndFeel::Colors::outline);
        g.drawRoundedRectangle(surfaceArea.toFloat(), 8.0f, 2.0f);
    }
    
    void StandardPaint::paintSectionHeader(juce::Graphics& g, 
                                         const juce::Rectangle<int>& area,
                                         const juce::String& title)
    {
        g.setColour(HyperPrismLookAndFeel::Colors::onSurfaceVariant);
        g.setFont(juce::Font(juce::FontOptions(11.0f).withStyle("Bold")));
        g.drawText(title, area, juce::Justification::centredLeft);
        
        // Underline
        auto areaCopy = area;
        auto lineArea = areaCopy.removeFromBottom(1);
        lineArea = lineArea.reduced(0, 5);
        g.setColour(HyperPrismLookAndFeel::Colors::outline.withAlpha(0.3f));
        g.fillRect(lineArea);
    }
    
    void StandardPaint::paintLayoutGuides(juce::Graphics& g, 
                                        const juce::Rectangle<int>& bounds,
                                        bool enabled)
    {
        if (!enabled) return;
        
        g.setColour(juce::Colours::red.withAlpha(0.2f));
        
        // Draw grid lines every 20 pixels
        for (int x = bounds.getX(); x < bounds.getRight(); x += 20)
        {
            g.drawVerticalLine(x, static_cast<float>(bounds.getY()), static_cast<float>(bounds.getBottom()));
        }
        
        for (int y = bounds.getY(); y < bounds.getBottom(); y += 20)
        {
            g.drawHorizontalLine(y, static_cast<float>(bounds.getX()), static_cast<float>(bounds.getRight()));
        }
    }
}
#!/bin/bash
# HyperPrism Reimagined - Mac Build Script
# Builds Universal Binary VST3s for distribution

set -e  # Exit on error

echo "=== HyperPrism Reimagined Mac Build Script ==="

# Configuration
PROJECT_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="$PROJECT_ROOT/Builds/MacOSX"
OUTPUT_DIR="$PROJECT_ROOT/Distribution/Mac"
CODESIGN_IDENTITY="AJU6TF97JM"  # Update with your identity

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Create output directory
mkdir -p "$OUTPUT_DIR/VST3"

# Function to build a plugin
build_plugin() {
    local plugin_name=$1
    local project_file="$PROJECT_ROOT/$plugin_name/$plugin_name.jucer"
    
    if [ ! -f "$project_file" ]; then
        echo -e "${YELLOW}Warning: $project_file not found, skipping...${NC}"
        return
    fi
    
    echo -e "${GREEN}Building $plugin_name...${NC}"
    
    # Build using Projucer (adjust path as needed)
    # Note: You may need to update the Projucer path
    /Applications/JUCE/Projucer.app/Contents/MacOS/Projucer --resave "$project_file"
    
    # Build both architectures
    cd "$BUILD_DIR"
    xcodebuild -project "$plugin_name.xcodeproj" \
               -configuration Release \
               -target "$plugin_name - VST3" \
               -arch x86_64 \
               -arch arm64 \
               ONLY_ACTIVE_ARCH=NO \
               BUILD_DIR="$BUILD_DIR/build"
    
    # Copy to output directory
    cp -R "$BUILD_DIR/build/Release/$plugin_name.vst3" "$OUTPUT_DIR/VST3/"
    
    # Code sign if identity is available
    if [ -n "$CODESIGN_IDENTITY" ]; then
        echo "Code signing $plugin_name.vst3..."
        codesign --force --deep --sign "$CODESIGN_IDENTITY" "$OUTPUT_DIR/VST3/$plugin_name.vst3"
    fi
    
    echo -e "${GREEN}✓ $plugin_name built successfully${NC}"
}

# List of all plugins
PLUGINS=(
    "AutoPan"
    "BandPass"
    "BandReject"
    "BassMaximiser"
    "Chorus"
    "Compressor"
    "Delay"
    "Echo"
    "Flanger"
    "FrequencyShifter"
    "HarmonicExciter"
    "HighPass"
    "HyperPhaser"
    "Limiter"
    "LowPass"
    "MoreStereo"
    "MSMatrix"
    "MultiDelay"
    "NoiseGate"
    "Pan"
    "Phaser"
    "PitchChanger"
    "QuasiStereo"
    "Reverb"
    "RingModulator"
    "SingleDelay"
    "SonicDecimator"
    "StereoDynamics"
    "Tremolo"
    "TubeTapeSaturation"
    "Vibrato"
    "Vocoder"
)

# Build all plugins
for plugin in "${PLUGINS[@]}"; do
    build_plugin "$plugin"
done

# Create a ZIP for distribution
echo -e "${GREEN}Creating distribution ZIP...${NC}"
cd "$OUTPUT_DIR"
zip -r "HyperPrism_Reimagined_v1.0_Mac.zip" VST3

echo -e "${GREEN}=== Build Complete ===${NC}"
echo "VST3s are in: $OUTPUT_DIR/VST3"
echo "Distribution ZIP: $OUTPUT_DIR/HyperPrism_Reimagined_v1.0_Mac.zip"

# Verify Universal Binary
echo -e "\n${GREEN}Verifying Universal Binary builds:${NC}"
for vst in "$OUTPUT_DIR/VST3"/*.vst3; do
    echo -n "$(basename "$vst"): "
    lipo -info "$vst/Contents/MacOS/$(basename "$vst" .vst3)" 2>/dev/null || echo "Binary not found"
done
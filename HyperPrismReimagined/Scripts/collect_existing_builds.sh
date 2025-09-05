#!/bin/bash
# HyperPrism Reimagined - Collect Existing Builds
# Collects already-built VST3s for distribution

set -e

echo "=== Collecting Existing HyperPrism Builds ==="

# Configuration
PROJECT_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="$PROJECT_ROOT/build"
OUTPUT_DIR="$PROJECT_ROOT/Distribution/Mac"
CODESIGN_IDENTITY="AJU6TF97JM"

# Colors
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

# Create output directory
mkdir -p "$OUTPUT_DIR/VST3"

# Collect all VST3s from build directory
echo -e "${GREEN}Collecting VST3 plugins from build directory...${NC}"

find "$BUILD_DIR" -name "*.vst3" -type d | while read -r vst_path; do
    vst_name=$(basename "$vst_path")
    echo "Found: $vst_name"
    
    # Copy to output directory
    cp -R "$vst_path" "$OUTPUT_DIR/VST3/"
    
    # Code sign
    if [ -n "$CODESIGN_IDENTITY" ]; then
        echo "  Code signing..."
        codesign --force --deep --sign "$CODESIGN_IDENTITY" "$OUTPUT_DIR/VST3/$vst_name" 2>/dev/null || echo "  Warning: Code signing failed"
    fi
done

# Create ZIP for distribution
echo -e "\n${GREEN}Creating distribution ZIP...${NC}"
cd "$OUTPUT_DIR"
zip -r "HyperPrism_Reimagined_v1.0_Mac.zip" VST3

# List what was collected
echo -e "\n${GREEN}Collected plugins:${NC}"
ls -1 "$OUTPUT_DIR/VST3" | sort

# Count
plugin_count=$(find "$OUTPUT_DIR/VST3" -name "*.vst3" -type d | wc -l | tr -d ' ')
echo -e "\n${GREEN}Total plugins collected: $plugin_count${NC}"
echo "Distribution ZIP: $OUTPUT_DIR/HyperPrism_Reimagined_v1.0_Mac.zip"
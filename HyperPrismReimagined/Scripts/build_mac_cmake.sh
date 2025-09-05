#!/bin/bash
# HyperPrism Reimagined - Mac Build Script (CMake version)
# Builds Universal Binary VST3s for distribution

set -e  # Exit on error

echo "=== HyperPrism Reimagined Mac Build Script (CMake) ==="

# Configuration
PROJECT_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="$PROJECT_ROOT/build"
OUTPUT_DIR="$PROJECT_ROOT/Distribution/Mac"
CODESIGN_IDENTITY="AJU6TF97JM"  # Your identity

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Create output directory
mkdir -p "$OUTPUT_DIR/VST3"

# Function to build all plugins with CMake
build_all_plugins() {
    echo -e "${GREEN}Building all plugins with CMake...${NC}"
    
    # Clean build directory
    if [ -d "$BUILD_DIR" ]; then
        echo "Cleaning existing build directory..."
        rm -rf "$BUILD_DIR"
    fi
    
    # Create build directory
    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"
    
    # Configure with CMake for Universal Binary
    echo -e "${GREEN}Configuring CMake...${NC}"
    cmake -DCMAKE_BUILD_TYPE=Release \
          -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" \
          ..
    
    # Build all targets
    echo -e "${GREEN}Building all plugins...${NC}"
    cmake --build . --config Release -j$(sysctl -n hw.ncpu)
    
    echo -e "${GREEN}✓ Build complete${NC}"
}

# Function to collect and sign VST3s
collect_and_sign_vsts() {
    echo -e "${GREEN}Collecting VST3 plugins...${NC}"
    
    # Find all VST3s in build directory
    find "$BUILD_DIR" -name "*.vst3" -type d | while read -r vst_path; do
        vst_name=$(basename "$vst_path")
        echo "Processing: $vst_name"
        
        # Copy to output directory
        cp -R "$vst_path" "$OUTPUT_DIR/VST3/"
        
        # Code sign if identity is available
        if [ -n "$CODESIGN_IDENTITY" ]; then
            echo "Code signing $vst_name..."
            codesign --force --deep --sign "$CODESIGN_IDENTITY" "$OUTPUT_DIR/VST3/$vst_name"
        fi
    done
    
    echo -e "${GREEN}✓ Collection complete${NC}"
}

# Function to verify Universal Binary
verify_universal_binary() {
    echo -e "\n${GREEN}Verifying Universal Binary builds:${NC}"
    
    for vst in "$OUTPUT_DIR/VST3"/*.vst3; do
        if [ -d "$vst" ]; then
            vst_name=$(basename "$vst" .vst3)
            binary_path="$vst/Contents/MacOS/$vst_name"
            
            # Try different possible binary names
            if [ ! -f "$binary_path" ]; then
                # Try without spaces
                binary_path="$vst/Contents/MacOS/$(echo "$vst_name" | tr -d ' ')"
            fi
            
            if [ -f "$binary_path" ]; then
                echo -n "$vst_name: "
                lipo -info "$binary_path" 2>/dev/null || echo "Binary check failed"
            else
                echo "$vst_name: Binary not found at expected location"
            fi
        fi
    done
}

# Main build process
echo "Starting build process..."

# Build all plugins
build_all_plugins

# Collect and sign
collect_and_sign_vsts

# Create distribution ZIP
echo -e "${GREEN}Creating distribution ZIP...${NC}"
cd "$OUTPUT_DIR"
zip -r "HyperPrism_Reimagined_v1.0_Mac.zip" VST3

# Verify builds
verify_universal_binary

echo -e "\n${GREEN}=== Build Complete ===${NC}"
echo "VST3s are in: $OUTPUT_DIR/VST3"
echo "Distribution ZIP: $OUTPUT_DIR/HyperPrism_Reimagined_v1.0_Mac.zip"

# Count plugins
plugin_count=$(find "$OUTPUT_DIR/VST3" -name "*.vst3" -type d | wc -l | tr -d ' ')
echo -e "\n${GREEN}Total plugins built: $plugin_count${NC}"
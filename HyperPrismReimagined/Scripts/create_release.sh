#!/bin/bash
# HyperPrism Reimagined - Master Release Script
# Creates complete distribution packages for Mac and Windows

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
VERSION="1.0.0"

echo "=== HyperPrism Reimagined Release Builder v$VERSION ==="
echo

# Colors
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'

# Function to check prerequisites
check_prerequisites() {
    echo "Checking prerequisites..."
    
    # Check for Projucer
    if [ ! -f "/Applications/JUCE/Projucer.app/Contents/MacOS/Projucer" ]; then
        echo -e "${YELLOW}Warning: Projucer not found at expected location${NC}"
        echo "Please update the path in build_mac.sh"
    fi
    
    # Check for Xcode
    if ! command -v xcodebuild &> /dev/null; then
        echo -e "${RED}Error: Xcode command line tools not found${NC}"
        exit 1
    fi
    
    echo -e "${GREEN}✓ Prerequisites check complete${NC}"
}

# Function to create distribution structure
setup_distribution() {
    echo "Setting up distribution directories..."
    
    mkdir -p "$PROJECT_ROOT/Distribution/Mac/VST3"
    mkdir -p "$PROJECT_ROOT/Distribution/Windows/VST3"
    mkdir -p "$PROJECT_ROOT/Distribution/Release"
    
    echo -e "${GREEN}✓ Distribution directories created${NC}"
}

# Function to build Mac version
build_mac() {
    echo
    echo "=== Building Mac Version ==="
    
    if [ -f "$SCRIPT_DIR/build_mac.sh" ]; then
        chmod +x "$SCRIPT_DIR/build_mac.sh"
        "$SCRIPT_DIR/build_mac.sh"
    else
        echo -e "${RED}Error: build_mac.sh not found${NC}"
        return 1
    fi
    
    # Build installer if on Mac
    if [[ "$OSTYPE" == "darwin"* ]]; then
        echo "Building Mac installer package..."
        chmod +x "$SCRIPT_DIR/mac_installer/build_installer.sh"
        "$SCRIPT_DIR/mac_installer/build_installer.sh"
    fi
    
    echo -e "${GREEN}✓ Mac build complete${NC}"
}

# Function to prepare Windows files
prepare_windows() {
    echo
    echo "=== Preparing Windows Build Files ==="
    
    # Create a note about Windows building
    cat > "$PROJECT_ROOT/Distribution/Windows/BUILD_ON_WINDOWS.txt" << EOF
HyperPrism Reimagined - Windows Build Instructions

The Windows version must be built on a Windows machine with:
- Visual Studio 2022
- JUCE framework
- Inno Setup (for installer)

To build:
1. Copy this entire project to your Windows machine
2. Run Scripts/build_windows.bat
3. Run Inno Setup on Scripts/windows_installer/HyperPrism_Installer.iss

The build script and installer configuration have been prepared for you.
EOF
    
    echo -e "${GREEN}✓ Windows build files prepared${NC}"
}

# Function to create release package
create_release_package() {
    echo
    echo "=== Creating Release Package ==="
    
    RELEASE_DIR="$PROJECT_ROOT/Distribution/Release/HyperPrism_Reimagined_v${VERSION}_Beta"
    mkdir -p "$RELEASE_DIR"
    
    # Copy documentation
    cp "$PROJECT_ROOT/Distribution/README_BETA_TESTERS.md" "$RELEASE_DIR/"
    
    # Copy Mac files if they exist
    if [ -f "$PROJECT_ROOT/Distribution/Mac/HyperPrism_Reimagined_v${VERSION}_Mac.zip" ]; then
        cp "$PROJECT_ROOT/Distribution/Mac/HyperPrism_Reimagined_v${VERSION}_Mac.zip" "$RELEASE_DIR/"
        echo "✓ Mac ZIP included"
    fi
    
    if [ -f "$PROJECT_ROOT/Distribution/Mac/HyperPrism_Reimagined_v${VERSION}_Mac_Installer.pkg" ]; then
        cp "$PROJECT_ROOT/Distribution/Mac/HyperPrism_Reimagined_v${VERSION}_Mac_Installer.pkg" "$RELEASE_DIR/"
        echo "✓ Mac installer included"
    fi
    
    # Copy Windows build files
    cp -r "$SCRIPT_DIR/build_windows.bat" "$RELEASE_DIR/"
    cp -r "$SCRIPT_DIR/windows_installer" "$RELEASE_DIR/"
    echo "✓ Windows build scripts included"
    
    # Create final ZIP
    cd "$PROJECT_ROOT/Distribution/Release"
    zip -r "HyperPrism_Reimagined_v${VERSION}_Beta_Release.zip" "HyperPrism_Reimagined_v${VERSION}_Beta"
    
    echo -e "${GREEN}✓ Release package created${NC}"
}

# Function to generate release notes
generate_release_notes() {
    cat > "$PROJECT_ROOT/Distribution/Release/RELEASE_NOTES_v${VERSION}.md" << EOF
# HyperPrism Reimagined v${VERSION} Beta - Release Notes

## Release Date: $(date +%Y-%m-%d)

### New in This Release
- Initial beta release of HyperPrism Reimagined
- 32 professional audio effect plugins
- Unified 650x600 interface design
- XY Pad control system
- Real-time visual feedback

### System Requirements
**macOS:**
- macOS 10.13 or later
- Intel or Apple Silicon processor
- VST3-compatible DAW

**Windows:**
- Windows 10 or later
- 64-bit processor
- VST3-compatible DAW

### Installation
See README_BETA_TESTERS.md for detailed installation instructions.

### Known Issues
- Plugins are not code-signed (security warnings on first launch)
- Some DAWs may require manual plugin scanning
- Parameter automation recording may vary by DAW

### Beta Test Focus Areas
Please pay special attention to:
1. Plugin stability during extended use
2. Parameter automation accuracy
3. CPU usage with multiple instances
4. Preset save/recall functionality
5. XY Pad responsiveness

### Reporting Issues
Please report all issues to: beta@hyperprism.com

Include:
- Plugin name
- DAW and version
- Operating system
- Steps to reproduce
- Any error messages

Thank you for testing HyperPrism Reimagined!
EOF
    
    echo -e "${GREEN}✓ Release notes generated${NC}"
}

# Main execution
echo "This script will create a complete release package for HyperPrism Reimagined"
echo "Continue? (y/n)"
read -r response

if [[ "$response" != "y" ]]; then
    echo "Release creation cancelled"
    exit 0
fi

check_prerequisites
setup_distribution

# Build based on platform
if [[ "$OSTYPE" == "darwin"* ]]; then
    echo "Detected macOS - building Mac version"
    build_mac
    prepare_windows
else
    echo "This script should be run on macOS for Mac builds"
    echo "Windows builds must be created on a Windows machine"
    prepare_windows
fi

create_release_package
generate_release_notes

echo
echo "=========================================="
echo -e "${GREEN}Release package created successfully!${NC}"
echo
echo "Location: $PROJECT_ROOT/Distribution/Release/"
echo
echo "Contents:"
echo "- HyperPrism_Reimagined_v${VERSION}_Beta_Release.zip"
echo "- RELEASE_NOTES_v${VERSION}.md"
echo
echo "Next steps:"
echo "1. Build Windows version on a Windows machine"
echo "2. Add Windows files to the release package"
echo "3. Upload to distribution server"
echo "4. Send to beta testers"
echo "=========================================="
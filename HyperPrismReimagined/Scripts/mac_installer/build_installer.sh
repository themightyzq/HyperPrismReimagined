#!/bin/bash
# Build Mac installer package for HyperPrism Reimagined

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
OUTPUT_DIR="$PROJECT_ROOT/Distribution/Mac"
INSTALLER_DIR="$SCRIPT_DIR"

# Version info
VERSION="1.0.0"
IDENTIFIER="com.hyperprism.reimagined"

echo "Building Mac installer package..."

# Create package structure
mkdir -p "$INSTALLER_DIR/package_root/Library/Audio/Plug-Ins/VST3"

# Copy VST3s to package root
cp -R "$OUTPUT_DIR/VST3/"*.vst3 "$INSTALLER_DIR/package_root/Library/Audio/Plug-Ins/VST3/"

# Create component package
pkgbuild --root "$INSTALLER_DIR/package_root" \
         --identifier "$IDENTIFIER.vst3" \
         --version "$VERSION" \
         --install-location "/" \
         "$INSTALLER_DIR/HyperPrism_VST3.pkg"

# Create distribution XML
cat > "$INSTALLER_DIR/distribution.xml" << EOF
<?xml version="1.0" encoding="utf-8"?>
<installer-gui-script minSpecVersion="2">
    <title>HyperPrism Reimagined</title>
    <organization>com.hyperprism</organization>
    <domains enable_localSystem="true"/>
    <options customize="never" require-scripts="true" rootVolumeOnly="true" />
    <welcome file="welcome.html" />
    <readme file="readme.html" />
    <license file="license.html" />
    <pkg-ref id="com.hyperprism.reimagined.vst3">
        <must-close>
            <app id="com.apple.logic10" />
            <app id="com.ableton.live" />
            <app id="com.steinberg.cubase" />
            <app id="com.presonus.studioone" />
        </must-close>
    </pkg-ref>
    <choices-outline>
        <line choice="default">
            <line choice="com.hyperprism.reimagined.vst3"/>
        </line>
    </choices-outline>
    <choice id="default"/>
    <choice id="com.hyperprism.reimagined.vst3" visible="false">
        <pkg-ref id="com.hyperprism.reimagined.vst3"/>
    </choice>
    <pkg-ref id="com.hyperprism.reimagined.vst3" version="$VERSION" onConclusion="none">HyperPrism_VST3.pkg</pkg-ref>
</installer-gui-script>
EOF

# Build final installer
productbuild --distribution "$INSTALLER_DIR/distribution.xml" \
             --package-path "$INSTALLER_DIR" \
             --resources "$INSTALLER_DIR/resources" \
             "$OUTPUT_DIR/HyperPrism_Reimagined_v${VERSION}_Mac_Installer.pkg"

# Clean up
rm -rf "$INSTALLER_DIR/package_root"
rm -f "$INSTALLER_DIR/HyperPrism_VST3.pkg"
rm -f "$INSTALLER_DIR/distribution.xml"

echo "Installer created: $OUTPUT_DIR/HyperPrism_Reimagined_v${VERSION}_Mac_Installer.pkg"
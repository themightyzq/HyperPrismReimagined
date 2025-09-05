#!/bin/bash

# Complete Build + Sign + Notarize Pipeline
# Run this for production releases

set -e

echo "=== HyperPrism Complete Build & Notarization Pipeline ==="
echo ""

# Check credentials first
if [ -z "$APPLE_ID" ] || [ -z "$APPLE_PASSWORD" ] || [ -z "$TEAM_ID" ]; then
    echo "❌ Apple credentials not set. For development builds only."
    echo "Set APPLE_ID, APPLE_PASSWORD, TEAM_ID for notarized builds."
    NOTARIZE=false
else
    echo "✅ Apple credentials found - will notarize after build"
    NOTARIZE=true
fi

echo ""
echo "Step 1: Building all plugins..."
cd HyperPrismReimagined
./Scripts/build_mac_cmake.sh

echo ""
echo "Step 2: Copying fresh builds to Distribution..."
rm -rf Distribution/Mac/VST3/*.vst3
find build -name "*.vst3" -type d -path "*/Release/VST3/*" | sort | uniq | while read plugin; do 
    echo "Copying $(basename "$plugin")"
    cp -R "$plugin" Distribution/Mac/VST3/
done

echo ""
echo "Step 3: Signing with Developer ID..."
cd Distribution/Mac/VST3
for plugin in *.vst3; do
    if [ -d "$plugin" ]; then
        echo "Signing $plugin..."
        codesign --force --deep --sign "Developer ID Application: Zachary Quarles (AJU6TF97JM)" "$plugin"
    fi
done
cd ../../../..

if [ "$NOTARIZE" = true ]; then
    echo ""
    echo "Step 4: Notarizing for distribution..."
    ./notarize_plugins.sh
    
    echo ""
    echo "🎉 BUILD COMPLETE - READY FOR PUBLIC DISTRIBUTION"
    echo "Plugins are signed and notarized - no security warnings"
else
    echo ""
    echo "⚠️  BUILD COMPLETE - DEVELOPMENT ONLY"  
    echo "Plugins are signed but NOT notarized - will show security warnings"
    echo "Set Apple credentials and re-run for public distribution"
fi

echo ""
echo "Next steps:"
echo "1. Test plugins in DAW"
echo "2. If everything works, distribute the plugins in:"
echo "   HyperPrismReimagined/Distribution/Mac/VST3/"
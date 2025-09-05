#!/bin/bash

# Notarize HyperPrism AU (Audio Unit) Plugins
# Same process as VST3 but for .component bundles

set -e

echo "=== HyperPrism AU Plugin Notarization ==="
echo ""

# Check for required credentials
if [ -z "$APPLE_ID" ] || [ -z "$APPLE_PASSWORD" ] || [ -z "$TEAM_ID" ]; then
    echo "❌ Apple credentials not set"
    echo "Please set: APPLE_ID, APPLE_PASSWORD, TEAM_ID"
    exit 1
fi

AU_DIR="HyperPrismReimagined/Distribution/Mac/AU"
NOTARIZE_DIR="notarization_temp_au"

# Create temporary directory
rm -rf "$NOTARIZE_DIR"
mkdir -p "$NOTARIZE_DIR"

echo "Step 1: Signing AU plugins..."
echo ""

for plugin in "$AU_DIR"/*.component; do
    if [ -d "$plugin" ]; then
        plugin_name=$(basename "$plugin")
        echo "🔏 Signing $plugin_name..."
        codesign --force --deep --sign "Developer ID Application: Zachary Quarles (AJU6TF97JM)" "$plugin"
    fi
done

echo ""
echo "Step 2: Creating archives for notarization..."
echo ""

for plugin in "$AU_DIR"/*.component; do
    if [ -d "$plugin" ]; then
        plugin_name=$(basename "$plugin")
        echo "📦 Archiving $plugin_name..."
        zip_name=$(echo "$plugin_name" | sed 's/ /_/g' | sed 's/.component/.zip/')
        zip -r "$NOTARIZE_DIR/$zip_name" "$plugin"
    fi
done

echo ""
echo "Step 3: Submitting for notarization..."
echo "This may take 5-15 minutes per plugin..."
echo ""

cd "$NOTARIZE_DIR"
for zip_file in *.zip; do
    if [ -f "$zip_file" ]; then
        plugin_name=$(basename "$zip_file" .zip)
        echo "🔄 Notarizing $plugin_name..."
        
        xcrun notarytool submit "$zip_file" \
            --apple-id "$APPLE_ID" \
            --password "$APPLE_PASSWORD" \
            --team-id "$TEAM_ID" \
            --wait
        
        if [ $? -eq 0 ]; then
            echo "✅ $plugin_name notarized successfully"
        else
            echo "❌ $plugin_name notarization failed"
        fi
        echo ""
    fi
done
cd -

echo ""
echo "Step 4: Stapling notarization tickets..."
echo ""

for plugin in "$AU_DIR"/*.component; do
    if [ -d "$plugin" ]; then
        plugin_name=$(basename "$plugin")
        echo "📎 Stapling $plugin_name..."
        xcrun stapler staple "$plugin"
        
        if [ $? -eq 0 ]; then
            echo "✅ $plugin_name stapled successfully"
        else
            echo "❌ $plugin_name stapling failed"
        fi
    fi
done

# Clean up
rm -rf "$NOTARIZE_DIR"

echo ""
echo "=== AU Notarization Complete ==="
echo ""
echo "✅ All AU plugins are now notarized!"
echo ""
echo "Installation locations:"
echo "  User: ~/Library/Audio/Plug-Ins/Components/"
echo "  System: /Library/Audio/Plug-Ins/Components/"
echo ""
echo "These will work in Logic Pro, GarageBand, and other AU hosts."
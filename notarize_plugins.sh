#!/bin/bash

# Notarize HyperPrism VST3 Plugins for Distribution
# This eliminates the "Apple could not verify" security warning

set -e

echo "=== HyperPrism Plugin Notarization Process ==="
echo ""

# Check for required credentials
if [ -z "$APPLE_ID" ]; then
    echo "❌ APPLE_ID environment variable not set"
    echo "Please set: export APPLE_ID=\"your-apple-id@email.com\""
    exit 1
fi

if [ -z "$APPLE_PASSWORD" ]; then
    echo "❌ APPLE_PASSWORD environment variable not set" 
    echo "Please set: export APPLE_PASSWORD=\"your-app-specific-password\""
    echo "Generate app-specific password at: https://appleid.apple.com/account/manage"
    exit 1
fi

if [ -z "$TEAM_ID" ]; then
    echo "❌ TEAM_ID environment variable not set"
    echo "Please set: export TEAM_ID=\"TEAMID\""
    exit 1
fi

DIST_DIR="HyperPrismReimagined/Distribution/Mac/VST3"
NOTARIZE_DIR="notarization_temp"

# Create temporary directory
rm -rf "$NOTARIZE_DIR"
mkdir -p "$NOTARIZE_DIR"

echo "Step 1: Creating zip archives for notarization..."
echo ""

for plugin in "$DIST_DIR"/*.vst3; do
    if [ -d "$plugin" ]; then
        plugin_name=$(basename "$plugin")
        echo "📦 Archiving $plugin_name..."
        zip_name=$(echo "$plugin_name" | sed 's/ /_/g' | sed 's/.vst3/.zip/')
        zip -r "$NOTARIZE_DIR/$zip_name" "$plugin"
    fi
done

echo ""
echo "Step 2: Submitting for notarization..."
echo "This may take 5-15 minutes per plugin..."
echo ""

cd "$NOTARIZE_DIR"
for zip_file in *.zip; do
    if [ -f "$zip_file" ]; then
        plugin_name=$(basename "$zip_file" .zip)
        echo "🔄 Notarizing $plugin_name..."
        
        # Submit for notarization
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
echo "Step 3: Stapling notarization tickets..."
echo ""

for plugin in "$DIST_DIR"/*.vst3; do
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
echo "=== Notarization Complete ==="
echo ""
echo "✅ All plugins are now notarized and will not show security warnings"
echo ""
echo "To verify notarization:"
echo "spctl -a -t exec -vv \"path/to/plugin.vst3\""
echo ""
echo "Users can now install without security warnings!"
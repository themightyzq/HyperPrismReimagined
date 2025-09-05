#!/bin/bash

# HyperPrism Revived - Production Plugin Installer
# Double-clickable script for safe plugin installation

# Get the directory of this script
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Run the production installer
"$SCRIPT_DIR/copy_plugins_PRODUCTION.sh"

# Keep terminal open on macOS when double-clicked
if [ -t 0 ]; then
    echo ""
    echo "Press any key to close this window..."
    read -n 1 -s
fi
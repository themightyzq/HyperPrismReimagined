#!/bin/bash

# HyperPrism Revived - Complete Build and Install Script
# Double-clickable script that builds and installs all plugins

set -euo pipefail  # Exit on error, undefined vars, pipe failures

# Script metadata
readonly SCRIPT_NAME="HyperPrism Build & Install"
readonly SCRIPT_VERSION="1.0.0"

# Color codes for output
readonly RED='\033[0;31m'
readonly GREEN='\033[0;32m'
readonly YELLOW='\033[1;33m'
readonly BLUE='\033[0;34m'
readonly CYAN='\033[0;36m'
readonly NC='\033[0m' # No Color

# Logging functions
log_info() { echo -e "${BLUE}ℹ️  $1${NC}"; }
log_success() { echo -e "${GREEN}✅ $1${NC}"; }
log_warning() { echo -e "${YELLOW}⚠️  $1${NC}"; }
log_error() { echo -e "${RED}❌ $1${NC}" >&2; }
log_header() { echo -e "${CYAN}🎵 $1 🎵${NC}"; }

# Get script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"

# Main function
main() {
    clear
    log_header "$SCRIPT_NAME v$SCRIPT_VERSION"
    echo "============================================================="
    echo ""
    
    # Step 1: Build all plugins
    log_info "Step 1: Building all HyperPrism plugins..."
    echo ""
    
    # Check if build directory exists
    if [ ! -d "$BUILD_DIR" ]; then
        log_warning "Build directory not found. Creating and configuring..."
        mkdir -p "$BUILD_DIR"
        cd "$BUILD_DIR"
        
        log_info "Running CMake configuration..."
        if cmake .. -DCMAKE_BUILD_TYPE=Release; then
            log_success "CMake configuration completed"
        else
            log_error "CMake configuration failed"
            exit 1
        fi
    else
        cd "$BUILD_DIR"
    fi
    
    # Build the project
    log_info "Building plugins (this may take several minutes)..."
    echo ""
    
    if cmake --build . --config Release --parallel; then
        echo ""
        log_success "All plugins built successfully!"
    else
        echo ""
        log_error "Build failed"
        exit 1
    fi
    
    echo ""
    echo "============================================================="
    
    # Step 2: Install plugins
    log_info "Step 2: Installing plugins to system..."
    echo ""
    
    # Go back to script directory and run installer
    cd "$SCRIPT_DIR"
    
    if [ -f "copy_plugins_PRODUCTION.sh" ]; then
        # Run the production installer
        if "./copy_plugins_PRODUCTION.sh"; then
            echo ""
            echo "============================================================="
            log_success "BUILD AND INSTALL COMPLETED SUCCESSFULLY!"
            echo ""
            log_info "All HyperPrism plugins are now ready to use in your DAW"
        else
            log_error "Plugin installation failed"
            exit 1
        fi
    else
        log_error "Plugin installer script not found: copy_plugins_PRODUCTION.sh"
        exit 1
    fi
}

# Error handling
trap 'log_error "Script interrupted or failed"; exit 1' INT TERM

# Run main function
main "$@"

# Keep terminal open on macOS when double-clicked
if [ -t 0 ]; then
    echo ""
    echo "Press any key to close this window..."
    read -n 1 -s
fi
#!/bin/bash

# HyperPrism Revived - Production Plugin Copy Script
# This script safely copies built plugins to the system VST3 directory

set -euo pipefail  # Exit on error, undefined vars, pipe failures

# Script metadata
readonly SCRIPT_NAME="HyperPrism Plugin Installer"
readonly SCRIPT_VERSION="1.0.0"
readonly MIN_DISK_SPACE_MB=100

# Color codes for output
readonly RED='\033[0;31m'
readonly GREEN='\033[0;32m'
readonly YELLOW='\033[1;33m'
readonly BLUE='\033[0;34m'
readonly NC='\033[0m' # No Color

# Logging functions
log_info() { echo -e "${BLUE}ℹ️  $1${NC}"; }
log_success() { echo -e "${GREEN}✅ $1${NC}"; }
log_warning() { echo -e "${YELLOW}⚠️  $1${NC}"; }
log_error() { echo -e "${RED}❌ $1${NC}" >&2; }

# Platform detection
detect_platform() {
    case "$(uname -s)" in
        Darwin*)    echo "macos";;
        Linux*)     echo "linux";;
        CYGWIN*|MINGW*|MSYS*) echo "windows";;
        *)          echo "unknown";;
    esac
}

# Get VST3 directory based on platform
get_vst3_directory() {
    local platform="$1"
    case "$platform" in
        macos)   echo "/Users/$(whoami)/Library/Audio/Plug-Ins/VST3";;
        linux)   echo "$HOME/.vst3";;
        windows) echo "$APPDATA/VST3";;
        *)       return 1;;
    esac
}

# Check disk space
check_disk_space() {
    local target_dir="$1"
    local required_mb="$2"
    
    if command -v df >/dev/null 2>&1; then
        local available_kb
        available_kb=$(df "$target_dir" | awk 'NR==2 {print $4}')
        local available_mb=$((available_kb / 1024))
        
        if [ "$available_mb" -lt "$required_mb" ]; then
            log_error "Insufficient disk space. Required: ${required_mb}MB, Available: ${available_mb}MB"
            return 1
        fi
        log_info "Disk space check passed: ${available_mb}MB available"
    else
        log_warning "Cannot check disk space (df command not found)"
    fi
}

# Validate VST3 plugin
validate_vst3_plugin() {
    local plugin_path="$1"
    
    # Basic validation - check if it's a directory with .vst3 extension
    if [ ! -d "$plugin_path" ]; then
        return 1
    fi
    
    # Check for required VST3 structure (Contents directory)
    if [ ! -d "$plugin_path/Contents" ]; then
        return 1
    fi
    
    # Check for executable (macOS specific)
    if [ "$(detect_platform)" = "macos" ]; then
        if [ ! -d "$plugin_path/Contents/MacOS" ]; then
            return 1
        fi
    fi
    
    return 0
}

# Create backup of existing plugin
create_backup() {
    local plugin_path="$1"
    local backup_dir="$2"
    
    if [ -d "$plugin_path" ]; then
        local plugin_name
        plugin_name=$(basename "$plugin_path")
        local backup_path="$backup_dir/${plugin_name}.backup.$(date +%Y%m%d_%H%M%S)"
        
        log_info "Creating backup: $backup_path"
        if cp -R "$plugin_path" "$backup_path"; then
            echo "$backup_path"
            return 0
        else
            log_error "Failed to create backup"
            return 1
        fi
    fi
    return 0
}

# Restore from backup
restore_backup() {
    local backup_path="$1"
    local target_path="$2"
    
    if [ -d "$backup_path" ]; then
        log_warning "Restoring from backup: $backup_path"
        rm -rf "$target_path" 2>/dev/null || true
        if mv "$backup_path" "$target_path"; then
            log_success "Backup restored successfully"
        else
            log_error "Failed to restore backup"
        fi
    fi
}

# Main installation function
install_plugins() {
    local build_dir="$1"
    local vst3_dir="$2"
    local backup_dir="$3"
    
    local copied_count=0
    local failed_count=0
    local found_count=0
    local backup_paths=()
    
    log_info "Searching for built VST3 plugins..."
    
    # Process each plugin
    while IFS= read -r -d '' plugin_path; do
        ((found_count++))
        local plugin_name
        plugin_name=$(basename "$plugin_path")
        local target_path="$vst3_dir/$plugin_name"
        
        log_info "Processing: $plugin_name"
        
        # Validate source plugin
        if ! validate_vst3_plugin "$plugin_path"; then
            log_error "Invalid VST3 plugin: $plugin_name"
            ((failed_count++))
            continue
        fi
        
        # Create backup if target exists
        local backup_path=""
        if [ -d "$target_path" ]; then
            if backup_path=$(create_backup "$target_path" "$backup_dir"); then
                backup_paths+=("$backup_path")
            else
                log_error "Cannot create backup for $plugin_name, skipping"
                ((failed_count++))
                continue
            fi
        fi
        
        # Remove existing plugin
        if [ -d "$target_path" ]; then
            rm -rf "$target_path"
        fi
        
        # Copy plugin
        if cp -R "$plugin_path" "$vst3_dir/"; then
            # Verify copy was successful
            if validate_vst3_plugin "$target_path"; then
                log_success "$plugin_name installed successfully"
                ((copied_count++))
            else
                log_error "$plugin_name copy corrupted, restoring backup"
                restore_backup "$backup_path" "$target_path"
                ((failed_count++))
            fi
        else
            log_error "$plugin_name copy failed, restoring backup"
            restore_backup "$backup_path" "$target_path"
            ((failed_count++))
        fi
        
    done < <(find "$build_dir" -name "*.vst3" -type d -print0)
    
    # Summary
    echo ""
    echo "============================================================="
    log_info "Installation Summary:"
    echo "   Found: $found_count"
    echo "   Installed: $copied_count"
    echo "   Failed: $failed_count"
    echo "   Backups created: ${#backup_paths[@]}"
    
    if [ "$failed_count" -eq 0 ] && [ "$copied_count" -gt 0 ]; then
        log_success "All plugins installed successfully!"
        
        # Clean up old backups (keep only 5 most recent)
        if [ ${#backup_paths[@]} -gt 0 ]; then
            log_info "Cleaning up old backups..."
            find "$backup_dir" -name "*.backup.*" -type d | sort | head -n -5 | xargs rm -rf 2>/dev/null || true
        fi
        
        return 0
    else
        if [ "$failed_count" -gt 0 ]; then
            log_error "$failed_count plugins failed to install"
        fi
        if [ "$copied_count" -eq 0 ]; then
            log_error "No plugins were installed"
        fi
        return 1
    fi
}

# Main script
main() {
    echo "🎵 $SCRIPT_NAME v$SCRIPT_VERSION 🎵"
    echo "============================================================="
    
    # Get script directory
    local script_dir
    script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
    local build_dir="$script_dir/build"
    
    # Detect platform
    local platform
    platform=$(detect_platform)
    if [ "$platform" = "unknown" ]; then
        log_error "Unsupported platform: $(uname -s)"
        exit 1
    fi
    log_info "Platform detected: $platform"
    
    # Get VST3 directory
    local vst3_dir
    if ! vst3_dir=$(get_vst3_directory "$platform"); then
        log_error "Cannot determine VST3 directory for platform: $platform"
        exit 1
    fi
    
    # Validate directories
    if [ ! -d "$build_dir" ]; then
        log_error "Build directory not found: $build_dir"
        log_info "Please run the build script first"
        exit 1
    fi
    
    if [ ! -d "$vst3_dir" ]; then
        log_warning "VST3 directory does not exist: $vst3_dir"
        read -p "Create VST3 directory? (y/n): " -n 1 -r
        echo
        if [[ $REPLY =~ ^[Yy]$ ]]; then
            mkdir -p "$vst3_dir" || {
                log_error "Failed to create VST3 directory"
                exit 1
            }
            log_success "Created VST3 directory: $vst3_dir"
        else
            log_error "Cannot proceed without VST3 directory"
            exit 1
        fi
    fi
    
    # Create backup directory
    local backup_dir="$script_dir/plugin_backups"
    mkdir -p "$backup_dir"
    
    # Check disk space
    check_disk_space "$vst3_dir" "$MIN_DISK_SPACE_MB" || exit 1
    
    # Check permissions
    if [ ! -w "$vst3_dir" ]; then
        log_error "No write permission to VST3 directory: $vst3_dir"
        exit 1
    fi
    
    log_info "Source: $build_dir"
    log_info "Target: $vst3_dir"
    log_info "Backup: $backup_dir"
    echo ""
    
    # Install plugins
    if install_plugins "$build_dir" "$vst3_dir" "$backup_dir"; then
        echo ""
        log_success "Installation completed successfully!"
        
        # List installed plugins
        echo ""
        log_info "Installed HyperPrism Plugins:"
        find "$vst3_dir" -name "*HyperPrism*" -type d 2>/dev/null | sed 's|.*\/||' | sed 's/^/   ✅ /' || log_warning "No plugins found"
        
        # Offer to open directory
        echo ""
        read -p "Would you like to open the VST3 plugins folder? (y/n): " -n 1 -r
        echo
        if [[ $REPLY =~ ^[Yy]$ ]]; then
            case "$platform" in
                macos)   open "$vst3_dir";;
                linux)   xdg-open "$vst3_dir" 2>/dev/null || log_warning "Cannot open file manager";;
                windows) explorer "$vst3_dir" 2>/dev/null || log_warning "Cannot open file manager";;
            esac
        fi
        
        exit 0
    else
        log_error "Installation failed"
        exit 1
    fi
}

# Run main function
main "$@"
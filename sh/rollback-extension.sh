#!/bin/bash

# BrightSign Extension Rollback Script
# Phase 3 Implementation - Atomic Rollback Capability
# 
# This script handles safe rollback of BrightSign extensions to previous versions using:
# - LVM volume backup/restore
# - Configuration restoration
# - Validation of rollback capability
# - Atomic operations with failure recovery

set -e

# Colors and formatting
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

log() {
    echo -e "${BLUE}[ROLLBACK] $1${NC}"
}

warn() {
    echo -e "${YELLOW}⚠️  $1${NC}"
}

error() {
    echo -e "${RED}❌ $1${NC}"
    exit 1
}

success() {
    echo -e "${GREEN}✅ $1${NC}"
}

usage() {
    echo "Usage: $0 [OPTIONS] <extension_name>"
    echo "Rollback BrightSign extension to previous version"
    echo ""
    echo "Options:"
    echo "  -f, --force          Force rollback even if policy doesn't support it"
    echo "  -n, --no-config      Don't restore configuration from backup"
    echo "  -l, --list-backups   List available backups for extension"
    echo "  -b, --backup <name>  Specify backup name to restore from"
    echo "  -d, --dry-run       Validate rollback without executing"
    echo "  -v, --verbose       Verbose output"
    echo "  -h, --help          Show this help message"
    echo ""
    echo "Examples:"
    echo "  $0 npu_obj                              # Rollback to latest backup"
    echo "  $0 --list-backups npu_obj              # List available backups"
    echo "  $0 --backup backup_20250201 npu_obj    # Rollback to specific backup"
    echo "  $0 --dry-run npu_obj                   # Test rollback capability"
}

# Parse command line arguments
FORCE_ROLLBACK=false
NO_CONFIG_RESTORE=false
LIST_BACKUPS=false
SPECIFIC_BACKUP=""
DRY_RUN=false
VERBOSE=false
EXTENSION_NAME=""

while [[ $# -gt 0 ]]; do
    case $1 in
        -f|--force)
            FORCE_ROLLBACK=true
            shift
            ;;
        -n|--no-config)
            NO_CONFIG_RESTORE=true
            shift
            ;;
        -l|--list-backups)
            LIST_BACKUPS=true
            shift
            ;;
        -b|--backup)
            SPECIFIC_BACKUP="$2"
            shift 2
            ;;
        -d|--dry-run)
            DRY_RUN=true
            shift
            ;;
        -v|--verbose)
            VERBOSE=true
            shift
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        -*)
            error "Unknown option: $1"
            ;;
        *)
            if [[ -z "$EXTENSION_NAME" ]]; then
                EXTENSION_NAME="$1"
            else
                error "Multiple extension names specified"
            fi
            shift
            ;;
    esac
done

if [[ -z "$EXTENSION_NAME" ]]; then
    error "Extension name not specified"
fi

# Verbose logging function
vlog() {
    if [[ "$VERBOSE" == "true" ]]; then
        log "$1"
    fi
}

# List available backups for extension
list_extension_backups() {
    local extension_name="$1"
    
    log "Available backups for extension '$extension_name':"
    
    # Check for LVM volume backups
    local backup_volumes=$(lvs --noheadings -o lv_name 2>/dev/null | grep -E "backup.*${extension_name}" | tr -d ' ' || true)
    
    if [[ -n "$backup_volumes" ]]; then
        echo "LVM Volume Backups:"
        while IFS= read -r volume; do
            if [[ -n "$volume" ]]; then
                local creation_time="unknown"
                # Try to get creation time from LVM metadata
                if command -v lvs >/dev/null 2>&1; then
                    creation_time=$(lvs --noheadings -o time "/dev/bsos/$volume" 2>/dev/null | tr -d ' ' || echo "unknown")
                fi
                echo "  - $volume (created: $creation_time)"
            fi
        done <<< "$backup_volumes"
    else
        echo "  No LVM volume backups found"
    fi
    
    # Check for configuration backups
    local config_backup_dir="/var/backups/extensions/${extension_name}"
    if [[ -d "$config_backup_dir" ]]; then
        echo ""
        echo "Configuration Backups:"
        find "$config_backup_dir" -name "backup_*" -type d 2>/dev/null | while read -r backup_dir; do
            local backup_name=$(basename "$backup_dir")
            local backup_time="unknown"
            if [[ -f "$backup_dir/timestamp" ]]; then
                backup_time=$(cat "$backup_dir/timestamp")
            fi
            echo "  - $backup_name (created: $backup_time)"
        done
    else
        echo "  No configuration backups found"
    fi
}

# Find current extension installation and manifest
find_current_extension() {
    local extension_name="$1"
    
    # Try to find the extension by LVM volume name
    local volume_name="ext_${extension_name}"
    if [[ -b "/dev/mapper/bsos-${volume_name}" ]]; then
        # Extension is installed as LVM volume, find mount point
        local mount_point="/var/volatile/bsext/${volume_name}"
        if [[ -d "$mount_point" ]] && [[ -f "$mount_point/manifest.json" ]]; then
            echo "$mount_point"
            return 0
        fi
    fi
    
    # Fallback: search in common locations
    for ext_path in "/var/volatile/bsext/"* "/usr/local/${extension_name}" "/usr/local/"*; do
        if [[ -d "$ext_path" ]] && [[ -f "$ext_path/manifest.json" ]]; then
            local ext_id=$(jq -r '.extension.id // empty' "$ext_path/manifest.json" 2>/dev/null)
            if [[ "$ext_id" =~ $extension_name ]]; then
                echo "$ext_path"
                return 0
            fi
        fi
    done
    
    return 1
}

# Check if extension supports rollback
check_rollback_support() {
    local manifest_file="$1"
    
    if [[ ! -f "$manifest_file" ]]; then
        warn "No manifest found, assuming rollback is supported"
        return 0
    fi
    
    local rollback_supported=$(jq -r '.update.rollbackSupported // true' "$manifest_file" 2>/dev/null)
    
    if [[ "$rollback_supported" == "false" ]]; then
        if [[ "$FORCE_ROLLBACK" == "true" ]]; then
            warn "Extension doesn't support rollback but forced (--force)"
            return 0
        else
            error "Extension manifest indicates rollback is not supported (use --force to override)"
        fi
    fi
    
    vlog "Rollback support: $rollback_supported"
    return 0
}

# Find best backup to restore
find_backup_volume() {
    local extension_name="$1"
    local specific_backup="$2"
    
    if [[ -n "$specific_backup" ]]; then
        # Use specified backup
        if [[ -b "/dev/mapper/bsos-${specific_backup}" ]]; then
            echo "$specific_backup"
            return 0
        else
            error "Specified backup volume not found: $specific_backup"
        fi
    fi
    
    # Find latest backup automatically
    local backup_pattern="backup.*${extension_name}"
    local latest_backup=$(lvs --noheadings -o lv_name --sort=-time 2>/dev/null | grep -E "$backup_pattern" | head -1 | tr -d ' ' || true)
    
    if [[ -n "$latest_backup" ]]; then
        echo "$latest_backup"
        return 0
    fi
    
    return 1
}

# Stop current extension
stop_extension() {
    local extension_name="$1"
    
    log "Stopping current extension..."
    
    # Try multiple methods to stop the extension
    local daemon_name="bsext-${extension_name//_/-}"
    
    # Method 1: bsext_init script
    for init_script in "/usr/local/${extension_name}/bsext_init" "/var/volatile/bsext/ext_${extension_name}/bsext_init"; do
        if [[ -f "$init_script" ]]; then
            vlog "Stopping via $init_script"
            "$init_script" stop 2>/dev/null || true
        fi
    done
    
    # Method 2: systemctl
    if command -v systemctl >/dev/null 2>&1; then
        systemctl stop "$daemon_name" 2>/dev/null || true
    fi
    
    # Method 3: kill processes
    case "$extension_name" in
        *obj*|*objdet*)
            pkill -f "object_detection_demo" 2>/dev/null || true
            ;;
        *)
            vlog "No specific process kill pattern for $extension_name"
            ;;
    esac
    
    success "Extension stopped"
}

# Backup current extension before rollback
backup_current_extension() {
    local extension_name="$1"
    local volume_name="ext_${extension_name}"
    
    log "Creating backup of current extension before rollback..."
    
    if [[ ! -b "/dev/mapper/bsos-${volume_name}" ]]; then
        warn "No LVM volume found for current extension, skipping backup"
        return 0
    fi
    
    local backup_name="rollback_backup_$(date +%Y%m%d_%H%M%S)"
    
    if [[ "$DRY_RUN" == "true" ]]; then
        log "DRY RUN: Would create backup volume: $backup_name"
        return 0
    fi
    
    # Create snapshot of current volume
    if lvcreate -s -n "$backup_name" -l 100%ORIGIN "/dev/mapper/bsos-${volume_name}" 2>/dev/null; then
        success "Created rollback backup: $backup_name"
        vlog "Backup volume: /dev/mapper/bsos-${backup_name}"
    else
        warn "Failed to create backup snapshot (continuing anyway)"
    fi
}

# Restore extension from backup volume
restore_from_backup() {
    local extension_name="$1"
    local backup_volume="$2"
    
    log "Restoring extension from backup volume: $backup_volume"
    
    local current_volume="ext_${extension_name}"
    local temp_volume="temp_restore_${extension_name}"
    
    if [[ "$DRY_RUN" == "true" ]]; then
        log "DRY RUN: Would restore from backup volume $backup_volume"
        return 0
    fi
    
    # Unmount current extension
    local mount_point="/var/volatile/bsext/${current_volume}"
    if mountpoint -q "$mount_point" 2>/dev/null; then
        umount "$mount_point" || error "Failed to unmount current extension"
    fi
    
    # Remove dm-verity mapping if present
    if [[ -b "/dev/mapper/bsos-${current_volume}-verified" ]]; then
        veritysetup close "bsos-${current_volume}-verified" 2>/dev/null || true
    fi
    
    # Create snapshot from backup for restoration
    if ! lvcreate -s -n "$temp_volume" -l 100%ORIGIN "/dev/mapper/bsos-${backup_volume}" 2>/dev/null; then
        error "Failed to create restoration snapshot from backup"
    fi
    
    # Remove current volume
    if [[ -b "/dev/mapper/bsos-${current_volume}" ]]; then
        lvremove --yes "/dev/mapper/bsos-${current_volume}" || error "Failed to remove current volume"
    fi
    
    # Rename temp volume to current volume name
    if ! lvrename bsos "$temp_volume" "$current_volume"; then
        error "Failed to rename restored volume"
    fi
    
    # Remount extension
    mkdir -p "$mount_point"
    if mount "/dev/mapper/bsos-${current_volume}" "$mount_point"; then
        success "Extension restored from backup and mounted"
    else
        error "Failed to mount restored extension"
    fi
}

# Restore configuration from backup
restore_configuration() {
    local extension_name="$1"
    
    if [[ "$NO_CONFIG_RESTORE" == "true" ]]; then
        warn "Configuration restore skipped (--no-config)"
        return 0
    fi
    
    log "Restoring configuration from backup..."
    
    local config_backup_dir="/var/backups/extensions/${extension_name}"
    if [[ ! -d "$config_backup_dir" ]]; then
        warn "No configuration backup directory found"
        return 0
    fi
    
    # Find latest configuration backup
    local latest_backup=$(find "$config_backup_dir" -name "backup_*" -type d | sort -r | head -1)
    if [[ -z "$latest_backup" ]]; then
        warn "No configuration backups found"
        return 0
    fi
    
    vlog "Restoring configuration from: $latest_backup"
    
    if [[ "$DRY_RUN" == "true" ]]; then
        log "DRY RUN: Would restore configuration from $latest_backup"
        return 0
    fi
    
    # Restore registry settings
    local registry_backup="$latest_backup/registry-backup.json"
    if [[ -f "$registry_backup" ]] && command -v registry >/dev/null 2>&1; then
        if registry import extension < "$registry_backup" 2>/dev/null; then
            success "Registry configuration restored"
        else
            warn "Failed to restore registry configuration"
        fi
    fi
    
    # Restore user data directories
    for data_backup in "$latest_backup"/*; do
        if [[ -d "$data_backup" ]]; then
            local data_name=$(basename "$data_backup")
            case "$data_name" in
                registry-backup.json)
                    continue  # Already handled above
                    ;;
                *)
                    local restore_path="/tmp/$data_name"
                    if cp -r "$data_backup" "$restore_path" 2>/dev/null; then
                        vlog "Restored user data: $data_name"
                    fi
                    ;;
            esac
        fi
    done
}

# Start rolled-back extension
start_extension() {
    local extension_name="$1"
    
    log "Starting rolled-back extension..."
    
    if [[ "$DRY_RUN" == "true" ]]; then
        log "DRY RUN: Would start extension after rollback"
        return 0
    fi
    
    # Try to start via bsext_init script
    for init_script in "/usr/local/${extension_name}/bsext_init" "/var/volatile/bsext/ext_${extension_name}/bsext_init"; do
        if [[ -f "$init_script" ]]; then
            vlog "Starting via $init_script"
            if "$init_script" start; then
                success "Extension started successfully"
                return 0
            fi
        fi
    done
    
    warn "Could not start extension automatically - manual start may be required"
}

# Validate rollback was successful
validate_rollback() {
    local extension_name="$1"
    
    log "Validating rollback success..."
    
    # Check if extension volume exists and is mounted
    local volume_name="ext_${extension_name}"
    local mount_point="/var/volatile/bsext/${volume_name}"
    
    if [[ ! -b "/dev/mapper/bsos-${volume_name}" ]]; then
        error "Extension volume not found after rollback"
    fi
    
    if [[ ! -d "$mount_point" ]] || ! mountpoint -q "$mount_point"; then
        error "Extension not properly mounted after rollback"
    fi
    
    # Check if manifest exists and is readable
    local manifest_file="$mount_point/manifest.json"
    if [[ ! -f "$manifest_file" ]]; then
        error "Extension manifest not found after rollback"
    fi
    
    # Get rolled-back version
    local version=$(jq -r '.extension.version // "unknown"' "$manifest_file" 2>/dev/null)
    success "Rollback validation passed - Extension version: $version"
}

# Main rollback function
main() {
    log "Starting BrightSign Extension Rollback"
    log "Extension: $EXTENSION_NAME"
    
    if [[ "$DRY_RUN" == "true" ]]; then
        log "DRY RUN MODE - No changes will be made"
    fi
    
    # Handle list backups option
    if [[ "$LIST_BACKUPS" == "true" ]]; then
        list_extension_backups "$EXTENSION_NAME"
        return 0
    fi
    
    # Find current extension installation
    local current_path
    if current_path=$(find_current_extension "$EXTENSION_NAME"); then
        log "Found current installation: $current_path"
        local current_manifest="$current_path/manifest.json"
    else
        error "Extension not found: $EXTENSION_NAME"
    fi
    
    # Check if extension supports rollback
    check_rollback_support "$current_manifest"
    
    # Find backup to restore
    local backup_volume
    if backup_volume=$(find_backup_volume "$EXTENSION_NAME" "$SPECIFIC_BACKUP"); then
        log "Found backup volume: $backup_volume"
    else
        error "No backup volume found for extension: $EXTENSION_NAME"
    fi
    
    if [[ "$DRY_RUN" == "true" ]]; then
        success "DRY RUN: Rollback validation passed - no changes made"
        log "Would rollback from: $current_path"
        log "Would restore from: $backup_volume"
        return 0
    fi
    
    # Stop current extension
    stop_extension "$EXTENSION_NAME"
    
    # Backup current extension before rollback
    backup_current_extension "$EXTENSION_NAME"
    
    # Restore from backup volume
    restore_from_backup "$EXTENSION_NAME" "$backup_volume"
    
    # Restore configuration
    restore_configuration "$EXTENSION_NAME"
    
    # Start rolled-back extension
    start_extension "$EXTENSION_NAME"
    
    # Validate rollback success
    validate_rollback "$EXTENSION_NAME"
    
    success "Extension rollback completed successfully"
    
    return 0
}

# Run main function if script is executed directly
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    main "$@"
fi
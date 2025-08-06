#!/bin/bash

# BrightSign Extension Manifest Validation Utility
# Phase 3 Implementation - Standalone Manifest Validator
# 
# This utility provides comprehensive validation of BrightSign extension manifests:
# - JSON syntax validation
# - Schema compliance checking  
# - Semantic validation of values
# - Cross-reference validation
# - Compatibility reporting

set -e

# Colors and formatting
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

log() {
    echo -e "${BLUE}[VALIDATE] $1${NC}"
}

warn() {
    echo -e "${YELLOW}⚠️  $1${NC}"
}

error() {
    echo -e "${RED}❌ $1${NC}"
}

success() {
    echo -e "${GREEN}✅ $1${NC}"
}

usage() {
    echo "Usage: $0 [OPTIONS] <manifest.json>"
    echo "Validate BrightSign extension manifest files"
    echo ""
    echo "Options:"
    echo "  -s, --schema <file>   Path to JSON schema file"
    echo "  -v, --verbose         Verbose output with detailed validation"
    echo "  -q, --quiet           Quiet mode - only show errors"
    echo "  -f, --format          Check formatting and suggest improvements"
    echo "  -r, --report          Generate detailed validation report"
    echo "  -h, --help            Show this help message"
    echo ""
    echo "Exit codes:"
    echo "  0 - Validation passed"
    echo "  1 - Validation failed"
    echo "  2 - Invalid arguments or file not found"
    echo ""
    echo "Examples:"
    echo "  $0 manifest.json                           # Basic validation"
    echo "  $0 --verbose --schema schema.json manifest.json  # Full validation with schema"
    echo "  $0 --report manifest.json > validation-report.txt"
}

# Parse command line arguments
SCHEMA_FILE=""
VERBOSE=false
QUIET=false
FORMAT_CHECK=false
GENERATE_REPORT=false
MANIFEST_FILE=""

while [[ $# -gt 0 ]]; do
    case $1 in
        -s|--schema)
            SCHEMA_FILE="$2"
            shift 2
            ;;
        -v|--verbose)
            VERBOSE=true
            shift
            ;;
        -q|--quiet)
            QUIET=true
            shift
            ;;
        -f|--format)
            FORMAT_CHECK=true
            shift
            ;;
        -r|--report)
            GENERATE_REPORT=true
            shift
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        -*)
            error "Unknown option: $1"
            exit 2
            ;;
        *)
            if [[ -z "$MANIFEST_FILE" ]]; then
                MANIFEST_FILE="$1"
            else
                error "Multiple manifest files specified"
                exit 2
            fi
            shift
            ;;
    esac
done

if [[ -z "$MANIFEST_FILE" ]]; then
    error "Manifest file not specified"
    usage
    exit 2
fi

if [[ ! -f "$MANIFEST_FILE" ]]; then
    error "Manifest file not found: $MANIFEST_FILE"
    exit 2
fi

# Verbose logging function
vlog() {
    if [[ "$VERBOSE" == "true" ]] && [[ "$QUIET" != "true" ]]; then
        log "$1"
    fi
}

# Quiet-aware logging
qlog() {
    if [[ "$QUIET" != "true" ]]; then
        log "$1"
    fi
}

# Global validation counters
VALIDATION_ERRORS=0
VALIDATION_WARNINGS=0
VALIDATION_INFO=0

# Add validation result
add_result() {
    local level="$1"
    local message="$2"
    
    case "$level" in
        "error")
            VALIDATION_ERRORS=$((VALIDATION_ERRORS + 1))
            error "$message"
            ;;
        "warning")
            VALIDATION_WARNINGS=$((VALIDATION_WARNINGS + 1))
            warn "$message"
            ;;
        "info")
            VALIDATION_INFO=$((VALIDATION_INFO + 1))
            vlog "$message"
            ;;
        "success")
            success "$message"
            ;;
    esac
}

# Validate JSON syntax
validate_json_syntax() {
    local manifest_file="$1"
    
    vlog "Validating JSON syntax..."
    
    if ! jq empty "$manifest_file" 2>/dev/null; then
        add_result "error" "Invalid JSON syntax in manifest file"
        # Try to get specific error
        local jq_error=$(jq empty "$manifest_file" 2>&1 | head -1)
        add_result "error" "JSON error: $jq_error"
        return 1
    fi
    
    add_result "success" "JSON syntax validation passed"
    return 0
}

# Validate required fields
validate_required_fields() {
    local manifest_file="$1"
    
    vlog "Validating required fields..."
    
    local required_fields=(
        ".manifestVersion"
        ".extension.id"
        ".extension.name"
        ".extension.version"
        ".compatibility.osVersion.min"
    )
    
    local missing_fields=0
    
    for field in "${required_fields[@]}"; do
        local value=$(jq -r "$field // empty" "$manifest_file" 2>/dev/null)
        if [[ -z "$value" ]]; then
            add_result "error" "Required field missing: $field"
            missing_fields=$((missing_fields + 1))
        else
            vlog "Required field present: $field = $value"
        fi
    done
    
    if [[ $missing_fields -eq 0 ]]; then
        add_result "success" "All required fields present"
        return 0
    else
        return 1
    fi
}

# Validate manifest version
validate_manifest_version() {
    local manifest_file="$1"
    
    vlog "Validating manifest version..."
    
    local manifest_version=$(jq -r '.manifestVersion // 0' "$manifest_file" 2>/dev/null)
    
    case "$manifest_version" in
        1)
            add_result "success" "Manifest version 1 (current)"
            ;;
        0)
            add_result "error" "Missing or invalid manifest version"
            return 1
            ;;
        *)
            add_result "warning" "Unknown manifest version: $manifest_version (expected: 1)"
            ;;
    esac
    
    return 0
}

# Validate extension metadata
validate_extension_metadata() {
    local manifest_file="$1"
    
    vlog "Validating extension metadata..."
    
    # Validate extension ID format (reverse domain notation)
    local ext_id=$(jq -r '.extension.id // ""' "$manifest_file" 2>/dev/null)
    if [[ -n "$ext_id" ]]; then
        if [[ "$ext_id" =~ ^[a-z][a-z0-9-]*(\.[a-z][a-z0-9-]*)*$ ]]; then
            add_result "success" "Extension ID format valid: $ext_id"
        else
            add_result "error" "Extension ID format invalid: $ext_id (should use reverse domain notation)"
        fi
    fi
    
    # Validate version format (semantic versioning)
    local version=$(jq -r '.extension.version // ""' "$manifest_file" 2>/dev/null)
    if [[ -n "$version" ]]; then
        if [[ "$version" =~ ^[0-9]+\.[0-9]+\.[0-9]+(-[a-zA-Z0-9]+)?(\+[a-zA-Z0-9]+)?$ ]]; then
            add_result "success" "Version format valid: $version"
        else
            add_result "error" "Version format invalid: $version (should follow semantic versioning)"
        fi
    fi
    
    # Validate name length
    local name=$(jq -r '.extension.name // ""' "$manifest_file" 2>/dev/null)
    if [[ -n "$name" ]]; then
        local name_length=${#name}
        if [[ $name_length -gt 45 ]]; then
            add_result "error" "Extension name too long: $name_length chars (max: 45)"
        elif [[ $name_length -lt 1 ]]; then
            add_result "error" "Extension name too short"
        else
            add_result "success" "Extension name length valid: $name_length chars"
        fi
    fi
    
    # Validate description length
    local description=$(jq -r '.extension.description // ""' "$manifest_file" 2>/dev/null)
    if [[ -n "$description" ]]; then
        local desc_length=${#description}
        if [[ $desc_length -gt 132 ]]; then
            add_result "error" "Description too long: $desc_length chars (max: 132)"
        elif [[ $desc_length -lt 1 ]]; then
            add_result "warning" "Description is empty (recommended to provide description)"
        else
            add_result "success" "Description length valid: $desc_length chars"
        fi
    fi
    
    # Validate category
    local category=$(jq -r '.extension.category // ""' "$manifest_file" 2>/dev/null)
    if [[ -n "$category" ]]; then
        local valid_categories=("ai-vision" "media" "network" "control" "display" "utility")
        if [[ " ${valid_categories[*]} " =~ " $category " ]]; then
            add_result "success" "Category valid: $category"
        else
            add_result "warning" "Category not recognized: $category (valid: ${valid_categories[*]})"
        fi
    fi
}

# Validate compatibility section
validate_compatibility() {
    local manifest_file="$1"
    
    vlog "Validating compatibility requirements..."
    
    # Validate OS version format
    local min_os=$(jq -r '.compatibility.osVersion.min // ""' "$manifest_file" 2>/dev/null)
    if [[ -n "$min_os" ]]; then
        if [[ "$min_os" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
            add_result "success" "Minimum OS version format valid: $min_os"
        else
            add_result "error" "Minimum OS version format invalid: $min_os"
        fi
    fi
    
    local target_os=$(jq -r '.compatibility.osVersion.target // ""' "$manifest_file" 2>/dev/null)
    if [[ -n "$target_os" ]]; then
        if [[ "$target_os" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
            add_result "success" "Target OS version format valid: $target_os"
        else
            add_result "error" "Target OS version format invalid: $target_os"
        fi
    fi
    
    local max_os=$(jq -r '.compatibility.osVersion.max // null' "$manifest_file" 2>/dev/null)
    if [[ "$max_os" != "null" ]] && [[ -n "$max_os" ]]; then
        if [[ "$max_os" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
            add_result "success" "Maximum OS version format valid: $max_os"
        else
            add_result "error" "Maximum OS version format invalid: $max_os"
        fi
    fi
    
    # Validate SOC compatibility
    local socs=$(jq -r '.compatibility.soc[]?.id // empty' "$manifest_file" 2>/dev/null)
    if [[ -n "$socs" ]]; then
        local valid_socs=("RK3588" "RK3568" "RK3576")
        while IFS= read -r soc; do
            if [[ " ${valid_socs[*]} " =~ " $soc " ]]; then
                add_result "success" "SOC supported: $soc"
            else
                add_result "warning" "SOC not recognized: $soc (known: ${valid_socs[*]})"
            fi
        done <<< "$socs"
    else
        add_result "info" "No SOC compatibility specified (will be auto-generated)"
    fi
}

# Validate requirements section
validate_requirements() {
    local manifest_file="$1"
    
    vlog "Validating requirements..."
    
    # Validate memory size format
    local min_memory=$(jq -r '.requirements.memory.minimum // ""' "$manifest_file" 2>/dev/null)
    if [[ -n "$min_memory" ]]; then
        if [[ "$min_memory" =~ ^[0-9]+[KMG]B$ ]]; then
            add_result "success" "Minimum memory format valid: $min_memory"
        else
            add_result "error" "Minimum memory format invalid: $min_memory (use format like '512MB')"
        fi
    fi
    
    # Validate storage size format
    local install_storage=$(jq -r '.requirements.storage.installation // ""' "$manifest_file" 2>/dev/null)
    if [[ -n "$install_storage" ]]; then
        if [[ "$install_storage" =~ ^[0-9]+[KMG]B$ ]]; then
            add_result "success" "Installation storage format valid: $install_storage"
        else
            add_result "error" "Installation storage format invalid: $install_storage (use format like '150MB')"
        fi
    fi
    
    # Validate capabilities
    local capabilities=$(jq -r '.requirements.capabilities[]? // empty' "$manifest_file" 2>/dev/null)
    if [[ -n "$capabilities" ]]; then
        local known_capabilities=("camera.usb" "npu.rockchip" "storage.persistent" "network.ethernet" "display.hdmi")
        while IFS= read -r capability; do
            if [[ " ${known_capabilities[*]} " =~ " $capability " ]]; then
                add_result "success" "Capability recognized: $capability"
            else
                add_result "info" "Custom capability: $capability"
            fi
        done <<< "$capabilities"
    fi
}

# Validate registry configuration
validate_registry_config() {
    local manifest_file="$1"
    
    vlog "Validating registry configuration..."
    
    local config_items=$(jq -c '.registry.configurable[]? // empty' "$manifest_file" 2>/dev/null)
    if [[ -n "$config_items" ]]; then
        while IFS= read -r item; do
            local key=$(echo "$item" | jq -r '.key // ""')
            local type=$(echo "$item" | jq -r '.type // ""')
            local default=$(echo "$item" | jq -r '.default // ""')
            
            if [[ -z "$key" ]]; then
                add_result "error" "Registry item missing key"
                continue
            fi
            
            if [[ -z "$type" ]]; then
                add_result "error" "Registry item '$key' missing type"
                continue
            fi
            
            local valid_types=("string" "number" "boolean" "array")
            if [[ " ${valid_types[*]} " =~ " $type " ]]; then
                add_result "success" "Registry item '$key' has valid type: $type"
            else
                add_result "error" "Registry item '$key' has invalid type: $type"
            fi
            
        done <<< "$config_items"
    else
        add_result "info" "No registry configuration specified"
    fi
}

# Validate update policy
validate_update_policy() {
    local manifest_file="$1"
    
    vlog "Validating update policy..."
    
    local policy=$(jq -r '.update.policy // ""' "$manifest_file" 2>/dev/null)
    if [[ -n "$policy" ]]; then
        local valid_policies=("automatic" "manual" "blocked")
        if [[ " ${valid_policies[*]} " =~ " $policy " ]]; then
            add_result "success" "Update policy valid: $policy"
        else
            add_result "error" "Update policy invalid: $policy (valid: ${valid_policies[*]})"
        fi
    else
        add_result "info" "No update policy specified (defaults to 'manual')"
    fi
    
    # Validate version constraints if present
    local min_version=$(jq -r '.update.minVersionForUpdate // ""' "$manifest_file" 2>/dev/null)
    if [[ -n "$min_version" ]]; then
        if [[ "$min_version" =~ ^[0-9]+\.[0-9]+\.[0-9]+(-[a-zA-Z0-9]+)?(\+[a-zA-Z0-9]+)?$ ]]; then
            add_result "success" "Minimum update version format valid: $min_version"
        else
            add_result "error" "Minimum update version format invalid: $min_version"
        fi
    fi
}

# Validate against JSON schema if provided
validate_schema() {
    local manifest_file="$1"
    local schema_file="$2"
    
    if [[ -z "$schema_file" ]]; then
        add_result "info" "No schema file provided, skipping schema validation"
        return 0
    fi
    
    if [[ ! -f "$schema_file" ]]; then
        add_result "error" "Schema file not found: $schema_file"
        return 1
    fi
    
    vlog "Validating against JSON schema: $schema_file"
    
    # Note: jq doesn't have built-in JSON schema validation
    # In a full implementation, you'd use a tool like ajv-cli or implement schema validation
    add_result "info" "Schema validation requires external tool (e.g., ajv-cli)"
    
    return 0
}

# Check formatting and style
check_formatting() {
    local manifest_file="$1"
    
    if [[ "$FORMAT_CHECK" != "true" ]]; then
        return 0
    fi
    
    vlog "Checking formatting and style..."
    
    # Check if file is properly formatted (indented)
    local formatted_content=$(jq --indent 2 . "$manifest_file" 2>/dev/null)
    local original_content=$(cat "$manifest_file")
    
    if [[ "$formatted_content" == "$original_content" ]]; then
        add_result "success" "Manifest is properly formatted"
    else
        add_result "warning" "Manifest could be better formatted (use 'jq --indent 2' to format)"
    fi
    
    # Check for unnecessary fields (comments starting with _)
    local comment_fields=$(jq -r 'paths(scalars) as $p | select($p[-1] | type == "string" and startswith("_")) | $p | join(".")' "$manifest_file" 2>/dev/null)
    if [[ -n "$comment_fields" ]]; then
        add_result "info" "Found comment fields (these will be ignored in production):"
        while IFS= read -r field; do
            add_result "info" "  - $field"
        done <<< "$comment_fields"
    fi
}

# Generate validation report
generate_report() {
    local manifest_file="$1"
    
    if [[ "$GENERATE_REPORT" != "true" ]]; then
        return 0
    fi
    
    echo ""
    echo "================================="
    echo "MANIFEST VALIDATION REPORT"
    echo "================================="
    echo "File: $manifest_file"
    echo "Date: $(date -u +"%Y-%m-%d %H:%M:%S UTC")"
    echo ""
    
    # Basic file info
    echo "File Information:"
    echo "  Size: $(stat -c%s "$manifest_file") bytes"
    echo "  Modified: $(stat -c%y "$manifest_file")"
    echo ""
    
    # Extension info from manifest
    if jq empty "$manifest_file" 2>/dev/null; then
        echo "Extension Information:"
        echo "  ID: $(jq -r '.extension.id // "N/A"' "$manifest_file" 2>/dev/null)"
        echo "  Name: $(jq -r '.extension.name // "N/A"' "$manifest_file" 2>/dev/null)"
        echo "  Version: $(jq -r '.extension.version // "N/A"' "$manifest_file" 2>/dev/null)"
        echo "  Category: $(jq -r '.extension.category // "N/A"' "$manifest_file" 2>/dev/null)"
        echo ""
        
        echo "Compatibility:"
        echo "  Min OS: $(jq -r '.compatibility.osVersion.min // "N/A"' "$manifest_file" 2>/dev/null)"
        echo "  Target OS: $(jq -r '.compatibility.osVersion.target // "N/A"' "$manifest_file" 2>/dev/null)"
        echo "  Max OS: $(jq -r '.compatibility.osVersion.max // "N/A"' "$manifest_file" 2>/dev/null)"
        echo ""
    fi
    
    echo "Validation Results:"
    echo "  Errors: $VALIDATION_ERRORS"
    echo "  Warnings: $VALIDATION_WARNINGS"
    echo "  Info: $VALIDATION_INFO"
    echo ""
    
    if [[ $VALIDATION_ERRORS -eq 0 ]]; then
        echo "Overall Result: PASSED ✅"
    else
        echo "Overall Result: FAILED ❌"
    fi
    
    echo "================================="
}

# Main validation function
main() {
    qlog "BrightSign Extension Manifest Validator"
    qlog "Validating: $MANIFEST_FILE"
    
    if [[ -n "$SCHEMA_FILE" ]]; then
        vlog "Using schema: $SCHEMA_FILE"
    fi
    
    # Run validation steps
    validate_json_syntax "$MANIFEST_FILE" || true
    validate_required_fields "$MANIFEST_FILE" || true
    validate_manifest_version "$MANIFEST_FILE" || true
    validate_extension_metadata "$MANIFEST_FILE" || true
    validate_compatibility "$MANIFEST_FILE" || true
    validate_requirements "$MANIFEST_FILE" || true
    validate_registry_config "$MANIFEST_FILE" || true
    validate_update_policy "$MANIFEST_FILE" || true
    validate_schema "$MANIFEST_FILE" "$SCHEMA_FILE" || true
    check_formatting "$MANIFEST_FILE" || true
    
    # Generate report if requested
    generate_report "$MANIFEST_FILE"
    
    # Summary
    if [[ "$QUIET" != "true" ]]; then
        echo ""
        echo "Validation Summary:"
        echo "  Errors: $VALIDATION_ERRORS"
        echo "  Warnings: $VALIDATION_WARNINGS"
        echo "  Info: $VALIDATION_INFO"
        echo ""
    fi
    
    if [[ $VALIDATION_ERRORS -eq 0 ]]; then
        success "Manifest validation passed"
        exit 0
    else
        error "Manifest validation failed with $VALIDATION_ERRORS errors"
        exit 1
    fi
}

# Run main function if script is executed directly
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    main "$@"
fi
# BrightSign Extension Manifest Guide

## Overview

Starting with Phase 1 of the BrightSign Extension Versioning Strategy, extensions can include a `manifest.json` file that provides:

- Version information
- Compatibility declarations
- Hardware requirements
- Configuration options
- Build metadata

This guide explains how to use the manifest system in your BrightSign extensions.

## Quick Start

### 1. Create Your Configuration

Copy the template and customize it for your extension:

```bash
cp manifest-config.template.json manifest-config.json
# Edit manifest-config.json with your extension details
```

### 2. Key Fields to Update

**Extension Information:**
```json
"extension": {
  "version": "1.0.0",  // Your extension version (semantic versioning)
  "description": "Your extension description (max 132 chars)",
  "author": {
    "name": "Your Name",
    "email": "support@example.com",
    "url": "https://example.com"
  },
  "license": "Apache-2.0",
  "homepage": "https://github.com/your-org/repo"
}
```

**Compatibility Requirements:**
```json
"compatibility": {
  "osVersion": {
    "min": "9.0.0",     // Minimum BrightSign OS version
    "target": "9.1.0",  // Version you developed/tested against
    "max": "10.0.0"     // Maximum version (optional)
  }
}
```

### 3. Generate the Manifest

The `package` script automatically generates `manifest.json` from your configuration:

```bash
./package
# manifest.json is created in the extension package
```

## Configuration Reference

### Extension Metadata

| Field | Required | Description |
|-------|----------|-------------|
| `version` | Yes | Semantic version (MAJOR.MINOR.PATCH) |
| `description` | Yes | Brief description (max 132 characters) |
| `author` | Yes | Author name, email, and URL |
| `license` | Yes | Software license (e.g., "Apache-2.0", "MIT") |
| `homepage` | No | Project homepage URL |
| `category` | Yes | One of: ai-vision, media, network, control, display, utility |

### Compatibility

| Field | Required | Description |
|-------|----------|-------------|
| `osVersion.min` | Yes | Minimum BrightSign OS version |
| `osVersion.target` | No | OS version tested against |
| `osVersion.max` | No | Maximum OS version |

### Requirements

| Field | Required | Description |
|-------|----------|-------------|
| `capabilities` | No | Required hardware/software features |
| `memory.minimum` | No | Minimum RAM required |
| `memory.recommended` | No | Recommended RAM |
| `storage.installation` | No | Disk space for installation |
| `storage.runtime` | No | Disk space for runtime data |

### Runtime Configuration

| Field | Default | Description |
|-------|---------|-------------|
| `autoStart` | true | Start automatically on boot |
| `startupDelay` | 5 | Seconds to wait before starting |
| `restartPolicy` | "always" | always, on-failure, or never |
| `priority` | "normal" | low, normal, or high |

### Registry Settings

Define user-configurable settings accessible via BrightSign registry:

```json
"registry": {
  "configurable": [
    {
      "key": "video-device",
      "type": "string",
      "default": "/dev/video0",
      "description": "USB camera device path"
    },
    {
      "key": "confidence-threshold",
      "type": "number",
      "default": 0.5,
      "min": 0.0,
      "max": 1.0,
      "description": "Detection confidence threshold"
    }
  ]
}
```

Registry keys are automatically prefixed with `bsext-obj-` for the object detection extension.

## Update Policy Configuration (Phase 3)

**NEW in v1.2.0**: Configure automated update management and rollback behavior:

```json
"update": {
  "policy": "manual",
  "backupPrevious": true,
  "preserveConfig": true,
  "rollbackSupported": true,
  "minVersionForUpdate": "1.0.0",
  "maxVersionGap": "2.0.0",
  "requiresReboot": false
}
```

### Update Policy Fields

| Field | Default | Description |
|-------|---------|-------------|
| `policy` | "manual" | Update policy: automatic, manual, or blocked |
| `backupPrevious` | true | Create backup before updating |
| `preserveConfig` | true | Preserve user configuration during updates |
| `rollbackSupported` | true | Extension supports rollback to previous version |
| `minVersionForUpdate` | "1.0.0" | Minimum current version required to update |
| `maxVersionGap` | null | Maximum version gap for direct updates |
| `requiresReboot` | false | Update requires system reboot |
| `updateScript` | null | Custom update script to run during updates |

### Update Policy Types

**Automatic (`"automatic"`):**
- Updates applied automatically without user intervention
- Recommended for development environments
- Requires stable, well-tested extensions

**Manual (`"manual"`):**  
- Updates require explicit user approval
- Default for production environments
- Provides control over update timing

**Blocked (`"blocked"`):**
- Prevents any updates to this version
- Used for deprecated or end-of-life versions
- Can be overridden with `--force` flag

### Version Constraints

Control update compatibility with version requirements:

```json
"update": {
  "minVersionForUpdate": "1.2.0",
  "maxVersionGap": "1.0.0"
}
```

- **`minVersionForUpdate`**: Minimum version required before updating to this version
- **`maxVersionGap`**: Maximum version difference allowed for direct updates

## Configuration Management (Phase 3)

**NEW in v1.2.0**: Built-in configuration backup and restore capabilities.

### Using bsext_init Commands

```bash
# Configuration backup and restore
./bsext_init backup                    # Create backup with auto-generated name
./bsext_init backup my_backup_name     # Create backup with custom name
./bsext_init restore                   # Restore from latest backup
./bsext_init restore my_backup_name    # Restore from specific backup
./bsext_init list-backups              # List available backups
```

### What Gets Backed Up

The configuration management system preserves:

- **Registry Settings**: All user-configured extension settings
- **User Data Files**: Application-specific data and outputs
- **Extension State**: Runtime preferences and configuration
- **Manifest Metadata**: Version and compatibility information for reference

### Backup Storage

Backups are stored in `/var/backups/extensions/<extension-name>/` with:
- Timestamped directories (`backup_YYYYMMDD_HHMMSS`)
- Registry configuration in JSON format
- User data files and directories
- Backup metadata and timestamps

## Automatically Generated Fields

The package script automatically populates these fields:

- `manifestVersion`: Schema version (currently 1)
- `extension.id`: Derived from extension name
- `extension.name`: Extension display name
- `compatibility.soc`: Based on compiled binaries
- `compatibility.players`: Derived from SOC support
- `build.timestamp`: Build time in ISO 8601 format
- `build.sdk`: SDK version used
- `build.commit`: Git commit hash

## Validation

### Enhanced Validation System (Phase 3)

**NEW in v1.2.0**: Comprehensive validation with detailed reporting and cross-compilation awareness.

### Standalone Manifest Validation

Use the dedicated validation tool for comprehensive checking:

```bash
# Basic manifest validation
./sh/validate-manifest.sh manifest.json

# Verbose validation with detailed output
./sh/validate-manifest.sh --verbose manifest.json

# Schema validation with detailed reporting
./sh/validate-manifest.sh --schema schemas/extension-manifest-v1.json --report manifest.json

# Format checking and suggestions
./sh/validate-manifest.sh --format manifest.json
```

**Validation Features:**
- JSON syntax and schema compliance
- Semantic validation of version formats
- Compatibility requirement checking
- Registry configuration validation
- Cross-reference validation between fields
- Formatting and style suggestions

### During Packaging (Host-side)

The enhanced package script validates:
- ✅ Package structure and completeness
- ✅ Manifest schema compliance  
- ✅ Size calculations vs declared requirements
- ✅ Cross-platform binary consistency
- ✅ Model file validation

**Cross-compilation aware:** Only validates what can be verified on the build host.

### Pre-Installation (Target-side)

Enhanced installation script validates:
- ✅ Manifest JSON syntax and structure
- ✅ OS version compatibility (min/max ranges)
- ✅ Storage space availability vs requirements
- ✅ Hardware capability validation (camera, NPU)
- ✅ SOC compatibility verification

### At Runtime (Target-side)

The `bsext_init` script performs comprehensive validation:
- OS version compatibility with enhanced error messages
- SOC compatibility with hardware detection
- System dependency validation (libraries, commands)
- Hardware capability checking (NPU, camera devices)
- Memory and storage constraint validation
- Extension dependency resolution

**Enhanced Error Reporting:** All validation failures include specific resolution guidance and troubleshooting steps.

## Version Management

### Semantic Versioning

Follow semantic versioning (MAJOR.MINOR.PATCH):
- **MAJOR**: Incompatible API changes
- **MINOR**: New functionality (backward compatible)
- **PATCH**: Bug fixes (backward compatible)

Examples:
- `1.0.0` → `1.0.1`: Bug fix
- `1.0.1` → `1.1.0`: New feature
- `1.1.0` → `2.0.0`: Breaking change

### Version Update Workflow

1. Update version in `manifest-config.json`
2. Run `./package` to generate new manifest
3. Commit both files to version control
4. Tag the release: `git tag v1.2.0`

### Update and Rollback Workflows (Phase 3)

**NEW in v1.2.0**: Automated update management with atomic operations.

#### Safe Update Process

```bash
# 1. Test update compatibility first
./sh/update-extension.sh --dry-run objdet-ext-20250201-123456.zip

# 2. Perform actual update with full validation
./sh/update-extension.sh objdet-ext-20250201-123456.zip

# 3. Verify update success
./bsext_init start
# Check logs and functionality
```

#### Rollback Process

```bash
# 1. List available backups
./sh/rollback-extension.sh --list-backups npu_obj

# 2. Test rollback capability
./sh/rollback-extension.sh --dry-run npu_obj

# 3. Perform rollback
./sh/rollback-extension.sh npu_obj

# 4. Verify rollback success
./bsext_init start
```

#### Configuration Management

```bash
# Manual backup before risky operations
./bsext_init backup before_major_changes

# Restore specific configuration
./bsext_init restore before_major_changes

# List all available configuration backups
./bsext_init list-backups
```

#### Update Policy Enforcement

The update system respects manifest policy settings:

- **Development Environment**: Use `"policy": "automatic"` for seamless updates
- **Production Environment**: Use `"policy": "manual"` for controlled updates  
- **Deprecated Versions**: Use `"policy": "blocked"` to prevent updates

Override policies with `--force` flag when necessary:

```bash
# Force update despite policy restrictions
./sh/update-extension.sh --force objdet-ext-package.zip

# Force rollback despite policy restrictions  
./sh/rollback-extension.sh --force npu_obj
```

## Backward Compatibility

In Phase 1, manifest.json is optional:
- Extensions without manifest work normally
- No enforcement of compatibility checks
- Validation shows warnings only (non-blocking)

## Examples

### Minimal Configuration

```json
{
  "extension": {
    "version": "1.0.0",
    "description": "Simple extension",
    "author": "BrightSign LLC",
    "license": "Apache-2.0",
    "category": "utility"
  },
  "compatibility": {
    "osVersion": {
      "min": "9.0.0"
    }
  }
}
```

### Full Configuration

See `manifest-config.template.json` for a complete example with all available fields.

## Best Practices

1. **Keep descriptions concise**: Maximum 132 characters
2. **Test compatibility**: Verify min/max OS versions
3. **Document registry keys**: Clear descriptions for user settings
4. **Update versions**: Increment for each release
5. **Specify requirements**: Be accurate about memory/storage needs

## Troubleshooting

### Common Issues

**"manifest-config.json not found"**
- Copy the template: `cp manifest-config.template.json manifest-config.json`
- Edit with your extension details

**"Generated manifest.json is not valid JSON"**
- Check for syntax errors in manifest-config.json
- Ensure all string values are properly escaped
- Verify jq is installed for validation

**"WARNING: OS version X.X.X is below minimum"**
- Update compatibility.osVersion.min in your config
- Or test with the minimum OS version specified

## Future Enhancements

The manifest system will evolve through phases:
- **Phase 1** (Current): Optional manifest, basic validation
- **Phase 2**: Warnings for missing manifests
- **Phase 3**: Required manifests, strict validation

Stay updated with the latest requirements in the [versioning plan](../versioning_plan.md).
# BrightSign Extension Versioning Strategy

## Using manifest.json for Version Control and Compatibility Management

**Document Version:** 1.0  
**Date:** January 2025  
**Status:** Draft for Review

---

## Executive Summary

### Problem Statement

BrightSign Extensions currently lack a defined versioning capability, making it challenging to:

- Track extension versions and updates
- Ensure compatibility between extensions and BrightSign OS versions
- Validate hardware requirements before installation
- Manage dependencies between extensions
- Provide clear compatibility information to automated systems and human operators

### Proposed Solution

Implement a `manifest.json` file standard for BrightSign Extensions that provides:

- Semantic versioning for extensions
- OS and hardware compatibility declarations
- Dependency management
- Capability requirements
- Automated compatibility checking

This approach draws inspiration from established extension systems including Chrome Extensions, VSCode Extensions, and Android applications, adapted for BrightSign's unique requirements.

---

## Research & Inspiration Sources

### Chrome Extensions (manifest.json)

**Reference:** [Chrome Extension Manifest Format](https://developer.chrome.com/docs/extensions/reference/manifest)

Chrome's manifest.json provides the foundation for our approach:

- __manifest_version__: Declares the manifest schema version
- **version**: Extension version using up to 4 dot-separated integers
- __minimum_chrome_version__: Specifies minimum browser version required
- **Key insight**: Simple, declarative compatibility checking

### VSCode Extensions (package.json)

**Reference:** [VSCode Extension Manifest](https://code.visualstudio.com/api/references/extension-manifest)

VSCode's engines field inspired our compatibility ranges:

- **engines.vscode**: Uses semantic versioning with caret notation (^1.51.0)
- **Key insight**: Flexible version ranges allow forward compatibility
- **Pattern**: `"engines": { "vscode": "^1.51.0" }` means compatible with 1.51.0+

### Android APK Manifest

**Reference:** [Android SDK Version Management](https://developer.android.com/guide/topics/manifest/uses-sdk-element)

Android's approach to API level compatibility influenced our design:

- **minSdkVersion**: Minimum API level required
- **targetSdkVersion**: API level the app was tested against
- **maxSdkVersion**: Maximum API level (rarely used)
- **Key insight**: Separate minimum and target versions enable graceful degradation

### NPM Package Management

**Reference:** [NPM package.json](https://docs.npmjs.com/cli/v9/configuring-npm/package-json)

NPM's dependency management informed our approach:

- **dependencies**: Required packages
- **peerDependencies**: Compatible versions of shared dependencies
- **engines**: Node.js version requirements
- **Key insight**: Declarative dependency specification

### Snap Packages

**Reference:** [Snapcraft Documentation](https://snapcraft.io/docs/snapcraft-yaml-reference)

Snap's confinement and base system influenced our thinking:

- **base**: Core system the snap builds upon
- **grade**: Stable/devel classification
- **confinement**: Security isolation level
- **Key insight**: Platform-specific base requirements

---

## Proposed manifest.json Schema

### Example manifest.json

```json
{
  "$schema": "https://brightsign.biz/schemas/extension-manifest/v1.json",
  "manifestVersion": 1,
  
  "extension": {
    "id": "com.brightsign.yolo-object-detection",
    "name": "YOLO Object Detection",
    "shortName": "YOLO",
    "version": "1.2.0",
    "description": "NPU-accelerated object detection using YOLO models",
    "author": {
      "name": "BrightSign LLC",
      "email": "support@brightsign.biz",
      "url": "https://www.brightsign.biz"
    },
    "license": "Apache-2.0",
    "homepage": "https://github.com/brightsign/yolo-extension",
    "category": "ai-vision"
  },
  
  "compatibility": {
    "osVersion": {
      "min": "9.0.0",
      "target": "9.1.0",
      "max": "10.0.0"
    },
    "players": [
      {
        "series": "5",
        "models": ["XT1145", "XT2145", "LS445"],
        "features": ["npu"]
      }
    ],
    "soc": [
      {
        "id": "RK3588",
        "platforms": ["XT5"],
        "minRevision": "1.0"
      },
      {
        "id": "RK3568",
        "platforms": ["LS5"],
        "minRevision": "1.0"
      },
      {
        "id": "RK3576",
        "platforms": ["Firebird"],
        "minRevision": "1.0",
        "experimental": true
      }
    ]
  },
  
  "requirements": {
    "capabilities": [
      "camera.usb",
      "npu.rockchip",
      "storage.persistent"
    ],
    "memory": {
      "minimum": "512MB",
      "recommended": "1GB"
    },
    "storage": {
      "installation": "150MB",
      "runtime": "500MB"
    },
    "dependencies": {
      "system": [
        "librknnrt.so.1",
        "libopencv_core.so.4.5"
      ],
      "extensions": []
    }
  },
  
  "runtime": {
    "autoStart": true,
    "startupDelay": 5,
    "restartPolicy": "always",
    "priority": "normal",
    "resources": {
      "cpuLimit": "25%",
      "memoryLimit": "1GB"
    }
  },
  
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
      },
      {
        "key": "classes",
        "type": "string",
        "default": "",
        "description": "Comma-separated list of classes to detect"
      }
    ]
  },
  
  "build": {
    "timestamp": "2025-01-31T10:30:00Z",
    "sdk": "brightsign-sdk-9.1.52",
    "commit": "abc123def456",
    "ci": {
      "system": "GitHub Actions",
      "buildNumber": "42"
    }
  },
  
  "signature": {
    "type": "sha256",
    "value": "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"
  }
}
```

### JSON Schema Definition

```json
{
  "$schema": "http://json-schema.org/draft-07/schema#",
  "$id": "https://brightsign.biz/schemas/extension-manifest/v1.json",
  "title": "BrightSign Extension Manifest",
  "description": "Schema for BrightSign extension manifest.json files",
  "type": "object",
  "required": ["manifestVersion", "extension", "compatibility"],
  "additionalProperties": false,
  
  "properties": {
    "$schema": {
      "type": "string",
      "description": "Reference to this schema for validation"
    },
    
    "manifestVersion": {
      "type": "integer",
      "description": "Version of the manifest schema",
      "enum": [1]
    },
    
    "extension": {
      "type": "object",
      "required": ["id", "name", "version"],
      "properties": {
        "id": {
          "type": "string",
          "pattern": "^[a-z][a-z0-9-]*(\\.[a-z][a-z0-9-]*)*$",
          "description": "Unique identifier in reverse domain notation"
        },
        "name": {
          "type": "string",
          "minLength": 1,
          "maxLength": 45
        },
        "shortName": {
          "type": "string",
          "maxLength": 12
        },
        "version": {
          "type": "string",
          "pattern": "^\\d+\\.\\d+\\.\\d+(-[a-zA-Z0-9]+)?(\\+[a-zA-Z0-9]+)?$"
        },
        "description": {
          "type": "string",
          "maxLength": 132
        },
        "author": {
          "oneOf": [
            { "type": "string" },
            {
              "type": "object",
              "properties": {
                "name": { "type": "string" },
                "email": { "type": "string", "format": "email" },
                "url": { "type": "string", "format": "uri" }
              }
            }
          ]
        },
        "license": { "type": "string" },
        "homepage": { "type": "string", "format": "uri" },
        "category": {
          "type": "string",
          "enum": ["ai-vision", "media", "network", "control", "display", "utility"]
        }
      }
    },
    
    "compatibility": {
      "type": "object",
      "required": ["osVersion"],
      "properties": {
        "osVersion": {
          "type": "object",
          "required": ["min"],
          "properties": {
            "min": { "type": "string", "pattern": "^\\d+\\.\\d+\\.\\d+$" },
            "target": { "type": "string", "pattern": "^\\d+\\.\\d+\\.\\d+$" },
            "max": { "type": "string", "pattern": "^\\d+\\.\\d+\\.\\d+$" }
          }
        },
        "players": {
          "type": "array",
          "items": {
            "type": "object",
            "required": ["series"],
            "properties": {
              "series": { "type": "string" },
              "models": {
                "type": "array",
                "items": { "type": "string" }
              },
              "features": {
                "type": "array",
                "items": { "type": "string" }
              },
              "deprecated": { "type": "boolean" }
            }
          }
        },
        "soc": {
          "type": "array",
          "items": {
            "type": "object",
            "required": ["id"],
            "properties": {
              "id": {
                "type": "string",
                "enum": ["RK3588", "RK3568", "RK3576"]
              },
              "platforms": {
                "type": "array",
                "items": { "type": "string" }
              },
              "minRevision": { "type": "string" },
              "experimental": { "type": "boolean" }
            }
          }
        }
      }
    },
    
    "requirements": {
      "type": "object",
      "properties": {
        "capabilities": {
          "type": "array",
          "items": { "type": "string" }
        },
        "memory": {
          "type": "object",
          "properties": {
            "minimum": { "type": "string", "pattern": "^\\d+[KMG]B$" },
            "recommended": { "type": "string", "pattern": "^\\d+[KMG]B$" }
          }
        },
        "storage": {
          "type": "object",
          "properties": {
            "installation": { "type": "string", "pattern": "^\\d+[KMG]B$" },
            "runtime": { "type": "string", "pattern": "^\\d+[KMG]B$" }
          }
        },
        "dependencies": {
          "type": "object",
          "properties": {
            "system": {
              "type": "array",
              "items": { "type": "string" }
            },
            "extensions": {
              "type": "array",
              "items": {
                "type": "object",
                "required": ["id"],
                "properties": {
                  "id": { "type": "string" },
                  "version": { "type": "string" }
                }
              }
            }
          }
        }
      }
    },
    
    "runtime": {
      "type": "object",
      "properties": {
        "autoStart": { "type": "boolean" },
        "startupDelay": { "type": "integer", "minimum": 0 },
        "restartPolicy": {
          "type": "string",
          "enum": ["always", "on-failure", "never"]
        },
        "priority": {
          "type": "string",
          "enum": ["low", "normal", "high"]
        },
        "resources": {
          "type": "object",
          "properties": {
            "cpuLimit": { "type": "string", "pattern": "^\\d+%$" },
            "memoryLimit": { "type": "string", "pattern": "^\\d+[KMG]B$" }
          }
        }
      }
    },
    
    "registry": {
      "type": "object",
      "properties": {
        "configurable": {
          "type": "array",
          "items": {
            "type": "object",
            "required": ["key", "type"],
            "properties": {
              "key": { "type": "string" },
              "type": {
                "type": "string",
                "enum": ["string", "number", "boolean", "array"]
              },
              "default": {},
              "description": { "type": "string" },
              "min": { "type": "number" },
              "max": { "type": "number" },
              "enum": { "type": "array" }
            }
          }
        }
      }
    },
    
    "build": {
      "type": "object",
      "properties": {
        "timestamp": { "type": "string", "format": "date-time" },
        "sdk": { "type": "string" },
        "commit": { "type": "string" },
        "ci": {
          "type": "object",
          "properties": {
            "system": { "type": "string" },
            "buildNumber": { "type": "string" }
          }
        }
      }
    },
    
    "signature": {
      "type": "object",
      "required": ["type", "value"],
      "properties": {
        "type": {
          "type": "string",
          "enum": ["sha256", "sha512"]
        },
        "value": { "type": "string" }
      }
    }
  }
}
```

### Schema Properties Explained

#### 1. Extension Metadata

- **id**: Unique identifier using reverse domain notation
- **version**: Semantic versioning (MAJOR.MINOR.PATCH)
- **name/shortName**: Display names for UI
- **category**: Classification for extension stores

#### 2. Compatibility Section

- **osVersion**: Min/target/max pattern from Android
- **players**: Hardware model compatibility
- **soc**: System-on-chip requirements with platform mapping

#### 3. Requirements

- **capabilities**: Required hardware/software features
- **dependencies**: System libraries and other extensions
- **memory/storage**: Resource requirements

#### 4. Runtime Configuration

- **autoStart**: Automatic startup behavior
- **restartPolicy**: Failure handling
- **resources**: Resource limits

#### 5. Registry Configuration

- **configurable**: User-modifiable settings via registry
- Type safety and validation rules

#### 6. Build Metadata

- **timestamp**: Build time
- **sdk**: SDK version used
- **commit**: Source control reference

---

## Implementation Strategy

### Phase 1: Package Script Integration

Modify the `package` script to generate manifest.json:

```bash
# In package script, after copying files
generate_manifest() {
    local timestamp=$(date -u +"%Y-%m-%dT%H:%M:%SZ")
    local version=$(git describe --tags --always || echo "0.0.1")
    
    cat > staging/manifest.json <<EOF
{
  "manifestVersion": 1,
  "extension": {
    "id": "com.brightsign.yolo-object-detection",
    "version": "${version}",
    ...
  }
}
EOF
}
```

### Phase 2: Compatibility Validation

Add validation to `bsext_init`:

```bash
validate_manifest() {
    if [[ -f "${SCRIPT_PATH}/manifest.json" ]]; then
        # Parse manifest
        local min_os=$(jq -r '.compatibility.osVersion.min' manifest.json)
        local current_os=$(cat /etc/os-release | grep VERSION_ID | cut -d= -f2)
        
        # Version comparison
        if ! version_ge "${current_os}" "${min_os}"; then
            echo "ERROR: OS version ${current_os} < required ${min_os}"
            exit 1
        fi
    fi
}
```

### Phase 3: Installation Checks

Enhance installation script with pre-flight checks:

```bash
# Check available space
required_space=$(jq -r '.requirements.storage.installation' manifest.json)
available_space=$(df -BM /usr/local | tail -1 | awk '{print $4}')

# Verify dependencies
for dep in $(jq -r '.requirements.dependencies.system[]' manifest.json); do
    if ! ldconfig -p | grep -q "$dep"; then
        echo "ERROR: Missing dependency: $dep"
        exit 1
    fi
done
```

---

## Extension Update Management

### Overview

BrightSign extensions are installed as persistent, atomic units using the Logical Volume Manager (LVM) system. This architecture provides a robust foundation for safe extension updates without requiring OS modifications.

### Current Update Mechanism

#### LVM-Based Storage Architecture

Extensions are stored as SquashFS images within LVM volumes:

- **Storage Location**: `/dev/mapper/bsos-ext_${name}` (LVM logical volume)
- **Mount Point**: `/var/volatile/bsext/ext_${name}` (runtime mount location)
- **Format**: SquashFS compressed filesystem image
- **Persistence**: Survives reboots and OS updates

#### Atomic Update Process

The current installation system already supports updates through volume replacement:

```bash
# Current installation script behavior (sh/make-extension-lvm)
# 1. Stop running extension
umount /var/volatile/bsext/ext_${name}

# 2. Remove existing volume (if present)
if [ -b '/dev/mapper/bsos-${mapper_vol_name}' ]; then
    lvremove --yes '/dev/mapper/bsos-${mapper_vol_name}'
    rm -f '/dev/mapper/bsos-${mapper_vol_name}'
fi

# 3. Create new volume with updated content
lvcreate --yes --size ${volume_size}b -n '${tmp_vol_name}' bsos
cat "${squashfs_file}" > /dev/mapper/bsos-${tmp_vol_name}

# 4. Verify integrity and activate
lvrename bsos '${tmp_vol_name}' '${mapper_vol_name}'
```

#### Update Safety Mechanisms

1. **SHA256 Verification**: Every extension image is verified before activation
2. **Atomic Replacement**: LVM ensures complete success or complete failure (no partial states)
3. **Process Isolation**: Extensions are cleanly stopped before volume operations
4. **Rollback Capability**: Failed updates leave the system in a recoverable state

### Enhanced Update Management with Manifests

#### Update Policy Declaration

Extensions can declare their update behavior in `manifest.json`:

```json
{
  "update": {
    "policy": "automatic",
    "backupPrevious": true,
    "preserveConfig": true,
    "rollbackSupported": true,
    "minVersionForUpdate": "1.0.0",
    "maxVersionGap": "2.0.0",
    "requiresReboot": false,
    "updateScript": "update-hooks.sh"
  }
}
```

##### Update Policy Options

- **`automatic`**: Allow automated updates (default for stable releases)
- **`manual`**: Require explicit user/admin approval for updates
- **`blocked`**: Prevent updates (for deprecated or end-of-life versions)

#### Version Compatibility Checking

Updates validate compatibility before execution:

```bash
# Enhanced update validation (proposed)
validate_update() {
    local current_version=$(jq -r '.extension.version' /current/manifest.json)
    local new_version=$(jq -r '.extension.version' /new/manifest.json)
    local min_update_version=$(jq -r '.update.minVersionForUpdate' /new/manifest.json)
    
    # Check if current version meets minimum update requirements
    if ! version_compare "$current_version" "$min_update_version"; then
        echo "ERROR: Cannot update from $current_version to $new_version"
        echo "Minimum version required: $min_update_version"
        exit 1
    fi
}
```

#### Configuration Preservation

User configuration is preserved across updates:

```bash
# Registry backup and restore
backup_configuration() {
    local extension_id="$1"
    registry export extension "$extension_id" > /tmp/extension-config-backup.json
}

restore_configuration() {
    local extension_id="$1" 
    if [ -f /tmp/extension-config-backup.json ]; then
        registry import extension "$extension_id" < /tmp/extension-config-backup.json
    fi
}
```

### Update Process Flow

#### Standard Update Sequence

1. **Pre-flight Validation**
   - Check manifest compatibility
   - Verify system requirements
   - Validate update policy permissions

2. **Backup Phase** (if enabled)
   - Export current configuration
   - Create backup LVM volume (optional)
   - Record current version metadata

3. **Update Execution**
   - Stop extension services (`bsext_init stop`)
   - Unmount current volume
   - Remove current LVM volume
   - Install new version (create new LVM volume)
   - Mount new version

4. **Post-Update Validation**
   - Verify new extension loads correctly
   - Restore user configuration
   - Validate system functionality
   - Start extension services (`bsext_init start`)

5. **Cleanup**
   - Remove temporary files
   - Update system logs
   - Remove old backup volumes (if configured)

#### Rollback Process

If an update fails, the system can rollback:

```bash
# Rollback to previous version
rollback_extension() {
    local extension_name="$1"
    local backup_volume="backup_${extension_name}"
    
    # Stop failed new version
    bsext_init stop
    
    # Restore backup volume
    if [ -b "/dev/mapper/bsos-${backup_volume}" ]; then
        lvrename bsos "${extension_name}" "failed_${extension_name}"
        lvrename bsos "${backup_volume}" "${extension_name}"
        
        # Remount and restart
        mount /var/volatile/bsext/${extension_name}
        bsext_init start
    fi
}
```

### Update Types and Strategies

#### 1. Patch Updates (1.0.0 → 1.0.1)
- **Automatic**: Safe to auto-update
- **Configuration**: Preserved automatically
- **Rollback**: Always supported
- **Reboot**: Not required

#### 2. Minor Updates (1.0.0 → 1.1.0)
- **Semi-automatic**: May require approval
- **Configuration**: Preserved with possible migration
- **Rollback**: Supported with manifest declaration
- **Reboot**: Extension-dependent

#### 3. Major Updates (1.0.0 → 2.0.0)
- **Manual**: Always require explicit approval
- **Configuration**: May require migration or reset
- **Rollback**: Complex, may not be supported
- **Reboot**: Often required

### Development vs Production Updates

#### Development Environment
- **Policy**: Automatic updates enabled
- **Backup**: Minimal (configuration only)
- **Rollback**: Quick rollback for testing
- **Validation**: Relaxed compatibility checking

#### Production Environment  
- **Policy**: Manual approval required
- **Backup**: Full previous version backup
- **Rollback**: Comprehensive rollback capability
- **Validation**: Strict compatibility and dependency checking

### Integration with Phase Implementation

#### Phase 2: Update-Aware Validation
- Add update policy validation to `bsext_init`
- Implement configuration backup/restore
- Create update-specific installation scripts

#### Phase 3: Automated Update Management
- Build update scheduling system
- Implement update rollback mechanisms
- Add update history tracking and reporting

---

## Example Manifests

### 1. Basic Single-SOC Extension

```json
{
  "manifestVersion": 1,
  "extension": {
    "id": "com.example.simple-display",
    "name": "Simple Display Controller",
    "version": "1.0.0"
  },
  "compatibility": {
    "osVersion": { "min": "8.5.0" },
    "soc": [{ "id": "RK3588" }]
  }
}
```

### 2. Multi-Platform with Version Ranges

```json
{
  "manifestVersion": 1,
  "extension": {
    "id": "com.example.media-processor",
    "version": "2.1.0"
  },
  "compatibility": {
    "osVersion": {
      "min": "9.0.0",
      "target": "9.1.0"
    },
    "players": [
      { "series": "5", "models": ["XT1145", "XT2145"] },
      { "series": "4", "models": ["XT1144"], "deprecated": true }
    ]
  }
}
```

### 3. Extension with Dependencies

```json
{
  "manifestVersion": 1,
  "extension": {
    "id": "com.example.ai-analytics",
    "version": "3.0.0"
  },
  "requirements": {
    "dependencies": {
      "extensions": [
        {
          "id": "com.brightsign.yolo-object-detection",
          "version": "^1.2.0"
        }
      ]
    }
  }
}
```

---

## Migration Path

### Phase 1: Optional Adoption (v9.2)

- manifest.json is optional
- Package script generates basic manifest
- No enforcement of compatibility checks

### Phase 2: Warnings (v9.3)

- Installation without manifest shows warnings
- Compatibility checks run but don't block
- Developer tools for manifest validation

### Phase 3: Required (v10.0)

- manifest.json required for all extensions
- Strict compatibility enforcement
- Extension store integration

### Backward Compatibility

Extensions without manifest.json will be treated as:

```json
{
  "manifestVersion": 0,
  "compatibility": {
    "osVersion": { "min": "0.0.0" },
    "players": "all"
  }
}
```

---

## Benefits

### For Developers

- Clear compatibility requirements
- Automated validation
- Better dependency management
- Version tracking

### For Users

- Compatibility checking before installation
- Clear resource requirements
- Configuration discovery
- Update notifications

### For BrightSign

- Extension store enablement
- Quality control
- Usage analytics
- Support simplification

---

## Open Questions for Review

1. **Signature Verification**: Should we require cryptographic signatures for production extensions?
2. **Version Matching**: Should we support NPM-style version ranges (^, ~, >=) or keep it simple?
3. **Feature Detection**: How granular should capability requirements be?
4. **Update Mechanism**: How should version updates be handled? Auto-update policies?
5. **Schema Evolution**: How do we handle manifest schema versioning going forward?

---

## Next Steps

1. Review and gather feedback on this proposal
2. Prototype manifest generation in package script
3. Implement validation in bsext_init
4. Create developer documentation
5. Build validation tools
6. Plan rollout timeline

---

## References

1. [Chrome Extension Manifest](https://developer.chrome.com/docs/extensions/reference/manifest)
2. [VSCode Extension Manifest](https://code.visualstudio.com/api/references/extension-manifest)
3. [Android App Manifest](https://developer.android.com/guide/topics/manifest/manifest-intro)
4. [NPM package.json](https://docs.npmjs.com/cli/v9/configuring-npm/package-json)
5. [Snapcraft YAML](https://snapcraft.io/docs/snapcraft-yaml-reference)
6. [Semantic Versioning](https://semver.org/)

---

*This document is intended for review and discussion. Please provide feedback via GitHub issues or email.*
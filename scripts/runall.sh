#!/bin/bash

# BrightSign YOLO Object Detection - Complete Build Script
# Runs all steps from Quick Start guide in sequence

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
cd "${PROJECT_DIR}"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
MAGENTA='\033[0;35m'
BOLD='\033[1m'
NC='\033[0m' # No Color

# Default configuration
AUTO_MODE=false
SKIP_ARCH_CHECK=false
SKIP_SETUP=false
SKIP_MODELS=false
SKIP_SDK_BUILD=false
SKIP_SDK_INSTALL=false
SKIP_APPS=false
SKIP_PACKAGE=false
VERBOSE=false
CLEAN_MODE=false

# Track timing
START_TIME=$(date +%s)
STEP_START_TIME=0

# Set project root
PROJECT_ROOT="${PROJECT_DIR}"
export project_root="${PROJECT_ROOT}"

usage() {
    echo "Usage: $0 [OPTIONS]"
    echo "Run complete BrightSign YOLO Object Detection build pipeline"
    echo ""
    echo "This script automates all Quick Start steps:"
    echo "  1. Setup environment (5-10 minutes)"
    echo "  2. Compile models (3-5 minutes)"
    echo "  3. Build SDK (30-45 minutes)"
    echo "  4. Install SDK (1 minute)"
    echo "  5. Build applications (3-8 minutes)"
    echo "  6. Package extension (1 minute)"
    echo ""
    echo "Total time: 60-90 minutes (first run)"
    echo ""
    echo "Options:"
    echo "  -auto, --auto          Run all steps without prompting for confirmation"
    echo "  -c, --clean            Clean all build artifacts and temporary files"
    echo "  --skip-arch-check      Skip x86_64 architecture check (for testing)"
    echo "  --skip-setup           Skip setup step (if already done)"
    echo "  --skip-models          Skip model compilation (if already done)"
    echo "  --skip-sdk-build       Skip SDK build (if already done)"
    echo "  --skip-sdk-install     Skip SDK installation (if already done)"
    echo "  --skip-apps            Skip application build"
    echo "  --skip-package         Skip packaging step"
    echo "  --from-step N          Start from step N (1-6)"
    echo "  --to-step N            Stop after step N (1-6)"
    echo "  --verbose              Show detailed output"
    echo "  -h, --help             Show this help message"
    echo ""
    echo "Examples:"
    echo "  $0                     # Run all steps interactively"
    echo "  $0 -auto               # Run all steps automatically"
    echo "  $0 --clean             # Clean all build artifacts"
    echo "  $0 --skip-setup        # Skip setup if already done"
    echo "  $0 --from-step 5       # Start from building apps"
    echo "  $0 --to-step 4         # Stop after SDK install"
}

# Parse command line arguments
FROM_STEP=1
TO_STEP=6

while [[ "$#" -gt 0 ]]; do
    case $1 in
        -auto|--auto) AUTO_MODE=true; shift ;;
        -c|--clean) CLEAN_MODE=true; shift ;;
        --skip-arch-check) SKIP_ARCH_CHECK=true; shift ;;
        --skip-setup) SKIP_SETUP=true; shift ;;
        --skip-models) SKIP_MODELS=true; shift ;;
        --skip-sdk-build) SKIP_SDK_BUILD=true; shift ;;
        --skip-sdk-install) SKIP_SDK_INSTALL=true; shift ;;
        --skip-apps) SKIP_APPS=true; shift ;;
        --skip-package) SKIP_PACKAGE=true; shift ;;
        --from-step) FROM_STEP="$2"; shift 2 ;;
        --to-step) TO_STEP="$2"; shift 2 ;;
        --verbose) VERBOSE=true; shift ;;
        -h|--help)
            usage
            exit 0
            ;;
        *) 
            echo "Unknown option: $1"
            echo "Use -h for help"
            exit 1
            ;;
    esac
done

# Validate step numbers
if [[ ! "$FROM_STEP" =~ ^[1-6]$ ]]; then
    echo "Error: --from-step must be between 1 and 6"
    exit 1
fi

if [[ ! "$TO_STEP" =~ ^[1-6]$ ]]; then
    echo "Error: --to-step must be between 1 and 6"
    exit 1
fi

if [[ $FROM_STEP -gt $TO_STEP ]]; then
    echo "Error: --from-step ($FROM_STEP) cannot be greater than --to-step ($TO_STEP)"
    exit 1
fi

log() {
    echo -e "${BLUE}[$(date +'%T')] $1${NC}"
}

warn() {
    echo -e "${YELLOW}⚠️  $1${NC}"
}

error() {
    echo -e "${RED}❌ $1${NC}"
    echo -e "${RED}Build failed at step $CURRENT_STEP${NC}"
    exit 1
}

success() {
    echo -e "${GREEN}✅ $1${NC}"
}

# Function to prompt user for continuation
prompt_continue() {
    if [ "$AUTO_MODE" = true ]; then
        log "Auto mode: Continuing automatically..."
        return 0
    fi

    local message="$1"
    echo -e "\n${YELLOW}NEXT STEPS:${NC}"
    echo "$message"
    echo
    read -p "Do you want to continue? (y/N): " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        log "Exiting..."
        exit 0
    fi
}

# Function to check if command exists
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Function to check if docker is running
check_docker_running() {
    if ! docker info >/dev/null 2>&1; then
        error "Docker is not running. Please start Docker and try again."
    fi
}

# Function to clean build artifacts and temporary files
clean_build_artifacts() {
    echo -e "${BOLD}${CYAN}════════════════════════════════════════════════════════════════════${NC}"
    echo -e "${BOLD}${CYAN}  Cleaning Build Artifacts${NC}"
    echo -e "${BOLD}${CYAN}════════════════════════════════════════════════════════════════════${NC}"
    echo ""
    
    local items_to_clean=(
        "build_firebird/"
        "build_ls5/"
        "build_xt5/"
        "sdk/"
        "install/"
        "yolo-*.zip"
        "brightsign-x86_64-cobra-toolchain-*.sh"
        "bsoe-recipes/build/"
        "bsoe-recipes/downloads/"
        "bsoe-recipes/sstate-cache/"
    )
    
    local cleaned_count=0
    local total_size_mb=0
    
    # Calculate approximate total size before cleaning
    for item in "${items_to_clean[@]}"; do
        if ls $item >/dev/null 2>&1; then
            # Use du to get size in MB, fallback to simple count if du fails
            size_kb=$(du -sk $item 2>/dev/null | cut -f1 | head -n1)
            if [[ -n "$size_kb" && "$size_kb" =~ ^[0-9]+$ ]]; then
                size_mb=$((size_kb / 1024))
                total_size_mb=$((total_size_mb + size_mb))
            fi
        fi
    done
    
    if [[ $total_size_mb -gt 0 ]]; then
        log "Found approximately ${total_size_mb}MB of build artifacts"
    else
        log "Scanning for build artifacts..."
    fi
    echo ""
    
    for item in "${items_to_clean[@]}"; do
        if ls $item >/dev/null 2>&1; then
            log "Removing $item..."
            rm -rf $item
            cleaned_count=$((cleaned_count + 1))
        else
            if [[ "$VERBOSE" == true ]]; then
                log "Not found: $item (skipping)"
            fi
        fi
    done
    
    # Clean any RKNN model files that might have been generated
    if [[ -f "compile-models" && -x "compile-models" ]]; then
        log "Cleaning compiled RKNN models..."
        find . -name "*.rknn" -type f -delete 2>/dev/null || true
    fi
    
    # Clean Docker artifacts related to the project
    log "Cleaning Docker artifacts..."
    if command_exists docker; then
        # Remove dangling images and containers
        docker container prune -f >/dev/null 2>&1 || true
        docker image prune -f >/dev/null 2>&1 || true
        # Remove project-specific images if they exist
        docker rmi -f $(docker images -q --filter "reference=*yolo*" --filter "reference=*brightsign*") >/dev/null 2>&1 || true
    fi
    
    # Remove any temporary and log files
    log "Cleaning temporary files..."
    find . -name "*.tmp" -type f -delete 2>/dev/null || true
    find . -name "*.log" -type f -delete 2>/dev/null || true
    find . -name "core.*" -type f -delete 2>/dev/null || true
    find . -name "*.pyc" -type f -delete 2>/dev/null || true
    find . -name "__pycache__" -type d -exec rm -rf {} + 2>/dev/null || true
    
    # Clean CMake build artifacts
    find . -name "CMakeCache.txt" -type f -delete 2>/dev/null || true
    find . -name "CMakeFiles" -type d -exec rm -rf {} + 2>/dev/null || true
    find . -name "cmake_install.cmake" -type f -delete 2>/dev/null || true
    find . -name "Makefile" -type f -not -path "./bsoe-recipes/*" -delete 2>/dev/null || true
    
    echo ""
    if [[ $cleaned_count -gt 0 ]]; then
        success "Cleaned $cleaned_count build artifact directories"
        if [[ $total_size_mb -gt 0 ]]; then
            success "Freed approximately ${total_size_mb}MB of disk space"
        fi
    else
        log "No major build artifacts found to clean"
    fi
    
    echo ""
    success "Clean operation completed successfully"
    log "You can now run a fresh build with './scripts/runall.sh'"
}

step_header() {
    local step_num="$1"
    local step_name="$2"
    local estimated_time="$3"
    echo ""
    echo -e "${BOLD}${CYAN}════════════════════════════════════════════════════════════════════${NC}"
    echo -e "${BOLD}${CYAN}  Step $step_num: $step_name${NC}"
    echo -e "${BOLD}${CYAN}  Estimated time: $estimated_time${NC}"
    echo -e "${BOLD}${CYAN}════════════════════════════════════════════════════════════════════${NC}"
    STEP_START_TIME=$(date +%s)
}

step_footer() {
    local step_name="$1"
    local step_end_time=$(date +%s)
    local step_duration=$((step_end_time - STEP_START_TIME))
    local minutes=$((step_duration / 60))
    local seconds=$((step_duration % 60))
    
    success "$step_name completed in ${minutes}m ${seconds}s"
    echo ""
}

print_summary() {
    local end_time=$(date +%s)
    local total_duration=$((end_time - START_TIME))
    local hours=$((total_duration / 3600))
    local minutes=$(((total_duration % 3600) / 60))
    local seconds=$((total_duration % 60))
    
    echo ""
    echo -e "${BOLD}${GREEN}════════════════════════════════════════════════════════════════════${NC}"
    echo -e "${BOLD}${GREEN}  BUILD COMPLETE!${NC}"
    echo -e "${BOLD}${GREEN}════════════════════════════════════════════════════════════════════${NC}"
    echo ""
    
    if [[ $hours -gt 0 ]]; then
        echo -e "${GREEN}Total build time: ${hours}h ${minutes}m ${seconds}s${NC}"
    else
        echo -e "${GREEN}Total build time: ${minutes}m ${seconds}s${NC}"
    fi
    
    echo ""
    echo "Extension packages created:"
    ls -lh yolo-*.zip 2>/dev/null | while read line; do
        echo "  $line"
    done
    
    echo ""
    echo "Next steps:"
    echo "1. Transfer package to BrightSign player"
    echo "2. Install: bash ./ext_npu_yolo_install-lvm.sh && reboot"
    echo "3. Extension will auto-start with USB camera"
}

check_prerequisites() {
    log "Checking prerequisites..."
    
    # Debug terminal environment
    if [[ "$VERBOSE" == true ]]; then
        log "Terminal environment debug:"
        log "  TTY available: $([ -t 0 ] && echo 'yes' || echo 'no')"
        log "  TERM: ${TERM:-'not set'}"
        log "  Running from: ${0}"
    fi
    
    # Check architecture
    if [ "$(uname -m)" != "x86_64" ] && [ "$SKIP_ARCH_CHECK" != true ]; then
        error "This script requires x86_64 architecture. Current: $(uname -m). Use --skip-arch-check to bypass this check for testing."
    elif [ "$SKIP_ARCH_CHECK" = true ]; then
        warn "Skipping architecture check - this is for testing only"
    fi
    
    # Check Docker
    if ! command_exists docker; then
        error "Docker is not installed. Please install Docker first: https://docs.docker.com/engine/install/"
    fi
    
    check_docker_running
    
    # Check other required tools
    local required_tools=("git" "cmake" "wget")
    for tool in "${required_tools[@]}"; do
        if ! command_exists "$tool"; then
            error "$tool is not installed. Please install $tool first."
        fi
    done
    log "All required tools are installed"
    
    # Check required build scripts exist
    local required_scripts=("setup" "compile-models" "build" "build-apps" "package")
    for script in "${required_scripts[@]}"; do
        if [[ ! -f "./$script" ]]; then
            error "Required script '$script' not found. Please ensure you're in the project root directory."
        fi
        if [[ ! -x "./$script" ]]; then
            warn "Script '$script' is not executable. Fixing..."
            chmod +x "./$script"
        fi
    done
    
    # Check disk space
    available_space=$(df . | awk 'NR==2 {print $4}')
    required_space=$((25 * 1024 * 1024)) # 25GB in KB
    
    if [[ $available_space -lt $required_space ]]; then
        warn "Less than 25GB free space available. Build may fail."
        warn "Available: $(($available_space / 1024 / 1024))GB, Required: 25GB+"
        if [[ "$AUTO_MODE" != true ]]; then
            read -p "Continue anyway? [y/N] " -n 1 -r
            echo
            if [[ ! $REPLY =~ ^[Yy]$ ]]; then
                exit 1
            fi
        fi
    fi
    
    success "Prerequisites check passed"
}

# Main execution
main() {
    echo -e "${BOLD}${MAGENTA}╔══════════════════════════════════════════════════════════════════╗${NC}"
    echo -e "${BOLD}${MAGENTA}║     BrightSign YOLO Object Detection - Complete Build Pipeline    ║${NC}"
    echo -e "${BOLD}${MAGENTA}╚══════════════════════════════════════════════════════════════════╝${NC}"
    echo ""
    
    # Handle clean mode
    if [ "$CLEAN_MODE" = true ]; then
        clean_build_artifacts
        exit 0
    fi
    
    if [ "$AUTO_MODE" = true ]; then
        log "Running in automatic mode - no prompts"
    else
        log "Running in interactive mode - will prompt between steps"
    fi
    
    log "Project root: $PROJECT_ROOT"
    echo ""
    echo "This will run all build steps from the Quick Start guide."
    echo "Estimated total time: 60-90 minutes (first run)"
    echo ""
    
    if [[ "$AUTO_MODE" != true ]]; then
        echo "Steps to be executed (${FROM_STEP} to ${TO_STEP}):"
        [[ $FROM_STEP -le 1 && $TO_STEP -ge 1 && "$SKIP_SETUP" != true ]] && echo "  1. Setup environment"
        [[ $FROM_STEP -le 2 && $TO_STEP -ge 2 && "$SKIP_MODELS" != true ]] && echo "  2. Compile models"
        [[ $FROM_STEP -le 3 && $TO_STEP -ge 3 && "$SKIP_SDK_BUILD" != true ]] && echo "  3. Build SDK"
        [[ $FROM_STEP -le 4 && $TO_STEP -ge 4 && "$SKIP_SDK_INSTALL" != true ]] && echo "  4. Install SDK"
        [[ $FROM_STEP -le 5 && $TO_STEP -ge 5 && "$SKIP_APPS" != true ]] && echo "  5. Build applications"
        [[ $FROM_STEP -le 6 && $TO_STEP -ge 6 && "$SKIP_PACKAGE" != true ]] && echo "  6. Package extension"
        echo ""
        
        read -p "Continue? [y/N] " -n 1 -r
        echo
        if [[ ! $REPLY =~ ^[Yy]$ ]]; then
            echo "Build cancelled."
            exit 0
        fi
    fi
    
    check_prerequisites
    
    CURRENT_STEP=0
    
    # Step 1: Setup environment
    if [[ $FROM_STEP -le 1 && $TO_STEP -ge 1 ]]; then
        CURRENT_STEP=1
        if [[ "$SKIP_SETUP" == true ]]; then
            log "Skipping setup (--skip-setup flag)"
        else
            step_header "1/6" "Setup Environment" "5-10 minutes"
            
            if [[ "$AUTO_MODE" == true ]]; then
                ./setup -y || error "Setup failed"
            else
                ./setup || error "Setup failed"
            fi
            
            step_footer "Setup"
        fi
    fi
    
    # Step 2: Compile models
    if [[ $FROM_STEP -le 2 && $TO_STEP -ge 2 ]]; then
        CURRENT_STEP=2
        if [[ "$SKIP_MODELS" == true ]]; then
            log "Skipping model compilation (--skip-models flag)"
        else
            step_header "2/6" "Compile ONNX Models to RKNN" "3-5 minutes"
            
            if [[ "$VERBOSE" == true ]]; then
                log "Running compile-models in verbose mode..."
                ./compile-models || error "Model compilation failed"
            else
                log "Running compile-models in quiet mode..."
                if ! ./compile-models --quiet; then
                    error "Model compilation failed. Try running with --verbose for more details."
                fi
            fi
            
            step_footer "Model compilation"
        fi
    fi
    
    # Step 3: Build SDK
    if [[ $FROM_STEP -le 3 && $TO_STEP -ge 3 ]]; then
        CURRENT_STEP=3
        if [[ "$SKIP_SDK_BUILD" == true ]]; then
            log "Skipping SDK build (--skip-sdk-build flag)"
        else
            step_header "3/6" "Build OpenEmbedded SDK" "30-45 minutes"
            
            log "This is the longest step. Building BrightSign OS SDK..."
            log "The build will download ~20GB and compile the SDK"
            
            if [[ "$VERBOSE" == true ]]; then
                ./build --extract-sdk || error "SDK build failed"
            else
                if ! ./build --extract-sdk > /dev/null 2>&1; then
                    error "SDK build failed"
                fi
            fi
            
            step_footer "SDK build"
        fi
    fi
    
    # Step 4: Install SDK
    if [[ $FROM_STEP -le 4 && $TO_STEP -ge 4 ]]; then
        CURRENT_STEP=4
        if [[ "$SKIP_SDK_INSTALL" == true ]]; then
            log "Skipping SDK installation (--skip-sdk-install flag)"
        else
            step_header "4/6" "Install SDK" "1 minute"
            
            # Find the SDK installer
            SDK_INSTALLER=$(ls brightsign-x86_64-cobra-toolchain-*.sh 2>/dev/null | head -n 1)
            
            if [[ -z "$SDK_INSTALLER" ]]; then
                error "SDK installer not found. Please run build step first."
            fi
            
            log "Installing SDK from $SDK_INSTALLER..."
            ./"$SDK_INSTALLER" -d ./sdk -y || error "SDK installation failed"
            
            step_footer "SDK installation"
        fi
    fi
    
    # Step 5: Build applications
    if [[ $FROM_STEP -le 5 && $TO_STEP -ge 5 ]]; then
        CURRENT_STEP=5
        if [[ "$SKIP_APPS" == true ]]; then
            log "Skipping application build (--skip-apps flag)"
        else
            step_header "5/6" "Build C++ Applications" "3-8 minutes"
            
            log "Building for all platforms (XT5, LS5, Firebird)..."
            
            if [[ "$VERBOSE" == true ]]; then
                ./build-apps || error "Application build failed"
            else
                if ! ./build-apps > /dev/null 2>&1; then
                    error "Application build failed"
                fi
            fi
            
            step_footer "Application build"
        fi
    fi
    
    # Step 6: Package extension
    if [[ $FROM_STEP -le 6 && $TO_STEP -ge 6 ]]; then
        CURRENT_STEP=6
        if [[ "$SKIP_PACKAGE" == true ]]; then
            log "Skipping packaging (--skip-package flag)"
        else
            step_header "6/6" "Package Extension" "1 minute"
            
            ./package || error "Packaging failed"
            
            step_footer "Packaging"
        fi
    fi
    
    print_summary
}

# Handle interrupts gracefully
trap 'echo -e "\n${RED}Build interrupted by user${NC}"; exit 130' INT TERM

# Run main function
main "$@"
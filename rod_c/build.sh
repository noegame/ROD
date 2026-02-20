#!/bin/bash
# CMake build script for rod_c project
# Automatically configures and builds the project
#
# Usage: ./build.sh [--run] [--camera real|emulated] [image_folder]
#   --run              Run rod_detection after building
#   --camera TYPE      Camera type: real or emulated (default: emulated when using --run)
#   image_folder       Path to image folder for emulated camera

set -e

# Parse arguments
RUN_AFTER_BUILD=false
CAMERA_TYPE="emulated"
IMAGE_FOLDER=""

while [[ $# -gt 0 ]]; do
    case $1 in
        --run)
            RUN_AFTER_BUILD=true
            shift
            ;;
        --camera)
            CAMERA_TYPE="$2"
            shift 2
            ;;
        *)
            IMAGE_FOLDER="$1"
            shift
            ;;
    esac
done

echo "=== Building rod_c with CMake ==="
echo ""

# Create build directory
mkdir -p build
cd build

# Configure with CMake
echo "[1/2] Configuring project with CMake..."
if ! cmake .. ; then
    echo ""
    echo "CMake configuration failed!"
    exit 1
fi

# Build the project
echo "[2/2] Building all targets..."
make -j$(nproc)

echo ""
echo "✓ Build successful!"
echo ""

# Run if requested
if [ "$RUN_AFTER_BUILD" = true ]; then
    echo "=== Running rod_detection ==="
    CMD="./rod_detection --camera $CAMERA_TYPE"
    if [ -n "$IMAGE_FOLDER" ]; then
        CMD="$CMD $IMAGE_FOLDER"
    fi
    echo "Command: $CMD"
    echo ""
    $CMD
fi


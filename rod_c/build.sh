#!/bin/bash
# CMake build script for rod_c project
# Automatically configures and builds the project

set -e

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


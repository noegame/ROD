#!/bin/bash
# Diagnostic script to show CMake configuration errors

echo "=== CMake Diagnostic ==="
echo ""

cd "$(dirname "$0")"

# Create build directory
mkdir -p build
cd build

echo "Running CMake configuration (showing all output)..."
echo "---------------------------------------------------"
cmake ..
CMAKE_EXIT_CODE=$?
echo "---------------------------------------------------"
echo ""

if [ $CMAKE_EXIT_CODE -ne 0 ]; then
    echo "❌ CMake configuration failed with exit code $CMAKE_EXIT_CODE"
    echo ""
    echo "Common issues on Raspberry Pi:"
    echo "  - libcamera-dev not installed: sudo apt install libcamera-dev"
    echo "  - OpenCV not found: check OpenCV installation"
    echo "  - Missing pkg-config: sudo apt install pkg-config"
    exit 1
else
    echo "✅ CMake configuration successful!"
    echo ""
    echo "Checking for key components:"
    echo ""
    
    # Check what was found
    if grep -q "libcamera: Found" CMakeCache.txt 2>/dev/null; then
        echo "  ✅ libcamera detected"
    else
        echo "  ⚠️  libcamera NOT detected (will use emulated camera only)"
    fi
    
    if grep -q "OpenCV_VERSION" CMakeCache.txt 2>/dev/null; then
        OPENCV_VER=$(grep "OpenCV_VERSION:" CMakeCache.txt | cut -d= -f2)
        echo "  ✅ OpenCV $OPENCV_VER detected"
    fi
    
    echo ""
    echo "To build, run: make -j\$(nproc)"
fi

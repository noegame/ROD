#!/bin/bash
# Test script to verify debug image generation

cd /home/noegame/ROD/rod_c

# Clean debug folder
rm -f ../pictures/debug/*

# Run rod_detection for a few seconds
timeout -s INT 3 ./build/rod_detection ../pictures/2026-01-16-playground-ready || true

# Wait a moment for files to finish writing
sleep 1

# Show generated files
echo "=== Generated debug images ==="
ls -lh ../pictures/debug/ 2>&1

# Show filenames with wildcards
echo ""
echo "=== PNG files (timestamp format) ==="
ls ../pictures/debug/*.png 2>/dev/null | head -5 || echo "No PNG files found"

echo ""
echo "=== Sample filenames ==="
ls ../pictures/debug/ | head -3

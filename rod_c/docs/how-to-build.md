# CMake Build System for rod_c

This document describes the CMake build system structure for the rod_c project.

## Project Structure

```
rod_c/
├── CMakeLists.txt               # Main CMake configuration
├── build.sh                     # Build script
├── rod_cv/
│   ├── CMakeLists.txt           # OpenCV wrapper library
│   ├── ...
├── rod_camera/
│   ├── CMakeLists.txt           # Camera libraries
│   └── ...
└── tests/
    ├── CMakeLists.txt           # Test executables
    └── ...
```

### Quick Build

Use the provided build script:

```bash
./build.sh
```

### Manual Build

```bash
mkdir -p build
cd build
cmake ..
make -j$(nproc)
```

### Build Specific Targets

```bash
cd build
make opencv_wrapper              # Build only OpenCV wrapper
make libcamera_wrapper          # Build only libcamera wrapper
make test_aruco_detection_pipeline  # Build only test executable
```

## Dependencies

- **Required:**
  - CMake 3.16 or higher
  - C compiler (C11 standard)
  - C++ compiler (C++17 standard)
  - OpenCV 4.x with contrib modules (opencv_aruco)
  - libcamera (for camera support)
  - pkg-config

## Configuration Options

The CMake configuration automatically:
- Detects available dependencies
- Enables/disables optional components based on availability
- Sets up proper include paths and linking
- Configures compilation flags

### Compilation Flags

- C standard: C11
- C++ standard: C++17
- Warnings: `-Wall -Wextra`

## Adding New Targets

### Adding a New Library

1. Create source files in appropriate subdirectory
2. Edit the subdirectory's `CMakeLists.txt`
3. Add library target with `add_library()`
4. Specify dependencies with `target_link_libraries()`

### Adding a New Executable

1. Add source file to `tests/` directory
2. Edit `tests/CMakeLists.txt`
3. Add executable target with `add_executable()`
4. Link required libraries with `target_link_libraries()`

## Installation

To install built libraries and executables:

```bash
cd build
sudo make install
```

Default installation paths:
- Libraries: `/usr/local/lib`
- Headers: `/usr/local/include/rod_cv`, `/usr/local/include/rod_camera`
- Executables: `/usr/local/bin`

## Troubleshooting

### OpenCV not found

```bash
sudo apt-get install libopencv-dev libopencv-contrib-dev
```

### libcamera not found (optional)

```bash
sudo apt-get install libcamera-dev
```

Or the build will proceed without libcamera support.

### Build errors

1. Clean build directory: `rm -rf build`
2. Reconfigure: `mkdir build && cd build && cmake ..`
3. Rebuild: `make -j$(nproc)`

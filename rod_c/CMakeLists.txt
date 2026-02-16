cmake_minimum_required(VERSION 3.16)
project(ROD_C VERSION 1.0 LANGUAGES C CXX)

# Set C and C++ standards
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add compilation flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")

# Find required packages
find_package(PkgConfig REQUIRED)

# Find OpenCV - try multiple methods for compatibility
find_package(OpenCV QUIET)

if(NOT OpenCV_FOUND)
    # Try pkg-config as fallback (common on Raspberry Pi)
    pkg_check_modules(OpenCV REQUIRED opencv4)
    if(NOT OpenCV_FOUND)
        pkg_check_modules(OpenCV REQUIRED opencv)
    endif()
    
    # Set variables to match find_package naming
    set(OpenCV_LIBS ${OpenCV_LIBRARIES})
    set(OpenCV_INCLUDE_DIRS ${OpenCV_INCLUDE_DIRS})
endif()

# Try to find libcamera (optional)
pkg_check_modules(LIBCAMERA libcamera)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/rod_cv
    ${CMAKE_CURRENT_SOURCE_DIR}/rod_camera
    ${CMAKE_CURRENT_SOURCE_DIR}/rod_config
    ${CMAKE_CURRENT_SOURCE_DIR}/rod_visualization
    ${CMAKE_CURRENT_SOURCE_DIR}/rod_socket
    ${OpenCV_INCLUDE_DIRS}
)

if(LIBCAMERA_FOUND)
    include_directories(${LIBCAMERA_INCLUDE_DIRS})
    add_compile_definitions(HAVE_LIBCAMERA)
endif()

# Add subdirectories
add_subdirectory(rod_cv)
add_subdirectory(rod_camera)
add_subdirectory(rod_config)
add_subdirectory(rod_visualization)
add_subdirectory(rod_socket)
add_subdirectory(tests)

# Build main rod_detection executable
add_executable(rod_detection rod_detection.c)
target_link_libraries(rod_detection 
    opencv_wrapper
    rod_cv
    rod_config
    rod_visualization
    rod_socket
    rod_camera
    ${OpenCV_LIBS}
    m  # Math library for atan2f
)

# Build rod_communication executable
add_executable(rod_communication rod_communication.c)
# No special libraries needed for socket communication

# Print configuration summary
message(STATUS "=== ROD_C Build Configuration ===")
message(STATUS "C Compiler: ${CMAKE_C_COMPILER}")
message(STATUS "C++ Compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "OpenCV Version: ${OpenCV_VERSION}")
message(STATUS "OpenCV Libraries: ${OpenCV_LIBS}")
if(LIBCAMERA_FOUND)
    message(STATUS "libcamera: Found")
else()
    message(STATUS "libcamera: Not found (camera support will be limited)")
endif()
message(STATUS "================================")

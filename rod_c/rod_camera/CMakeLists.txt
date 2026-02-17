# rod_camera - Camera interface library
# Provides unified interface for different camera backends

# Source files for camera library
set(CAMERA_SOURCES "")

# Add unified camera interface (always include)
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/camera_interface.c)
    list(APPEND CAMERA_SOURCES camera_interface.c)
endif()

# Add IMX477 camera backend if it exists
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/backends/imx477/camera.c)
    list(APPEND CAMERA_SOURCES backends/imx477/camera.c)
endif()

# Add emulated camera backend if it exists
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/backends/emulated/emulated_camera.c)
    list(APPEND CAMERA_SOURCES backends/emulated/emulated_camera.c)
endif()

# Build libcamera wrapper if libcamera is available
if(LIBCAMERA_FOUND)
    add_library(libcamera_wrapper SHARED
        wrappers/libcamera_wrapper.cpp
    )
    
    target_link_libraries(libcamera_wrapper
        ${LIBCAMERA_LDFLAGS}
    )
    
    target_include_directories(libcamera_wrapper PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/wrappers
        ${CMAKE_CURRENT_SOURCE_DIR}/backends/imx477
        ${LIBCAMERA_INCLUDE_DIRS}
    )
    
    set_target_properties(libcamera_wrapper PROPERTIES
        VERSION ${PROJECT_VERSION}
        SOVERSION 1
        PUBLIC_HEADER "wrappers/libcamera_wrapper.h"
    )
    
    # Install rules
    install(TARGETS libcamera_wrapper
        LIBRARY DESTINATION lib
        PUBLIC_HEADER DESTINATION include/rod_camera/wrappers
    )
    
    message(STATUS "Building libcamera_wrapper")
else()
    message(STATUS "Skipping libcamera_wrapper (libcamera not found)")
endif()

# If there are camera sources, build a camera library
if(CAMERA_SOURCES)
    add_library(rod_camera SHARED
        ${CAMERA_SOURCES}
    )
    
    # Link with opencv_wrapper if emulated_camera is included
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/backends/emulated/emulated_camera.c)
        target_link_libraries(rod_camera
            opencv_wrapper
        )
        target_include_directories(rod_camera PRIVATE
            ${CMAKE_SOURCE_DIR}/rod_cv
        )
    endif()
    
    # Link with libcamera_wrapper if camera.c is included and libcamera is available
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/backends/imx477/camera.c AND LIBCAMERA_FOUND)
        target_link_libraries(rod_camera
            libcamera_wrapper
        )
    endif()
    
    target_include_directories(rod_camera PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/backends/imx477
        ${CMAKE_CURRENT_SOURCE_DIR}/backends/emulated
        ${CMAKE_CURRENT_SOURCE_DIR}/wrappers
    )
    
    set_target_properties(rod_camera PROPERTIES
        VERSION ${PROJECT_VERSION}
        SOVERSION 1
        PUBLIC_HEADER "camera_interface.h;backends/imx477/camera.h;backends/emulated/emulated_camera.h"
    )
    
    # Install rules
    install(TARGETS rod_camera
        LIBRARY DESTINATION lib
        PUBLIC_HEADER DESTINATION include/rod_camera
    )
    
    message(STATUS "Building rod_camera with sources: ${CAMERA_SOURCES}")
else()
    message(STATUS "No camera sources to build")
endif()

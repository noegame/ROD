# rod_camera - Camera interface library
# Provides interfaces for different camera backends

# Source files for camera library
set(CAMERA_SOURCES "")

# Add camera.c if it has content (currently empty)
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/camera.c)
    list(APPEND CAMERA_SOURCES camera.c)
endif()

# Add emulated_camera.c if it has content (currently empty)
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/emulated_camera.c)
    list(APPEND CAMERA_SOURCES emulated_camera.c)
endif()

# Build libcamera wrapper if libcamera is available
if(LIBCAMERA_FOUND)
    add_library(libcamera_wrapper SHARED
        libcamera_wrapper.cpp
    )
    
    target_link_libraries(libcamera_wrapper
        ${LIBCAMERA_LDFLAGS}
    )
    
    target_include_directories(libcamera_wrapper PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${LIBCAMERA_INCLUDE_DIRS}
    )
    
    set_target_properties(libcamera_wrapper PROPERTIES
        VERSION ${PROJECT_VERSION}
        SOVERSION 1
        PUBLIC_HEADER "libcamera_wrapper.h"
    )
    
    # Install rules
    install(TARGETS libcamera_wrapper
        LIBRARY DESTINATION lib
        PUBLIC_HEADER DESTINATION include/rod_camera
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
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/emulated_camera.c)
        target_link_libraries(rod_camera
            opencv_wrapper
        )
        target_include_directories(rod_camera PRIVATE
            ${CMAKE_SOURCE_DIR}/rod_cv
        )
    endif()
    
    # Link with libcamera_wrapper if camera.c is included and libcamera is available
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/camera.c AND LIBCAMERA_FOUND)
        target_link_libraries(rod_camera
            libcamera_wrapper
        )
        target_include_directories(rod_camera PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}
        )
    endif()
    
    target_include_directories(rod_camera PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
    )
    
    set_target_properties(rod_camera PROPERTIES
        VERSION ${PROJECT_VERSION}
        SOVERSION 1
        PUBLIC_HEADER "camera.h;emulated_camera.h"
    )
    
    # Install rules
    install(TARGETS rod_camera
        LIBRARY DESTINATION lib
        PUBLIC_HEADER DESTINATION include/rod_camera
    )
    
    message(STATUS "Building rod_camera with sources: ${CAMERA_SOURCES}")
else()
    message(STATUS "No camera sources to build (camera.c and emulated_camera.c are empty)")
endif()

# Test executables

# ========================================
# 1. ArUco Detection Pipeline Test
# ========================================
# Tests: sharpen → resize → detect pipeline
add_executable(test_aruco_detection
    test_aruco_detection.c
)

target_link_libraries(test_aruco_detection
    opencv_wrapper
    rod_cv
    rod_config
    rod_visualization
    m  # Math library for sqrt, etc.
)

target_include_directories(test_aruco_detection PRIVATE
    ${CMAKE_SOURCE_DIR}/rod_cv
    ${CMAKE_SOURCE_DIR}/rod_config
    ${CMAKE_SOURCE_DIR}/rod_visualization
)

# ========================================
# 2. Detection Dataset Test
# ========================================
# Tests: Detection accuracy on known images
add_executable(test_detection_dataset
    test_detection_dataset.c
)

target_link_libraries(test_detection_dataset
    opencv_wrapper
    rod_cv
    rod_config
    rod_visualization
    m
)

target_include_directories(test_detection_dataset PRIVATE
    ${CMAKE_SOURCE_DIR}/rod_cv
    ${CMAKE_SOURCE_DIR}/rod_config
    ${CMAKE_SOURCE_DIR}/rod_visualization
)

# ========================================
# 3. Camera Interface Contract Test
# ========================================
# Tests: camera.h API contract (emulated implementation)
add_executable(test_camera_interface
    test_camera_interface.c
)

target_link_libraries(test_camera_interface
    rod_camera
    opencv_wrapper
)

target_include_directories(test_camera_interface PRIVATE
    ${CMAKE_SOURCE_DIR}/rod_camera
    ${CMAKE_SOURCE_DIR}/rod_cv
)

# ========================================
# 4. Emulated Camera Implementation Test
# ========================================
# Tests: emulated_camera.c specific behavior
add_executable(test_emulated_camera_impl
    test_emulated_camera_impl.c
)

target_link_libraries(test_emulated_camera_impl
    rod_camera
    opencv_wrapper
)

target_include_directories(test_emulated_camera_impl PRIVATE
    ${CMAKE_SOURCE_DIR}/rod_camera
    ${CMAKE_SOURCE_DIR}/rod_cv
)

# ========================================
# 6. Camera Parameters Test (Hardware)
# ========================================
# Tests: libcamera parameter tuning (REAL HARDWARE ONLY)
if(LIBCAMERA_FOUND)
    add_executable(test_camera_parameters
        test_camera_parameters.c
    )

    target_link_libraries(test_camera_parameters
        rod_camera
        opencv_wrapper
    )

    target_include_directories(test_camera_parameters PRIVATE
        ${CMAKE_SOURCE_DIR}/rod_camera
        ${CMAKE_SOURCE_DIR}/rod_cv
    )
    
    message(STATUS "Building test_camera_parameters (libcamera support detected)")
else()
    message(STATUS "Skipping test_camera_parameters (libcamera not found)")
endif()

# ========================================
# Legacy Tests (ArUco Pose Estimation)
# ========================================
# Kept for reference but not part of main test suite
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/test_aruco_pose_estimation.c)
    add_executable(test_aruco_pose_estimation
        test_aruco_pose_estimation.c
    )
    
    target_link_libraries(test_aruco_pose_estimation
        opencv_wrapper
        rod_cv
        rod_config
        m
    )
    
    target_include_directories(test_aruco_pose_estimation PRIVATE
        ${CMAKE_SOURCE_DIR}/rod_cv
        ${CMAKE_SOURCE_DIR}/rod_config
    )
endif()

# ========================================
# Install Rules
# ========================================
install(TARGETS 
    test_aruco_detection
    test_detection_dataset
    test_camera_interface
    test_emulated_camera_impl
    test_emulated_camera
    RUNTIME DESTINATION bin
)

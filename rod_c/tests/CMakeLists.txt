# Test executables

# Main ArUco detection pipeline test
add_executable(test_aruco_detection_pipeline
    test_rod_cv.c
)

target_link_libraries(test_aruco_detection_pipeline
    opencv_wrapper
    rod_cv
    rod_config
    rod_visualization
    m  # Math library for sqrt, etc.
)

target_include_directories(test_aruco_detection_pipeline PRIVATE
    ${CMAKE_SOURCE_DIR}/rod_cv
    ${CMAKE_SOURCE_DIR}/rod_config
    ${CMAKE_SOURCE_DIR}/rod_visualization
)

# Legacy name alias for test_rod_cv
add_executable(test_rod_cv
    test_rod_cv.c
)

target_link_libraries(test_rod_cv
    opencv_wrapper
    rod_cv
    rod_config
    rod_visualization
    m
)

target_include_directories(test_rod_cv PRIVATE
    ${CMAKE_SOURCE_DIR}/rod_cv
    ${CMAKE_SOURCE_DIR}/rod_config
    ${CMAKE_SOURCE_DIR}/rod_visualization
)

# Emulated camera test
add_executable(test_emulated_camera
    test_emulated_camera.c
)

target_link_libraries(test_emulated_camera
    rod_camera
    opencv_wrapper
)

target_include_directories(test_emulated_camera PRIVATE
    ${CMAKE_SOURCE_DIR}/rod_camera
    ${CMAKE_SOURCE_DIR}/rod_cv
)

# Real camera test (libcamera-based)
if(LIBCAMERA_FOUND)
    add_executable(test_rod_camera
        test_rod_camera.c
    )

    target_link_libraries(test_rod_camera
        rod_camera
        opencv_wrapper
    )

    target_include_directories(test_rod_camera PRIVATE
        ${CMAKE_SOURCE_DIR}/rod_camera
        ${CMAKE_SOURCE_DIR}/rod_cv
    )
    
    message(STATUS "Building test_rod_camera (libcamera support detected)")
else()
    message(STATUS "Skipping test_rod_camera (libcamera not found)")
endif()

# Install rules
install(TARGETS test_aruco_detection_pipeline test_rod_cv test_emulated_camera
    RUNTIME DESTINATION bin
)

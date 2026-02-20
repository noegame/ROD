/**
 * @file rod_config.h
 * @brief Centralized configuration for ROD system
 * @author Noé Game
 * @date 15/02/2026
 * @see rod_config.c
 * @copyright Cecill-C (Cf. LICENCE.txt)
 * 
 * This module provides centralized configuration for:
 * - Valid ArUco marker IDs (Eurobot 2026 rules)
 * - ArUco detector parameters (optimized for competition)
 * - System constants (paths, intervals, buffer sizes)
 */

#pragma once

/* ******************************************************* Includes ****************************************************** */

#include "opencv_wrapper.h"

/* ***************************************************** Public macros *************************************************** */

// Socket configuration
#define ROD_SOCKET_PATH "/tmp/rod_detection.sock"
#define ROD_MAX_DETECTION_SIZE 1024

// Debug configuration
#define ROD_PICTURES_BASE_FOLDER "/var/roboteseo/pictures/camera"
#define ROD_DEBUG_BASE_FOLDER "/var/roboteseo/pictures/debug"
#define ROD_SAVE_DEBUG_IMAGE_INTERVAL 1  // Save every N frames

// Camera test configuration
#define ROD_CAMERA_TESTS_OUTPUT_FOLDER "/var/roboteseo/pictures/camera_tests"

// Default paths
#define ROD_DEFAULT_IMAGE_FOLDER "/var/roboteseo/pictures/camera/2026-01-16"

/* ************************************************** Public types definition ******************************************** */

/**
 * @brief Eurobot 2026 marker categories
 */
typedef enum {
    MARKER_CATEGORY_ROBOT_BLUE,    // IDs 1-5
    MARKER_CATEGORY_ROBOT_YELLOW,  // IDs 6-10
    MARKER_CATEGORY_FIXED,         // IDs 20-23
    MARKER_CATEGORY_BOX_BLUE,      // ID 36
    MARKER_CATEGORY_BOX_EMPTY,     // ID 41
    MARKER_CATEGORY_BOX_YELLOW,    // ID 47
    MARKER_CATEGORY_INVALID
} MarkerCategory;

/* *********************************************** Public functions declarations ***************************************** */

/**
 * @brief Check if a marker ID is valid according to Eurobot 2026 rules
 * @param id Marker ID to validate
 * @return 1 if valid, 0 otherwise
 * 
 * Valid IDs:
 * - 1-5: Blue team robots
 * - 6-10: Yellow team robots  
 * - 20-23: Fixed markers on field
 * - 36: Blue box
 * - 41: Empty box (black)
 * - 47: Yellow box
 */
int rod_config_is_valid_marker_id(int id);

/**
 * @brief Get the category of a marker ID
 * @param id Marker ID
 * @return Marker category
 */
MarkerCategory rod_config_get_marker_category(int id);

/**
 * @brief Configure ArUco detector with optimized parameters for Eurobot 2026
 * @param params Detector parameters handle
 * 
 * These parameters are optimized for Eurobot 2026 detection:
 * - Enables detection of ~40 markers instead of just 7
 * - Matches the Python implementation parameters
 * - Critical values - do not modify without extensive testing
 */
void rod_config_configure_detector_parameters(DetectorParametersHandle* params);

/**
 * @brief Get the ArUco dictionary type used for Eurobot 2026
 * @return Dictionary type constant (DICT_4X4_50)
 */
int rod_config_get_aruco_dictionary_type(void);

/**
 * @brief Get the camera calibration matrix (3x3) for fisheye lens
 * @return Pointer to 3x3 camera matrix (row-major order)
 * 
 * Camera matrix K from fisheye calibration:
 * [fx  0  cx]
 * [0  fy  cy]
 * [0   0   1]
 */
const float* rod_config_get_camera_matrix(void);

/**
 * @brief Get the distortion coefficients for fisheye lens
 * @return Pointer to 4 distortion coefficients [k1, k2, k3, k4]
 */
const float* rod_config_get_distortion_coeffs(void);

/**
 * @brief Create directory recursively (like mkdir -p)
 * @param path Directory path to create
 * @return 0 on success, -1 on failure
 */
int rod_config_create_directory_recursive(const char* path);

/**
 * @brief Generate date folder name in YYYY_MM_DD format
 * @param buffer Buffer to store the result (min 11 bytes)
 * @param buffer_size Size of the buffer
 */
void rod_config_generate_date_folder(char* buffer, size_t buffer_size);

/**
 * @brief Generate timestamp for filename in YYYYMMDD_HHMMSS_MS format
 * @param buffer Buffer to store the result (min 21 bytes)
 * @param buffer_size Size of the buffer
 */
void rod_config_generate_filename_timestamp(char* buffer, size_t buffer_size);

/**
 * @brief Ensure date-based subfolder exists
 * @param base_folder Base folder path (e.g., "/var/roboteseo/pictures")
 * @param date_subfolder Output buffer for the full path (min 256 bytes)
 * @param buffer_size Size of the output buffer
 * @return 0 on success, -1 on failure
 */
int rod_config_ensure_date_folder(const char* base_folder, char* date_subfolder, size_t buffer_size);

#ifdef __cplusplus
}
#endif

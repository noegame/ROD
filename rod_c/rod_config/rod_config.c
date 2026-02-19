/**
 * @file rod_config.c
 * @brief Centralized configuration for ROD system
 * @author Noé Game
 * @date 15/02/2026
 * @see rod_config.h
 * @copyright Cecill-C (Cf. LICENCE.txt)
 */

/* ******************************************************* Includes ****************************************************** */

#include "rod_config.h"
#include <time.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>

/* ******************************************* Public callback functions declarations ************************************ */

/* ********************************************* Function implementations *********************************************** */

int rod_config_is_valid_marker_id(int id) {
    return (id >= 1 && id <= 10) ||    // Robots (blue 1-5, yellow 6-10)
           (id >= 20 && id <= 23) ||   // Fixed markers
           (id == 36) ||                // Blue box
           (id == 41) ||                // Empty box (black)
           (id == 47);                  // Yellow box
}

MarkerCategory rod_config_get_marker_category(int id) {
    if (id >= 1 && id <= 5) {
        return MARKER_CATEGORY_ROBOT_BLUE;
    } else if (id >= 6 && id <= 10) {
        return MARKER_CATEGORY_ROBOT_YELLOW;
    } else if (id >= 20 && id <= 23) {
        return MARKER_CATEGORY_FIXED;
    } else if (id == 36) {
        return MARKER_CATEGORY_BOX_BLUE;
    } else if (id == 41) {
        return MARKER_CATEGORY_BOX_EMPTY;
    } else if (id == 47) {
        return MARKER_CATEGORY_BOX_YELLOW;
    } else {
        return MARKER_CATEGORY_INVALID;
    }
}

void rod_config_configure_detector_parameters(DetectorParametersHandle* params) {
    // Adaptive thresholding parameters
    // These values are CRITICAL - validated through extensive testing
    setAdaptiveThreshWinSizeMin(params, 3);
    setAdaptiveThreshWinSizeMax(params, 53);
    setAdaptiveThreshWinSizeStep(params, 4);
    
    // Marker size constraints
    setMinMarkerPerimeterRate(params, 0.01);
    setMaxMarkerPerimeterRate(params, 4.0);
    
    // Polygon approximation accuracy
    setPolygonalApproxAccuracyRate(params, 0.05);
    
    // Corner refinement for sub-pixel accuracy
    setCornerRefinementMethod(params, CORNER_REFINE_SUBPIX);
    setCornerRefinementWinSize(params, 5);
    setCornerRefinementMaxIterations(params, 50);
    
    // Detection constraints
    setMinDistanceToBorder(params, 0);
    setMinOtsuStdDev(params, 2.0);
    
    // Perspective removal
    setPerspectiveRemoveIgnoredMarginPerCell(params, 0.15);
}

int rod_config_get_aruco_dictionary_type(void) {
    return DICT_4X4_50;
}

const float* rod_config_get_camera_matrix(void) {
    // Camera calibration matrix from fisheye calibration
    // Matches values from Python implementation (rod_python/lab/8 detect aruco tags...)
    static const float camera_matrix[9] = {
        2493.62477, 0.0, 1977.18701,
        0.0, 2493.11358, 2034.91176,
        0.0, 0.0, 1.0
    };
    return camera_matrix;
}

const float* rod_config_get_distortion_coeffs(void) {
    // Fisheye distortion coefficients (k1, k2, k3, k4)
    // Matches values from Python implementation
    static const float dist_coeffs[4] = {
        -0.1203345, 0.06802544, -0.13779641, 0.08243704
    };
    return dist_coeffs;
}

int rod_config_create_directory_recursive(const char* path) {
    char tmp[512];
    struct stat st;
    
    // Check if directory already exists
    if (stat(path, &st) == 0 && S_ISDIR(st.st_mode)) {
        return 0;  // Already exists
    }
    
    snprintf(tmp, sizeof(tmp), "%s", path);
    
    for (char* p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            if (stat(tmp, &st) != 0) {
                if (mkdir(tmp, 0755) != 0 && errno != EEXIST) {
                    fprintf(stderr, "Failed to create directory %s: %s\n", tmp, strerror(errno));
                    return -1;
                }
            }
            *p = '/';
        }
    }
    
    // Create final directory
    if (stat(tmp, &st) != 0) {
        if (mkdir(tmp, 0755) != 0 && errno != EEXIST) {
            fprintf(stderr, "Failed to create directory %s: %s\n", tmp, strerror(errno));
            return -1;
        }
    }
    
    return 0;
}

void rod_config_generate_date_folder(char* buffer, size_t buffer_size) {
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    
    // Generate date folder: YYYY_MM_DD
    snprintf(buffer, buffer_size, "%04d_%02d_%02d",
             tm_info->tm_year + 1900,
             tm_info->tm_mon + 1,
             tm_info->tm_mday);
}

void rod_config_generate_filename_timestamp(char* buffer, size_t buffer_size) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    
    struct tm* tm_info = localtime(&tv.tv_sec);
    int milliseconds = tv.tv_usec / 1000;
    
    // Generate timestamp: YYYYMMDD_HHMMSS_MS
    snprintf(buffer, buffer_size, "%04d%02d%02d_%02d%02d%02d_%03d",
             tm_info->tm_year + 1900,
             tm_info->tm_mon + 1,
             tm_info->tm_mday,
             tm_info->tm_hour,
             tm_info->tm_min,
             tm_info->tm_sec,
             milliseconds);
}

int rod_config_ensure_date_folder(const char* base_folder, char* date_subfolder, size_t buffer_size) {
    char date_name[16];
    rod_config_generate_date_folder(date_name, sizeof(date_name));
    
    // Build full path: base_folder/YYYY_MM_DD
    snprintf(date_subfolder, buffer_size, "%s/%s", base_folder, date_name);
    
    // Ensure the folder exists
    return rod_config_create_directory_recursive(date_subfolder);
}

/**
 * @file rod_cv.h
 * @brief Computer vision helper functions for ROD
 * @author Noé Game
 * @date 15/02/2026
 * @see rod_cv.c
 * @copyright Cecill-C (Cf. LICENCE.txt)
 */

#pragma once

/* ******************************************************* Includes ****************************************************** */

#include "opencv_wrapper.h"

/* ***************************************************** Public macros *************************************************** */

/* ************************************************** Public types definition ******************************************** */

/**
 * @brief Structure to hold marker detection data (standardized across ROD)
 */
typedef struct {
    int id;
    float x;
    float y;
    float angle;
} MarkerData;

/**
 * @brief Structure to hold marker counts by category
 */
typedef struct {
    int black_markers;   // ID 41
    int blue_markers;    // ID 36
    int yellow_markers;  // ID 47
    int robot_markers;   // IDs 1-10
    int fixed_markers;   // IDs 20-23
    int total;
} MarkerCounts;

/**
 * @brief Structure to hold 3D position and orientation
 */
typedef struct {
    float x;
    float y;
    float z;
    float roll;
    float pitch;
    float yaw;
} Pose3D;

/**
 * @brief Structure to hold 2D position and orientation
 */
typedef struct {
    float x;
    float y;
    float angle;
} Pose2D;

/* *********************************************** Public functions declarations ***************************************** */

/**
 * @brief Calculate the center point of a marker from its corners
 * @param corners Array of 4 corner points [x,y]
 * @return Center point
 */
Point2f calculate_marker_center(float corners[4][2]);

/**
 * @brief Calculate the angle of a marker from its corners
 * @param corners Array of 4 corner points [x,y]
 * @return Angle in radians (-PI to PI)
 */
float calculate_marker_angle(float corners[4][2]);

/**
 * @brief Calculate the perimeter of a marker
 * @param corners Array of 4 corner points [x,y]
 * @return Perimeter in pixels
 */
float calculate_marker_perimeter(float corners[4][2]);

/**
 * @brief Calculate the area of a marker
 * @param corners Array of 4 corner points [x,y]
 * @return Area in square pixels
 */
float calculate_marker_area(float corners[4][2]);

/**
 * @brief Convert angle from radians to degrees
 * @param radians Angle in radians
 * @return Angle in degrees
 */
float rad_to_deg(float radians);

/**
 * @brief Convert angle from degrees to radians
 * @param degrees Angle in degrees
 * @return Angle in radians
 */
float deg_to_rad(float degrees);

/**
 * @brief Normalize angle to [-PI, PI] range
 * @param angle Angle in radians
 * @return Normalized angle
 */
float normalize_angle(float angle);

/**
 * @brief Filter detection results to keep only valid marker IDs
 * @param result Original detection result
 * @param filtered_markers Output array for filtered markers (must be allocated by caller)
 * @param max_markers Maximum number of markers to store in output
 * @return Number of valid markers found
 * 
 * This function converts DetectionResult to MarkerData array, filtering invalid IDs.
 * Caller must allocate filtered_markers array with sufficient size.
 */
int filter_valid_markers(DetectionResult* result, MarkerData* filtered_markers, int max_markers);

/**
 * @brief Count markers by category
 * @param markers Array of marker data
 * @param count Number of markers
 * @return MarkerCounts structure with counts by category
 */
MarkerCounts count_markers_by_category(MarkerData* markers, int count);

/**
 * @brief Pose estimation of aruco marker in a picture
 * @param image Image handle
 * @param marker_handle Marker handle containing marker ID and corners
 * @note This is a placeholder for future 3D pose estimation implementation
 */
// void estimate_marker_pose(ImageHandle* image, MarkerHandle* marker_handle);

/**
 * @brief Create a mask for the playing field based on fixed markers
 * @param image_path Path to the image (for initial detection)
 * @param detector ArUco detector handle
 * @param output_width Output mask width (should match target image)
 * @param output_height Output mask height (should match target image)
 * @param scale_y Vertical scale factor for mask (1.0 = normal, >1.0 = extend vertically)
 * @param homography_inv Output parameter for inverse homography matrix (3x3), can be NULL
 * @return Mask image handle (grayscale, 255=valid area, 0=masked), or NULL on failure
 * 
 * This function:
 * 1. Detects fixed markers (IDs 20, 21, 22, 23) in the image
 * 2. Calculates homography from real-world coordinates to image coordinates
 * 3. Projects the playing field boundaries (2000x3000mm) into the image
 * 4. Creates a binary mask covering the playing field area
 * 
 * Matches the Python implementation in find_mask().
 */
ImageHandle* create_field_mask(const char* image_path, 
                                ArucoDetectorHandle* detector,
                                int output_width, 
                                int output_height,
                                float scale_y,
                                float* homography_inv);

/**
 * @brief Create field mask from in-memory image (no disk I/O)
 * 
 * Same as create_field_mask but takes ImageHandle* instead of file path.
 * This allows creating mask dynamically from captured frames.
 */
ImageHandle* create_field_mask_from_image(ImageHandle* image,
                                           ArucoDetectorHandle* detector,
                                           int output_width, 
                                           int output_height,
                                           float scale_y,
                                           float* homography_inv);


/* ******************************************* Public callback functions declarations ************************************ */

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
 * @brief Estimate 3D pose of a marker using SolvePnP
 * @param corners Array of 4 corner points [x,y] in image coordinates
 * @param marker_size Size of the marker in mm (default 100mm)
 * @param camera_matrix Camera intrinsic matrix (3x3)
 * @param dist_coeffs Distortion coefficients (4 elements for fisheye)
 * @return PnPResult containing rvec, tvec, and success flag
 */
PnPResult estimate_marker_pose_camera_frame(float corners[4][2], 
                                             float marker_size,
                                             const float* camera_matrix,
                                             const float* dist_coeffs);

/**
 * @brief Compute transformation matrix from camera frame to playground frame
 * @param detection Detection result containing all markers
 * @param camera_matrix Camera intrinsic matrix (3x3)
 * @param dist_coeffs Distortion coefficients (4 elements)
 * @param marker_size Size of markers in mm
 * @param transform_matrix Output 4x4 transformation matrix (rotation + translation)
 * @return 0 on success, -1 if not enough fixed markers found
 * 
 * This function:
 * 1. Finds the 4 fixed markers (IDs 20, 21, 22, 23) in the detection
 * 2. Computes their 3D positions in camera frame using SolvePnP
 * 3. Uses known playground positions to compute transformation matrix
 */
int compute_camera_to_playground_transform(DetectionResult* detection,
                                           const float* camera_matrix,
                                           const float* dist_coeffs,
                                           float marker_size,
                                           float* transform_matrix);

/**
 * @brief Transform a 3D point from camera frame to playground frame
 * @param camera_point Point in camera frame (x, y, z)
 * @param transform_matrix 4x4 transformation matrix
 * @param playground_point Output point in playground frame (x, y, z)
 */
void transform_camera_to_playground(const float* camera_point,
                                    const float* transform_matrix,
                                    float* playground_point);

/**
 * @brief Convert detection results from pixel coordinates to playground coordinates
 * @param detection Detection result with pixel coordinates
 * @param markers Output array of markers with playground coordinates (x,y in mm, z from tvec)
 * @param max_markers Maximum number of markers to process
 * @param camera_matrix Camera intrinsic matrix
 * @param dist_coeffs Distortion coefficients
 * @return Number of markers successfully localized, -1 on error
 * 
 * This function:
 * 1. Computes camera-to-playground transformation using fixed markers (100mm)
 * 2. For each valid marker, estimates pose in camera frame using correct size per ID
 * 3. Transforms to playground coordinates
 * 
 * Note: Marker sizes are automatically determined from IDs:
 * - Fixed markers (20-23): 100mm
 * - Robot markers (1-10): 70mm  
 * - Game elements (36,41,47): 40mm
 */
int localize_markers_in_playground(DetectionResult* detection,
                                   MarkerData* markers,
                                   int max_markers,
                                   const float* camera_matrix,
                                   const float* dist_coeffs);

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

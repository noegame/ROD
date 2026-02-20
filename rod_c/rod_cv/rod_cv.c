/**
 * @file rod_cv.c
 * @brief Computer vision helper functions for ROD
 * @author Noé Game
 * @date 15/02/2026
 * @see rod_cv.h
 * @copyright Cecill-C (Cf. LICENCE.txt)
 * 
 * This module provides computer vision utility functions for the ROD project:
 * - Pose estimation of ArUco markers
 * - Coordinate transformations
 * - Advanced marker detection utilities
 */

/* ******************************************************* Includes ****************************************************** */

#include "rod_cv.h"
#include "opencv_wrapper.h"
#include "../rod_config/rod_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

/* ***************************************************** Public macros *************************************************** */

// Mathematical constants
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* ************************************************** Public types definition ******************************************** */

/* *********************************************** Public functions declarations ***************************************** */

/* ******************************************* Public callback functions declarations ************************************ */

/* ********************************************* Function implementations *********************************************** */

Point2f calculate_marker_center(float corners[4][2]) {
    Point2f center;
    center.x = (corners[0][0] + corners[1][0] + corners[2][0] + corners[3][0]) / 4.0f;
    center.y = (corners[0][1] + corners[1][1] + corners[2][1] + corners[3][1]) / 4.0f;
    return center;
}

float calculate_marker_angle(float corners[4][2]) {
    // Calculate angle from corner 0 to corner 1 (top edge of marker)
    // This represents the orientation of the marker
    float dx = corners[1][0] - corners[0][0];
    float dy = corners[1][1] - corners[0][1];
    return atan2f(dy, dx);
}

float calculate_marker_perimeter(float corners[4][2]) {
    float perimeter = 0.0f;
    
    for (int i = 0; i < 4; i++) {
        int next = (i + 1) % 4;
        float dx = corners[next][0] - corners[i][0];
        float dy = corners[next][1] - corners[i][1];
        perimeter += sqrtf(dx * dx + dy * dy);
    }
    
    return perimeter;
}

float calculate_marker_area(float corners[4][2]) {
    // Use the Shoelace formula for polygon area
    float area = 0.0f;
    
    for (int i = 0; i < 4; i++) {
        int next = (i + 1) % 4;
        area += corners[i][0] * corners[next][1];
        area -= corners[next][0] * corners[i][1];
    }
    
    return fabsf(area) / 2.0f;
}

float rad_to_deg(float radians) {
    return radians * 180.0f / M_PI;
}

float deg_to_rad(float degrees) {
    return degrees * M_PI / 180.0f;
}

float normalize_angle(float angle) {
    // Normalize angle to [-PI, PI] range
    while (angle > M_PI) {
        angle -= 2.0f * M_PI;
    }
    while (angle < -M_PI) {
        angle += 2.0f * M_PI;
    }
    return angle;
}

int filter_valid_markers(DetectionResult* result, MarkerData* filtered_markers, int max_markers) {
    if (result == NULL || filtered_markers == NULL || max_markers <= 0) {
        return 0;
    }
    
    int valid_count = 0;
    
    for (int i = 0; i < result->count && valid_count < max_markers; i++) {
        DetectedMarker* marker = &result->markers[i];
        
        // Only keep valid marker IDs
        if (!rod_config_is_valid_marker_id(marker->id)) {
            continue;
        }
        
        // Calculate center and angle
        Point2f center = calculate_marker_center(marker->corners);
        float angle = calculate_marker_angle(marker->corners);
        
        // Store marker data (in pixel coordinates for this function)
        filtered_markers[valid_count].id = marker->id;
        filtered_markers[valid_count].x = center.x;  // Pixel coordinates
        filtered_markers[valid_count].y = center.y;  // Pixel coordinates
        filtered_markers[valid_count].angle = angle;
        filtered_markers[valid_count].pixel_x = center.x;  // Same as x for this function
        filtered_markers[valid_count].pixel_y = center.y;  // Same as y for this function
        valid_count++;
    }
    
    return valid_count;
}

MarkerCounts count_markers_by_category(MarkerData* markers, int count) {
    MarkerCounts counts;
    memset(&counts, 0, sizeof(MarkerCounts));
    
    for (int i = 0; i < count; i++) {
        int id = markers[i].id;
        
        if (id == 41) {
            counts.black_markers++;
        } else if (id == 36) {
            counts.blue_markers++;
        } else if (id == 47) {
            counts.yellow_markers++;
        } else if (id >= 1 && id <= 10) {
            counts.robot_markers++;
        } else if (id >= 20 && id <= 23) {
            counts.fixed_markers++;
        }
        counts.total++;
    }
    
    return counts;
}

PnPResult estimate_marker_pose_camera_frame(float corners[4][2], 
                                             float marker_size,
                                             const float* camera_matrix,
                                             const float* dist_coeffs) {
    // Define 3D points of marker corners in marker's local coordinate system
    // Corner order matches OpenCV ArUco detection (top-left, top-right, bottom-right, bottom-left)
    float half_size = marker_size / 2.0f;
    Point3f object_points[4] = {
        {-half_size, half_size, 0.0f},   // Top-left
        {half_size, half_size, 0.0f},    // Top-right
        {half_size, -half_size, 0.0f},   // Bottom-right
        {-half_size, -half_size, 0.0f}   // Bottom-left
    };
    
    // Convert corners to Point2f array
    Point2f image_points[4];
    for (int i = 0; i < 4; i++) {
        image_points[i].x = corners[i][0];
        image_points[i].y = corners[i][1];
    }
    
    // Call OpenCV wrapper's solve_pnp
    return solve_pnp(object_points, image_points, 4, 
                     (float*)camera_matrix, (float*)dist_coeffs);
}

int compute_camera_to_playground_transform(DetectionResult* detection,
                                           const float* camera_matrix,
                                           const float* dist_coeffs,
                                           float marker_size,
                                           float* transform_matrix) {
    if (!detection || !transform_matrix) {
        return -1;
    }
    
    // Known playground positions of fixed markers (in mm)
    // ID -> (x, y, z) in playground frame
    float fixed_markers_playground[4][4] = {
        {20, 600, 600, 30},      // ID 20
        {21, 600, 2400, 30},     // ID 21
        {22, 1400, 600, 30},     // ID 22
        {23, 1400, 2400, 30}     // ID 23
    };
    
    // Find fixed markers and get their camera frame positions
    Point3f camera_points[4];
    Point3f playground_points[4];
    int found_count = 0;
    
    for (int i = 0; i < detection->count && found_count < 4; i++) {
        int marker_id = detection->markers[i].id;
        
        // Check if this is a fixed marker
        for (int j = 0; j < 4; j++) {
            if (marker_id == (int)fixed_markers_playground[j][0]) {
                // Estimate pose in camera frame
                PnPResult pose = estimate_marker_pose_camera_frame(
                    detection->markers[i].corners,
                    marker_size,
                    camera_matrix,
                    dist_coeffs
                );
                
                if (pose.success) {
                    // Store camera frame position (tvec is the marker center in camera frame)
                    camera_points[found_count].x = pose.tvec[0];
                    camera_points[found_count].y = pose.tvec[1];
                    camera_points[found_count].z = pose.tvec[2];
                    
                    // Store corresponding playground position
                    playground_points[found_count].x = fixed_markers_playground[j][1];
                    playground_points[found_count].y = fixed_markers_playground[j][2];
                    playground_points[found_count].z = fixed_markers_playground[j][3];
                    
                    found_count++;
                }
                break;
            }
        }
    }
    
    if (found_count < 4) {
        fprintf(stderr, "compute_camera_to_playground_transform: only %d/4 fixed markers found\n", found_count);
        return -1;
    }
    
    // Compute centroids
    Point3f camera_centroid = {0, 0, 0};
    Point3f playground_centroid = {0, 0, 0};
    for (int i = 0; i < 4; i++) {
        camera_centroid.x += camera_points[i].x;
        camera_centroid.y += camera_points[i].y;
        camera_centroid.z += camera_points[i].z;
        playground_centroid.x += playground_points[i].x;
        playground_centroid.y += playground_points[i].y;
        playground_centroid.z += playground_points[i].z;
    }
    camera_centroid.x /= 4.0f;
    camera_centroid.y /= 4.0f;
    camera_centroid.z /= 4.0f;
    playground_centroid.x /= 4.0f;
    playground_centroid.y /= 4.0f;
    playground_centroid.z /= 4.0f;
    
    // Compute covariance matrix H = sum((p_playground - centroid_playground) * (p_camera - centroid_camera)^T)
    float H[3][3] = {{0}};
    for (int i = 0; i < 4; i++) {
        float dp_x = playground_points[i].x - playground_centroid.x;
        float dp_y = playground_points[i].y - playground_centroid.y;
        float dp_z = playground_points[i].z - playground_centroid.z;
        
        float dc_x = camera_points[i].x - camera_centroid.x;
        float dc_y = camera_points[i].y - camera_centroid.y;
        float dc_z = camera_points[i].z - camera_centroid.z;
        
        H[0][0] += dp_x * dc_x;
        H[0][1] += dp_x * dc_y;
        H[0][2] += dp_x * dc_z;
        H[1][0] += dp_y * dc_x;
        H[1][1] += dp_y * dc_y;
        H[1][2] += dp_y * dc_z;
        H[2][0] += dp_z * dc_x;
        H[2][1] += dp_z * dc_y;
        H[2][2] += dp_z * dc_z;
    }
    
    // Simplified rotation estimation (using approximation for small rotations)
    // For a more robust solution, SVD should be used, but that requires additional libraries
    // This approximation works well when camera is roughly aligned with playground
    
    // Build 4x4 transformation matrix [R | t; 0 0 0 1]
    // For simplicity, we'll use a scale + translation approach
    // R is approximated as identity (assumes camera Z-axis roughly parallel to playground plane)
    float scale = 1.0f;
    
    // Initialize as identity matrix
    for (int i = 0; i < 16; i++) {
        transform_matrix[i] = 0.0f;
    }
    transform_matrix[0] = scale;   // R[0][0]
    transform_matrix[5] = scale;   // R[1][1]
    transform_matrix[10] = scale;  // R[2][2]
    transform_matrix[15] = 1.0f;   // Homogeneous coordinate
    
    // Translation: t = playground_centroid - R * camera_centroid
    transform_matrix[3] = playground_centroid.x - scale * camera_centroid.x;   // tx
    transform_matrix[7] = playground_centroid.y - scale * camera_centroid.y;   // ty
    transform_matrix[11] = playground_centroid.z - scale * camera_centroid.z;  // tz
    
    return 0;
}

void transform_camera_to_playground(const float* camera_point,
                                    const float* transform_matrix,
                                    float* playground_point) {
    // Apply 4x4 transformation: p_playground = T * p_camera
    // T is in column-major order: [R | t]
    //                              [0   1]
    
    playground_point[0] = transform_matrix[0] * camera_point[0] +
                          transform_matrix[1] * camera_point[1] +
                          transform_matrix[2] * camera_point[2] +
                          transform_matrix[3];
    
    playground_point[1] = transform_matrix[4] * camera_point[0] +
                          transform_matrix[5] * camera_point[1] +
                          transform_matrix[6] * camera_point[2] +
                          transform_matrix[7];
    
    playground_point[2] = transform_matrix[8] * camera_point[0] +
                          transform_matrix[9] * camera_point[1] +
                          transform_matrix[10] * camera_point[2] +
                          transform_matrix[11];
}

int localize_markers_in_playground(DetectionResult* detection,
                                   MarkerData* markers,
                                   int max_markers,
                                   const float* homography_inv) {
    if (!detection || !markers || max_markers <= 0 || !homography_inv) {
        return -1;
    }
    
    int valid_count = 0;
    
    for (int i = 0; i < detection->count && valid_count < max_markers; i++) {
        DetectedMarker* marker = &detection->markers[i];
        
        // Only process valid marker IDs
        if (!rod_config_is_valid_marker_id(marker->id)) {
            continue;
        }
        
        // Calculate pixel center from corners
        Point2f pixel_center = calculate_marker_center(marker->corners);
        float angle = calculate_marker_angle(marker->corners);
        
        // Transform pixel coordinates to playground coordinates using homography
        Point2f pixel_point = {pixel_center.x, pixel_center.y};
        Point2f* terrain_point = perspective_transform(&pixel_point, 1, (float*)homography_inv);
        
        if (!terrain_point) {
            // Fallback: use pixel coordinates if transformation fails
            markers[valid_count].id = marker->id;
            markers[valid_count].x = pixel_center.x;
            markers[valid_count].y = pixel_center.y;
            markers[valid_count].angle = angle;
            markers[valid_count].pixel_x = pixel_center.x;
            markers[valid_count].pixel_y = pixel_center.y;
            valid_count++;
            continue;
        }
        
        // Store marker data with playground coordinates (mm) and pixel coordinates
        markers[valid_count].id = marker->id;
        markers[valid_count].x = terrain_point[0].x;  // X in mm (terrain)
        markers[valid_count].y = terrain_point[0].y;  // Y in mm (terrain)
        markers[valid_count].angle = angle;
        markers[valid_count].pixel_x = pixel_center.x;  // X in pixels (for visualization)
        markers[valid_count].pixel_y = pixel_center.y;  // Y in pixels (for visualization)
        
        // Free allocated memory
        free_points_2f(terrain_point);
        
        valid_count++;
    }
    
    return valid_count;
}

ImageHandle* create_field_mask_from_image(ImageHandle* image,
                                           ArucoDetectorHandle* detector,
                                           int output_width, 
                                           int output_height,
                                           float scale_y,
                                           float* homography_inv) {
    if (!image || !detector) {
        fprintf(stderr, "create_field_mask_from_image: invalid parameters\n");
        return NULL;
    }
    
    // Detect ArUco tags
    DetectionResult* detection = detectMarkersWithConfidence(detector, image);
    
    if (!detection || detection->count == 0) {
        fprintf(stderr, "create_field_mask: no markers detected\n");
        if (detection) releaseDetectionResult(detection);
        return NULL;
    }
    
    // Known real-world positions of fixed tags (in mm)
    // Tag ID -> (x, y) position
    float tag_irl[4][3] = {
        {20, 600, 600},
        {21, 600, 2400},
        {22, 1400, 600},
        {23, 1400, 2400}
    };
    
    // Find detected fixed tags and build point correspondences
    Point2f src_pts[4];  // Real-world positions
    Point2f dst_pts[4];  // Image positions
    int found_count = 0;
    
    for (int i = 0; i < detection->count && found_count < 4; i++) {
        int id = detection->markers[i].id;
        
        // Check if this is one of the fixed tags
        for (int j = 0; j < 4; j++) {
            if ((int)tag_irl[j][0] == id) {
                // Store real-world position
                src_pts[found_count].x = tag_irl[j][1];
                src_pts[found_count].y = tag_irl[j][2];
                
                // Calculate marker center in image
                Point2f center = calculate_marker_center(detection->markers[i].corners);
                dst_pts[found_count].x = center.x;
                dst_pts[found_count].y = center.y;
                
                found_count++;
                break;
            }
        }
    }
    
    releaseDetectionResult(detection);
    
    if (found_count != 4) {
        fprintf(stderr, "create_field_mask: only %d/4 fixed tags detected\n", found_count);
        return NULL;
    }
    
    // Get calibration parameters
    const float* K = rod_config_get_camera_matrix();
    const float* D = rod_config_get_distortion_coeffs();
    
    // Undistort the image points before calculating homography
    Point2f* dst_pts_undistorted = fisheye_undistort_points(dst_pts, 4, 
                                                             (float*)K, (float*)D, (float*)K);
    if (!dst_pts_undistorted) {
        fprintf(stderr, "create_field_mask: failed to undistort points\n");
        return NULL;
    }
    
    // Calculate homography (real-world -> image)
    float* H = find_homography(src_pts, dst_pts_undistorted, 4);
    free_points_2f(dst_pts_undistorted);
    
    if (!H) {
        fprintf(stderr, "create_field_mask: failed to calculate homography\n");
        return NULL;
    }
    
    // Store inverse homography if requested (for image -> real-world transformation)
    if (homography_inv) {
        // Calculate inverse homography using simple matrix inversion
        // For 3x3 matrix: inv(H) can be computed but we'll use find_homography with swapped points
        Point2f* inv_dst_undistorted = fisheye_undistort_points(dst_pts, 4,
                                                                 (float*)K, (float*)D, (float*)K);
        float* H_inv = find_homography(inv_dst_undistorted, src_pts, 4);
        free_points_2f(inv_dst_undistorted);
        
        if (H_inv) {
            memcpy(homography_inv, H_inv, 9 * sizeof(float));
            free_matrix(H_inv);
        }
    }
    
    // Define field corners in real-world coordinates (2000mm x 3000mm)
    Point2f field_corners[4] = {
        {0, 0},
        {2000, 0},
        {2000, 3000},
        {0, 3000}
    };
    
    // Transform field corners to image coordinates
    Point2f* field_img = perspective_transform(field_corners, 4, H);
    free_matrix(H);
    
    if (!field_img) {
        fprintf(stderr, "create_field_mask: failed to transform field corners\n");
        return NULL;
    }
    
    // Apply vertical scaling if requested
    if (scale_y != 1.0f) {
        float center_y = (field_img[0].y + field_img[1].y + field_img[2].y + field_img[3].y) / 4.0f;
        for (int i = 0; i < 4; i++) {
            field_img[i].y = center_y + (field_img[i].y - center_y) * scale_y;
        }
    }
    
    // Clip coordinates to image bounds
    for (int i = 0; i < 4; i++) {
        if (field_img[i].x < 0) field_img[i].x = 0;
        if (field_img[i].x >= output_width) field_img[i].x = output_width - 1;
        if (field_img[i].y < 0) field_img[i].y = 0;
        if (field_img[i].y >= output_height) field_img[i].y = output_height - 1;
    }
    
    // Create mask image (grayscale)
    ImageHandle* mask = create_empty_image(output_width, output_height, 1);
    if (!mask) {
        fprintf(stderr, "create_field_mask: failed to create mask image\n");
        free_points_2f(field_img);
        return NULL;
    }
    
    // Convert Point2f array to float array for fill_poly
    float points[8];
    for (int i = 0; i < 4; i++) {
        points[i * 2] = field_img[i].x;
        points[i * 2 + 1] = field_img[i].y;
    }
    free_points_2f(field_img);
    
    // Fill polygon with white (255)
    Color white = {255, 255, 255};
    ImageHandle* filled_mask = fill_poly(mask, points, 4, white);
    release_image(mask);
    
    if (!filled_mask) {
        fprintf(stderr, "create_field_mask_from_image: failed to fill polygon\n");
        return NULL;
    }
    
    return filled_mask;
}

ImageHandle* create_field_mask(const char* image_path, 
                                ArucoDetectorHandle* detector,
                                int output_width, 
                                int output_height,
                                float scale_y,
                                float* homography_inv) {
    if (!image_path || !detector) {
        fprintf(stderr, "create_field_mask: invalid parameters\n");
        return NULL;
    }
    
    // Load reference image
    ImageHandle* img = load_image(image_path);
    if (!img) {
        fprintf(stderr, "create_field_mask: failed to load image %s\n", image_path);
        return NULL;
    }
    
    // Use the from_image version
    ImageHandle* mask = create_field_mask_from_image(img, detector, output_width, output_height, scale_y, homography_inv);
    release_image(img);
    
    return mask;}
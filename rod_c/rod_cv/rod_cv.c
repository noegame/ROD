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
        
        // Store marker data
        filtered_markers[valid_count].id = marker->id;
        filtered_markers[valid_count].x = center.x;
        filtered_markers[valid_count].y = center.y;
        filtered_markers[valid_count].angle = angle;
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
    
    int img_width = get_image_width(img);
    int img_height = get_image_height(img);
    
    // Detect ArUco tags
    DetectionResult* detection = detectMarkersWithConfidence(detector, img);
    release_image(img);
    
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
        fprintf(stderr, "create_field_mask: failed to fill polygon\n");
        return NULL;
    }
    
    return filled_mask;
}

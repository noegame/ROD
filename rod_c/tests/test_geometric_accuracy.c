/**
 * test_geometric_accuracy.c
 * 
 * Geometric Accuracy Benchmark Test
 * 
 * This test validates the accuracy of the image-to-world coordinate 
 * transformation by comparing detected marker positions against known 
 * ground truth positions from the Eurobot 2026 field layout.
 * 
 * Test Pipeline:
 * 1. Load test image with markers at known positions
 * 2. Detect ArUco markers (full pipeline: sharpen → resize → detect)
 * 3. Compute homography using fixed field markers (IDs 20-23)
 * 4. Transform detected pixel positions to world coordinates
 * 5. Match detected markers to ground truth by ID and proximity
 * 6. Calculate position and angle errors
 * 7. Report detailed metrics and pass/fail status
 * 
 * This test measures the most critical ROD capability: accurate 
 * real-world positioning for the robotics competition.
 */

#include "opencv_wrapper.h"
#include "rod_cv.h"
#include "rod_config.h"
#include "rod_visualization.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>

// ANSI color codes
#define COLOR_RED "\033[1;31m"
#define COLOR_GREEN "\033[1;32m"
#define COLOR_YELLOW "\033[1;33m"
#define COLOR_CYAN "\033[1;36m"
#define COLOR_RESET "\033[0m"

// Test pass/fail thresholds
#define MIN_DETECTION_RATE 0.80       // 80% of expected markers must be found
#define MAX_MEAN_POSITION_ERROR 30.0  // mm
#define MAX_MAX_POSITION_ERROR 60.0   // mm
#define MAX_MEAN_ANGLE_ERROR 10.0     // degrees
#define OUTLIER_THRESHOLD 40.0        // mm

// Matching tolerance (max distance to consider same marker)
#define MATCHING_DISTANCE_THRESHOLD 100.0  // mm

/**
 * Ground truth marker structure
 */
typedef struct {
    int marker_id;           // ArUco ID (can be 36, 41, or 47 for game elements)
    const char* zone_name;   // Human-readable location (e.g., "ZONE_1")
    float world_x;           // Expected X coordinate (mm)
    float world_y;           // Expected Y coordinate (mm)
    float tolerance_pos;     // Position tolerance (mm) - unused for now
    float world_angle;       // Expected rotation (degrees)
} GroundTruthMarker;

/**
 * Detected marker with world coordinates
 */
typedef struct {
    int marker_id;
    float pixel_x;
    float pixel_y;
    float world_x;
    float world_y;
    float angle;
    int matched;  // 1 if matched to ground truth, 0 otherwise
} DetectedWorldMarker;

/**
 * Error statistics
 */
typedef struct {
    float mean_position_error;
    float median_position_error;
    float max_position_error;
    float mean_angle_error;
    float median_angle_error;
    float max_angle_error;
    int outlier_count;
} ErrorStats;

// Ground truth dataset (Eurobot 2026 initial positions)
// Format: {marker_id, zone_name, x, y, tolerance, angle}
// Note: For positions with multiple possible IDs [36, 47], we'll create separate entries
static const GroundTruthMarker GROUND_TRUTH[] = {
    // Zone 1 - Black boxes (ID 41)
    {41, "ZONE_1", 325, 750, 30, 0},
    {41, "ZONE_1", 325, 800, 30, 0},
    {41, "ZONE_1", 325, 850, 30, 0},
    
    // Zone 2 - Black boxes (ID 41)
    {41, "ZONE_2", 325, 2150, 30, 0},
    {41, "ZONE_2", 325, 2200, 30, 0},
    {41, "ZONE_2", 325, 2250, 30, 0},
    
    // Zone 3 - Colored boxes (ID 36 or 47)
    {36, "ZONE_3", 725, 200, 30, 90},
    {47, "ZONE_3", 725, 200, 30, 90},
    {36, "ZONE_3", 775, 200, 30, 90},
    {47, "ZONE_3", 775, 200, 30, 90},
    {36, "ZONE_3", 825, 200, 30, 90},
    {47, "ZONE_3", 825, 200, 30, 90},
    {36, "ZONE_3", 875, 200, 30, 90},
    {47, "ZONE_3", 875, 200, 30, 90},
    
    // Zone 4 - Colored boxes
    {36, "ZONE_4", 725, 2800, 30, 90},
    {47, "ZONE_4", 725, 2800, 30, 90},
    {36, "ZONE_4", 775, 2800, 30, 90},
    {47, "ZONE_4", 775, 2800, 30, 90},
    {36, "ZONE_4", 825, 2800, 30, 90},
    {47, "ZONE_4", 825, 2800, 30, 90},
    {36, "ZONE_4", 875, 2800, 30, 90},
    {47, "ZONE_4", 875, 2800, 30, 90},
    
    // Zone 5 - Colored boxes
    {36, "ZONE_5", 1200, 1075, 30, 0},
    {47, "ZONE_5", 1200, 1075, 30, 0},
    {36, "ZONE_5", 1200, 1125, 30, 0},
    {47, "ZONE_5", 1200, 1125, 30, 0},
    {36, "ZONE_5", 1200, 1175, 30, 0},
    {47, "ZONE_5", 1200, 1175, 30, 0},
    {36, "ZONE_5", 1200, 1225, 30, 0},
    {47, "ZONE_5", 1200, 1225, 30, 0},
    
    // Zone 6 - Colored boxes
    {36, "ZONE_6", 1200, 1775, 30, 0},
    {47, "ZONE_6", 1200, 1775, 30, 0},
    {36, "ZONE_6", 1200, 1825, 30, 0},
    {47, "ZONE_6", 1200, 1825, 30, 0},
    {36, "ZONE_6", 1200, 1875, 30, 0},
    {47, "ZONE_6", 1200, 1875, 30, 0},
    {36, "ZONE_6", 1200, 1925, 30, 0},
    {47, "ZONE_6", 1200, 1925, 30, 0},
    
    // Zone 7 - Colored boxes
    {36, "ZONE_7", 1525, 200, 30, 90},
    {47, "ZONE_7", 1525, 200, 30, 90},
    {36, "ZONE_7", 1575, 200, 30, 90},
    {47, "ZONE_7", 1575, 200, 30, 90},
    {36, "ZONE_7", 1625, 200, 30, 90},
    {47, "ZONE_7", 1625, 200, 30, 90},
    {36, "ZONE_7", 1675, 200, 30, 90},
    {47, "ZONE_7", 1675, 200, 30, 90},
    
    // Zone 8 - Colored boxes
    {36, "ZONE_8", 1525, 2800, 30, 90},
    {47, "ZONE_8", 1525, 2800, 30, 90},
    {36, "ZONE_8", 1575, 2800, 30, 90},
    {47, "ZONE_8", 1575, 2800, 30, 90},
    {36, "ZONE_8", 1625, 2800, 30, 90},
    {47, "ZONE_8", 1625, 2800, 30, 90},
    {36, "ZONE_8", 1675, 2800, 30, 90},
    {47, "ZONE_8", 1675, 2800, 30, 90},
    
    // Zone 9 - Colored boxes
    {36, "ZONE_9", 1800, 1025, 30, 0},
    {47, "ZONE_9", 1800, 1025, 30, 0},
    {36, "ZONE_9", 1800, 1075, 30, 0},
    {47, "ZONE_9", 1800, 1075, 30, 0},
    {36, "ZONE_9", 1800, 1125, 30, 0},
    {47, "ZONE_9", 1800, 1125, 30, 0},
    {36, "ZONE_9", 1800, 1175, 30, 0},
    {47, "ZONE_9", 1800, 1175, 30, 0},
    
    // Zone 10 - Colored boxes
    {36, "ZONE_10", 1800, 1825, 30, 0},
    {47, "ZONE_10", 1800, 1825, 30, 0},
    {36, "ZONE_10", 1800, 1875, 30, 0},
    {47, "ZONE_10", 1800, 1875, 30, 0},
    {36, "ZONE_10", 1800, 1925, 30, 0},
    {47, "ZONE_10", 1800, 1925, 30, 0},
    {36, "ZONE_10", 1800, 1975, 30, 0},
    {47, "ZONE_10", 1800, 1975, 30, 0},
};

#define NUM_GROUND_TRUTH (sizeof(GROUND_TRUTH) / sizeof(GroundTruthMarker))

/**
 * Calculate Euclidean distance between two points
 */
float calculate_distance(float x1, float y1, float x2, float y2) {
    float dx = x2 - x1;
    float dy = y2 - y1;
    return sqrtf(dx * dx + dy * dy);
}

/**
 * Calculate angle difference (handling wraparound)
 */
float calculate_angle_difference(float angle1, float angle2) {
    float diff = fabs(angle1 - angle2);
    // Handle wraparound (e.g., 355° vs 5° should be 10° not 350°)
    if (diff > 180.0f) {
        diff = 360.0f - diff;
    }
    return diff;
}

/**
 * Comparison function for qsort (ascending order)
 */
int compare_float(const void* a, const void* b) {
    float fa = *(const float*)a;
    float fb = *(const float*)b;
    if (fa < fb) return -1;
    if (fa > fb) return 1;
    return 0;
}

/**
 * Calculate median of an array
 */
float calculate_median(float* values, int count) {
    if (count == 0) return 0.0f;
    
    // Sort values
    qsort(values, count, sizeof(float), compare_float);
    
    // Return median
    if (count % 2 == 1) {
        return values[count / 2];
    } else {
        return (values[count / 2 - 1] + values[count / 2]) / 2.0f;
    }
}

/**
 * Run full detection pipeline on image
 * Returns detection result with marker data
 */
DetectionResult* detect_markers_in_image(const char* image_path) {
    // Load image
    ImageHandle* image = load_image(image_path);
    if (!image) {
        fprintf(stderr, "ERROR: Failed to load image: %s\n", image_path);
        return NULL;
    }
    
    // Sharpen
    ImageHandle* sharpened = sharpen_image(image);
    release_image(image);
    if (!sharpened) {
        fprintf(stderr, "ERROR: Failed to sharpen image\n");
        return NULL;
    }
    
    // Resize 1.5x
    int orig_w = get_image_width(sharpened);
    int orig_h = get_image_height(sharpened);
    int new_w = (int)(orig_w * 1.5f);
    int new_h = (int)(orig_h * 1.5f);
    
    ImageHandle* resized = resize_image(sharpened, new_w, new_h);
    release_image(sharpened);
    if (!resized) {
        fprintf(stderr, "ERROR: Failed to resize image\n");
        return NULL;
    }
    
    // Create detector
    ArucoDictionaryHandle* dictionary = getPredefinedDictionary(rod_config_get_aruco_dictionary_type());
    if (!dictionary) {
        release_image(resized);
        fprintf(stderr, "ERROR: Failed to create ArUco dictionary\n");
        return NULL;
    }
    
    DetectorParametersHandle* params = createDetectorParameters();
    if (!params) {
        releaseArucoDictionary(dictionary);
        release_image(resized);
        fprintf(stderr, "ERROR: Failed to create detector parameters\n");
        return NULL;
    }
    
    rod_config_configure_detector_parameters(params);
    
    ArucoDetectorHandle* detector = createArucoDetector(dictionary, params);
    if (!detector) {
        releaseDetectorParameters(params);
        releaseArucoDictionary(dictionary);
        release_image(resized);
        fprintf(stderr, "ERROR: Failed to create ArUco detector\n");
        return NULL;
    }
    
    // Detect markers
    DetectionResult* result = detectMarkersWithConfidence(detector, resized);
    
    // Scale corners back to original size (detected on 1.5x image)
    if (result) {
        for (int i = 0; i < result->count; i++) {
            for (int j = 0; j < 4; j++) {
                result->markers[i].corners[j][0] /= 1.5f;
                result->markers[i].corners[j][1] /= 1.5f;
            }
        }
    }
    
    // Cleanup
    releaseArucoDetector(detector);
    releaseDetectorParameters(params);
    releaseArucoDictionary(dictionary);
    release_image(resized);
    
    return result;
}

/**
 * Compute inverse homography for image-to-world transformation
 * Uses fixed field markers (IDs 20-23)
 */
float* compute_inverse_homography(const char* image_path) {
    // Create detector for fixed markers
    ArucoDictionaryHandle* dictionary = getPredefinedDictionary(rod_config_get_aruco_dictionary_type());
    DetectorParametersHandle* params = createDetectorParameters();
    rod_config_configure_detector_parameters(params);
    ArucoDetectorHandle* detector = createArucoDetector(dictionary, params);
    
    // Allocate homography matrix
    float* H_inv = (float*)malloc(9 * sizeof(float));
    if (!H_inv) {
        releaseArucoDetector(detector);
        releaseDetectorParameters(params);
        releaseArucoDictionary(dictionary);
        return NULL;
    }
    
    // Use create_field_mask to compute homography
    // We don't need the mask, just the homography
    ImageHandle* dummy_mask = create_field_mask(image_path, detector, 100, 100, 1.0f, H_inv);
    
    // Cleanup
    if (dummy_mask) {
        release_image(dummy_mask);
    }
    releaseArucoDetector(detector);
    releaseDetectorParameters(params);
    releaseArucoDictionary(dictionary);
    
    // Check if homography was computed
    int is_valid = 0;
    for (int i = 0; i < 9; i++) {
        if (H_inv[i] != 0.0f) {
            is_valid = 1;
            break;
        }
    }
    
    if (!is_valid || !dummy_mask) {
        free(H_inv);
        fprintf(stderr, "ERROR: Failed to compute homography (are fixed markers 20-23 visible?)\n");
        return NULL;
    }
    
    return H_inv;
}

/**
 * Transform detected markers from pixel to world coordinates
 */
int transform_markers_to_world(DetectionResult* detections, float* H_inv, 
                                DetectedWorldMarker* world_markers, int max_markers) {
    if (!detections || !H_inv || !world_markers) {
        return 0;
    }
    
    // Get camera calibration for undistortion
    const float* K = rod_config_get_camera_matrix();
    const float* D = rod_config_get_distortion_coeffs();
    
    int count = 0;
    
    for (int i = 0; i < detections->count && count < max_markers; i++) {
        // Only process valid game element markers
        if (!rod_config_is_valid_marker_id(detections->markers[i].id)) {
            continue;
        }
        
        // Skip fixed markers (IDs 20-23)
        int id = detections->markers[i].id;
        if (id >= 20 && id <= 23) {
            continue;
        }
        
        // Calculate marker center in pixel coordinates
        Point2f center = calculate_marker_center(detections->markers[i].corners);
        
        // Undistort pixel coordinates
        Point2f undistorted_pts[1];
        Point2f* undistorted = fisheye_undistort_points(&center, 1, (float*)K, (float*)D, (float*)K);
        if (!undistorted) {
            continue;
        }
        undistorted_pts[0] = undistorted[0];
        free_points_2f(undistorted);
        
        // Transform to world coordinates using inverse homography
        Point2f* world_pts = perspective_transform(undistorted_pts, 1, H_inv);
        if (!world_pts) {
            continue;
        }
        
        // Store result
        world_markers[count].marker_id = id;
        world_markers[count].pixel_x = center.x;
        world_markers[count].pixel_y = center.y;
        world_markers[count].world_x = world_pts[0].x;
        world_markers[count].world_y = world_pts[0].y;
        world_markers[count].angle = 0.0f;  // TODO: Calculate angle from marker orientation
        world_markers[count].matched = 0;
        
        free_points_2f(world_pts);
        count++;
    }
    
    return count;
}

/**
 * Match detected markers to ground truth using nearest-neighbor
 * Returns number of matches
 */
int match_markers_to_ground_truth(DetectedWorldMarker* detected, int detected_count,
                                   const GroundTruthMarker* ground_truth, int gt_count,
                                   int* matched_gt_indices, float* position_errors,
                                   float* angle_errors) {
    int match_count = 0;
    
    // For each ground truth marker, find closest detected marker with same ID
    int* gt_matched = (int*)calloc(gt_count, sizeof(int));
    
    for (int i = 0; i < detected_count; i++) {
        float min_distance = FLT_MAX;
        int best_gt_idx = -1;
        
        for (int j = 0; j < gt_count; j++) {
            // Skip if already matched
            if (gt_matched[j]) continue;
            
            // Check if IDs match
            if (detected[i].marker_id != ground_truth[j].marker_id) continue;
            
            // Calculate distance
            float dist = calculate_distance(detected[i].world_x, detected[i].world_y,
                                           ground_truth[j].world_x, ground_truth[j].world_y);
            
            // Keep track of closest match
            if (dist < min_distance && dist < MATCHING_DISTANCE_THRESHOLD) {
                min_distance = dist;
                best_gt_idx = j;
            }
        }
        
        // If we found a match, record it
        if (best_gt_idx >= 0) {
            detected[i].matched = 1;
            gt_matched[best_gt_idx] = 1;
            matched_gt_indices[match_count] = best_gt_idx;
            position_errors[match_count] = min_distance;
            angle_errors[match_count] = calculate_angle_difference(
                detected[i].angle, ground_truth[best_gt_idx].world_angle);
            match_count++;
        }
    }
    
    free(gt_matched);
    return match_count;
}

/**
 * Calculate error statistics
 */
ErrorStats calculate_error_stats(float* position_errors, float* angle_errors, int count) {
    ErrorStats stats = {0};
    
    if (count == 0) return stats;
    
    // Calculate means and max
    float pos_sum = 0.0f;
    float angle_sum = 0.0f;
    stats.max_position_error = 0.0f;
    stats.max_angle_error = 0.0f;
    
    for (int i = 0; i < count; i++) {
        pos_sum += position_errors[i];
        angle_sum += angle_errors[i];
        
        if (position_errors[i] > stats.max_position_error) {
            stats.max_position_error = position_errors[i];
        }
        if (angle_errors[i] > stats.max_angle_error) {
            stats.max_angle_error = angle_errors[i];
        }
        if (position_errors[i] > OUTLIER_THRESHOLD) {
            stats.outlier_count++;
        }
    }
    
    stats.mean_position_error = pos_sum / count;
    stats.mean_angle_error = angle_sum / count;
    
    // Calculate medians
    stats.median_position_error = calculate_median(position_errors, count);
    stats.median_angle_error = calculate_median(angle_errors, count);
    
    return stats;
}

/**
 * Count unique ground truth positions (ignoring duplicate IDs at same location)
 */
int count_unique_ground_truth_positions() {
    int unique_count = 0;
    
    for (size_t i = 0; i < NUM_GROUND_TRUTH; i++) {
        int is_duplicate = 0;
        
        // Check if this position already counted
        for (size_t j = 0; j < i; j++) {
            if (fabs(GROUND_TRUTH[i].world_x - GROUND_TRUTH[j].world_x) < 1.0f &&
                fabs(GROUND_TRUTH[i].world_y - GROUND_TRUTH[j].world_y) < 1.0f) {
                is_duplicate = 1;
                break;
            }
        }
        
        if (!is_duplicate) {
            unique_count++;
        }
    }
    
    return unique_count;
}

/**
 * Main test function
 */
int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: %s <test_image_path>\n", argv[0]);
        printf("\n");
        printf("Geometric Accuracy Benchmark Test\n");
        printf("----------------------------------\n");
        printf("Tests the accuracy of image-to-world coordinate transformation\n");
        printf("by comparing detected marker positions against known ground truth.\n");
        printf("\n");
        printf("Requirements:\n");
        printf("  - Image must contain fixed markers (IDs 20-23) for homography\n");
        printf("  - Game element markers (IDs 36, 41, 47) at known positions\n");
        printf("\n");
        return 1;
    }
    
    const char* image_path = argv[1];
    
    printf("\n");
    printf("========================================\n");
    printf("   GEOMETRIC ACCURACY BENCHMARK TEST\n");
    printf("========================================\n");
    printf("\n");
    printf("Image: %s\n", image_path);
    
    // Count unique ground truth positions
    int unique_gt_count = count_unique_ground_truth_positions();
    printf("Ground truth positions: %d unique locations\n", unique_gt_count);
    printf("Ground truth entries: %d (including duplicate IDs)\n", (int)NUM_GROUND_TRUTH);
    printf("\n");
    
    // Step 1: Detect markers
    printf("Step 1: Detecting ArUco markers...\n");
    DetectionResult* detections = detect_markers_in_image(image_path);
    if (!detections) {
        printf(COLOR_RED "FAILED: Could not detect markers\n" COLOR_RESET);
        return 1;
    }
    printf("  Raw detections: %d markers\n", detections->count);
    
    // Filter valid markers
    MarkerData filtered[100];
    int valid_count = filter_valid_markers(detections, filtered, 100);
    printf("  Valid markers: %d (after filtering)\n", valid_count);
    
    // Step 2: Compute homography
    printf("\nStep 2: Computing image-to-world homography...\n");
    float* H_inv = compute_inverse_homography(image_path);
    if (!H_inv) {
        releaseDetectionResult(detections);
        printf(COLOR_RED "FAILED: Could not compute homography\n" COLOR_RESET);
        printf("  Make sure fixed markers (IDs 20-23) are visible in the image\n");
        return 1;
    }
    printf("  Homography computed successfully\n");
    
    // Step 3: Transform to world coordinates
    printf("\nStep 3: Transforming to world coordinates...\n");
    DetectedWorldMarker world_markers[100];
    int world_count = transform_markers_to_world(detections, H_inv, world_markers, 100);
    printf("  Transformed %d game element markers\n", world_count);
    
    releaseDetectionResult(detections);
    
    if (world_count == 0) {
        free_matrix(H_inv);
        printf(COLOR_YELLOW "WARNING: No game element markers detected\n" COLOR_RESET);
        return 1;
    }
    
    // Step 4: Match to ground truth
    printf("\nStep 4: Matching to ground truth...\n");
    int matched_indices[100];
    float position_errors[100];
    float angle_errors[100];
    
    int match_count = match_markers_to_ground_truth(world_markers, world_count,
                                                     GROUND_TRUTH, NUM_GROUND_TRUTH,
                                                     matched_indices, position_errors, angle_errors);
    
    printf("  Matched markers: %d\n", match_count);
    
    float detection_rate = (float)match_count / (float)unique_gt_count;
    printf("  Detection rate: %.1f%%\n", detection_rate * 100.0f);
    
    if (match_count == 0) {
        free_matrix(H_inv);
        printf(COLOR_YELLOW "WARNING: No markers matched to ground truth\n" COLOR_RESET);
        return 1;
    }
    
    // Step 5: Calculate statistics
    printf("\n========================================\n");
    printf("   ACCURACY METRICS\n");
    printf("========================================\n");
    
    ErrorStats stats = calculate_error_stats(position_errors, angle_errors, match_count);
    
    printf("\nPosition Accuracy (mm):\n");
    printf("  Mean error:   %.1f mm\n", stats.mean_position_error);
    printf("  Median error: %.1f mm\n", stats.median_position_error);
    printf("  Max error:    %.1f mm\n", stats.max_position_error);
    printf("  Outliers (>%.0fmm): %d\n", OUTLIER_THRESHOLD, stats.outlier_count);
    
    printf("\nAngle Accuracy (degrees):\n");
    printf("  Mean error:   %.1f°\n", stats.mean_angle_error);
    printf("  Median error: %.1f°\n", stats.median_angle_error);
    printf("  Max error:    %.1f°\n", stats.max_angle_error);
    
    // Show outliers
    if (stats.outlier_count > 0) {
        printf("\n" COLOR_YELLOW "Outliers (error > %.0fmm):\n" COLOR_RESET, OUTLIER_THRESHOLD);
        for (int i = 0; i < match_count; i++) {
            if (position_errors[i] > OUTLIER_THRESHOLD) {
                int gt_idx = matched_indices[i];
                printf("  ID %d @ %s: expected (%.0f, %.0f) detected (%.0f, %.0f) error=%.1fmm\n",
                       GROUND_TRUTH[gt_idx].marker_id,
                       GROUND_TRUTH[gt_idx].zone_name,
                       GROUND_TRUTH[gt_idx].world_x,
                       GROUND_TRUTH[gt_idx].world_y,
                       world_markers[i].world_x,
                       world_markers[i].world_y,
                       position_errors[i]);
            }
        }
    }
    
    // Show missing markers
    int missing_count = unique_gt_count - match_count;
    if (missing_count > 0) {
        printf("\n" COLOR_YELLOW "Missing markers: %d\n" COLOR_RESET, missing_count);
        // We could list them but with duplicate IDs it's complex
    }
    
    // Determine pass/fail
    printf("\n========================================\n");
    printf("   TEST RESULTS\n");
    printf("========================================\n");
    
    int passed = 1;
    
    printf("\nCriteria:\n");
    
    // Detection rate
    if (detection_rate >= MIN_DETECTION_RATE) {
        printf(COLOR_GREEN "  ✓ Detection rate: %.1f%% (>= %.0f%%)\n" COLOR_RESET,
               detection_rate * 100.0f, MIN_DETECTION_RATE * 100.0f);
    } else {
        printf(COLOR_RED "  ✗ Detection rate: %.1f%% (< %.0f%%)\n" COLOR_RESET,
               detection_rate * 100.0f, MIN_DETECTION_RATE * 100.0f);
        passed = 0;
    }
    
    // Mean position error
    if (stats.mean_position_error <= MAX_MEAN_POSITION_ERROR) {
        printf(COLOR_GREEN "  ✓ Mean position error: %.1fmm (<= %.0fmm)\n" COLOR_RESET,
               stats.mean_position_error, MAX_MEAN_POSITION_ERROR);
    } else {
        printf(COLOR_RED "  ✗ Mean position error: %.1fmm (> %.0fmm)\n" COLOR_RESET,
               stats.mean_position_error, MAX_MEAN_POSITION_ERROR);
        passed = 0;
    }
    
    // Max position error
    if (stats.max_position_error <= MAX_MAX_POSITION_ERROR) {
        printf(COLOR_GREEN "  ✓ Max position error: %.1fmm (<= %.0fmm)\n" COLOR_RESET,
               stats.max_position_error, MAX_MAX_POSITION_ERROR);
    } else {
        printf(COLOR_RED "  ✗ Max position error: %.1fmm (> %.0fmm)\n" COLOR_RESET,
               stats.max_position_error, MAX_MAX_POSITION_ERROR);
        passed = 0;
    }
    
    // Mean angle error
    if (stats.mean_angle_error <= MAX_MEAN_ANGLE_ERROR) {
        printf(COLOR_GREEN "  ✓ Mean angle error: %.1f° (<= %.0f°)\n" COLOR_RESET,
               stats.mean_angle_error, MAX_MEAN_ANGLE_ERROR);
    } else {
        printf(COLOR_RED "  ✗ Mean angle error: %.1f° (> %.0f°)\n" COLOR_RESET,
               stats.mean_angle_error, MAX_MEAN_ANGLE_ERROR);
        passed = 0;
    }
    
    printf("\n");
    if (passed) {
        printf(COLOR_GREEN "Status: PASSED ✓\n" COLOR_RESET);
    } else {
        printf(COLOR_RED "Status: FAILED ✗\n" COLOR_RESET);
    }
    
    printf("\n========================================\n\n");
    
    // Cleanup
    free_matrix(H_inv);
    
    return passed ? 0 : 1;
}

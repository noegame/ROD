/**
 * test_aruco_detection_pipeline.c
 * 
 * Test program for ArUco detection pipeline using OpenCV wrapper.
 * This follows the same detection pipeline as the Python implementation:
 * - Load image
 * - Apply sharpening filter
 * - Resize image (scale 1.5x)
 * - Detect ArUco markers
 * - Calculate marker centers
 * - Annotate image with IDs, centers, and counter
 * - Save annotated image
 */

#define _POSIX_C_SOURCE 199309L  // Required for clock_gettime and CLOCK_MONOTONIC

#include "opencv_wrapper.h"
#include "rod_cv.h"
#include "rod_config.h"
#include "rod_visualization.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <stdbool.h>

// Camera calibration parameters (from get_camera_matrix and get_distortion_matrix in aruco.py)
static float CAMERA_MATRIX[9] = {
    2493.62477, 0.0, 1977.18701,
    0.0, 2493.11358, 2034.91176,
    0.0, 0.0, 1.0
};

static float DIST_COEFFS[4] = {
    -0.1203345, 0.06802544, -0.13779641, 0.08243704
};

// Structure to hold marker center coordinates (used for real_coords annotation)
typedef struct {
    float x;
    float y;
} MarkerCenter;

/**
 * Calculate the center coordinates of a marker from its corners
 * (wrapper for test-specific MarkerCenter type)
 */
MarkerCenter get_marker_center_test(DetectedMarker* marker) {
    MarkerCenter center;
    Point2f pt = calculate_marker_center(marker->corners);
    center.x = pt.x;
    center.y = pt.y;
    return center;
}

/**
 * Annotate image with real-world coordinates
 * Follows the logic from annotate_img_with_real_coords in aruco.py
 * This is specific to test_rod_cv and uses 3D coordinates
 */
void annotate_with_real_coords(ImageHandle* image, MarkerCenter* centers, 
                                Point3f* real_coords, int count) {
    Color black = {0, 0, 0};
    Color cyan = {255, 255, 0};
    double font_scale = 0.4;
    
    for (int i = 0; i < count; i++) {
        char text[64];
        snprintf(text, sizeof(text), "(%d,%d,%d)mm", 
                 (int)real_coords[i].x, (int)real_coords[i].y, (int)real_coords[i].z);
        
        int x = (int)centers[i].x + 50;
        int y = (int)centers[i].y;
        
        // Black outline
        put_text(image, text, x, y, font_scale, black, 3);
        // Cyan text
        put_text(image, text, x, y, font_scale, cyan, 1);
    }
}

/**
 * @brief Get current time in milliseconds
 * @return Time in milliseconds  
 */
static double get_time_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1000000.0;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: %s <image_path> [output_path]\n", argv[0]);
        printf("  image_path: Input image to process\n");
        printf("  output_path: Path to save annotated image (default: output_annotated.jpg)\n");
        printf("\nThis program follows the Python ArUco detection pipeline:\n");
        printf("  1. Load image\n");
        printf("  2. Apply sharpening filter\n");
        printf("  3. Create and apply field mask\n");
        printf("  4. Resize image (1.5x scale)\n");
        printf("  5. Detect ArUco markers (DICT_4X4_50)\n");
        printf("  6. Calculate marker centers\n");
        printf("  7. Annotate image with IDs, centers, and counter\n");
        printf("  8. Save annotated image (RGB format)\n");
        return -1;
    }

    const char* input_path = argv[1];
    const char* output_path = (argc >= 3) ? argv[2] : "output_annotated.jpg";
    
    printf("=== ArUco Detection Pipeline Test ===\n\n");
    
    double t_total_start = get_time_ms();
    
    // ========== STEP 1: LOAD IMAGE ==========
    double t_load_start = get_time_ms();
    printf("[1/8] Loading image: %s\n", input_path);
    ImageHandle* image = load_image(input_path);
    if (image == NULL) {
        fprintf(stderr, "Error: Could not load image from %s\n", input_path);
        return -1;
    }
    
    int orig_width = get_image_width(image);
    int orig_height = get_image_height(image);
    double t_load_end = get_time_ms();
    printf("      Image loaded: %dx%d pixels (%.1fms)\n", orig_width, orig_height, t_load_end - t_load_start);
    
    // ========== STEP 2: APPLY SHARPENING ==========
    double t_sharpen_start = get_time_ms();
    printf("[2/8] Applying sharpening filter...\n");
    ImageHandle* sharpened = sharpen_image(image);
    if (sharpened == NULL) {
        fprintf(stderr, "Error: Failed to sharpen image\n");
        release_image(image);
        return -1;
    }
    double t_sharpen_end = get_time_ms();
    printf("      Sharpening applied (%.1fms)\n", t_sharpen_end - t_sharpen_start);
    
    // Need to create detector early for mask creation
    ArucoDictionaryHandle* dictionary = getPredefinedDictionary(rod_config_get_aruco_dictionary_type());
    if (dictionary == NULL) {
        fprintf(stderr, "Error: Could not create ArUco dictionary\n");
        release_image(image);
        release_image(sharpened);
        return -1;
    }
    
    DetectorParametersHandle* params = createDetectorParameters();
    if (params == NULL) {
        fprintf(stderr, "Error: Could not create detector parameters\n");
        releaseArucoDictionary(dictionary);
        release_image(image);
        release_image(sharpened);
        return -1;
    }
    
    rod_config_configure_detector_parameters(params);
    
    ArucoDetectorHandle* detector = createArucoDetector(dictionary, params);
    if (detector == NULL) {
        fprintf(stderr, "Error: Could not create ArUco detector\n");
        releaseDetectorParameters(params);
        releaseArucoDictionary(dictionary);
        release_image(image);
        release_image(sharpened);
        return -1;
    }
    
    // ========== STEP 3: CREATE AND APPLY FIELD MASK ==========
    double t_mask_start = get_time_ms();
    printf("[3/8] Creating and applying field mask...\n");
    
    ImageHandle* field_mask = create_field_mask_from_image(sharpened, detector, orig_width, orig_height, 1.1f, NULL);
    ImageHandle* masked_image = sharpened;  // Default to sharpened if mask creation fails
    
    if (field_mask) {
        printf("      Field mask created successfully\n");
        ImageHandle* temp_masked = bitwise_and_mask(sharpened, field_mask);
        if (temp_masked) {
            release_image(sharpened);
            masked_image = temp_masked;
            printf("      Field mask applied\n");
        } else {
            fprintf(stderr, "      Warning: Failed to apply mask, using unmasked image\n");
        }
        release_image(field_mask);
    } else {
        printf("      Warning: Could not create field mask (need 4 fixed markers), proceeding without mask\n");
    }
    
    double t_mask_end = get_time_ms();
    printf("      Masking complete (%.1fms)\n", t_mask_end - t_mask_start);
    
    release_image(image);  // Free original image (no longer needed)
    
    // ========== STEP 4: RESIZE IMAGE ==========
    double t_resize_start = get_time_ms();
    float scale = 1.5f;
    int new_width = (int)(orig_width * scale);
    int new_height = (int)(orig_height * scale);
    
    printf("[4/8] Resizing image (scale: %.1fx)\n", scale);
    ImageHandle* resized = resize_image(masked_image, new_width, new_height);
    if (resized == NULL) {
        fprintf(stderr, "Error: Failed to resize image\n");
        release_image(masked_image);
        releaseArucoDetector(detector);
        releaseDetectorParameters(params);
        releaseArucoDictionary(dictionary);
        return -1;
    }
    release_image(masked_image);  // Free masked image
    double t_resize_end = get_time_ms();
    printf("      Resized to: %dx%d pixels (%.1fms)\n", new_width, new_height, t_resize_end - t_resize_start);
    
    // ========== STEP 5: DETECT MARKERS ==========
    double t_detect_start = get_time_ms();
    printf("[5/8] Detecting ArUco markers (DICT_4X4_50)...\n");
    
    // Detect markers on resized image
    DetectionResult* result_raw = detectMarkersWithConfidence(detector, resized);
    if (result_raw == NULL) {
        fprintf(stderr, "Error: Detection failed\n");
        releaseArucoDetector(detector);
        releaseDetectorParameters(params);
        releaseArucoDictionary(dictionary);
        release_image(resized);
        return -1;
    }
    
    double t_detect_end = get_time_ms();
    printf("      Detected %d marker(s) (raw) (%.1fms)\n", result_raw->count, t_detect_end - t_detect_start);
    
    // Filter to keep only valid marker IDs using rod_cv module
    MarkerData markers_filtered[100];  // Max 100 markers
    int valid_count = filter_valid_markers(result_raw, markers_filtered, 100);
    int rejected_count = result_raw->count - valid_count;
    
    printf("      Filtered to %d valid marker(s) (rejected %d invalid ID(s))\n", 
           valid_count, rejected_count);
    
    // ========== STEP 6: CALCULATE CENTERS (for display) ==========
    double t_process_start = get_time_ms();
    printf("[6/8] Calculating marker centers...\n");
    
    MarkerCenter* centers = NULL;
    if (valid_count > 0) {
        centers = (MarkerCenter*)malloc(sizeof(MarkerCenter) * valid_count);
        if (centers == NULL) {
            fprintf(stderr, "Error: Memory allocation failed\n");
            releaseDetectionResult(result_raw);
            releaseArucoDetector(detector);
            releaseDetectorParameters(params);
            releaseArucoDictionary(dictionary);
            release_image(resized);
            return -1;
        }
        
        for (int i = 0; i < valid_count; i++) {
            // Scale centers back to original coordinates (divide by scale)
            centers[i].x = markers_filtered[i].x / scale;
            centers[i].y = markers_filtered[i].y / scale;
            
            printf("      Marker ID %d: center at (%.1f, %.1f)\n", 
                   markers_filtered[i].id, centers[i].x, centers[i].y);
        }
    }
    
    // Release resized image as we need to work with original size
    release_image(resized);
    
    // Scale corner coordinates back to original size in DetectionResult
    for (int i = 0; i < result_raw->count; i++) {
        for (int j = 0; j < 4; j++) {
            result_raw->markers[i].corners[j][0] /= scale;
            result_raw->markers[i].corners[j][1] /= scale;
        }
    }
    
    double t_process_end = get_time_ms();
    printf("      Centers calculated (%.1fms)\n", t_process_end - t_process_start);
    
    // Reload original image for annotation
    double t_reload_start = get_time_ms();
    image = load_image(input_path);
    if (image == NULL) {
        fprintf(stderr, "Error: Could not reload original image\n");
        if (centers) free(centers);
        releaseDetectionResult(result_raw);
        releaseArucoDetector(detector);
        releaseDetectorParameters(params);
        releaseArucoDictionary(dictionary);
        return -1;
    }
    
    // Scale marker coordinates back to original size for annotation
    MarkerData markers_scaled[100];
    for (int i = 0; i < valid_count; i++) {
        markers_scaled[i].id = markers_filtered[i].id;
        markers_scaled[i].x = markers_filtered[i].x / scale;
        markers_scaled[i].y = markers_filtered[i].y / scale;
        markers_scaled[i].angle = markers_filtered[i].angle;
    }
    
    double t_reload_end = get_time_ms();
    printf("      Image reloaded (%.1fms)\n", t_reload_end - t_reload_start);
    
    // ========== STEP 7: ANNOTATE IMAGE ==========
    double t_annotate_start = get_time_ms();
    printf("[7/8] Annotating image using rod_visualization module...\n");
    
    // Count markers by category using rod_cv module
    MarkerCounts counts = count_markers_by_category(markers_scaled, valid_count);
    
    if (valid_count > 0) {
        rod_viz_annotate_with_colored_quadrilaterals(image, result_raw);
        rod_viz_annotate_with_counter(image, counts);
        rod_viz_annotate_with_full_info(image, markers_scaled, valid_count);
        printf("      Annotations added: colored quadrilaterals, categorized counts, full marker info (ID, x, y, angle)\n");
    } else {
        rod_viz_annotate_with_counter(image, counts);  // Still show counts (all zeros)
        printf("      No markers to annotate\n");
    }
    
    double t_annotate_end = get_time_ms();
    printf("      Annotations complete (%.1fms)\n", t_annotate_end - t_annotate_start);
    
    // ========== STEP 8: SAVE ANNOTATED IMAGE (RGB FORMAT) ==========
    double t_save_start = get_time_ms();
    printf("[8/8] Saving annotated image to: %s\n", output_path);
    
    // Convert BGR to RGB for output (OpenCV always loads images as BGR internally)
    printf("      Converting BGR to RGB...\n");
    ImageHandle* image_rgb = convert_bgr_to_rgb(image);
    bool conversion_success = (image_rgb != NULL);
    if (image_rgb == NULL) {
        fprintf(stderr, "ERROR: BGR→RGB conversion failed! Check opencv_wrapper implementation.\n");
        fprintf(stderr, "       Saving in BGR format - colors will be swapped in viewers\n");
        image_rgb = image;  // Fallback to original
    } else {
        printf("      BGR→RGB conversion successful\n");
        release_image(image);  // Free BGR version
        image = image_rgb;     // Use RGB version
    }
    
    double t_save_end;
    if (save_image(output_path, image)) {
        t_save_end = get_time_ms();
        if (conversion_success) {
            printf("      Annotated image saved successfully in RGB format (%.1fms)\n", t_save_end - t_save_start);
        } else {
            printf("      Annotated image saved in BGR format (%.1fms)\n", t_save_end - t_save_start);
        }
    } else {
        t_save_end = get_time_ms();
        fprintf(stderr, "Error: Failed to save annotated image\n");
    }
    
    double t_total_end = get_time_ms();
    
    // ========== TIMING SUMMARY ==========
    printf("\n=== Timing Summary ===\n");
    printf("Load:      %.1fms\n", t_load_end - t_load_start);
    printf("Sharpen:   %.1fms\n", t_sharpen_end - t_sharpen_start);
    printf("Mask:      %.1fms\n", t_mask_end - t_mask_start);
    printf("Resize:    %.1fms\n", t_resize_end - t_resize_start);
    printf("Detect:    %.1fms\n", t_detect_end - t_detect_start);
    printf("Process:   %.1fms\n", t_process_end - t_process_start);
    printf("Reload:    %.1fms\n", t_reload_end - t_reload_start);
    printf("Annotate:  %.1fms\n", t_annotate_end - t_annotate_start);
    printf("Save:      %.1fms\n", t_save_end - t_save_start);
    printf("TOTAL:     %.1fms\n", t_total_end - t_total_start);
    
    // ========== RESULTS SUMMARY ==========
    printf("\n=== Detection Results Summary ===\n");
    printf("Black markers  : %d\n", counts.black_markers);
    printf("Blue markers   : %d\n", counts.blue_markers);
    printf("Yellow markers : %d\n", counts.yellow_markers);
    printf("Robots markers : %d\n", counts.robot_markers);
    printf("Fixed markers  : %d\n", counts.fixed_markers);
    printf("Total markers  : %d\n\n", counts.total);
    
    for (int i = 0; i < valid_count; i++) {
        printf("Marker #%d:\n", i + 1);
        printf("  ID: %d\n", markers_scaled[i].id);
        printf("  Center: (%.1f, %.1f)\n", centers[i].x, centers[i].y);
        printf("  Angle: %.3f radians\n\n", markers_scaled[i].angle);
    }
    
    // ========== CLEANUP ==========
    if (centers) free(centers);
    releaseDetectionResult(result_raw);
    releaseArucoDetector(detector);
    releaseDetectorParameters(params);
    releaseArucoDictionary(dictionary);
    release_image(image);
    
    printf("Pipeline test complete!\n");
    return 0;
}

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

#include "opencv_wrapper.h"
#include "rod_cv.h"
#include "rod_config.h"
#include "rod_visualization.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

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

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: %s <image_path> [output_path]\n", argv[0]);
        printf("  image_path: Input image to process\n");
        printf("  output_path: Path to save annotated image (default: output_annotated.jpg)\n");
        printf("\nThis program follows the Python ArUco detection pipeline:\n");
        printf("  1. Load image\n");
        printf("  2. Apply sharpening filter\n");
        printf("  3. Resize image (1.5x scale)\n");
        printf("  4. Detect ArUco markers (DICT_4X4_50)\n");
        printf("  5. Calculate marker centers\n");
        printf("  6. Annotate image with IDs, centers, and counter\n");
        printf("  7. Save annotated image\n");
        return -1;
    }

    const char* input_path = argv[1];
    const char* output_path = (argc >= 3) ? argv[2] : "output_annotated.jpg";
    
    printf("=== ArUco Detection Pipeline Test ===\n\n");
    
    // ========== STEP 1: LOAD IMAGE ==========
    printf("[1/7] Loading image: %s\n", input_path);
    ImageHandle* image = load_image(input_path);
    if (image == NULL) {
        fprintf(stderr, "Error: Could not load image from %s\n", input_path);
        return -1;
    }
    
    int orig_width = get_image_width(image);
    int orig_height = get_image_height(image);
    printf("      Image loaded: %dx%d pixels\n", orig_width, orig_height);
    
    // ========== STEP 2: APPLY SHARPENING ==========
    printf("[2/7] Applying sharpening filter...\n");
    ImageHandle* sharpened = sharpen_image(image);
    if (sharpened == NULL) {
        fprintf(stderr, "Error: Failed to sharpen image\n");
        release_image(image);
        return -1;
    }
    release_image(image);  // Free original image
    printf("      Sharpening applied\n");
    
    // ========== STEP 3: RESIZE IMAGE ==========
    float scale = 1.5f;
    int new_width = (int)(orig_width * scale);
    int new_height = (int)(orig_height * scale);
    
    printf("[3/7] Resizing image (scale: %.1fx)\n", scale);
    ImageHandle* resized = resize_image(sharpened, new_width, new_height);
    if (resized == NULL) {
        fprintf(stderr, "Error: Failed to resize image\n");
        release_image(sharpened);
        return -1;
    }
    release_image(sharpened);  // Free sharpened image
    printf("      Resized to: %dx%d pixels\n", new_width, new_height);
    
    // ========== STEP 4: DETECT MARKERS ==========
    printf("[4/7] Detecting ArUco markers (DICT_4X4_50)...\n");
    
    // Create detector with DICT_4X4_50 (same as Python code)
    ArucoDictionaryHandle* dictionary = getPredefinedDictionary(rod_config_get_aruco_dictionary_type());
    if (dictionary == NULL) {
        fprintf(stderr, "Error: Could not create ArUco dictionary\n");
        release_image(resized);
        return -1;
    }
    
    DetectorParametersHandle* params = createDetectorParameters();
    if (params == NULL) {
        fprintf(stderr, "Error: Could not create detector parameters\n");
        releaseArucoDictionary(dictionary);
        release_image(resized);
        return -1;
    }
    
    // Configure detector parameters to match Python implementation (using rod_config module)
    // This enables detection of ~40 markers instead of just 7
    rod_config_configure_detector_parameters(params);
    printf("      Detector parameters configured (using rod_config module)\n");
    
    ArucoDetectorHandle* detector = createArucoDetector(dictionary, params);
    if (detector == NULL) {
        fprintf(stderr, "Error: Could not create ArUco detector\n");
        releaseDetectorParameters(params);
        releaseArucoDictionary(dictionary);
        release_image(resized);
        return -1;
    }
    
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
    
    printf("      Detected %d marker(s) (raw)\n", result_raw->count);
    
    // Filter to keep only valid marker IDs using rod_cv module
    MarkerData markers_filtered[100];  // Max 100 markers
    int valid_count = filter_valid_markers(result_raw, markers_filtered, 100);
    int rejected_count = result_raw->count - valid_count;
    
    printf("      Filtered to %d valid marker(s) (rejected %d invalid ID(s))\n", 
           valid_count, rejected_count);
    
    // ========== STEP 5: CALCULATE CENTERS (for display) ==========
    printf("[5/7] Calculating marker centers...\n");
    
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
    
    // Reload original image for annotation
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
    
    // ========== STEP 6: ANNOTATE IMAGE ==========
    printf("[6/7] Annotating image using rod_visualization module...\n");
    
    // Count markers by category using rod_cv module
    MarkerCounts counts = count_markers_by_category(markers_scaled, valid_count);
    
    if (valid_count > 0) {
        rod_viz_annotate_with_counter(image, counts);
        rod_viz_annotate_with_ids(image, markers_scaled, valid_count);
        rod_viz_annotate_with_centers(image, markers_scaled, valid_count);
        printf("      Annotations added: categorized counts, IDs, centers\n");
    } else {
        rod_viz_annotate_with_counter(image, counts);  // Still show counts (all zeros)
        printf("      No markers to annotate\n");
    }
    
    // ========== STEP 7: SAVE ANNOTATED IMAGE ==========
    printf("[7/7] Saving annotated image to: %s\n", output_path);
    
    if (save_image(output_path, image)) {
        printf("      Annotated image saved successfully!\n");
    } else {
        fprintf(stderr, "Error: Failed to save annotated image\n");
    }
    
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

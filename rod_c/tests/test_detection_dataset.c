/**
 * test_detection_dataset.c
 * 
 * Tests detection accuracy on known dataset with ground truth.
 * - Loads test images from a known folder
 * - Runs full detection pipeline (sharpen → resize → detect)
 * - Compares detected marker counts against expected values
 * - Regression test to prevent algorithm degradation
 * 
 * This test validates that detection parameters remain effective
 * and that code changes don't break marker detection.
 */

#include "opencv_wrapper.h"
#include "rod_cv.h"
#include "rod_config.h"
#include "rod_visualization.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Test case counter
static int test_passed = 0;
static int test_failed = 0;

// Helper macro for test assertions
#define TEST_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            fprintf(stderr, "    FAILED: %s\n", message); \
            return -1; \
        } \
    } while(0)

/**
 * Test case structure: image path + expected marker count
 */
typedef struct {
    const char* image_path;
    int expected_markers;  // Expected number of VALID markers
    int tolerance;         // Allowed difference (+/-)
} TestImage;

/**
 * Run detection pipeline on a single image
 * Returns number of valid markers detected
 */
int detect_markers_in_image(const char* image_path, int* total_detected) {
    // Load image
    ImageHandle* image = load_image(image_path);
    if (!image) {
        fprintf(stderr, "Failed to load image: %s\n", image_path);
        return -1;
    }
    
    // Sharpen
    ImageHandle* sharpened = sharpen_image(image);
    release_image(image);
    if (!sharpened) {
        fprintf(stderr, "Failed to sharpen image\n");
        return -1;
    }
    
    // Resize 1.5x
    int orig_w = get_image_width(sharpened);
    int orig_h = get_image_height(sharpened);
    int new_w = (int)(orig_w * 1.5f);
    int new_h = (int)(orig_h * 1.5f);
    
    ImageHandle* resized = resize_image(sharpened, new_w, new_h);
    release_image(sharpened);
    if (!resized) {
        fprintf(stderr, "Failed to resize image\n");
        return -1;
    }
    
    // Create detector
    ArucoDictionaryHandle* dictionary = getPredefinedDictionary(rod_config_get_aruco_dictionary_type());
    if (!dictionary) {
        release_image(resized);
        return -1;
    }
    
    DetectorParametersHandle* params = createDetectorParameters();
    if (!params) {
        releaseArucoDictionary(dictionary);
        release_image(resized);
        return -1;
    }
    
    // Configure critical parameters (same as production)
    rod_config_configure_detector_parameters(params);
    
    ArucoDetectorHandle* detector = createArucoDetector(dictionary, params);
    if (!detector) {
        releaseDetectorParameters(params);
        releaseArucoDictionary(dictionary);
        release_image(resized);
        return -1;
    }
    
    // Detect markers
    DetectionResult* result = detectMarkersWithConfidence(detector, resized);
    if (!result) {
        releaseArucoDetector(detector);
        releaseDetectorParameters(params);
        releaseArucoDictionary(dictionary);
        release_image(resized);
        return -1;
    }
    
    int raw_count = result->count;
    if (total_detected) {
        *total_detected = raw_count;
    }
    
    // Filter valid markers
    MarkerData filtered_markers[100];
    int valid_count = filter_valid_markers(result, filtered_markers, 100);
    
    // Cleanup
    releaseDetectionResult(result);
    releaseArucoDetector(detector);
    releaseDetectorParameters(params);
    releaseArucoDictionary(dictionary);
    release_image(resized);
    
    return valid_count;
}

/**
 * Test a single image against expected marker count
 */
int test_single_image(const TestImage* test) {
    int total_detected = 0;
    int valid_detected = detect_markers_in_image(test->image_path, &total_detected);
    
    if (valid_detected < 0) {
        printf("\n      Error processing image\n");
        return -1;
    }
    
    int difference = abs(valid_detected - test->expected_markers);
    
    printf("\n      Total detected: %d | Valid: %d | Expected: %d (±%d)",
           total_detected, valid_detected, test->expected_markers, test->tolerance);
    
    if (difference > test->tolerance) {
        printf(" | OUTSIDE TOLERANCE");
        return -1;
    }
    
    printf(" | OK");
    return 0;
}

/**
 * Dataset: Known test images with expected marker counts
 * 
 * NOTE: These values should be validated manually first.
 * Run test_aruco_detection.c on each image to get ground truth.
 */
static const TestImage TEST_DATASET[] = {
    // Add your test images here with expected counts
    // Format: {path, expected_valid_markers, tolerance}
    
    // Example (adjust based on actual images in your dataset):
    // {"pictures/camera/2026-01-16-playground-ready/IMG_1415.JPG", 42, 2},
    // {"pictures/camera/2026-01-16-playground-ready/IMG_1416.JPG", 38, 2},
    
    // For now, we'll use a placeholder that checks if we can detect at least some markers
    {"pictures/camera/2026-01-16-playground-ready/IMG_1415.JPG", 30, 15}  // Very tolerant for initial test
};

#define NUM_DATASET_TESTS (sizeof(TEST_DATASET) / sizeof(TestImage))

/**
 * Test: Run detection on entire dataset
 */
int test_detection_dataset() {
    if (NUM_DATASET_TESTS == 0) {
        printf("\n      WARNING: No test images configured in dataset\n");
        printf("      Add images to TEST_DATASET array with expected marker counts\n");
        return 0;  // Pass if no tests configured (not a failure)
    }
    
    int passed = 0;
    int failed = 0;
    
    for (size_t i = 0; i < NUM_DATASET_TESTS; i++) {
        printf("\n    Image %zu/%zu: %s", 
               i + 1, NUM_DATASET_TESTS, TEST_DATASET[i].image_path);
        
        if (test_single_image(&TEST_DATASET[i]) == 0) {
            passed++;
        } else {
            failed++;
        }
    }
    
    printf("\n    Dataset results: %d passed, %d failed\n", passed, failed);
    
    return (failed == 0) ? 0 : -1;
}

/**
 * Test: Verify detector parameters can be configured
 */
int test_detector_parameters() {
    DetectorParametersHandle* params = createDetectorParameters();
    TEST_ASSERT(params != NULL, "Failed to create detector parameters");
    
    // Configure parameters using rod_config
    rod_config_configure_detector_parameters(params);
    
    printf("\n      Detector parameters configured successfully");
    printf("\n      (Parameters are validated through detection results)");
    
    releaseDetectorParameters(params);
    
    return 0;
}

/**
 * Test: Verify sharpen filter is working
 */
int test_sharpen_filter() {
    // Load a test image
    const char* test_image = "pictures/camera/2026-01-16-playground-ready/IMG_1415.JPG";
    
    ImageHandle* original = load_image(test_image);
    TEST_ASSERT(original != NULL, "Failed to load test image");
    
    ImageHandle* sharpened = sharpen_image(original);
    TEST_ASSERT(sharpened != NULL, "Sharpen filter failed");
    
    // Verify dimensions remain the same
    int orig_w = get_image_width(original);
    int orig_h = get_image_height(original);
    int sharp_w = get_image_width(sharpened);
    int sharp_h = get_image_height(sharpened);
    
    TEST_ASSERT(orig_w == sharp_w && orig_h == sharp_h, 
                "Sharpen changed image dimensions");
    
    printf("\n      Sharpen filter preserved dimensions: %dx%d", orig_w, orig_h);
    
    release_image(original);
    release_image(sharpened);
    
    return 0;
}

/**
 * Test: Verify resize scales correctly
 */
int test_resize_scaling() {
    const char* test_image = "pictures/camera/2026-01-16-playground-ready/IMG_1415.JPG";
    
    ImageHandle* original = load_image(test_image);
    TEST_ASSERT(original != NULL, "Failed to load test image");
    
    int orig_w = get_image_width(original);
    int orig_h = get_image_height(original);
    
    // Resize by 1.5x
    int new_w = (int)(orig_w * 1.5f);
    int new_h = (int)(orig_h * 1.5f);
    
    ImageHandle* resized = resize_image(original, new_w, new_h);
    TEST_ASSERT(resized != NULL, "Resize failed");
    
    int resized_w = get_image_width(resized);
    int resized_h = get_image_height(resized);
    
    TEST_ASSERT(resized_w == new_w && resized_h == new_h,
                "Resize produced incorrect dimensions");
    
    printf("\n      Resize successful: %dx%d → %dx%d (1.5x scale)",
           orig_w, orig_h, resized_w, resized_h);
    
    release_image(original);
    release_image(resized);
    
    return 0;
}

// Test suite definition
typedef struct {
    const char* name;
    int (*func)();
} TestCase;

static const TestCase TESTS[] = {
    {"Detector parameters verification", test_detector_parameters},
    {"Sharpen filter functionality", test_sharpen_filter},
    {"Resize scaling accuracy", test_resize_scaling},
    {"Detection on dataset", test_detection_dataset}
};

#define NUM_TESTS (sizeof(TESTS) / sizeof(TestCase))

int main(int argc, char* argv[]) {
    printf("========================================\n");
    printf("ArUco Detection Dataset Test\n");
    printf("========================================\n");
    printf("Testing: Detection accuracy and pipeline\n");
    printf("Number of tests: %d\n", NUM_TESTS);
    printf("Dataset images: %zu\n", NUM_DATASET_TESTS);
    printf("========================================\n\n");
    
    for (size_t i = 0; i < NUM_TESTS; i++) {
        printf("[%zu/%d] %s...", i + 1, NUM_TESTS, TESTS[i].name);
        fflush(stdout);
        
        if (TESTS[i].func() == 0) {
            printf(" PASS\n");
            test_passed++;
        } else {
            printf(" FAIL\n");
            test_failed++;
        }
    }
    
    printf("\n========================================\n");
    printf("Results: %d passed, %d failed\n", test_passed, test_failed);
    printf("========================================\n");
    
    if (test_failed > 0) {
        printf("\nNOTE: If dataset tests fail, verify expected marker counts\n");
        printf("by running test_aruco_detection on each test image first.\n");
    }
    
    return (test_failed == 0) ? 0 : 1;
}

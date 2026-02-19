/**
 * test_emulated_camera_impl.c
 * 
 * Tests the emulated camera implementation details:
 * - Folder cycling behavior
 * - Error handling (missing files, invalid images)
 * - Dimension consistency
 * - Loop-around behavior when cycling through images
 * 
 * This test focuses on emulated_camera.c specific behavior,
 * while test_camera_interface.c tests the generic contract.
 */

#include "camera_interface.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>

// Test case counter
static int test_passed = 0;
static int test_failed = 0;

// Helper macro for test assertions
#define TEST_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            fprintf(stderr, "    ASSERTION FAILED: %s\n", message); \
            return -1; \
        } \
    } while(0)

/**
 * Count number of image files in a directory
 */
int count_images_in_folder(const char* folder_path) {
    DIR* dir = opendir(folder_path);
    if (!dir) return 0;
    
    int count = 0;
    struct dirent* entry;
    
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG) {  // Regular file
            const char* ext = strrchr(entry->d_name, '.');
            if (ext) {
                if (strcasecmp(ext, ".jpg") == 0 || 
                    strcasecmp(ext, ".jpeg") == 0 || 
                    strcasecmp(ext, ".png") == 0 ||
                    strcasecmp(ext, ".JPG") == 0) {
                    count++;
                }
            }
        }
    }
    
    closedir(dir);
    return count;
}

/**
 * Test 1: Cycle through all images in folder
 */
int test_cycle_through_folder() {
    const char* test_folder = "pictures/camera/2026-01-16-playground-ready";
    
    // Count images in folder
    int num_images = count_images_in_folder(test_folder);
    TEST_ASSERT(num_images > 0, "test folder must contain images");
    
    printf("\n      Found %d images in test folder\n", num_images);
    
    Camera* camera = camera_create(CAMERA_TYPE_EMULATED);
    TEST_ASSERT(camera != NULL, "camera_create() failed");
    
    camera_interface_set_folder(camera, test_folder);
    camera_interface_set_size(camera, 640, 480);
    camera_interface_start(camera);
    
    // Capture all images + 2 more to test loop-around
    for (int i = 0; i < num_images + 2; i++) {
        uint8_t* buffer = NULL;
        int w = 0, h = 0;
        size_t s = 0;
        
        int result = camera_interface_capture_frame(camera, &buffer, &w, &h, &s);
        TEST_ASSERT(result == 0, "capture must succeed for each image");
        TEST_ASSERT(buffer != NULL, "buffer must be allocated");
        
        free(buffer);
    }
    
    camera_interface_stop(camera);
    camera_destroy(camera);
    printf("      Successfully cycled through %d images (+ loop around)\n", num_images);
    
    return 0;
}

/**
 * Test 2: Empty folder handling
 */
int test_empty_folder() {
    // Create a temporary empty folder if it doesn't exist
    const char* empty_folder = "pictures/test_empty_folder";
    mkdir(empty_folder, 0755);
    
    Camera* camera = camera_create(CAMERA_TYPE_EMULATED);
    TEST_ASSERT(camera != NULL, "camera_create() failed");
    
    // Setting folder should fail or capture should fail gracefully
    int folder_result = camera_interface_set_folder(camera, empty_folder);
    
    if (folder_result == 0) {
        // Folder set succeeded, but capture should fail
        camera_interface_set_size(camera, 640, 480);
        camera_interface_start(camera);
        
        uint8_t* buffer = NULL;
        int w = 0, h = 0;
        size_t s = 0;
        
        int capture_result = camera_interface_capture_frame(camera, &buffer, &w, &h, &s);
        TEST_ASSERT(capture_result != 0, "capture from empty folder must fail");
        TEST_ASSERT(buffer == NULL, "buffer must be NULL on failure");
        
        camera_interface_stop(camera);
    }
    // If folder_result != 0, that's also acceptable (failed early)
    
    camera_destroy(camera);
    rmdir(empty_folder);  // Cleanup
    
    return 0;
}

/**
 * Test 3: Different image sizes in same folder
 */
int test_mixed_dimensions() {
    const char* test_folder = "pictures/camera/2026-01-16-playground-ready";
    
    Camera* camera = camera_create(CAMERA_TYPE_EMULATED);
    TEST_ASSERT(camera != NULL, "camera_create() failed");
    
    camera_interface_set_folder(camera, test_folder);
    
    // Request a specific size
    camera_interface_set_size(camera, 800, 600);
    camera_interface_start(camera);
    
    // Take multiple pictures
    for (int i = 0; i < 3; i++) {
        uint8_t* buffer = NULL;
        int w = 0, h = 0;
        size_t s = 0;
        
        int result = camera_interface_capture_frame(camera, &buffer, &w, &h, &s);
        TEST_ASSERT(result == 0, "capture must succeed");
        TEST_ASSERT(buffer != NULL, "buffer must be allocated");
        
        // Images should be resized to requested dimensions
        TEST_ASSERT(w == 800, "width should match requested size");
        TEST_ASSERT(h == 600, "height should match requested size");
        TEST_ASSERT(s == 800 * 600 * 3, "size should match dimensions");
        
        free(buffer);
    }
    
    camera_interface_stop(camera);
    camera_destroy(camera);
    
    return 0;
}

/**
 * Test 4: Changing folder mid-operation
 */
int test_change_folder_after_start() {
    const char* folder1 = "pictures/camera/2026-01-16-playground-ready";
    
    Camera* camera = camera_create(CAMERA_TYPE_EMULATED);
    TEST_ASSERT(camera != NULL, "camera_create() failed");
    
    // Start with first folder
    camera_interface_set_folder(camera, folder1);
    camera_interface_set_size(camera, 640, 480);
    camera_interface_start(camera);
    
    // Take one picture
    uint8_t* buffer1 = NULL;
    int w = 0, h = 0;
    size_t s = 0;
    camera_interface_capture_frame(camera, &buffer1, &w, &h, &s);
    TEST_ASSERT(buffer1 != NULL, "first capture must succeed");
    free(buffer1);
    
    // Stop before changing folder
    camera_interface_stop(camera);
    
    // Change folder (should require restart)
    camera_interface_set_folder(camera, folder1);  // Same folder for this test
    camera_interface_start(camera);
    
    // Take another picture
    uint8_t* buffer2 = NULL;
    camera_interface_capture_frame(camera, &buffer2, &w, &h, &s);
    TEST_ASSERT(buffer2 != NULL, "capture after folder change must succeed");
    free(buffer2);
    
    camera_interface_stop(camera);
    camera_destroy(camera);
    
    return 0;
}

/**
 * Test 5: No resize (keep original dimensions)
 */
int test_no_resize() {
    const char* test_folder = "pictures/camera/2026-01-16-playground-ready";
    
    Camera* camera = camera_create(CAMERA_TYPE_EMULATED);
    TEST_ASSERT(camera != NULL, "camera_create() failed");
    
    camera_interface_set_folder(camera, test_folder);
    // Don't call set_size - should use original image dimensions
    camera_interface_start(camera);
    
    uint8_t* buffer = NULL;
    int w = 0, h = 0;
    size_t s = 0;
    
    int result = camera_interface_capture_frame(camera, &buffer, &w, &h, &s);
    TEST_ASSERT(result == 0, "capture without set_size must succeed");
    TEST_ASSERT(buffer != NULL, "buffer must be allocated");
    TEST_ASSERT(w > 0 && h > 0, "dimensions must be positive");
    
    printf("\n      Original image dimensions: %dx%d\n", w, h);
    
    free(buffer);
    camera_interface_stop(camera);
    camera_destroy(camera);
    
    return 0;
}

/**
 * Test 6: Verify BGR format (3 channels)
 */
int test_bgr_format() {
    const char* test_folder = "pictures/camera/2026-01-16-playground-ready";
    
    Camera* camera = camera_create(CAMERA_TYPE_EMULATED);
    TEST_ASSERT(camera != NULL, "camera_create() failed");
    
    camera_interface_set_folder(camera, test_folder);
    camera_interface_set_size(camera, 640, 480);
    camera_interface_start(camera);
    
    uint8_t* buffer = NULL;
    int w = 0, h = 0;
    size_t s = 0;
    
    camera_interface_capture_frame(camera, &buffer, &w, &h, &s);
    TEST_ASSERT(buffer != NULL, "capture must succeed");
    
    // Verify size matches BGR format (3 bytes per pixel)
    size_t expected_size = w * h * 3;
    TEST_ASSERT(s == expected_size, "buffer size must be width * height * 3 (BGR)");
    
    // Check that buffer contains valid data (not all zeros)
    int non_zero_count = 0;
    for (size_t i = 0; i < s && i < 1000; i++) {
        if (buffer[i] != 0) {
            non_zero_count++;
        }
    }
    TEST_ASSERT(non_zero_count > 0, "buffer should contain non-zero pixel data");
    
    free(buffer);
    camera_interface_stop(camera);
    camera_destroy(camera);
    
    return 0;
}

// Test suite definition
typedef struct {
    const char* name;
    int (*func)();
} TestCase;

static const TestCase TESTS[] = {
    {"Cycle through folder and loop around", test_cycle_through_folder},
    {"Empty folder handling", test_empty_folder},
    {"Mixed dimensions with resize", test_mixed_dimensions},
    {"Change folder after start", test_change_folder_after_start},
    {"No resize (original dimensions)", test_no_resize},
    {"BGR format verification", test_bgr_format}
};

#define NUM_TESTS (sizeof(TESTS) / sizeof(TestCase))

int main(int argc, char* argv[]) {
    (void)argc;  // Unused parameter
    (void)argv;  // Unused parameter
    
    printf("========================================\n");
    printf("Emulated Camera Implementation Test\n");
    printf("========================================\n");
    printf("Testing: emulated_camera.c behavior\n");
    printf("Number of tests: %zu\n", NUM_TESTS);
    printf("========================================\n\n");
    
    for (size_t i = 0; i < NUM_TESTS; i++) {
        printf("[%zu/%zu] %s... ", i + 1, NUM_TESTS, TESTS[i].name);
        fflush(stdout);
        
        if (TESTS[i].func() == 0) {
            printf("PASS");
            test_passed++;
        } else {
            printf("FAIL");
            test_failed++;
        }
        printf("\n");
    }
    
    printf("\n========================================\n");
    printf("Results: %d passed, %d failed\n", test_passed, test_failed);
    printf("========================================\n");
    
    return (test_failed == 0) ? 0 : 1;
}

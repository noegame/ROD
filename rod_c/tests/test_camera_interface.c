/**
 * test_camera_interface.c
 * 
 * Validates that camera_interface.h is correctly implemented.
 * 
 * This is a BEHAVIORAL test - ensures camera implementations
 * respond correctly to the same API calls.
 * 
 * Tests:
 * - Init/cleanup lifecycle
 * - Start/stop behavior
 * - Take picture with/without start
 * - Multiple consecutive captures
 * - Size setting
 * - Error handling
 */

#include "camera_interface.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

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
 * Test 1: Basic initialization and cleanup
 */
int test_init_cleanup() {
    Camera* camera = camera_create(CAMERA_TYPE_EMULATED);
    TEST_ASSERT(camera != NULL, "camera_create() must return non-NULL");
    
    camera_destroy(camera);
    return 0;
}

/**
 * Test 2: Set size before start (should succeed)
 */
int test_set_size_before_start() {
    Camera* camera = camera_create(CAMERA_TYPE_EMULATED);
    TEST_ASSERT(camera != NULL, "camera_create() failed");
    
    int result = camera_interface_set_size(camera, 640, 480);
    TEST_ASSERT(result == 0, "set_size before start must succeed");
    
    camera_destroy(camera);
    return 0;
}

/**
 * Test 3: Take picture without start (should fail gracefully)
 */
int test_take_picture_without_start() {
    Camera* camera = camera_create(CAMERA_TYPE_EMULATED);
    TEST_ASSERT(camera != NULL, "camera_create() failed");
    
    camera_interface_set_size(camera, 640, 480);
    
    uint8_t* buffer = NULL;
    int w = 0, h = 0;
    size_t s = 0;
    
    int result = camera_interface_capture_frame(camera, &buffer, &w, &h, &s);
    TEST_ASSERT(result != 0, "take_picture without start must fail");
    TEST_ASSERT(buffer == NULL, "buffer must remain NULL on failure");
    
    camera_destroy(camera);
    return 0;
}

/**
 * Test 4: Basic capture workflow (init → set_size → start → capture → stop → cleanup)
 */
int test_basic_capture() {
    Camera* camera = camera_create(CAMERA_TYPE_EMULATED);
    TEST_ASSERT(camera != NULL, "camera_create() failed");
    
    // Set folder for emulated camera
    const char* test_folder = "pictures/camera/2026-01-16-playground-ready";
    int folder_result = camera_interface_set_folder(camera, test_folder);
    TEST_ASSERT(folder_result == 0, "set_folder must succeed");
    
    // Set size
    int size_result = camera_interface_set_size(camera, 320, 240);
    TEST_ASSERT(size_result == 0, "set_size must succeed");
    
    // Start camera
    int start_result = camera_interface_start(camera);
    TEST_ASSERT(start_result == 0, "start must succeed");
    
    // Take picture
    uint8_t* buffer = NULL;
    int w = 0, h = 0;
    size_t s = 0;
    
    int capture_result = camera_interface_capture_frame(camera, &buffer, &w, &h, &s);
    TEST_ASSERT(capture_result == 0, "capture must succeed after start");
    TEST_ASSERT(buffer != NULL, "buffer must be allocated");
    TEST_ASSERT(w == 320, "width must match request");
    TEST_ASSERT(h == 240, "height must match request");
    TEST_ASSERT(s == 320 * 240 * 3, "size must be width*height*3 for BGR");
    
    // Cleanup buffer
    free(buffer);
    
    // Stop camera
    int stop_result = camera_interface_stop(camera);
    TEST_ASSERT(stop_result == 0, "stop must succeed");
    
    camera_destroy(camera);
    return 0;
}

/**
 * Test 5: Multiple consecutive captures
 */
int test_multiple_captures() {
    Camera* camera = camera_create(CAMERA_TYPE_EMULATED);
    TEST_ASSERT(camera != NULL, "camera_create() failed");
    
    camera_interface_set_folder(camera, "pictures/camera/2026-01-16-playground-ready");
    camera_interface_set_size(camera, 320, 240);
    camera_interface_start(camera);
    
    // Take 5 consecutive pictures
    for (int i = 0; i < 5; i++) {
        uint8_t* buffer = NULL;
        int w = 0, h = 0;
        size_t s = 0;
        
        int result = camera_interface_capture_frame(camera, &buffer, &w, &h, &s);
        TEST_ASSERT(result == 0, "consecutive captures must succeed");
        TEST_ASSERT(buffer != NULL, "buffer must be allocated on each capture");
        
        free(buffer);
    }
    
    camera_interface_stop(camera);
    camera_destroy(camera);
    return 0;
}

/**
 * Test 6: Stop/restart cycle
 */
int test_restart_cycle() {
    Camera* camera = camera_create(CAMERA_TYPE_EMULATED);
    TEST_ASSERT(camera != NULL, "camera_create() failed");
    
    camera_interface_set_folder(camera, "pictures/camera/2026-01-16-playground-ready");
    camera_interface_set_size(camera, 640, 480);
    
    // First cycle
    camera_interface_start(camera);
    
    uint8_t* buffer1 = NULL;
    int w = 0, h = 0;
    size_t s = 0;
    camera_interface_capture_frame(camera, &buffer1, &w, &h, &s);
    TEST_ASSERT(buffer1 != NULL, "first capture must succeed");
    free(buffer1);
    
    camera_interface_stop(camera);
    
    // Second cycle
    camera_interface_start(camera);
    
    uint8_t* buffer2 = NULL;
    camera_interface_capture_frame(camera, &buffer2, &w, &h, &s);
    TEST_ASSERT(buffer2 != NULL, "capture after restart must succeed");
    free(buffer2);
    
    camera_interface_stop(camera);
    camera_destroy(camera);
    return 0;
}

/**
 * Test 7: Invalid folder handling (emulated camera specific)
 */
int test_invalid_folder() {
    Camera* camera = camera_create(CAMERA_TYPE_EMULATED);
    TEST_ASSERT(camera != NULL, "camera_create() failed");
    
    // Set a non-existent folder
    int result = camera_interface_set_folder(camera, "/nonexistent/folder/path");
    TEST_ASSERT(result != 0, "set_folder with invalid path should fail");
    
    camera_destroy(camera);
    return 0;
}

/**
 * Test 8: Zero or negative dimensions (should fail)
 */
int test_invalid_dimensions() {
    Camera* camera = camera_create(CAMERA_TYPE_EMULATED);
    TEST_ASSERT(camera != NULL, "camera_create() failed");
    
    // Zero width
    int result1 = camera_interface_set_size(camera, 0, 480);
    TEST_ASSERT(result1 != 0, "set_size with zero width must fail");
    
    // Zero height
    int result2 = camera_interface_set_size(camera, 640, 0);
    TEST_ASSERT(result2 != 0, "set_size with zero height must fail");
    
    // Negative dimensions
    int result3 = camera_interface_set_size(camera, -640, 480);
    TEST_ASSERT(result3 != 0, "set_size with negative width must fail");
    
    camera_destroy(camera);
    return 0;
}

// Test suite definition
typedef struct {
    const char* name;
    int (*func)();
} TestCase;

static const TestCase TESTS[] = {
    {"Init/Cleanup lifecycle", test_init_cleanup},
    {"Set size before start", test_set_size_before_start},
    {"Take picture without start (should fail)", test_take_picture_without_start},
    {"Basic capture workflow", test_basic_capture},
    {"Multiple consecutive captures", test_multiple_captures},
    {"Stop/restart cycle", test_restart_cycle},
    {"Invalid folder handling", test_invalid_folder},
    {"Invalid dimensions handling", test_invalid_dimensions}
};

#define NUM_TESTS (sizeof(TESTS) / sizeof(TestCase))

int main(int argc, char* argv[]) {
    printf("========================================\n");
    printf("Camera Interface Conformance Test\n");
    printf("========================================\n");
    printf("Testing: EMULATED CAMERA implementation\n");
    printf("Number of tests: %d\n", NUM_TESTS);
    printf("========================================\n\n");
    
    for (size_t i = 0; i < NUM_TESTS; i++) {
        printf("[%zu/%d] %s... ", i + 1, NUM_TESTS, TESTS[i].name);
        fflush(stdout);
        
        if (TESTS[i].func() == 0) {
            printf("PASS\n");
            test_passed++;
        } else {
            printf("FAIL\n");
            test_failed++;
        }
    }
    
    printf("\n========================================\n");
    printf("Results: %d passed, %d failed\n", test_passed, test_failed);
    printf("========================================\n");
    
    return (test_failed == 0) ? 0 : 1;
}

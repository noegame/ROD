/**
 * test_rod_camera.c
 * 
 * Test program for libcamera-based camera capture.
 * Captures a single image and saves it to verify functionality.
 */

#include "camera.h"
#include "../rod_cv/opencv_wrapper.h"
#include "../rod_config/rod_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <libgen.h>
#include <unistd.h>
#include <time.h>

// Test configuration structure
typedef struct {
    const char* name;           // Test name (for file output)
    const char* category;       // Category folder (auto/manual/optimized)
    CameraParameters params;    // Camera parameters to test
} TestConfig;

// Predefined test configurations
// Note: Based on camera.txt metadata from imx477 sensor
static const TestConfig TEST_CONFIGS[] = {
    // === AUTO CATEGORY ===
    // Baseline: current default behavior (all auto)
    {
        .name = "default_auto",
        .category = "auto",
        .params = {
            .ae_enable = 1,
            .exposure_time = -1,
            .analogue_gain = -1,
            .noise_reduction_mode = 2,  // HighQuality
            .sharpness = -1,
            .contrast = -1,
            .brightness = -1,
            .saturation = -1,
            .awb_enable = 1,
            .colour_temperature = -1,
            .frame_duration_min = -1,
            .frame_duration_max = -1
        }
    },
    
    // Auto with high sharpness (may help ArUco edge detection)
    {
        .name = "auto_sharp_high",
        .category = "auto",
        .params = {
            .ae_enable = 1,
            .exposure_time = -1,
            .analogue_gain = -1,
            .noise_reduction_mode = 2,
            .sharpness = 8.0,  // High sharpness for marker edges
            .contrast = -1,
            .brightness = -1,
            .saturation = -1,
            .awb_enable = 1,
            .colour_temperature = -1,
            .frame_duration_min = -1,
            .frame_duration_max = -1
        }
    },
    
    // Auto with minimal noise reduction (faster processing, sharper but noisier)
    {
        .name = "auto_nr_minimal",
        .category = "auto",
        .params = {
            .ae_enable = 1,
            .exposure_time = -1,
            .analogue_gain = -1,
            .noise_reduction_mode = 3,  // Minimal
            .sharpness = -1,
            .contrast = -1,
            .brightness = -1,
            .saturation = -1,
            .awb_enable = 1,
            .colour_temperature = -1,
            .frame_duration_min = -1,
            .frame_duration_max = -1
        }
    },
    
    // === MANUAL CATEGORY ===
    // Manual exposure - normal lighting (based on camera.txt observed values)
    {
        .name = "manual_normal",
        .category = "manual",
        .params = {
            .ae_enable = 0,  // Disable auto-exposure
            .exposure_time = 33962,  // From camera.txt metadata (~34ms)
            .analogue_gain = 2.0,    // From camera.txt metadata
            .noise_reduction_mode = 2,
            .sharpness = -1,
            .contrast = -1,
            .brightness = -1,
            .saturation = -1,
            .awb_enable = 1,  // Keep AWB enabled
            .colour_temperature = -1,
            .frame_duration_min = -1,
            .frame_duration_max = -1
        }
    },
    
    // Manual - low light scenario (longer exposure, higher gain)
    {
        .name = "manual_lowlight",
        .category = "manual",
        .params = {
            .ae_enable = 0,
            .exposure_time = 100000,  // 100ms exposure
            .analogue_gain = 8.0,     // High gain for low light
            .noise_reduction_mode = 2,  // HighQuality to combat noise
            .sharpness = -1,
            .contrast = -1,
            .brightness = -1,
            .saturation = -1,
            .awb_enable = 1,
            .colour_temperature = -1,
            .frame_duration_min = -1,
            .frame_duration_max = -1
        }
    },
    
    // Manual - bright lighting (short exposure, low gain)
    {
        .name = "manual_bright",
        .category = "manual",
        .params = {
            .ae_enable = 0,
            .exposure_time = 10000,   // 10ms exposure
            .analogue_gain = 1.0,     // Minimal gain
            .noise_reduction_mode = 1,  // Fast (less needed in bright light)
            .sharpness = -1,
            .contrast = -1,
            .brightness = -1,
            .saturation = -1,
            .awb_enable = 1,
            .colour_temperature = -1,
            .frame_duration_min = -1,
            .frame_duration_max = -1
        }
    },
    
    // === OPTIMIZED CATEGORY ===
    // Optimized for ArUco detection (empirically tuned)
    {
        .name = "aruco_optimized",
        .category = "optimized",
        .params = {
            .ae_enable = 1,  // Auto-exposure for adaptability
            .exposure_time = -1,
            .analogue_gain = -1,
            .noise_reduction_mode = 2,  // HighQuality
            .sharpness = 4.0,   // Moderate sharpness boost
            .contrast = 1.5,    // Slight contrast boost for black/white markers
            .brightness = 0.0,
            .saturation = -1,
            .awb_enable = 1,
            .colour_temperature = -1,
            .frame_duration_min = -1,
            .frame_duration_max = -1
        }
    }
};

#define NUM_CONFIGS (sizeof(TEST_CONFIGS) / sizeof(TestConfig))

/**
 * Create directory recursively (like mkdir -p)
 */
static int create_directory_recursive(const char* path) {
    char tmp[512];
    snprintf(tmp, sizeof(tmp), "%s", path);
    
    for (char* p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            mkdir(tmp, 0755);
            *p = '/';
        }
    }
    mkdir(tmp, 0755);  // Create final directory
    return 0;
}

/**
 * Log test metadata to results file
 */
static void log_test_metadata(const char* output_dir, const TestConfig* config, 
                              int width, int height, unsigned long avg_b, 
                              unsigned long avg_g, unsigned long avg_r, int success) {
    char log_path[512];
    snprintf(log_path, sizeof(log_path), "%s/test_results.txt", output_dir);
    
    FILE* f = fopen(log_path, "a");
    if (!f) {
        fprintf(stderr, "Warning: Could not open log file %s\n", log_path);
        return;
    }
    
    // Get timestamp
    time_t now = time(NULL);
    char timestamp[64];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", localtime(&now));
    
    fprintf(f, "Config: %s (category: %s)\n", config->name, config->category);
    fprintf(f, "Resolution: %dx%d\n", width, height);
    fprintf(f, "Capture time: %s\n", timestamp);
    fprintf(f, "Parameters:\n");
    fprintf(f, "  ae_enable=%d, exposure_time=%d, analogue_gain=%.2f\n",
            config->params.ae_enable, config->params.exposure_time, config->params.analogue_gain);
    fprintf(f, "  noise_reduction=%d, sharpness=%.1f, contrast=%.1f\n",
            config->params.noise_reduction_mode, config->params.sharpness, config->params.contrast);
    fprintf(f, "  awb_enable=%d, colour_temp=%d\n",
            config->params.awb_enable, config->params.colour_temperature);
    fprintf(f, "Average BGR: (%lu, %lu, %lu)\n", avg_b, avg_g, avg_r);
    fprintf(f, "Status: %s\n", success ? "SUCCESS" : "FAILED");
    fprintf(f, "---\n");
    
    fclose(f);
}

/**
 * Test a single camera configuration
 * Returns 0 on success, -1 on failure
 */
static int test_single_config(const TestConfig* config, int width, int height, const char* output_dir) {
    // Initialize camera
    CameraContext* camera = camera_init();
    if (!camera) {
        fprintf(stderr, "Failed to initialize camera\n");
        return -1;
    }
    
    // Set size
    if (camera_set_size(camera, width, height) != 0) {
        fprintf(stderr, "Failed to set camera size\n");
        camera_cleanup(camera);
        return -1;
    }
    
    // Set parameters
    if (camera_set_parameters(camera, &config->params) != 0) {
        fprintf(stderr, "Failed to set camera parameters\n");
        camera_cleanup(camera);
        return -1;
    }
    
    // Start camera
    if (camera_start(camera) != 0) {
        fprintf(stderr, "Failed to start camera\n");
        camera_cleanup(camera);
        return -1;
    }
    
    // Warm-up: capture several frames to let camera adjust
    printf("  Warming up (10 frames)...\n");
    for (int i = 0; i < 10; i++) {
        uint8_t* warmup_buffer = NULL;
        int w, h;
        size_t s;
        if (camera_take_picture(camera, &warmup_buffer, &w, &h, &s) == 0) {
            free(warmup_buffer);
        }
        usleep(200000);  // 200ms between frames
    }
    
    // Capture final image
    printf("  Capturing image...\n");
    uint8_t* buffer = NULL;
    int img_width, img_height;
    size_t img_size;
    
    if (camera_take_picture(camera, &buffer, &img_width, &img_height, &img_size) != 0) {
        fprintf(stderr, "Failed to capture image\n");
        camera_stop(camera);
        camera_cleanup(camera);
        return -1;
    }
    
    // Calculate average BGR for verification
    unsigned long avg_b = 0, avg_g = 0, avg_r = 0;
    if (buffer && img_size > 0) {
        unsigned long sum_b = 0, sum_g = 0, sum_r = 0;
        int num_pixels = img_width * img_height;
        
        for (int p = 0; p < num_pixels; p++) {
            sum_b += buffer[p * 3 + 0];
            sum_g += buffer[p * 3 + 1];
            sum_r += buffer[p * 3 + 2];
        }
        
        avg_b = sum_b / num_pixels;
        avg_g = sum_g / num_pixels;
        avg_r = sum_r / num_pixels;
        printf("  Average BGR: (%lu, %lu, %lu)\n", avg_b, avg_g, avg_r);
    }
    
    // Create OpenCV image
    ImageHandle* image = create_image_from_buffer(buffer, img_width, img_height, 3, 0);
    free(buffer);
    
    if (!image) {
        fprintf(stderr, "Failed to create image from buffer\n");
        camera_stop(camera);
        camera_cleanup(camera);
        return -1;
    }
    
    // Build output path: {output_dir}/{category}/{name}_{width}x{height}.jpg
    char output_path[512];
    snprintf(output_path, sizeof(output_path), "%s/%s/%s_%dx%d.jpg",
             output_dir, config->category, config->name, width, height);
    
    // Create category directory
    char category_dir[512];
    snprintf(category_dir, sizeof(category_dir), "%s/%s", output_dir, config->category);
    create_directory_recursive(category_dir);
    
    // Save image
    printf("  Saving to %s...\n", output_path);
    if (!save_image(output_path, image)) {
        fprintf(stderr, "Failed to save image to %s\n", output_path);
        log_test_metadata(output_dir, config, width, height, avg_b, avg_g, avg_r, 0);
        release_image(image);
        camera_stop(camera);
        camera_cleanup(camera);
        return -1;
    }
    
    // Log metadata
    log_test_metadata(output_dir, config, width, height, avg_b, avg_g, avg_r, 1);
    
    // Cleanup
    release_image(image);
    camera_stop(camera);
    camera_cleanup(camera);
    
    printf("  SUCCESS\n");
    return 0;
}

int main(int argc, char* argv[]) {
    int width = 640;
    int height = 480;
    const char* output_dir = ROD_CAMERA_TESTS_OUTPUT_FOLDER;
    
    // Parse optional arguments: [width] [height] [output_dir]
    if (argc >= 3) {
        width = atoi(argv[1]);
        height = atoi(argv[2]);
    }
    
    if (argc >= 4) {
        output_dir = argv[3];
    }
    
    printf("========================================\n");
    printf("Camera Parameter Test Suite\n");
    printf("========================================\n");
    printf("Resolution: %dx%d\n", width, height);
    printf("Output directory: %s\n", output_dir);
    printf("Number of configurations: %d\n", NUM_CONFIGS);
    printf("========================================\n\n");
    
    // Create base output directory
    create_directory_recursive(output_dir);
    
    // Run all test configurations
    int passed = 0;
    int failed = 0;
    
    for (int i = 0; i < NUM_CONFIGS; i++) {
        printf("[%d/%d] Testing: %s (category: %s)\n", 
               i + 1, NUM_CONFIGS, 
               TEST_CONFIGS[i].name, 
               TEST_CONFIGS[i].category);
        
        if (test_single_config(&TEST_CONFIGS[i], width, height, output_dir) == 0) {
            passed++;
        } else {
            fprintf(stderr, "  FAILED\n");
            failed++;
        }
        
        printf("\n");
    }
    
    // Print summary
    printf("========================================\n");
    printf("Test Summary\n");
    printf("========================================\n");
    printf("Total:  %d\n", NUM_CONFIGS);
    printf("Passed: %d\n", passed);
    printf("Failed: %d\n", failed);
    printf("========================================\n");
    
    if (failed == 0) {
        printf("\nAll tests completed successfully!\n");
        printf("Images saved to: %s/\n", output_dir);
        return 0;
    } else {
        fprintf(stderr, "\nSome tests failed. Check error messages above.\n");
        return 1;
    }
}


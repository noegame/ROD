/**
 * test_rod_camera.c
 * 
 * Test program for libcamera-based camera capture.
 * Captures a single image and saves it to verify functionality.
 */

#include "camera.h"
#include "../rod_cv/opencv_wrapper.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <libgen.h>
#include <unistd.h>

int main(int argc, char* argv[]) {
    int width = 640;
    int height = 480;
    const char* output_path = "/var/pictures/debug/test_libcamera.jpg";
    
    // Parse optional arguments
    if (argc >= 3) {
        width = atoi(argv[1]);
        height = atoi(argv[2]);
        printf("Using resolution: %dx%d\n", width, height);
    }
    
    if (argc >= 4) {
        output_path = argv[3];
    }
    
    printf("Initializing camera...\n");
    CameraContext* camera = camera_init();
    if (!camera) {
        fprintf(stderr, "Failed to initialize camera\n");
        return 1;
    }
    
    printf("Setting camera size to %dx%d...\n", width, height);
    if (camera_set_size(camera, width, height) != 0) {
        fprintf(stderr, "Failed to set camera size\n");
        camera_cleanup(camera);
        return 1;
    }
    
    printf("Starting camera...\n");
    if (camera_start(camera) != 0) {
        fprintf(stderr, "Failed to start camera\n");
        camera_cleanup(camera);
        return 1;
    }
    
    // Warm-up: capture several frames to let camera adjust (exposure, white balance)
    // Auto-exposure needs 5-10 frames to stabilize in low light conditions
    printf("Warming up camera (capturing 10 frames to stabilize auto-exposure)...\n");
    for (int i = 0; i < 10; i++) {
        uint8_t* warmup_buffer = NULL;
        int w, h;
        size_t s;
        if (camera_take_picture(camera, &warmup_buffer, &w, &h, &s) == 0) {
            free(warmup_buffer);
            printf("  Warmup frame %d captured\n", i + 1);
        }
        usleep(200000);  // 200ms between warmup frames (allow AE to adjust)
    }
    
    printf("Capturing final image...\n");
    uint8_t* buffer = NULL;
    int img_width, img_height;
    size_t img_size;
    
    if (camera_take_picture(camera, &buffer, &img_width, 
                           &img_height, &img_size) != 0) {
        fprintf(stderr, "Failed to capture image\n");
        camera_stop(camera);
        camera_cleanup(camera);
        return 1;
    }
    
    printf("Captured image: %dx%d, %zu bytes (BGR format)\n", 
           img_width, img_height, img_size);
    
    // Verify buffer dimensions match expected size
    size_t expected_size = img_width * img_height * 3;
    if (img_size != expected_size) {
        fprintf(stderr, "Warning: Size mismatch - expected %zu, got %zu\n",
                expected_size, img_size);
    }
    
    // Calculate average color values for verification
    if (buffer && img_size > 0) {
        unsigned long sum_b = 0, sum_g = 0, sum_r = 0;
        int num_pixels = img_width * img_height;
        
        for (int p = 0; p < num_pixels; p++) {
            sum_b += buffer[p * 3 + 0];  // B
            sum_g += buffer[p * 3 + 1];  // G
            sum_r += buffer[p * 3 + 2];  // R
        }
        
        printf("Average BGR: (%lu, %lu, %lu)\n", 
               sum_b / num_pixels, sum_g / num_pixels, sum_r / num_pixels);
    }
    
    // Create OpenCV image from BGR buffer (format=0 for BGR)
    printf("Creating OpenCV image...\n");
    ImageHandle* image = create_image_from_buffer(buffer, img_width, img_height, 3, 0);
    free(buffer);  // Buffer is copied by create_image_from_buffer
    
    if (!image) {
        fprintf(stderr, "Failed to create image from buffer\n");
        camera_stop(camera);
        camera_cleanup(camera);
        return 1;
    }
    
    // Create directory if needed - recursive creation
    char* path_copy = strdup(output_path);
    char* dir = dirname(path_copy);
    
    // Create parent directories recursively
    char tmp[256];
    snprintf(tmp, sizeof(tmp), "%s", dir);
    for (char* p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            mkdir(tmp, 0755);
            *p = '/';
        }
    }
    mkdir(tmp, 0755);  // Create final directory
    free(path_copy);
    
    // Save image to file
    char abs_path[256];
    if (output_path[0] != '/') {
        // Convert to absolute path for display
        getcwd(abs_path, sizeof(abs_path));
        snprintf(abs_path, sizeof(abs_path), "%s/%s", abs_path, output_path);
    } else {
        strncpy(abs_path, output_path, sizeof(abs_path));
    }
    printf("Saving image to %s...\n", abs_path);
    if (!save_image(output_path, image)) {
        fprintf(stderr, "Failed to save image to %s\n", output_path);
        fprintf(stderr, "Possible causes:\n");
        fprintf(stderr, "  - Directory doesn't exist or no write permissions\n");
        fprintf(stderr, "  - Try: sudo mkdir -p /var/pictures/debug\n");
        fprintf(stderr, "  - Try: sudo chmod 777 /var/pictures/debug\n");
        release_image(image);
        camera_stop(camera);
        camera_cleanup(camera);
        return 1;
    }
    
    printf("Image saved successfully!\n");
    
    // Cleanup
    release_image(image);
    camera_stop(camera);
    camera_cleanup(camera);
    
    printf("Test completed successfully.\n");
    return 0;
}

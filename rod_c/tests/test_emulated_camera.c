/**
 * test_emulated_camera.c
 * 
 * Simple test program to demonstrate the camera interface with emulated camera.
 * Reads images from a folder and cycles through them.
 */

#include "camera_interface.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <image_folder_path> [width] [height]\n", argv[0]);
        fprintf(stderr, "Example: %s /path/to/images 640 480\n", argv[0]);
        return 1;
    }
    
    const char* folder_path = argv[1];
    int width = 0;
    int height = 0;
    
    // Parse optional width and height
    if (argc >= 4) {
        width = atoi(argv[2]);
        height = atoi(argv[3]);
        printf("Will resize images to: %dx%d\n", width, height);
    }
    
    // Initialize emulated camera using unified interface
    printf("Initializing emulated camera via camera interface...\n");
    Camera* camera = camera_create(CAMERA_TYPE_EMULATED);
    if (!camera) {
        fprintf(stderr, "Failed to initialize camera\n");
        return 1;
    }
    
    // Set folder path
    if (camera_interface_set_folder(camera, folder_path) != 0) {
        fprintf(stderr, "Failed to set folder path\n");
        camera_destroy(camera);
        return 1;
    }
    
    // Set size if specified
    if (width > 0 && height > 0) {
        if (camera_interface_set_size(camera, width, height) != 0) {
            fprintf(stderr, "Failed to set camera size\n");
            camera_destroy(camera);
            return 1;
        }
    }
    
    // Start camera (loads image list)
    if (camera_interface_start(camera) != 0) {
        fprintf(stderr, "Failed to start camera\n");
        camera_destroy(camera);
        return 1;
    }
    
    // Capture a few images to demonstrate cycling
    printf("\nCapturing images...\n");
    for (int i = 0; i < 5; i++) {
        uint8_t* buffer = NULL;
        int img_width, img_height;
        size_t img_size;
        
        if (camera_interface_capture_frame(camera, &buffer, &img_width, 
                                 &img_height, &img_size) == 0) {
            printf("  Image %d: %dx%d, %zu bytes (BGR format)\n", 
                   i + 1, img_width, img_height, img_size);
            
            // Calculate some statistics on the image data
            if (buffer && img_size > 0) {
                unsigned long sum_b = 0, sum_g = 0, sum_r = 0;
                int num_pixels = img_width * img_height;
                
                for (int p = 0; p < num_pixels; p++) {
                    sum_b += buffer[p * 3 + 0];
                    sum_g += buffer[p * 3 + 1];
                    sum_r += buffer[p * 3 + 2];
                }
                
                printf("    Average BGR: (%lu, %lu, %lu)\n", 
                       sum_b / num_pixels, sum_g / num_pixels, sum_r / num_pixels);
            }
            
            // Free the buffer
            free(buffer);
        } else {
            fprintf(stderr, "Failed to capture image %d\n", i + 1);
        }
    }
    
    // Stop and cleanup
    printf("\nStopping camera...\n");
    camera_interface_stop(camera);
    
    printf("Cleaning up...\n");
    camera_destroy(camera);
    
    printf("Test completed successfully!\n");
    return 0;
}

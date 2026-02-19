#include "emulated_camera.h"
#include "opencv_wrapper.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>

#define MAX_PATH_LENGTH 1024
#define MAX_IMAGE_FILES 1000

// Internal definition of EmulatedCameraContext
struct EmulatedCameraContext {
    char image_folder[MAX_PATH_LENGTH];
    char** image_files;         // Array of image file paths
    int num_images;             // Number of images found
    int current_index;          // Current image index (for cycling)
    int width;                  // Desired width (0 = original size)
    int height;                 // Desired height (0 = original size)
    int is_started;             // Whether camera has been started
};

/**
 * Check if a file has a supported image extension.
 */
static int is_image_file(const char* filename) {
    if (!filename) return 0;
    
    size_t len = strlen(filename);
    if (len < 4) return 0;
    
    const char* ext = filename + len - 4;
    
    return (strcasecmp(ext, ".jpg") == 0 ||
            strcasecmp(ext, ".png") == 0 ||
            strcasecmp(ext, "jpeg") == 0);
}

/**
 * Compare function for qsort to sort image file paths alphabetically.
 */
static int compare_strings(const void* a, const void* b) {
    return strcmp(*(const char**)a, *(const char**)b);
}

/**
 * Load the list of image files from the configured folder.
 */
static int load_image_list(EmulatedCameraContext* ctx) {
    DIR* dir = opendir(ctx->image_folder);
    if (!dir) {
        fprintf(stderr, "Error: Cannot open image folder: %s\n", ctx->image_folder);
        return -1;
    }
    
    // Allocate array for image file paths
    ctx->image_files = (char**)malloc(sizeof(char*) * MAX_IMAGE_FILES);
    if (!ctx->image_files) {
        closedir(dir);
        fprintf(stderr, "Error: Memory allocation failed for image files list\n");
        return -1;
    }
    
    ctx->num_images = 0;
    struct dirent* entry;
    
    // Scan directory for image files
    while ((entry = readdir(dir)) != NULL && ctx->num_images < MAX_IMAGE_FILES) {
        if (entry->d_type == DT_REG || entry->d_type == DT_UNKNOWN) {
            if (is_image_file(entry->d_name)) {
                // Construct full path
                char full_path[MAX_PATH_LENGTH];
                snprintf(full_path, MAX_PATH_LENGTH, "%s/%s", 
                        ctx->image_folder, entry->d_name);
                
                // Allocate and store path
                ctx->image_files[ctx->num_images] = strdup(full_path);
                if (ctx->image_files[ctx->num_images]) {
                    ctx->num_images++;
                }
            }
        }
    }
    
    closedir(dir);
    
    if (ctx->num_images == 0) {
        fprintf(stderr, "Error: No image files found in folder: %s\n", ctx->image_folder);
        free(ctx->image_files);
        ctx->image_files = NULL;
        return -1;
    }
    
    // Sort image files alphabetically for consistent ordering
    qsort(ctx->image_files, ctx->num_images, sizeof(char*), compare_strings);
    
    printf("Emulated camera initialized with %d images from: %s\n", 
           ctx->num_images, ctx->image_folder);
    
    return 0;
}

EmulatedCameraContext* emulated_camera_init() {
    EmulatedCameraContext* ctx = (EmulatedCameraContext*)malloc(sizeof(EmulatedCameraContext));
    if (!ctx) {
        fprintf(stderr, "Error: Failed to allocate emulated camera context\n");
        return NULL;
    }
    
    // Initialize fields
    memset(ctx->image_folder, 0, MAX_PATH_LENGTH);
    ctx->image_files = NULL;
    ctx->num_images = 0;
    ctx->current_index = 0;
    ctx->width = 0;
    ctx->height = 0;
    ctx->is_started = 0;
    
    return ctx;
}

int emulated_camera_set_folder(EmulatedCameraContext* ctx, const char* folder_path) {
    if (!ctx || !folder_path) {
        fprintf(stderr, "Error: Invalid context or folder path\n");
        return -1;
    }
    
    // Check if folder exists
    struct stat st;
    if (stat(folder_path, &st) != 0 || !S_ISDIR(st.st_mode)) {
        fprintf(stderr, "Error: Folder does not exist: %s\n", folder_path);
        return -1;
    }
    
    strncpy(ctx->image_folder, folder_path, MAX_PATH_LENGTH - 1);
    ctx->image_folder[MAX_PATH_LENGTH - 1] = '\0';
    
    printf("Emulated camera folder set to: %s\n", ctx->image_folder);
    return 0;
}

int emulated_camera_set_size(EmulatedCameraContext* ctx, int width, int height) {
    if (!ctx) {
        fprintf(stderr, "Error: Invalid context\n");
        return -1;
    }
    
    // Validate dimensions
    if (width <= 0 || height <= 0) {
        fprintf(stderr, "Error: Invalid dimensions %dx%d (must be positive)\n", width, height);
        return -1;
    }
    
    ctx->width = width;
    ctx->height = height;
    
    printf("Emulated camera size set to: %dx%d\n", width, height);
    return 0;
}

int emulated_camera_start(EmulatedCameraContext* ctx) {
    if (!ctx) {
        fprintf(stderr, "Error: Invalid context\n");
        return -1;
    }
    
    if (strlen(ctx->image_folder) == 0) {
        fprintf(stderr, "Error: Image folder not set. Call emulated_camera_set_folder() first.\n");
        return -1;
    }
    
    if (ctx->is_started) {
        printf("Warning: Emulated camera already started\n");
        return 0;
    }
    
    // Load list of images from folder
    if (load_image_list(ctx) != 0) {
        return -1;
    }
    
    ctx->is_started = 1;
    ctx->current_index = 0;
    
    printf("Emulated camera started successfully\n");
    return 0;
}

int emulated_camera_take_picture(EmulatedCameraContext* ctx, 
                                  uint8_t** out_buffer,
                                  int* out_width,
                                  int* out_height,
                                  size_t* out_size) {
    if (!ctx || !out_buffer || !out_width || !out_height || !out_size) {
        fprintf(stderr, "Error: Invalid parameters\n");
        return -1;
    }
    
    if (!ctx->is_started) {
        fprintf(stderr, "Error: Camera not started. Call emulated_camera_start() first.\n");
        return -1;
    }
    
    if (ctx->num_images == 0) {
        fprintf(stderr, "Error: No images available\n");
        return -1;
    }
    
    // Get current image path
    const char* image_path = ctx->image_files[ctx->current_index];
    
    // Load image using OpenCV wrapper
    ImageHandle* image = load_image(image_path);
    if (!image) {
        fprintf(stderr, "Error: Failed to load image: %s\n", image_path);
        return -1;
    }
    
    // Get image dimensions
    int img_width = get_image_width(image);
    int img_height = get_image_height(image);
    
    // Resize if dimensions are specified
    ImageHandle* final_image = image;
    int should_release_original = 0;
    
    if (ctx->width > 0 && ctx->height > 0) {
        ImageHandle* resized = resize_image(image, ctx->width, ctx->height);
        if (!resized) {
            fprintf(stderr, "Error: Failed to resize image\n");
            release_image(image);
            return -1;
        }
        final_image = resized;
        should_release_original = 1;
        img_width = ctx->width;
        img_height = ctx->height;
    }
    
    // Calculate buffer size (BGR format: 3 bytes per pixel)
    size_t buffer_size = img_width * img_height * 3;
    
    // Get raw image data (already in BGR format from OpenCV load)
    uint8_t* image_data = get_image_data(final_image);
    if (!image_data) {
        fprintf(stderr, "Error: Failed to get image data\n");
        if (should_release_original) release_image(image);
        release_image(final_image);
        return -1;
    }
    
    // Allocate output buffer and copy data
    uint8_t* buffer = (uint8_t*)malloc(buffer_size);
    if (!buffer) {
        fprintf(stderr, "Error: Failed to allocate image buffer\n");
        if (should_release_original) release_image(image);
        release_image(final_image);
        return -1;
    }
    
    // Copy image data to buffer
    memcpy(buffer, image_data, buffer_size);
    
    // Clean up images
    if (should_release_original) {
        release_image(image);
        release_image(final_image);
    } else {
        // image and final_image are the same pointer, only release once
        release_image(final_image);
    }
    
    // Set output parameters
    *out_buffer = buffer;
    *out_width = img_width;
    *out_height = img_height;
    *out_size = buffer_size;
    
    // Move to next image (circular buffer)
    ctx->current_index = (ctx->current_index + 1) % ctx->num_images;
    
    printf("Emulated camera captured image: %s (%dx%d)\n", 
           image_path, img_width, img_height);
    
    return 0;
}

void emulated_camera_stop(EmulatedCameraContext* ctx) {
    if (!ctx) return;
    
    // Free image file paths
    if (ctx->image_files) {
        for (int i = 0; i < ctx->num_images; i++) {
            free(ctx->image_files[i]);
        }
        free(ctx->image_files);
        ctx->image_files = NULL;
    }
    
    ctx->num_images = 0;
    ctx->current_index = 0;
    ctx->is_started = 0;
    
    printf("Emulated camera stopped\n");
}

void emulated_camera_cleanup(EmulatedCameraContext* ctx) {
    if (!ctx) return;
    
    emulated_camera_stop(ctx);
    free(ctx);
    
    printf("Emulated camera cleaned up\n");
}

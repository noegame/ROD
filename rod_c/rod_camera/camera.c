/**
 * camera.c
 * 
 * High-level camera interface using libcamera.
 * Provides API compatible with emulated_camera for easy switching.
 */

#include "camera.h"
#include "libcamera_wrapper.h"
#include <stdlib.h>
#include <stdio.h>

// Internal definition of CameraContext
struct CameraContext {
    LibCameraContext* libcamera_ctx;
    int width;
    int height;
    int configured;
    int started;
};

CameraContext* camera_init() {
    CameraContext* ctx = (CameraContext*)malloc(sizeof(CameraContext));
    if (!ctx) {
        fprintf(stderr, "Failed to allocate CameraContext\n");
        return NULL;
    }

    ctx->libcamera_ctx = libcamera_init();
    if (!ctx->libcamera_ctx) {
        fprintf(stderr, "Failed to initialize libcamera\n");
        free(ctx);
        return NULL;
    }

    // Open first camera (index 0)
    if (libcamera_open_camera(ctx->libcamera_ctx, 0) != 0) {
        fprintf(stderr, "Failed to open camera\n");
        libcamera_cleanup(ctx->libcamera_ctx);
        free(ctx);
        return NULL;
    }

    ctx->width = 640;       // Default resolution
    ctx->height = 480;
    ctx->configured = 0;
    ctx->started = 0;

    return ctx;
}

int camera_set_size(CameraContext* ctx, int width, int height) {
    if (!ctx) {
        return -1;
    }

    if (ctx->started) {
        fprintf(stderr, "Cannot set size after camera is started\n");
        return -1;
    }

    ctx->width = width;
    ctx->height = height;

    return 0;
}

int camera_start(CameraContext* ctx) {
    if (!ctx || !ctx->libcamera_ctx) {
        return -1;
    }

    if (ctx->started) {
        return 0;  // Already started
    }

    // Configure camera if not already done
    if (!ctx->configured) {
        if (libcamera_configure(ctx->libcamera_ctx, ctx->width, ctx->height) != 0) {
            fprintf(stderr, "Failed to configure camera\n");
            return -1;
        }
        ctx->configured = 1;
    }

    // Start camera
    if (libcamera_start(ctx->libcamera_ctx) != 0) {
        fprintf(stderr, "Failed to start camera\n");
        return -1;
    }

    ctx->started = 1;
    return 0;
}

int camera_take_picture(CameraContext* ctx, 
                       uint8_t** out_buffer,
                       int* out_width,
                       int* out_height,
                       size_t* out_size) {
    if (!ctx || !ctx->libcamera_ctx) {
        return -1;
    }

    if (!ctx->started) {
        fprintf(stderr, "Camera not started\n");
        return -1;
    }

    // Capture frame with 1000ms timeout
    int ret = libcamera_capture_frame(ctx->libcamera_ctx, 
                                     out_buffer, 
                                     out_width, 
                                     out_height,
                                     out_size, 
                                     1000);
    
    if (ret != 0) {
        fprintf(stderr, "Failed to capture frame\n");
        return -1;
    }

    return 0;
}

void camera_stop(CameraContext* ctx) {
    if (!ctx || !ctx->libcamera_ctx) {
        return;
    }

    if (ctx->started) {
        libcamera_stop(ctx->libcamera_ctx);
        ctx->started = 0;
    }
}

void camera_cleanup(CameraContext* ctx) {
    if (!ctx) {
        return;
    }

    if (ctx->libcamera_ctx) {
        libcamera_cleanup(ctx->libcamera_ctx);
    }

    free(ctx);
}

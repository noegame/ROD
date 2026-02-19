#include "camera_interface.h"
#include "backends/imx477/camera.h"
#include "backends/emulated/emulated_camera.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Internal camera structure with function pointers
struct Camera {
    CameraType type;
    void* backend_context;  // Points to CameraContext or EmulatedCameraContext
    int width;
    int height;
};

Camera* camera_create(CameraType type) {
    Camera* cam = (Camera*)malloc(sizeof(Camera));
    if (!cam) {
        fprintf(stderr, "Failed to allocate camera interface\n");
        return NULL;
    }
    
    cam->type = type;
    cam->backend_context = NULL;
    cam->width = 640;
    cam->height = 480;
    
    // Initialize backend
    if (type == CAMERA_TYPE_IMX477) {
        CameraContext* ctx = camera_init();
        if (!ctx) {
            fprintf(stderr, "Failed to initialize real camera\n");
            free(cam);
            return NULL;
        }
        cam->backend_context = ctx;
    } else if (type == CAMERA_TYPE_EMULATED) {
        EmulatedCameraContext* ctx = emulated_camera_init();
        if (!ctx) {
            fprintf(stderr, "Failed to initialize emulated camera\n");
            free(cam);
            return NULL;
        }
        cam->backend_context = ctx;
    } else {
        fprintf(stderr, "Unknown camera type: %d\n", type);
        free(cam);
        return NULL;
    }
    
    return cam;
}

int camera_interface_set_size(Camera* camera, int width, int height) {
    if (!camera) {
        return -1;
    }
    
    camera->width = width;
    camera->height = height;
    
    if (camera->type == CAMERA_TYPE_IMX477) {
        CameraContext* ctx = (CameraContext*)camera->backend_context;
        return camera_set_size(ctx, width, height);
    } else if (camera->type == CAMERA_TYPE_EMULATED) {
        EmulatedCameraContext* ctx = (EmulatedCameraContext*)camera->backend_context;
        return emulated_camera_set_size(ctx, width, height);
    }
    
    return -1;
}

int camera_interface_set_folder(Camera* camera, const char* folder_path) {
    if (!camera) {
        return -1;
    }
    
    if (camera->type == CAMERA_TYPE_EMULATED) {
        EmulatedCameraContext* ctx = (EmulatedCameraContext*)camera->backend_context;
        return emulated_camera_set_folder(ctx, folder_path);
    }
    
    // Real camera doesn't use folder path
    return 0;
}

int camera_interface_set_parameters(Camera* camera, const RodCameraParameters* params) {
    if (!camera || !params) {
        return -1;
    }
    
    if (camera->type == CAMERA_TYPE_IMX477) {
        CameraContext* ctx = (CameraContext*)camera->backend_context;
        
        // Convert RodCameraParameters to backend CameraParameters
        struct CameraParameters backend_params;
        backend_params.ae_enable = params->aec_enable;
        backend_params.exposure_time = params->exposure_time;
        backend_params.analogue_gain = (double)params->analogue_gain;
        backend_params.noise_reduction_mode = params->noise_reduction_mode;
        backend_params.sharpness = (double)params->sharpness;
        backend_params.contrast = (double)params->contrast;
        backend_params.brightness = (double)params->brightness;
        backend_params.saturation = (double)params->saturation;
        backend_params.awb_enable = params->awb_enable;
        backend_params.colour_temperature = -1;
        backend_params.frame_duration_min = -1;
        backend_params.frame_duration_max = -1;
        
        return camera_set_parameters(ctx, &backend_params);
    }
    
    // Emulated camera doesn't use parameters
    return 0;
}

int camera_interface_start(Camera* camera) {
    if (!camera) {
        return -1;
    }
    
    if (camera->type == CAMERA_TYPE_IMX477) {
        CameraContext* ctx = (CameraContext*)camera->backend_context;
        return camera_start(ctx);
    } else if (camera->type == CAMERA_TYPE_EMULATED) {
        EmulatedCameraContext* ctx = (EmulatedCameraContext*)camera->backend_context;
        return emulated_camera_start(ctx);
    }
    
    return -1;
}

int camera_interface_capture_frame(Camera* camera, uint8_t** out_buffer, 
                         int* out_width, int* out_height, size_t* out_size) {
    if (!camera || !out_buffer || !out_width || !out_height || !out_size) {
        return -1;
    }
    
    int result = -1;
    
    if (camera->type == CAMERA_TYPE_IMX477) {
        CameraContext* ctx = (CameraContext*)camera->backend_context;
        result = camera_take_picture(ctx, out_buffer, out_width, out_height, out_size);
    } else if (camera->type == CAMERA_TYPE_EMULATED) {
        EmulatedCameraContext* ctx = (EmulatedCameraContext*)camera->backend_context;
        result = emulated_camera_take_picture(ctx, out_buffer, out_width, out_height, out_size);
    }
    
    if (result == 0) {
        // Update internal dimensions from actual capture
        camera->width = *out_width;
        camera->height = *out_height;
    }
    
    return result;
}

int camera_interface_get_width(Camera* camera) {
    if (!camera) {
        return -1;
    }
    return camera->width;
}

int camera_interface_get_height(Camera* camera) {
    if (!camera) {
        return -1;
    }
    return camera->height;
}

int camera_interface_stop(Camera* camera) {
    if (!camera) {
        return -1;
    }
    
    if (camera->type == CAMERA_TYPE_IMX477) {
        CameraContext* ctx = (CameraContext*)camera->backend_context;
        camera_stop(ctx);
    } else if (camera->type == CAMERA_TYPE_EMULATED) {
        EmulatedCameraContext* ctx = (EmulatedCameraContext*)camera->backend_context;
        emulated_camera_stop(ctx);
    }
    
    return 0;
}

void camera_destroy(Camera* camera) {
    if (!camera) {
        return;
    }
    
    if (camera->type == CAMERA_TYPE_IMX477) {
        CameraContext* ctx = (CameraContext*)camera->backend_context;
        camera_cleanup(ctx);
    } else if (camera->type == CAMERA_TYPE_EMULATED) {
        EmulatedCameraContext* ctx = (EmulatedCameraContext*)camera->backend_context;
        emulated_camera_cleanup(ctx);
    }
    
    free(camera);
}

CameraType camera_get_type(Camera* camera) {
    if (!camera) {
        return CAMERA_TYPE_EMULATED;  // Default fallback
    }
    return camera->type;
}

void camera_get_default_parameters(RodCameraParameters* params) {
    if (!params) {
        return;
    }
    
    params->exposure_time = -1;
    params->analogue_gain = -1.0f;
    params->brightness = -1.0f;
    params->contrast = -1.0f;
    params->saturation = -1.0f;
    params->sharpness = -1.0f;
    params->awb_enable = -1;
    params->aec_enable = -1;
    params->noise_reduction_mode = -1;
}

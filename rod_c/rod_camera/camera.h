#ifndef CAMERA_H
#define CAMERA_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Opaque handle to Camera context
typedef struct CameraContext CameraContext;

/**
 * Camera control parameters.
 * Convention: -1 means "use libcamera default value"
 * Based on Raspberry Pi HQ camera capabilities (imx477 sensor)
 */
typedef struct CameraParameters {
    // Exposure control
    int ae_enable;              // -1=default(true), 0=false, 1=true
    int exposure_time;          // -1=auto, else microseconds (110-694422939)
    double analogue_gain;       // -1=auto, else 1.0-22.26
    
    // Image processing
    int noise_reduction_mode;   // -1=default(2=HighQuality), 0=Off, 1=Fast, 2=HighQuality, 3=Minimal, 4=ZSL
    double sharpness;           // -1=default(1.0), else 0.0-16.0
    double contrast;            // -1=default(1.0), else 0.0-32.0
    double brightness;          // -1=default(0.0), else -1.0-1.0
    double saturation;          // -1=default(1.0), else 0.0-32.0
    
    // White balance
    int awb_enable;             // -1=default(true), 0=false, 1=true
    int colour_temperature;     // -1=auto, else 100-100000 K
    
    // Frame timing
    int64_t frame_duration_min; // -1=default(100), else nanoseconds
    int64_t frame_duration_max; // -1=default(1000000000), else nanoseconds
} CameraParameters;

/**
 * Initialize default camera parameters (all -1 = use libcamera defaults)
 */
static inline CameraParameters camera_default_parameters() {
    CameraParameters params;
    params.ae_enable = -1;
    params.exposure_time = -1;
    params.analogue_gain = -1;
    params.noise_reduction_mode = -1;
    params.sharpness = -1;
    params.contrast = -1;
    params.brightness = -1;
    params.saturation = -1;
    params.awb_enable = -1;
    params.colour_temperature = -1;
    params.frame_duration_min = -1;
    params.frame_duration_max = -1;
    return params;
}

/**
 * Initialize a camera context using libcamera.
 * @return Pointer to the context, or NULL on failure
 */
CameraContext* camera_init();

/**
 * Set desired image dimensions.
 * Must be called before camera_start().
 * @param ctx The camera context
 * @param width Desired width
 * @param height Desired height
 * @return 0 on success, -1 on failure
 */
int camera_set_size(CameraContext* ctx, int width, int height);

/**
 * Set camera control parameters.
 * Must be called before camera_start().
 * @param ctx The camera context
 * @param params Parameter structure (pass camera_default_parameters() for defaults)
 * @return 0 on success, -1 on failure
 */
int camera_set_parameters(CameraContext* ctx, const CameraParameters* params);

/**
 * Start the camera.
 * @param ctx The camera context
 * @return 0 on success, -1 on failure
 */
int camera_start(CameraContext* ctx);

/**
 * Capture a single image from the camera.
 * Returns BGR888 format buffer (OpenCV native format - no conversion needed).
 * The caller must free() the returned buffer.
 * @param ctx The camera context
 * @param out_buffer Pointer to receive image data (BGR format)
 * @param out_width Pointer to receive image width
 * @param out_height Pointer to receive image height
 * @param out_size Pointer to receive buffer size in bytes
 * @return 0 on success, -1 on failure
 */
int camera_take_picture(CameraContext* ctx, 
                       uint8_t** out_buffer,
                       int* out_width,
                       int* out_height,
                       size_t* out_size);

/**
 * Stop the camera.
 * @param ctx The camera context
 */
void camera_stop(CameraContext* ctx);

/**
 * Cleanup and release all camera resources.
 * @param ctx The camera context
 */
void camera_cleanup(CameraContext* ctx);

#ifdef __cplusplus
}
#endif

#endif // CAMERA_H

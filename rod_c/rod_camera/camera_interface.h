#ifndef CAMERA_INTERFACE_H
#define CAMERA_INTERFACE_H

#include <stddef.h>
#include <stdint.h>

/**
 * Unified camera interface for ROD project
 * 
 * This interface provides a common API for different camera backends
 * (real hardware camera and emulated camera for testing).
 * 
 * Usage:
 *   Camera* cam = camera_create(CAMERA_TYPE_EMULATED);
 *   camera_interface_set_size(cam, 640, 480);
 *   camera_interface_set_folder(cam, "path/to/images");  // For emulated only
 *   camera_interface_start(cam);
 *   unsigned char* frame;
 *   camera_interface_capture_frame(cam, &frame, &w, &h, &size);
 *   // ... process frame ...
 *   camera_interface_stop(cam);
 *   camera_destroy(cam);
 */

// Camera types
typedef enum {
    CAMERA_TYPE_REAL,      // Hardware camera (IMX477 via libcamera)
    CAMERA_TYPE_EMULATED   // Emulated camera (reads from image folder)
} CameraType;

// Opaque camera handle
typedef struct Camera Camera;

// Simplified camera parameters for the unified interface
// Convention: -1 means "use default value"
typedef struct RodCameraParameters {
    int exposure_time;       // Microseconds, -1 for auto
    float analogue_gain;     // 1.0 to 22.26, -1 for auto
    float brightness;        // -1.0 to 1.0, -1 for auto
    float contrast;          // 0.0 to 32.0, -1 for auto
    float saturation;        // 0.0 to 32.0, -1 for auto
    float sharpness;         // 0.0 to 16.0, -1 for auto
    int awb_enable;          // 0 or 1, -1 for auto
    int aec_enable;          // 0 or 1, -1 for auto
    int noise_reduction_mode; // 0-4, -1 for auto
} RodCameraParameters;

/**
 * Create a camera instance
 * 
 * @param type Camera type (CAMERA_TYPE_REAL or CAMERA_TYPE_EMULATED)
 * @return Camera instance or NULL on failure
 */
Camera* camera_create(CameraType type);

/**
 * Set camera resolution
 * Must be called before camera_interface_start()
 * 
 * @param camera Camera instance
 * @param width Image width
 * @param height Image height
 * @return 0 on success, -1 on failure
 */
int camera_interface_set_size(Camera* camera, int width, int height);

/**
 * Set image folder (emulated camera only)
 * Must be called before camera_interface_start() for emulated cameras
 * 
 * @param camera Camera instance
 * @param folder_path Path to folder containing images
 * @return 0 on success, -1 on failure
 */
int camera_interface_set_folder(Camera* camera, const char* folder_path);

/**
 * Set camera parameters (real camera only)
 * 
 * @param camera Camera instance
 * @param params Camera parameters
 * @return 0 on success, -1 on failure
 */
int camera_interface_set_parameters(Camera* camera, const RodCameraParameters* params);

/**
 * Get default camera parameters (all values set to -1 for auto)
 * 
 * @param params Output parameter struct
 */
void camera_get_default_parameters(RodCameraParameters* params);

/**
 * Start camera capture
 * 
 * @param camera Camera instance
 * @return 0 on success, -1 on failure
 */
int camera_interface_start(Camera* camera);

/**
 * Capture a frame
 * Returns BGR888 format image data
 * 
 * @param camera Camera instance
 * @param out_buffer Pointer to receive image data (malloc'd, caller must free)
 * @param out_width Pointer to receive image width
 * @param out_height Pointer to receive image height
 * @param out_size Pointer to receive buffer size in bytes
 * @return 0 on success, -1 on failure
 */
int camera_interface_capture_frame(Camera* camera, uint8_t** out_buffer, 
                                   int* out_width, int* out_height, size_t* out_size);

/**
 * Get current image width
 * 
 * @param camera Camera instance
 * @return Image width, or -1 on error
 */
int camera_interface_get_width(Camera* camera);

/**
 * Get current image height
 * 
 * @param camera Camera instance
 * @return Image height, or -1 on error
 */
int camera_interface_get_height(Camera* camera);

/**
 * Stop camera capture
 * 
 * @param camera Camera instance
 * @return 0 on success, -1 on failure
 */
int camera_interface_stop(Camera* camera);

/**
 * Destroy camera instance and free resources
 * 
 * @param camera Camera instance
 */
void camera_destroy(Camera* camera);

/**
 * Get current camera type
 * 
 * @param camera Camera instance
 * @return Camera type
 */
CameraType camera_get_type(Camera* camera);

#endif // CAMERA_INTERFACE_H

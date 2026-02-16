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

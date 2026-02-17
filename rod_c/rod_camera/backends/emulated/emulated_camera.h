#ifndef EMULATED_CAMERA_H
#define EMULATED_CAMERA_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Opaque handle to EmulatedCamera context
typedef struct EmulatedCameraContext EmulatedCameraContext;

/**
 * Initialize an emulated camera context.
 * @return Pointer to the context, or NULL on failure
 */
EmulatedCameraContext* emulated_camera_init();

/**
 * Set the image folder path for the emulated camera.
 * Must be called before emulated_camera_start().
 * @param ctx The camera context
 * @param folder_path Path to folder containing images
 * @return 0 on success, -1 on failure
 */
int emulated_camera_set_folder(EmulatedCameraContext* ctx, const char* folder_path);

/**
 * Set desired image dimensions (optional).
 * Images will be loaded at their original size if not set.
 * @param ctx The camera context
 * @param width Desired width
 * @param height Desired height
 * @return 0 on success, -1 on failure
 */
int emulated_camera_set_size(EmulatedCameraContext* ctx, int width, int height);

/**
 * Start the emulated camera (load image list from folder).
 * @param ctx The camera context
 * @return 0 on success, -1 on failure
 */
int emulated_camera_start(EmulatedCameraContext* ctx);

/**
 * Capture the next image from the folder (cycles through images).
 * @param ctx The camera context
 * @param out_buffer Pointer to receive image data (RGB format)
 * @param out_width Pointer to receive image width
 * @param out_height Pointer to receive image height
 * @param out_size Pointer to receive buffer size in bytes
 * @return 0 on success, -1 on failure
 * 
 * Note: The returned buffer must be freed by the caller using free().
 */
int emulated_camera_take_picture(EmulatedCameraContext* ctx, 
                                  uint8_t** out_buffer,
                                  int* out_width,
                                  int* out_height,
                                  size_t* out_size);

/**
 * Stop the emulated camera and reset state.
 * @param ctx The camera context
 */
void emulated_camera_stop(EmulatedCameraContext* ctx);

/**
 * Clean up and free the emulated camera context.
 * @param ctx The camera context
 */
void emulated_camera_cleanup(EmulatedCameraContext* ctx);

#ifdef __cplusplus
}
#endif

#endif // EMULATED_CAMERA_H

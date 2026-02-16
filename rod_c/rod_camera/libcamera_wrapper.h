#ifndef LIBCAMERA_WRAPPER_H
#define LIBCAMERA_WRAPPER_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Opaque handle to LibCamera context (actual definition in .cpp file)
typedef struct LibCameraContext LibCameraContext;

LibCameraContext* libcamera_init();
int libcamera_open_camera(LibCameraContext* ctx, int camera_index);
int libcamera_configure(LibCameraContext* ctx, int width, int height);
int libcamera_start(LibCameraContext* ctx);
int libcamera_stop(LibCameraContext* ctx);
int libcamera_capture_frame(LibCameraContext* ctx, uint8_t** out_buffer,
                            int* out_width, int* out_height,
                            size_t* out_size, int timeout_ms);
void libcamera_cleanup(LibCameraContext* ctx);

#ifdef __cplusplus
}
#endif

#endif // LIBCAMERA_WRAPPER_H

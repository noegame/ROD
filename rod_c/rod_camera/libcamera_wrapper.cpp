#include "libcamera_wrapper.h"
#include <libcamera/libcamera.h>
#include <memory>
#include <iostream>
#include <thread>
#include <chrono>
#include <cstring>
#include <sys/mman.h>
#include <vector>

using namespace libcamera;

// Internal definition of LibCameraContext (C++ types)
struct LibCameraContext {
    std::unique_ptr<CameraManager> camera_manager;
    std::shared_ptr<Camera> camera;
    std::unique_ptr<CameraConfiguration> config;
    FrameBufferAllocator *allocator;
    std::vector<std::unique_ptr<Request>> requests;
};

extern "C" {

LibCameraContext* libcamera_init() {
    LibCameraContext* ctx = new LibCameraContext();
    ctx->camera_manager = std::make_unique<CameraManager>();
    ctx->allocator = nullptr;

    int ret = ctx->camera_manager->start();
    if (ret) {
        delete ctx;
        return nullptr;
    }

    return ctx;
}

int libcamera_open_camera(LibCameraContext* ctx, int camera_index) {
    if (!ctx || !ctx->camera_manager)
        return -1;

    auto cameras = ctx->camera_manager->cameras();
    if (cameras.empty() || camera_index >= (int)cameras.size())
        return -1;

    ctx->camera = cameras[camera_index];
    if (ctx->camera->acquire()) {
        return -1;
    }

    return 0;
}

int libcamera_configure(LibCameraContext* ctx, int width, int height) {
    if (!ctx || !ctx->camera)
        return -1;

    ctx->config = ctx->camera->generateConfiguration({StreamRole::StillCapture});
    if (!ctx->config)
        return -1;

    StreamConfiguration &streamConfig = ctx->config->at(0);
    streamConfig.size.width = width;
    streamConfig.size.height = height;
    streamConfig.pixelFormat = PixelFormat::fromString("BGR888");

    if (ctx->config->validate() == CameraConfiguration::Invalid)
        return -1;

    if (ctx->camera->configure(ctx->config.get()) < 0)
        return -1;

    return 0;
}

int libcamera_start(LibCameraContext* ctx) {
    if (!ctx || !ctx->camera)
        return -1;

    ctx->allocator = new FrameBufferAllocator(ctx->camera);

    Stream *stream = ctx->config->at(0).stream();
    if (ctx->allocator->allocate(stream) < 0)
        return -1;

    // Configure camera controls (matching Python picamera2 settings)
    ControlList controls;
    
    // Enable auto-exposure (critical for proper exposure)
    controls.set(controls::AeEnable, true);
    
    // Set noise reduction to high quality (from camera.txt: NoiseReductionMode.HighQuality = 2)
    controls.set(controls::draft::NoiseReductionModeEnum, controls::draft::NoiseReductionModeHighQuality);
    
    // Set frame duration limits (from camera.txt: (100, 1000000000) = 100ns to 1s)
    controls.set(controls::FrameDurationLimits, Span<const int64_t, 2>({static_cast<int64_t>(100), static_cast<int64_t>(1000000000)}));
    
    // Apply controls and start camera
    if (ctx->camera->start(&controls) < 0)
        return -1;

    return 0;
}

int libcamera_stop(LibCameraContext* ctx) {
    if (!ctx || !ctx->camera)
        return -1;

    return ctx->camera->stop();
}

/**
 * Capture a single frame and return its buffer, dimensions and size.
 * Returns BGR888 format buffer (OpenCV native format).
 * The caller must free() the returned buffer.
 * Returns 0 on success, -1 on failure.
 */
int libcamera_capture_frame(LibCameraContext* ctx, uint8_t** out_buffer,
                            int* out_width, int* out_height,
                            size_t* out_size, int timeout_ms) {
    if (!ctx || !ctx->camera || !ctx->allocator)
        return -1;

    std::unique_ptr<Request> request = ctx->camera->createRequest();
    if (!request)
        return -1;

    Stream *stream = ctx->config->at(0).stream();
    const auto &buffers = ctx->allocator->buffers(stream);
    if (buffers.empty())
        return -1;

    request->addBuffer(stream, buffers[0].get());

    int ret = ctx->camera->queueRequest(request.get());
    if (ret < 0)
        return -1;

    // Wait for completion - simplified synchronous approach
    // For MVP: poll with sleep (proper event loop could be added later)
    std::this_thread::sleep_for(std::chrono::milliseconds(timeout_ms));

    FrameBuffer *buffer = request->findBuffer(stream);
    if (!buffer)
        return -1;

    // Get stream configuration for dimensions
    const StreamConfiguration &cfg = ctx->config->at(0);
    *out_width = cfg.size.width;
    *out_height = cfg.size.height;

    // Map buffer and copy data to caller-owned buffer
    const FrameBuffer::Plane &plane = buffer->planes().front();
    void *mem = mmap(nullptr, plane.length, PROT_READ, MAP_SHARED, plane.fd.get(), 0);
    if (mem == MAP_FAILED)
        return -1;

    // Allocate buffer for caller (BGR888: 3 bytes per pixel)
    size_t data_size = (*out_width) * (*out_height) * 3;
    *out_buffer = (uint8_t*)malloc(data_size);
    if (!(*out_buffer)) {
        munmap(mem, plane.length);
        return -1;
    }

    // Copy data from mmap'd buffer
    memcpy(*out_buffer, mem, data_size);
    *out_size = data_size;

    // Unmap the buffer
    munmap(mem, plane.length);

    return 0;
}

void libcamera_cleanup(LibCameraContext* ctx) {
    if (!ctx)
        return;

    if (ctx->camera) {
        ctx->camera->stop();
        ctx->camera->release();
    }

    if (ctx->allocator)
        delete ctx->allocator;

    if (ctx->camera_manager)
        ctx->camera_manager->stop();

    delete ctx;
}



}

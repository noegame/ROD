#include "libcamera_wrapper.h"
#include "camera.h"
#include <libcamera/libcamera.h>
#include <memory>
#include <iostream>
#include <thread>
#include <chrono>
#include <cstring>
#include <sys/mman.h>
#include <vector>
#include <queue>
#include <condition_variable>
#include <mutex>

using namespace libcamera;

// Internal definition of LibCameraContext (C++ types)
struct LibCameraContext {
    std::unique_ptr<CameraManager> camera_manager;
    std::shared_ptr<Camera> camera;
    std::unique_ptr<CameraConfiguration> config;
    FrameBufferAllocator *allocator;
    std::vector<std::unique_ptr<Request>> requests;
    
    // Synchronization for request completion
    std::mutex request_mutex;
    std::condition_variable request_cv;
    std::queue<Request*> completed_requests;  // Queue of completed requests (not just one)
    bool running;  // Flag to control continuous capture
};

// Static context pointer for signal callback (single camera support)
static LibCameraContext *g_active_context = nullptr;
static std::mutex g_context_mutex;

// Static callback handler for request completion signal
// Follows libcamera pattern: add to queue, requeue immediately, notify
static void request_completed_handler(Request *request) {
    std::lock_guard<std::mutex> ctx_lock(g_context_mutex);
    
    if (!g_active_context || !request) return;
    
    {
        std::lock_guard<std::mutex> lock(g_active_context->request_mutex);
        
        // Add to queue for processing (only if status is good)
        if (request->status() == Request::RequestComplete) {
            g_active_context->completed_requests.push(request);
            g_active_context->request_cv.notify_one();
        } else if (request->status() == Request::RequestCancelled) {
            // Don't queue cancelled requests
            std::cerr << "Request cancelled in callback" << std::endl;
        }
    }
    
    // Requeue immediately for continuous capture (standard libcamera pattern)
    // This happens OUTSIDE the queue lock to avoid deadlock
    if (g_active_context->running && request->status() == Request::RequestComplete) {
        request->reuse(Request::ReuseBuffers);
        g_active_context->camera->queueRequest(request);
    }
}

extern "C" {

LibCameraContext* libcamera_init() {
    LibCameraContext* ctx = new LibCameraContext();
    ctx->camera_manager = std::make_unique<CameraManager>();
    ctx->allocator = nullptr;
    // completed_requests queue is constructed empty by default
    ctx->running = false;

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

    // Register this context as the active one
    {
        std::lock_guard<std::mutex> lock(g_context_mutex);
        g_active_context = ctx;
    }

    // Connect static callback handler for request completion
    ctx->camera->requestCompleted.connect(&request_completed_handler);

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

/**
 * Build ControlList based on CameraParameters.
 * If params is NULL or a field is -1, use default value.
 */
static ControlList build_control_list(const struct CameraParameters* params) {
    ControlList controls;
    
    // Auto-exposure (default: true)
    bool ae_enable = true;
    if (params && params->ae_enable >= 0) {
        ae_enable = (params->ae_enable != 0);
    }
    controls.set(controls::AeEnable, ae_enable);
    
    // Manual exposure time (only if AE is disabled)
    if (params && !ae_enable && params->exposure_time >= 0) {
        controls.set(controls::ExposureTime, static_cast<int32_t>(params->exposure_time));
    }
    
    // Analogue gain (manual or hint for AE)
    if (params && params->analogue_gain >= 0.0) {
        controls.set(controls::AnalogueGain, static_cast<float>(params->analogue_gain));
    }
    
    // Noise reduction mode (default: HighQuality = 2)
    int nr_mode = 2; // HighQuality
    if (params && params->noise_reduction_mode >= 0) {
        nr_mode = params->noise_reduction_mode;
    }
    // Map to libcamera enum
    switch (nr_mode) {
        case 0: controls.set(controls::draft::NoiseReductionMode, static_cast<int32_t>(controls::draft::NoiseReductionModeOff)); break;
        case 1: controls.set(controls::draft::NoiseReductionMode, static_cast<int32_t>(controls::draft::NoiseReductionModeFast)); break;
        case 2: controls.set(controls::draft::NoiseReductionMode, static_cast<int32_t>(controls::draft::NoiseReductionModeHighQuality)); break;
        case 3: controls.set(controls::draft::NoiseReductionMode, static_cast<int32_t>(controls::draft::NoiseReductionModeMinimal)); break;
        case 4: controls.set(controls::draft::NoiseReductionMode, static_cast<int32_t>(controls::draft::NoiseReductionModeZSL)); break;
        default: controls.set(controls::draft::NoiseReductionMode, static_cast<int32_t>(controls::draft::NoiseReductionModeHighQuality));
    }
    
    // Sharpness (default: 1.0)
    if (params && params->sharpness >= 0.0) {
        controls.set(controls::Sharpness, static_cast<float>(params->sharpness));
    }
    
    // Contrast (default: 1.0)
    if (params && params->contrast >= 0.0) {
        controls.set(controls::Contrast, static_cast<float>(params->contrast));
    }
    
    // Brightness (default: 0.0)
    if (params && params->brightness >= -1.0) {
        controls.set(controls::Brightness, static_cast<float>(params->brightness));
    }
    
    // Saturation (default: 1.0)
    if (params && params->saturation >= 0.0) {
        controls.set(controls::Saturation, static_cast<float>(params->saturation));
    }
    
    // Auto white balance (default: true)
    bool awb_enable = true;
    if (params && params->awb_enable >= 0) {
        awb_enable = (params->awb_enable != 0);
    }
    controls.set(controls::AwbEnable, awb_enable);
    
    // Colour temperature (only if AWB is disabled)
    if (params && !awb_enable && params->colour_temperature >= 0) {
        controls.set(controls::ColourTemperature, static_cast<int32_t>(params->colour_temperature));
    }
    
    // Frame duration limits (default: 100ns to 1s)
    int64_t frame_min = 100;
    int64_t frame_max = 1000000000;
    if (params && params->frame_duration_min >= 0) {
        frame_min = params->frame_duration_min;
    }
    if (params && params->frame_duration_max >= 0) {
        frame_max = params->frame_duration_max;
    }
    controls.set(controls::FrameDurationLimits, Span<const int64_t, 2>({frame_min, frame_max}));
    
    return controls;
}

int libcamera_start_with_params(LibCameraContext* ctx, const struct CameraParameters* params) {
    if (!ctx || !ctx->camera)
        return -1;

    // Only create allocator if it doesn't exist (prevents memory leak)
    if (!ctx->allocator) {
        ctx->allocator = new FrameBufferAllocator(ctx->camera);

        Stream *stream = ctx->config->at(0).stream();
        if (ctx->allocator->allocate(stream) < 0) {
            delete ctx->allocator;
            ctx->allocator = nullptr;
            return -1;
        }
        
        // Create requests for each allocated buffer (per libcamera docs pattern)
        const auto &buffers = ctx->allocator->buffers(stream);
        for (const auto &buffer : buffers) {
            std::unique_ptr<Request> request = ctx->camera->createRequest();
            if (!request) {
                std::cerr << "Failed to create request" << std::endl;
                return -1;
            }
            
            if (request->addBuffer(stream, buffer.get()) < 0) {
                std::cerr << "Failed to add buffer to request" << std::endl;
                return -1;
            }
            
            ctx->requests.push_back(std::move(request));
        }
    }

    // Build control list from parameters
    ControlList controls = build_control_list(params);
    
    // Apply controls and start camera
    if (ctx->camera->start(&controls) < 0)
        return -1;
    
    // Set running flag before queuing requests
    ctx->running = true;
    
    // Queue all requests to start continuous capture (per libcamera docs)
    for (auto &request : ctx->requests) {
        if (ctx->camera->queueRequest(request.get()) < 0) {
            std::cerr << "Failed to queue initial request" << std::endl;
            ctx->running = false;
            return -1;
        }
    }

    return 0;
}

int libcamera_stop(LibCameraContext* ctx) {
    if (!ctx || !ctx->camera)
        return -1;

    // Stop requeueing in callback before stopping camera
    ctx->running = false;
    
    // Give time for any pending requests to complete without requeue
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Stop camera
    int ret = ctx->camera->stop();
    
    // Clear the completed requests queue
    {
        std::lock_guard<std::mutex> lock(ctx->request_mutex);
        while (!ctx->completed_requests.empty()) {
            ctx->completed_requests.pop();
        }
    }
    
    // Clear requests and allocator for clean restart
    ctx->requests.clear();
    
    if (ctx->allocator) {
        delete ctx->allocator;
        ctx->allocator = nullptr;
    }
    
    return ret;
}

/**
 * Capture a single frame and return its buffer, dimensions and size.
 * Returns BGR888 format buffer (OpenCV native format).
 * The caller must free() the returned buffer.
 * Returns 0 on success, -1 on failure.
 * 
 * Note: Requests are automatically requeued by the completion callback,
 * so this function only waits for and processes the buffer.
 */
int libcamera_capture_frame(LibCameraContext* ctx, uint8_t** out_buffer,
                            int* out_width, int* out_height,
                            size_t* out_size, int timeout_ms) {
    if (!ctx || !ctx->camera || !ctx->allocator)
        return -1;

    // Wait for a completed request to be available in the queue
    std::unique_lock<std::mutex> lock(ctx->request_mutex);
    bool has_request = ctx->request_cv.wait_for(
        lock,
        std::chrono::milliseconds(timeout_ms),
        [ctx]() { return !ctx->completed_requests.empty(); }
    );

    if (!has_request) {
        std::cerr << "Frame capture timeout after " << timeout_ms << "ms" << std::endl;
        return -1;
    }

    // Pop the oldest completed request from the queue
    Request* request = ctx->completed_requests.front();
    ctx->completed_requests.pop();
    lock.unlock();  // Release lock while processing buffer

    if (!request) {
        std::cerr << "Null request in queue" << std::endl;
        return -1;
    }

    // Get buffer from request
    Stream *stream = ctx->config->at(0).stream();
    FrameBuffer *buffer = request->findBuffer(stream);
    if (!buffer) {
        std::cerr << "No buffer found in completed request" << std::endl;
        return -1;
    }

    // Get stream configuration for dimensions
    const StreamConfiguration &cfg = ctx->config->at(0);
    *out_width = cfg.size.width;
    *out_height = cfg.size.height;

    // Map buffer and copy data to caller-owned buffer
    const FrameBuffer::Plane &plane = buffer->planes().front();
    void *mem = mmap(nullptr, plane.length, PROT_READ, MAP_SHARED, plane.fd.get(), 0);
    if (mem == MAP_FAILED) {
        std::cerr << "Failed to mmap buffer" << std::endl;
        return -1;
    }

    // Allocate buffer for caller (BGR888: 3 bytes per pixel)
    size_t data_size = (*out_width) * (*out_height) * 3;
    *out_buffer = (uint8_t*)malloc(data_size);
    if (!(*out_buffer)) {
        munmap(mem, plane.length);
        return -1;
    }
    
    // Debug: Check if buffer size matches expectation
    if (plane.length < data_size) {
        std::cerr << "Warning: Buffer size mismatch - expected " << data_size 
                  << " bytes but got " << plane.length << " bytes" << std::endl;
    }

    // Copy data from mmap'd buffer
    memcpy(*out_buffer, mem, data_size);
    *out_size = data_size;

    // Unmap the buffer
    munmap(mem, plane.length);
    
    // Request is already requeued by the callback - nothing more to do here

    return 0;
}

void libcamera_cleanup(LibCameraContext* ctx) {
    if (!ctx)
        return;

    // Unregister context if it's the active one
    {
        std::lock_guard<std::mutex> lock(g_context_mutex);
        if (g_active_context == ctx) {
            g_active_context = nullptr;
        }
    }

    if (ctx->camera) {
        // Disconnect signal handler first
        ctx->camera->requestCompleted.disconnect(&request_completed_handler);
        
        // Stop camera if still running (ignore error if already stopped)
        if (ctx->running) {
            ctx->running = false;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            ctx->camera->stop();
        }
    }
    
    // Clear the completed requests queue
    {
        std::lock_guard<std::mutex> lock(ctx->request_mutex);
        while (!ctx->completed_requests.empty()) {
            ctx->completed_requests.pop();
        }
    }
    
    // Clear requests before deleting allocator
    ctx->requests.clear();
    
    // Delete allocator after camera is stopped
    if (ctx->allocator) {
        delete ctx->allocator;
        ctx->allocator = nullptr;
    }

    // Release camera
    if (ctx->camera) {
        ctx->camera->release();
        ctx->camera.reset();
    }

    // Stop camera manager last
    if (ctx->camera_manager)
        ctx->camera_manager->stop();

    delete ctx;
}



}

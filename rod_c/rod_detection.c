/**
 * @file rod_detection.c
 * @brief Computer vision thread for ArUco marker detection
 * @author Noé Game
 * @date 15/02/2026
 * @see rod_detection.c
 * @copyright Cecill-C (Cf. LICENCE.txt)
 * 
 * This program implements the computer vision thread that:
 * - Captures images using the emulated camera
 * - Detects ArUco markers on game elements
 * - Sends detected positions to the IPC thread via socket communication
 */

#define _POSIX_C_SOURCE 199309L  // Required for clock_gettime and CLOCK_MONOTONIC
#define _DEFAULT_SOURCE          // Required for usleep

/* ******************************************************* Includes ****************************************************** */

#include "camera_interface.h"
#include "opencv_wrapper.h"
#include "rod_cv.h"
#include "rod_config.h"
#include "rod_visualization.h"
#include "rod_socket.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <signal.h>
#include <stdbool.h>
#include <time.h>

/* ***************************************************** Public macros *************************************************** */

// Default image folder path for emulated camera
#define DEFAULT_IMAGE_FOLDER ROD_DEFAULT_IMAGE_FOLDER

// Debug image output base folder
#define DEBUG_BASE_FOLDER ROD_DEBUG_BASE_FOLDER

// Camera pictures base folder
#define PICTURES_BASE_FOLDER ROD_PICTURES_BASE_FOLDER

// Socket configuration
#define SOCKET_PATH ROD_SOCKET_PATH
#define MAX_DETECTION_SIZE ROD_MAX_DETECTION_SIZE

// Debug image saving (save one annotated image every N frames)
#define SAVE_DEBUG_IMAGE_INTERVAL ROD_SAVE_DEBUG_IMAGE_INTERVAL

// Detection pipeline parameters (must match Python implementation)
#define DETECTION_SCALE_FACTOR 1.0f  // Resize scale for better detection

/* ************************************************** Public types definition ******************************************** */

/**
 * @brief Application context
 */
typedef struct {
    Camera* camera;
    ArucoDetectorHandle* detector;
    ArucoDictionaryHandle* dictionary;
    DetectorParametersHandle* params;
    RodSocketServer* socket_server;
    ImageHandle* field_mask;  // Field mask for filtering detections
    
    // Reusable buffers to reduce memory allocations
    ImageHandle* buffer_sharpened;  // Buffer for sharpened image
    ImageHandle* buffer_masked;     // Buffer for masked image
    ImageHandle* buffer_resized;    // Buffer for resized image
    
    bool running;
} AppContext;

/* *********************************************** Public functions declarations ***************************************** */

/**
 * @brief Initialize application context
 * @param ctx Application context
 * @param camera_type Camera type (CAMERA_TYPE_REAL or CAMERA_TYPE_EMULATED)
 * @param image_folder Path to image folder (for emulated camera)
 * @return 0 on success, -1 on failure
 */
static int init_app_context(AppContext* ctx, CameraType camera_type, const char* image_folder);

/**
 * @brief Cleanup application context
 * @param ctx Application context
 */
static void cleanup_app_context(AppContext* ctx);



/**
 * @brief Signal handler for graceful shutdown
 * @param signum Signal number
 */
static void signal_handler(int signum);

/* ******************************************* Global variables ******************************************************* */

static volatile bool g_running = true;

/* ******************************************* Public callback functions declarations ************************************ */

/**
 * @brief Get current time in milliseconds
 * @return Time in milliseconds  
 */
static double get_time_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1000000.0;
}

static void signal_handler(int signum) {
    (void)signum;  // Unused parameter
    g_running = false;
    printf("\nReceived interrupt signal, shutting down...\n");
}

static int init_app_context(AppContext* ctx, CameraType camera_type, const char* image_folder) {
    memset(ctx, 0, sizeof(AppContext));
    ctx->socket_server = NULL;
    ctx->field_mask = NULL;
    ctx->buffer_sharpened = NULL;
    ctx->buffer_masked = NULL;
    ctx->buffer_resized = NULL;
    ctx->running = true;
    
    // Initialize camera based on type
    printf("Initializing %s camera...\n", 
           camera_type == CAMERA_TYPE_EMULATED ? "emulated" : "real");
    ctx->camera = camera_create(camera_type);
    if (!ctx->camera) {
        fprintf(stderr, "Failed to initialize camera\n");
        return -1;
    }
    
    // Set camera resolution to full IMX477 sensor resolution (4056x3040)
    if (camera_interface_set_size(ctx->camera, 4056, 3040) != 0) {
        fprintf(stderr, "Failed to set camera resolution to 4056x3040\n");
        camera_destroy(ctx->camera);
        return -1;
    }
    printf("Camera resolution set to 4056x3040\n");
    
    // Configure camera based on type
    if (camera_type == CAMERA_TYPE_EMULATED) {
        // Set image folder for emulated camera
        if (camera_interface_set_folder(ctx->camera, image_folder) != 0) {
            fprintf(stderr, "Failed to set image folder: %s\n", image_folder);
            return -1;
        }
        printf("Emulated camera folder: %s\n", image_folder);
    } else {
        // Configure real camera with "match" parameters from test_camera_parameters.c
        // ArUco optimized for full resolution (4056x3040)
        RodCameraParameters params;
        params.exposure_time = -1;           // Let AE decide (auto-exposure)
        params.analogue_gain = -1.0f;        // Let AE decide (auto-exposure)
        params.brightness = 0.0f;            // Default brightness
        params.contrast = 1.5f;              // Slight contrast boost for black/white markers
        params.saturation = -1.0f;           // Default saturation (auto)
        params.sharpness = 4.0f;             // Moderate sharpness boost for ArUco detection
        params.awb_enable = 1;               // Auto white balance enabled
        params.aec_enable = 1;               // Auto-exposure enabled for adaptability
        params.noise_reduction_mode = 2;     // HighQuality
        
        camera_interface_set_parameters(ctx->camera, &params);
        printf("Real camera using 'match' parameters (4056x3040, ArUco optimized)\n");
    }
    
    // Start camera
    if (camera_interface_start(ctx->camera) != 0) {
        fprintf(stderr, "Failed to start camera\n");
        return -1;
    }
    printf("Camera started successfully\n");
    
    // Initialize ArUco detector
    printf("Initializing ArUco detector...\n");
    ctx->dictionary = getPredefinedDictionary(rod_config_get_aruco_dictionary_type());
    if (!ctx->dictionary) {
        fprintf(stderr, "Failed to create ArUco dictionary\n");
        return -1;
    }
    
    ctx->params = createDetectorParameters();
    if (!ctx->params) {
        fprintf(stderr, "Failed to create detector parameters\n");
        return -1;
    }
    
    // Configure detector with optimized parameters
    rod_config_configure_detector_parameters(ctx->params);
    
    ctx->detector = createArucoDetector(ctx->dictionary, ctx->params);
    if (!ctx->detector) {
        fprintf(stderr, "Failed to create ArUco detector\n");
        return -1;
    }
    printf("ArUco detector initialized (DICT_4X4_50)\n");
    
    // Field mask will be created dynamically from first captured frame
    // that contains all 4 fixed markers (IDs 20-23)
    printf("Field mask will be created dynamically from captured frames\n");
    ctx->field_mask = NULL;
    
    return 0;
}

static void cleanup_app_context(AppContext* ctx) {
    if (!ctx) return;
    
    // Release reusable buffers
    if (ctx->buffer_resized) {
        release_image(ctx->buffer_resized);
        ctx->buffer_resized = NULL;
    }
    
    if (ctx->buffer_masked) {
        release_image(ctx->buffer_masked);
        ctx->buffer_masked = NULL;
    }
    
    if (ctx->buffer_sharpened) {
        release_image(ctx->buffer_sharpened);
        ctx->buffer_sharpened = NULL;
    }
    
    // Release field mask
    if (ctx->field_mask) {
        release_image(ctx->field_mask);
        ctx->field_mask = NULL;
    }
    
    // Cleanup ArUco detector
    if (ctx->detector) {
        releaseArucoDetector(ctx->detector);
        ctx->detector = NULL;
    }
    
    if (ctx->dictionary) {
        releaseArucoDictionary(ctx->dictionary);
        ctx->dictionary = NULL;
    }
    
    if (ctx->params) {
        releaseDetectorParameters(ctx->params);
        ctx->params = NULL;
    }
    
    // Cleanup camera
    if (ctx->camera) {
        camera_interface_stop(ctx->camera);
        camera_destroy(ctx->camera);
        ctx->camera = NULL;
    }
    
    // Close socket
    if (ctx->socket_server) {
        rod_socket_server_destroy(ctx->socket_server);
        ctx->socket_server = NULL;
    }
}

/**
 * @brief Main function of the program
 * Takes a picture with the camera, find the aruco markers position with rod-cv, 
 * and send the position to rod-com via socket.
 */
int main(int argc, char* argv[]) {
    AppContext ctx;
    const char* image_folder = DEFAULT_IMAGE_FOLDER;
    CameraType camera_type = CAMERA_TYPE_IMX477;  // Default to real camera
    
    // Parse command line arguments
    // Usage: rod_detection [--camera real|emulated] [image_folder]
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--camera") == 0 && i + 1 < argc) {
            i++;
            if (strcmp(argv[i], "real") == 0) {
                camera_type = CAMERA_TYPE_IMX477;
            } else if (strcmp(argv[i], "emulated") == 0) {
                camera_type = CAMERA_TYPE_EMULATED;
            } else {
                fprintf(stderr, "Unknown camera type: %s (use 'real' or 'emulated')\n", argv[i]);
                return 1;
            }
        } else {
            // Assume it's the image folder path
            image_folder = argv[i];
        }
    }
    
    // Check environment variable for camera type (command-line takes precedence)
    const char* env_camera = getenv("ROD_CAMERA_TYPE");
    if (env_camera && argc == 1) {  // Only use env var if no command-line args
        if (strcmp(env_camera, "real") == 0) {
            camera_type = CAMERA_TYPE_IMX477;
        } else if (strcmp(env_camera, "emulated") == 0) {
            camera_type = CAMERA_TYPE_EMULATED;
        }
    }
    
    printf("=== ROD Detection - Computer Vision Thread ===\n");
    printf("Camera type: %s\n", camera_type == CAMERA_TYPE_IMX477 ? "Real (IMX477)" : "Emulated");
    if (camera_type == CAMERA_TYPE_EMULATED) {
        printf("Image folder: %s\n", image_folder);
    }
    printf("\n");
    
    // Setup signal handler for graceful shutdown
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // Initialize application context
    if (init_app_context(&ctx, camera_type, image_folder) != 0) {
        fprintf(stderr, "Failed to initialize application\n");
        cleanup_app_context(&ctx);
        return 1;
    }
    
    // Initialize socket server
    ctx.socket_server = rod_socket_server_create(SOCKET_PATH);
    if (!ctx.socket_server) {
        fprintf(stderr, "Failed to initialize socket server\n");
        cleanup_app_context(&ctx);
        return 1;
    }
    
    printf("\nStarting detection loop (Ctrl+C to stop)...\n");
    
    // Main detection loop
    int frame_count = 0;
    while (g_running && ctx.running) {
        frame_count++;
        double t_loop_start = get_time_ms();
        
        // Generate timestamp for this frame (used for both logging and file naming)
        char frame_timestamp[32];
        rod_config_generate_filename_timestamp(frame_timestamp, sizeof(frame_timestamp));
        
        // Try to accept a client connection if not already connected
        rod_socket_server_accept(ctx.socket_server);
        
        // Capture image from camera
        uint8_t* image_buffer = NULL;
        int width, height;
        size_t size;
        double t_capture_start = get_time_ms();
        
        if (camera_interface_capture_frame(ctx.camera, &image_buffer, 
                                &width, &height, &size) != 0) {
            fprintf(stderr, "Failed to capture image\n");
            usleep(10000);  // Wait 10ms before retry
            continue;
        }
        double t_capture_end = get_time_ms();
        
        // Create OpenCV image handle from BGR buffer
        // Camera returns BGR format (format=0, OpenCV native - no conversion)
        double t_create_start = get_time_ms();
        ImageHandle* original_image = create_image_from_buffer(image_buffer, width, height, 3, 0);
        free(image_buffer);  // Buffer is copied by create_image_from_buffer
        double t_create_end = get_time_ms();
        
        if (!original_image) {
            fprintf(stderr, "Failed to create image from buffer\n");
            usleep(10000);  // Wait 10ms before retry
            continue;
        }
        
        // ===== PREPROCESSING PIPELINE (matching Python implementation) =====
        // Step 1: Apply sharpening filter to enhance marker edges (reuse buffer)
        double t_sharpen_start = get_time_ms();
        ctx.buffer_sharpened = sharpen_image_reuse(original_image, ctx.buffer_sharpened);
        double t_sharpen_end = get_time_ms();
        if (!ctx.buffer_sharpened) {
            fprintf(stderr, "Failed to sharpen image\n");
            release_image(original_image);
            usleep(10000);
            continue;
        }
        
        // Step 2: Create field mask if not already created (from current frame)
        double t_mask_start = get_time_ms();
        if (!ctx.field_mask) {
            // Try to create mask from current sharpened image
            ctx.field_mask = create_field_mask_from_image(ctx.buffer_sharpened, ctx.detector, width, height, 1.1f, NULL);
            if (ctx.field_mask) {
                printf("[Frame %d] Field mask created successfully from captured frame\n", frame_count);
            }
        }
        
        // Step 3: Apply field mask to filter out areas outside the playing field (reuse buffer)
        ImageHandle* masked_image = ctx.buffer_sharpened;  // Default to sharpened
        if (ctx.field_mask) {
            ctx.buffer_masked = bitwise_and_mask_reuse(ctx.buffer_sharpened, ctx.field_mask, ctx.buffer_masked);
            if (!ctx.buffer_masked) {
                fprintf(stderr, "Failed to apply mask, using unmasked image\n");
                masked_image = ctx.buffer_sharpened;
            } else {
                masked_image = ctx.buffer_masked;
            }
        }
        double t_mask_end = get_time_ms();
        
        // Step 4: Resize image (1.5x scale) for better detection of small/distant markers (reuse buffer)
        double t_resize_start = get_time_ms();
        int new_width = (int)(width * DETECTION_SCALE_FACTOR);
        int new_height = (int)(height * DETECTION_SCALE_FACTOR);
        ctx.buffer_resized = resize_image_reuse(masked_image, new_width, new_height, ctx.buffer_resized);
        
        double t_resize_end = get_time_ms();
        if (!ctx.buffer_resized) {
            fprintf(stderr, "Failed to resize image\n");
            release_image(original_image);
            usleep(10000);
            continue;
        }
        
        // Step 5: Detect ArUco markers on preprocessed image
        double t_detect_start = get_time_ms();
        DetectionResult* detection = detectMarkersWithConfidence(ctx.detector, ctx.buffer_resized);
        // Note: We keep buffer_resized for reuse in next frame
        double t_detect_end = get_time_ms();
        
        // Step 6: Scale coordinates back to original image size
        double t_process_start = get_time_ms();
        if (detection && detection->count > 0) {
            for (int i = 0; i < detection->count; i++) {
                for (int j = 0; j < 4; j++) {
                    detection->markers[i].corners[j][0] /= DETECTION_SCALE_FACTOR;
                    detection->markers[i].corners[j][1] /= DETECTION_SCALE_FACTOR;
                }
            }
        }
        
        if (detection && detection->count > 0) {
            // Filter and process detected markers using rod_cv module
            MarkerData markers[100];  // Max 100 markers
            int valid_count = filter_valid_markers(detection, markers, 100);
            
            // Count markers by category for reporting
            MarkerCounts marker_counts = count_markers_by_category(markers, valid_count);
            
            // Send detection results
            if (valid_count > 0) {
                rod_socket_server_send_detections(ctx.socket_server, markers, valid_count);
            }
            double t_send_end = get_time_ms();
            
            // Save images periodically (raw camera + debug annotated)
            double t_annotate_start = get_time_ms();
            double t_annotate_end = t_annotate_start;  // Will be updated if annotation happens
            double t_save_start = get_time_ms();
            if (frame_count % SAVE_DEBUG_IMAGE_INTERVAL == 0) {
                // Ensure date folders exist
                char pictures_date_folder[256];
                char debug_date_folder[256];
                if (rod_config_ensure_date_folder(PICTURES_BASE_FOLDER, pictures_date_folder, sizeof(pictures_date_folder)) == 0 &&
                    rod_config_ensure_date_folder(DEBUG_BASE_FOLDER, debug_date_folder, sizeof(debug_date_folder)) == 0) {
                    
                    // 1. Save raw camera image: /var/roboteseo/pictures/YYYY_MM_DD/YYYYMMDD_HHMMSS_MS.jpg
                    char filename_camera[512];
                    snprintf(filename_camera, sizeof(filename_camera), "%s/%s.jpg", pictures_date_folder, frame_timestamp);
                    save_image(filename_camera, original_image);
                    
                    // 2. Save annotated debug image: /var/roboteseo/pictures/debug/YYYY_MM_DD/YYYYMMDD_HHMMSS_MS_debug.png
                    int img_width = get_image_width(original_image);
                    int img_height = get_image_height(original_image);
                    int img_channels = get_image_channels(original_image);
                    uint8_t* img_data = get_image_data(original_image);
                    size_t img_data_size = get_image_data_size(original_image);
                    
                    if (img_data && img_data_size > 0) {
                        uint8_t* data_copy = (uint8_t*)malloc(img_data_size);
                        if (data_copy) {
                            memcpy(data_copy, img_data, img_data_size);
                            ImageHandle* annotated = create_image_from_buffer(data_copy, img_width, img_height, img_channels, 0);
                            free(data_copy);
                            
                            if (annotated) {
                                t_annotate_start = get_time_ms();
                                rod_viz_annotate_with_colored_quadrilaterals(annotated, detection);
                                rod_viz_annotate_with_counter(annotated, marker_counts);
                                rod_viz_annotate_with_ids(annotated, markers, valid_count);
                                rod_viz_annotate_with_centers(annotated, markers, valid_count);
                                t_annotate_end = get_time_ms();
                                
                                // Convert BGR to RGB for output
                                ImageHandle* annotated_rgb = convert_bgr_to_rgb(annotated);
                                if (annotated_rgb) {
                                    release_image(annotated);
                                    annotated = annotated_rgb;
                                }
                                
                                char filename_debug[512];
                                snprintf(filename_debug, sizeof(filename_debug), "%s/%s_debug.jpg", debug_date_folder, frame_timestamp);
                                save_image(filename_debug, annotated);
                                release_image(annotated);
                            }
                        }
                    }
                    
                    // Frame saved message will be printed below
                }
            }
            double t_save_end = get_time_ms();
            
            // Print detection summary and timing breakdown
            double t_loop_end = get_time_ms();
            
            printf("\n=== Frame %s ===\n", frame_timestamp);
            printf("\n=== Detection Summary ===\n");
            printf("Black markers: %d\n", marker_counts.black_markers);
            printf("Blue markers: %d\n", marker_counts.blue_markers);
            printf("Yellow markers: %d\n", marker_counts.yellow_markers);
            printf("Robots markers: %d\n", marker_counts.robot_markers);
            printf("Fixed markers: %d\n", marker_counts.fixed_markers);
            printf("TOTAL: %d\n", valid_count);
            
            printf("\n=== Timing Summary ===\n");
            printf("Capture: %.1fms\n", t_capture_end - t_capture_start);
            printf("Load: %.1fms\n", t_create_end - t_create_start);
            printf("Sharpen: %.1fms\n", t_sharpen_end - t_sharpen_start);
            printf("Mask: %.1fms\n", t_mask_end - t_mask_start);
            printf("Resize: %.1fms\n", t_resize_end - t_resize_start);
            printf("Detect: %.1fms\n", t_detect_end - t_detect_start);
            printf("Process: %.1fms\n", t_send_end - t_process_start);
            printf("Reload: 0.0ms\n");  // Buffers are reused, no reload
            printf("Annotate: %.1fms\n", t_annotate_end - t_annotate_start);
            printf("Save: %.1fms\n", t_save_end - t_save_start);
            printf("TOTAL: %.1fms\n", t_loop_end - t_loop_start);
            
            releaseDetectionResult(detection);
        } else {
            // No markers detected
            double t_loop_end = get_time_ms();
            if (frame_count % 10 == 0) {
                printf("\n=== Frame %s ===\n", frame_timestamp);
                printf("\n=== Detection Summary ===\n");
                printf("Black markers: 0\n");
                printf("Blue markers: 0\n");
                printf("Yellow markers: 0\n");
                printf("Robots markers: 0\n");
                printf("Fixed markers: 0\n");
                printf("TOTAL: 0\n");
                
                printf("\n=== Timing Summary ===\n");
                printf("Capture: %.1fms\n", t_capture_end - t_capture_start);
                printf("Load: %.1fms\n", t_create_end - t_create_start);
                printf("Sharpen: %.1fms\n", t_sharpen_end - t_sharpen_start);
                printf("Mask: %.1fms\n", t_mask_end - t_mask_start);
                printf("Resize: %.1fms\n", t_resize_end - t_resize_start);
                printf("Detect: %.1fms\n", t_detect_end - t_detect_start);
                printf("Process: 0.0ms\n");  // No processing when no markers
                printf("Reload: 0.0ms\n");
                printf("Annotate: 0.0ms\n");
                printf("Save: 0.0ms\n");  // No save on this frame (if not SAVE_DEBUG_IMAGE_INTERVAL)
                printf("TOTAL: %.1fms\n", t_loop_end - t_loop_start);
            }
            
            // Save images periodically even when no markers detected
            if (frame_count % SAVE_DEBUG_IMAGE_INTERVAL == 0) {
                // Ensure date folders exist
                char pictures_date_folder[256];
                char debug_date_folder[256];
                if (rod_config_ensure_date_folder(PICTURES_BASE_FOLDER, pictures_date_folder, sizeof(pictures_date_folder)) == 0 &&
                    rod_config_ensure_date_folder(DEBUG_BASE_FOLDER, debug_date_folder, sizeof(debug_date_folder)) == 0) {
                    
                    // Save raw camera image
                    char filename_camera[512];
                    snprintf(filename_camera, sizeof(filename_camera), "%s/%s.jpg", pictures_date_folder, frame_timestamp);
                    save_image(filename_camera, original_image);
                    
                    // Save debug image (no annotations, but in debug folder) in RGB format
                    ImageHandle* debug_rgb = convert_bgr_to_rgb(original_image);
                    if (debug_rgb) {
                        char filename_debug[512];
                        snprintf(filename_debug, sizeof(filename_debug), "%s/%s_debug.jpg", debug_date_folder, frame_timestamp);
                        save_image(filename_debug, debug_rgb);
                        release_image(debug_rgb);
                    }
                    
                    // Image saved (no markers case)
                }
            }
        }
        
        // Release only original_image (buffers are reused and freed in cleanup)
        release_image(original_image);
    }
    
    printf("\nShutting down...\n");
    printf("Total frames processed: %d\n", frame_count);
    
    // Cleanup
    cleanup_app_context(&ctx);
    
    printf("ROD Detection stopped successfully\n");
    return 0;
}
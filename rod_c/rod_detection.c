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
#define DETECTION_SCALE_FACTOR 1.5f  // Resize scale for better detection

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

static void signal_handler(int signum) {
    (void)signum;  // Unused parameter
    g_running = false;
    printf("\nReceived interrupt signal, shutting down...\n");
}

static int init_app_context(AppContext* ctx, CameraType camera_type, const char* image_folder) {
    memset(ctx, 0, sizeof(AppContext));
    ctx->socket_server = NULL;
    ctx->running = true;
    
    // Initialize camera based on type
    printf("Initializing %s camera...\n", 
           camera_type == CAMERA_TYPE_IMX477 ? "real" : "emulated");
    ctx->camera = camera_create(camera_type);
    if (!ctx->camera) {
        fprintf(stderr, "Failed to initialize camera\n");
        return -1;
    }
    
    // Configure camera based on type
    if (camera_type == CAMERA_TYPE_EMULATED) {
        // Set image folder for emulated camera
        if (camera_interface_set_folder(ctx->camera, image_folder) != 0) {
            fprintf(stderr, "Failed to set image folder: %s\n", image_folder);
            return -1;
        }
        printf("Emulated camera folder: %s\n", image_folder);
    } else {
        // Configure real camera parameters
        RodCameraParameters params;
        camera_get_default_parameters(&params);
        // Use defaults for now, can be customized here
        camera_interface_set_parameters(ctx->camera, &params);
        printf("Real camera using default parameters\n");
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
    
    // Create field mask from first image in folder
    // This mask will be used to filter detections outside the playing field
    printf("Creating field mask...\n");
    
    // Get first image path from the folder
    char first_image[512];
    snprintf(first_image, sizeof(first_image), "%s/IMG_1415.JPG", image_folder);
    
    // Create mask with 1.1x vertical scale (to include slightly outside field)
    // Mask dimensions will match camera output (need to get from camera)
    // For now, use standard resolution - will be resized if needed
    ctx->field_mask = create_field_mask(first_image, ctx->detector, 4032, 3024, 1.1f, NULL);
    
    if (!ctx->field_mask) {
        fprintf(stderr, "Warning: Failed to create field mask, continuing without masking\n");
        ctx->field_mask = NULL;
    } else {
        printf("Field mask created successfully\n");
    }
    
    return 0;
}

static void cleanup_app_context(AppContext* ctx) {
    if (!ctx) return;
    
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
    CameraType camera_type = CAMERA_TYPE_EMULATED;  // Default to emulated
    
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
        
        // Try to accept a client connection if not already connected
        rod_socket_server_accept(ctx.socket_server);
        
        // Capture image from camera
        uint8_t* image_buffer = NULL;
        int width, height;
        size_t size;
        
        if (camera_interface_capture_frame(ctx.camera, &image_buffer, 
                                &width, &height, &size) != 0) {
            fprintf(stderr, "Failed to capture image\n");
            usleep(10000);  // Wait 10ms before retry
            continue;
        }
        
        // Create OpenCV image handle from BGR buffer
        // Camera returns BGR format (format=0, OpenCV native - no conversion)
        ImageHandle* original_image = create_image_from_buffer(image_buffer, width, height, 3, 0);
        free(image_buffer);  // Buffer is copied by create_image_from_buffer
        
        if (!original_image) {
            fprintf(stderr, "Failed to create image from buffer\n");
            usleep(10000);  // Wait 10ms before retry
            continue;
        }
        
        // ===== PREPROCESSING PIPELINE (matching Python implementation) =====
        // Step 1: Apply sharpening filter to enhance marker edges
        ImageHandle* sharpened_image = sharpen_image(original_image);
        if (!sharpened_image) {
            fprintf(stderr, "Failed to sharpen image\n");
            release_image(original_image);
            usleep(10000);
            continue;
        }
        
        // Step 2: Apply field mask to filter out areas outside the playing field
        ImageHandle* masked_image = sharpened_image;
        if (ctx.field_mask) {
            masked_image = bitwise_and_mask(sharpened_image, ctx.field_mask);
            if (!masked_image) {
                fprintf(stderr, "Failed to apply mask, using unmasked image\n");
                masked_image = sharpened_image;
            } else {
                // Release sharpened_image since we now have masked_image
                release_image(sharpened_image);
            }
        }
        
        // Step 3: Resize image (1.5x scale) for better detection of small/distant markers
        int new_width = (int)(width * DETECTION_SCALE_FACTOR);
        int new_height = (int)(height * DETECTION_SCALE_FACTOR);
        ImageHandle* resized_image = resize_image(masked_image, new_width, new_height);
        
        if (!resized_image) {
            fprintf(stderr, "Failed to resize image\n");
            release_image(original_image);
            usleep(10000);
            continue;
        }
        
        // Step 3: Detect ArUco markers on preprocessed image
        DetectionResult* detection = detectMarkersWithConfidence(ctx.detector, resized_image);
        release_image(resized_image);  // Don't need resized image anymore
        
        // Step 4: Scale coordinates back to original image size
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
            
            // Send detection results
            if (valid_count > 0) {
                rod_socket_server_send_detections(ctx.socket_server, markers, valid_count);
            }
            
            // Save images periodically (raw camera + debug annotated)
            if (frame_count % SAVE_DEBUG_IMAGE_INTERVAL == 0) {
                // Generate timestamp for filenames
                char timestamp[32];
                rod_config_generate_filename_timestamp(timestamp, sizeof(timestamp));
                
                // Ensure date folders exist
                char pictures_date_folder[256];
                char debug_date_folder[256];
                if (rod_config_ensure_date_folder(PICTURES_BASE_FOLDER, pictures_date_folder, sizeof(pictures_date_folder)) == 0 &&
                    rod_config_ensure_date_folder(DEBUG_BASE_FOLDER, debug_date_folder, sizeof(debug_date_folder)) == 0) {
                    
                    // 1. Save raw camera image: /var/roboteseo/pictures/YYYY_MM_DD/YYYYMMDD_HHMMSS_MS.png
                    char filename_camera[512];
                    snprintf(filename_camera, sizeof(filename_camera), "%s/%s.png", pictures_date_folder, timestamp);
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
                                MarkerCounts marker_counts = count_markers_by_category(markers, valid_count);
                                rod_viz_annotate_with_counter(annotated, marker_counts);
                                rod_viz_annotate_with_ids(annotated, markers, valid_count);
                                rod_viz_annotate_with_centers(annotated, markers, valid_count);
                                
                                char filename_debug[512];
                                snprintf(filename_debug, sizeof(filename_debug), "%s/%s_debug.png", debug_date_folder, timestamp);
                                save_image(filename_debug, annotated);
                                release_image(annotated);
                            }
                        }
                    }
                    
                    printf("Images saved: %s.png and %s_debug.png (markers: %d)\n", timestamp, timestamp, valid_count);
                }
            }
            
            releaseDetectionResult(detection);
        } else {
            // No markers detected
            if (frame_count % 10 == 0) {
                printf("Frame %d: No markers detected\n", frame_count);
            }
            
            // Save images periodically even when no markers detected
            if (frame_count % SAVE_DEBUG_IMAGE_INTERVAL == 0) {
                char timestamp[32];
                rod_config_generate_filename_timestamp(timestamp, sizeof(timestamp));
                
                // Ensure date folders exist
                char pictures_date_folder[256];
                char debug_date_folder[256];
                if (rod_config_ensure_date_folder(PICTURES_BASE_FOLDER, pictures_date_folder, sizeof(pictures_date_folder)) == 0 &&
                    rod_config_ensure_date_folder(DEBUG_BASE_FOLDER, debug_date_folder, sizeof(debug_date_folder)) == 0) {
                    
                    // Save raw camera image
                    char filename_camera[512];
                    snprintf(filename_camera, sizeof(filename_camera), "%s/%s.png", pictures_date_folder, timestamp);
                    save_image(filename_camera, original_image);
                    
                    // Save debug image (no annotations, but in debug folder)
                    char filename_debug[512];
                    snprintf(filename_debug, sizeof(filename_debug), "%s/%s_debug.png", debug_date_folder, timestamp);
                    save_image(filename_debug, original_image);
                    
                    printf("Images saved: %s.png and %s_debug.png (no markers)\n", timestamp, timestamp);
                }
            }
        }
        
        // Release images
        release_image(masked_image);
        release_image(original_image);
    }
    
    printf("\nShutting down...\n");
    printf("Total frames processed: %d\n", frame_count);
    
    // Cleanup
    cleanup_app_context(&ctx);
    
    printf("ROD Detection stopped successfully\n");
    return 0;
}
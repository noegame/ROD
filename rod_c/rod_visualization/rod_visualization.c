/**
 * @file rod_visualization.c
 * @brief Visualization and annotation utilities for ROD
 * @author Noé Game
 * @date 15/02/2026
 * @see rod_visualization.h
 * @copyright Cecill-C (Cf. LICENCE.txt)
 */

/* ******************************************************* Includes ****************************************************** */

#include "rod_visualization.h"
#include "opencv_wrapper.h"
#include "rod_cv.h"
#include "rod_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ***************************************************** Public macros *************************************************** */

/* ************************************************** Public types definition ******************************************** */

/* *********************************************** Public functions declarations ***************************************** */

/* ******************************************* Public callback functions declarations ************************************ */

/* ********************************************* Function implementations *********************************************** */

void rod_viz_annotate_with_ids(ImageHandle* image, MarkerData* markers, int count) {
    Color black = {0, 0, 0};
    Color green = {0, 255, 0};
    double font_scale = 0.5;
    
    for (int i = 0; i < count; i++) {
        char text[32];
        snprintf(text, sizeof(text), "ID:%d", markers[i].id);
        
        int x = (int)markers[i].x;
        int y = (int)markers[i].y;
        
        // Black outline for better visibility
        put_text(image, text, x, y, font_scale, black, 3);
        // Green text
        put_text(image, text, x, y, font_scale, green, 1);
    }
}

void rod_viz_annotate_with_centers(ImageHandle* image, MarkerData* markers, int count) {
    Color black = {0, 0, 0};
    Color blue = {255, 0, 0};  // OpenCV uses BGR
    double font_scale = 0.5;
    
    for (int i = 0; i < count; i++) {
        char text[64];
        snprintf(text, sizeof(text), "(%d,%d)", (int)markers[i].x, (int)markers[i].y);
        
        int x = (int)markers[i].x;
        int y = (int)markers[i].y - 20;  // Position above the marker
        
        // Black outline for better visibility
        put_text(image, text, x, y, font_scale, black, 3);
        // Blue text
        put_text(image, text, x, y, font_scale, blue, 1);
    }
}

void rod_viz_annotate_with_counter(ImageHandle* image, MarkerCounts counts) {
    Color black = {0, 0, 0};
    Color green = {0, 255, 0};
    double font_scale = 0.8;
    int line_height = 35;
    int start_x = 30;
    int start_y = 40;
    
    char text[64];
    
    // Black markers (empty boxes)
    snprintf(text, sizeof(text), "black markers : %d", counts.black_markers);
    put_text(image, text, start_x, start_y, font_scale, black, 3);
    put_text(image, text, start_x, start_y, font_scale, green, 2);
    
    // Blue markers
    snprintf(text, sizeof(text), "blue markers : %d", counts.blue_markers);
    put_text(image, text, start_x, start_y + line_height, font_scale, black, 3);
    put_text(image, text, start_x, start_y + line_height, font_scale, green, 2);
    
    // Yellow markers
    snprintf(text, sizeof(text), "yellow markers : %d", counts.yellow_markers);
    put_text(image, text, start_x, start_y + line_height * 2, font_scale, black, 3);
    put_text(image, text, start_x, start_y + line_height * 2, font_scale, green, 2);
    
    // Robot markers
    snprintf(text, sizeof(text), "robots markers : %d", counts.robot_markers);
    put_text(image, text, start_x, start_y + line_height * 3, font_scale, black, 3);
    put_text(image, text, start_x, start_y + line_height * 3, font_scale, green, 2);
    
    // Fixed markers
    snprintf(text, sizeof(text), "fixed markers : %d", counts.fixed_markers);
    put_text(image, text, start_x, start_y + line_height * 4, font_scale, black, 3);
    put_text(image, text, start_x, start_y + line_height * 4, font_scale, green, 2);
    
    // Total
    snprintf(text, sizeof(text), "total : %d", counts.total);
    put_text(image, text, start_x, start_y + line_height * 5, font_scale, black, 3);
    put_text(image, text, start_x, start_y + line_height * 5, font_scale, green, 2);
}

void rod_viz_generate_timestamp(char* buffer, size_t buffer_size) {
    // Delegate to rod_config function
    rod_config_generate_filename_timestamp(buffer, buffer_size);
}

int rod_viz_save_debug_image(ImageHandle* image, MarkerData* markers, int count, 
                              int frame_count __attribute__((unused)), const char* output_folder) {
    if (!image || !output_folder) {
        return -1;
    }
    
    // Ensure date-based subfolder exists
    char date_folder[256];
    if (rod_config_ensure_date_folder(output_folder, date_folder, sizeof(date_folder)) != 0) {
        fprintf(stderr, "Failed to create date folder\n");
        return -1;
    }
    
    // Create a copy of the image by extracting its data
    int width = get_image_width(image);
    int height = get_image_height(image);
    int channels = get_image_channels(image);
    uint8_t* data = get_image_data(image);
    size_t data_size = get_image_data_size(image);
    
    if (!data || data_size == 0) {
        fprintf(stderr, "Failed to get image data for debug output\n");
        return -1;
    }
    
    // Create a copy of the data
    uint8_t* data_copy = (uint8_t*)malloc(data_size);
    if (!data_copy) {
        fprintf(stderr, "Failed to allocate memory for image copy\n");
        return -1;
    }
    memcpy(data_copy, data, data_size);
    
    // Create new image from the copy (format=0 for BGR)
    ImageHandle* annotated = create_image_from_buffer(data_copy, width, height, channels, 0);
    free(data_copy);  // Buffer is copied by create_image_from_buffer
    
    if (!annotated) {
        fprintf(stderr, "Failed to create image copy for debug output\n");
        return -1;
    }
    
    // Count markers by category
    MarkerCounts marker_counts = count_markers_by_category(markers, count);
    
    // Annotate the copied image
    rod_viz_annotate_with_counter(annotated, marker_counts);
    if (count > 0) {
        rod_viz_annotate_with_ids(annotated, markers, count);
        rod_viz_annotate_with_centers(annotated, markers, count);
    }
    
    // Generate timestamp for filename
    char timestamp[32];
    rod_config_generate_filename_timestamp(timestamp, sizeof(timestamp));
    
    // Build filename: date_folder/YYYYMMDD_HHMMSS_MS_debug.png
    char filename[512];
    snprintf(filename, sizeof(filename), "%s/%s_debug.png", date_folder, timestamp);
    
    // Save image
    int success = save_image(filename, annotated);
    
    if (success) {
        printf("Debug image saved: %s (markers: %d)\n", filename, count);
    } else {
        fprintf(stderr, "Failed to save debug image: %s\n", filename);
    }
    
    // Release annotated image
    release_image(annotated);
    
    return success ? 0 : -1;
}

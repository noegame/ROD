/**
 * @file rod_visualization.h
 * @brief Visualization and annotation utilities for ROD
 * @author Noé Game
 * @date 15/02/2026
 * @see rod_visualization.c
 * @copyright Cecill-C (Cf. LICENCE.txt)
 * 
 * This module provides functions for annotating images and creating debug outputs:
 * - Annotate images with marker IDs, centers, and counts
 * - Save debug images with annotations
 * - Visualization utilities for ArUco detection results
 */

#pragma once

/* ******************************************************* Includes ****************************************************** */

#include "opencv_wrapper.h"
#include "rod_cv.h"

/* ***************************************************** Public macros *************************************************** */

/* ************************************************** Public types definition ******************************************** */

/* *********************************************** Public functions declarations ***************************************** */

/**
 * @brief Annotate image with marker IDs at marker centers
 * @param image Image handle (will be modified in place)
 * @param markers Array of marker data
 * @param count Number of markers
 * 
 * Draws green text with black outline showing "ID:XX" at each marker center
 */
void rod_viz_annotate_with_ids(ImageHandle* image, MarkerData* markers, int count);

/**
 * @brief Annotate image with marker center coordinates
 * @param image Image handle (will be modified in place)
 * @param markers Array of marker data
 * @param count Number of markers
 * 
 * Draws blue text with black outline showing "(x,y)" above each marker
 */
void rod_viz_annotate_with_centers(ImageHandle* image, MarkerData* markers, int count);

/**
 * @brief Annotate image with categorized marker counts
 * @param image Image handle (will be modified in place)
 * @param counts Marker counts by category
 * 
 * Draws text overlay in top-left corner showing counts for each marker category
 */
void rod_viz_annotate_with_counter(ImageHandle* image, MarkerCounts counts);

/**
 * @brief Generate timestamp-based filename
 * @param buffer Output buffer for the timestamp string
 * @param buffer_size Size of the output buffer
 * 
 * Generates timestamp string in format: YYYYMMDD_HHMMSS_mmm
 * Example: 20260216_173855_456
 */
void rod_viz_generate_timestamp(char* buffer, size_t buffer_size);

/**
 * @brief Save annotated debug image
 * @param image Original image
 * @param markers Array of marker data
 * @param count Number of markers
 * @param frame_count Frame number for filename
 * @param output_folder Folder to save debug images
 * @return 0 on success, -1 on failure
 * 
 * Creates a copy of the image, annotates it with all visualization data,
 * and saves it to the specified folder with the naming: frame_XXXXXX.jpg
 */
int rod_viz_save_debug_image(ImageHandle* image, MarkerData* markers, int count, 
                              int frame_count, const char* output_folder);

#ifdef __cplusplus
}
#endif

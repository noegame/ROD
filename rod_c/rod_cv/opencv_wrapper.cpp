#include <opencv2/opencv.hpp>
#include <opencv2/aruco.hpp>
#include "opencv_wrapper.h"
#include <cstring>   // For strlen
#include <cstdio>    // For fprintf

extern "C" {

ImageHandle* load_image(const char* path) {
    cv::Mat* image = new cv::Mat();
    *image = cv::imread(path, cv::IMREAD_COLOR);
    
    if (image->empty()) {
        delete image;
        return nullptr;
    }
    
    return reinterpret_cast<ImageHandle*>(image);
}

ImageHandle* create_image_from_buffer(uint8_t* data, int width, int height, int channels, int format) {
    if (data == nullptr || width <= 0 || height <= 0 || channels <= 0) {
        return nullptr;
    }
    
    // Determine OpenCV type based on channels
    int cv_type;
    if (channels == 1) {
        cv_type = CV_8UC1;
    } else if (channels == 3) {
        cv_type = CV_8UC3;
    } else if (channels == 4) {
        cv_type = CV_8UC4;
    } else {
        return nullptr;
    }
    
    // Create Mat from buffer (makes a copy of the data)
    cv::Mat* image = new cv::Mat(height, width, cv_type);
    memcpy(image->data, data, width * height * channels);
    
    // Convert from RGB to BGR if needed (OpenCV uses BGR by default)
    if (format == 1 && channels == 3) {  // RGB format
        cv::cvtColor(*image, *image, cv::COLOR_RGB2BGR);
    } else if (format == 2 && channels == 4) {  // RGBA format
        cv::cvtColor(*image, *image, cv::COLOR_RGBA2BGRA);
    }
    // format 0 (BGR), 3 (BGRA), and 4 (GRAY) don't need conversion
    
    return reinterpret_cast<ImageHandle*>(image);
}

void release_image(ImageHandle* handle) {
    if (handle != nullptr) {
        cv::Mat* image = reinterpret_cast<cv::Mat*>(handle);
        delete image;
    }
}

int get_image_width(ImageHandle* handle) {
    if (handle == nullptr) return 0;
    cv::Mat* image = reinterpret_cast<cv::Mat*>(handle);
    return image->cols;
}

int get_image_height(ImageHandle* handle) {
    if (handle == nullptr) return 0;
    cv::Mat* image = reinterpret_cast<cv::Mat*>(handle);
    return image->rows;
}

int get_image_channels(ImageHandle* handle) {
    if (handle == nullptr) return 0;
    cv::Mat* image = reinterpret_cast<cv::Mat*>(handle);
    return image->channels();
}

uint8_t* get_image_data(ImageHandle* handle) {
    if (handle == nullptr) return nullptr;
    cv::Mat* image = reinterpret_cast<cv::Mat*>(handle);
    if (image->empty()) return nullptr;
    return image->data;
}

size_t get_image_data_size(ImageHandle* handle) {
    if (handle == nullptr) return 0;
    cv::Mat* image = reinterpret_cast<cv::Mat*>(handle);
    if (image->empty()) return 0;
    return image->total() * image->elemSize();
}

ImageHandle* convert_bgr_to_rgb(ImageHandle* handle) {
    if (handle == nullptr) return nullptr;
    
    cv::Mat* src = reinterpret_cast<cv::Mat*>(handle);
    cv::Mat* rgb = new cv::Mat();
    
    cv::cvtColor(*src, *rgb, cv::COLOR_BGR2RGB);
    
    return reinterpret_cast<ImageHandle*>(rgb);
}

ImageHandle* convert_to_grayscale(ImageHandle* handle) {
    if (handle == nullptr) return nullptr;
    
    cv::Mat* src = reinterpret_cast<cv::Mat*>(handle);
    cv::Mat* gray = new cv::Mat();
    
    cv::cvtColor(*src, *gray, cv::COLOR_BGR2GRAY);
    
    return reinterpret_cast<ImageHandle*>(gray);
}

int save_image(const char* path, ImageHandle* handle) {
    if (handle == nullptr) {
        fprintf(stderr, "save_image: handle is null\n");
        return 0;
    }
    
    cv::Mat* image = reinterpret_cast<cv::Mat*>(handle);
    
    // Validate image is not empty
    if (image->empty()) {
        fprintf(stderr, "save_image: image is empty\n");
        return 0;
    }
    
    // Validate image has valid dimensions
    if (image->cols <= 0 || image->rows <= 0) {
        fprintf(stderr, "save_image: invalid dimensions %dx%d\n", image->cols, image->rows);
        return 0;
    }
    
    // Validate path is not null
    if (path == nullptr || strlen(path) == 0) {
        fprintf(stderr, "save_image: invalid path\n");
        return 0;
    }
    
    // Determine file extension and set appropriate compression parameters
    std::vector<int> compression_params;
    const char* ext = strrchr(path, '.');
    if (ext != nullptr) {
        // PNG format: use low compression (1 = fastest, minimal compression)
        // Lower compression = faster saving, slightly larger files
        if (strcasecmp(ext, ".png") == 0) {
            compression_params.push_back(cv::IMWRITE_PNG_COMPRESSION);
            compression_params.push_back(1);  // Compression level 1 (fast, low compression)
        }
        // JPEG format: use high quality (95 = high quality)
        else if (strcasecmp(ext, ".jpg") == 0 || strcasecmp(ext, ".jpeg") == 0) {
            compression_params.push_back(cv::IMWRITE_JPEG_QUALITY);
            compression_params.push_back(95);
        }
    }
    
    // Try to save the image with compression parameters
    bool success = cv::imwrite(path, *image, compression_params);
    if (!success) {
        fprintf(stderr, "save_image: cv::imwrite failed for path %s\n", path);
    }
    
    return success ? 1 : 0;
}

// Structure to hold detector state for OpenCV 4.6 compatibility
struct ArucoDetectorState {
    cv::Ptr<cv::aruco::Dictionary> dictionary;
    cv::Ptr<cv::aruco::DetectorParameters> parameters;
};

ArucoDictionaryHandle* getPredefinedDictionary(int dict_id) {
    // OpenCV 4.7+ changed the return type to cv::aruco::Dictionary (not cv::Ptr)
    #if CV_VERSION_MAJOR > 4 || (CV_VERSION_MAJOR == 4 && CV_VERSION_MINOR >= 7)
        cv::aruco::Dictionary dict = cv::aruco::getPredefinedDictionary(dict_id);
        cv::Ptr<cv::aruco::Dictionary> dict_ptr = cv::makePtr<cv::aruco::Dictionary>(dict);
    #else
        cv::Ptr<cv::aruco::Dictionary> dict_ptr = cv::aruco::getPredefinedDictionary(dict_id);
    #endif
    
    cv::Ptr<cv::aruco::Dictionary>* result = new cv::Ptr<cv::aruco::Dictionary>(dict_ptr);
    return reinterpret_cast<ArucoDictionaryHandle*>(result);
}

DetectorParametersHandle* createDetectorParameters() {
    // OpenCV 4.7+ removed DetectorParameters::create()
    #if CV_VERSION_MAJOR > 4 || (CV_VERSION_MAJOR == 4 && CV_VERSION_MINOR >= 7)
        cv::Ptr<cv::aruco::DetectorParameters> params = cv::makePtr<cv::aruco::DetectorParameters>();
    #else
        cv::Ptr<cv::aruco::DetectorParameters> params = cv::aruco::DetectorParameters::create();
    #endif
    
    cv::Ptr<cv::aruco::DetectorParameters>* params_ptr = new cv::Ptr<cv::aruco::DetectorParameters>(params);
    return reinterpret_cast<DetectorParametersHandle*>(params_ptr);
}

ArucoDetectorHandle* createArucoDetector(ArucoDictionaryHandle* dict, DetectorParametersHandle* params) {
    if (dict == nullptr || params == nullptr) return nullptr;
    
    ArucoDetectorState* state = new ArucoDetectorState();
    state->dictionary = *reinterpret_cast<cv::Ptr<cv::aruco::Dictionary>*>(dict);
    state->parameters = *reinterpret_cast<cv::Ptr<cv::aruco::DetectorParameters>*>(params);
    
    return reinterpret_cast<ArucoDetectorHandle*>(state);
}

void releaseArucoDetector(ArucoDetectorHandle* detector) {
    if (detector != nullptr) {
        ArucoDetectorState* state = reinterpret_cast<ArucoDetectorState*>(detector);
        delete state;
    }
}

void releaseArucoDictionary(ArucoDictionaryHandle* dict) {
    if (dict != nullptr) {
        cv::Ptr<cv::aruco::Dictionary>* dictionary = reinterpret_cast<cv::Ptr<cv::aruco::Dictionary>*>(dict);
        delete dictionary;
    }
}

void releaseDetectorParameters(DetectorParametersHandle* params) {
    if (params != nullptr) {
        cv::Ptr<cv::aruco::DetectorParameters>* detectorParams = reinterpret_cast<cv::Ptr<cv::aruco::DetectorParameters>*>(params);
        delete detectorParams;
    }
}

// Detector parameters setters (matching Python implementation)
void setAdaptiveThreshWinSizeMin(DetectorParametersHandle* params, int value) {
    if (params == nullptr) return;
    cv::Ptr<cv::aruco::DetectorParameters>* p = reinterpret_cast<cv::Ptr<cv::aruco::DetectorParameters>*>(params);
    (*p)->adaptiveThreshWinSizeMin = value;
}

void setAdaptiveThreshWinSizeMax(DetectorParametersHandle* params, int value) {
    if (params == nullptr) return;
    cv::Ptr<cv::aruco::DetectorParameters>* p = reinterpret_cast<cv::Ptr<cv::aruco::DetectorParameters>*>(params);
    (*p)->adaptiveThreshWinSizeMax = value;
}

void setAdaptiveThreshWinSizeStep(DetectorParametersHandle* params, int value) {
    if (params == nullptr) return;
    cv::Ptr<cv::aruco::DetectorParameters>* p = reinterpret_cast<cv::Ptr<cv::aruco::DetectorParameters>*>(params);
    (*p)->adaptiveThreshWinSizeStep = value;
}

void setMinMarkerPerimeterRate(DetectorParametersHandle* params, double value) {
    if (params == nullptr) return;
    cv::Ptr<cv::aruco::DetectorParameters>* p = reinterpret_cast<cv::Ptr<cv::aruco::DetectorParameters>*>(params);
    (*p)->minMarkerPerimeterRate = value;
}

void setMaxMarkerPerimeterRate(DetectorParametersHandle* params, double value) {
    if (params == nullptr) return;
    cv::Ptr<cv::aruco::DetectorParameters>* p = reinterpret_cast<cv::Ptr<cv::aruco::DetectorParameters>*>(params);
    (*p)->maxMarkerPerimeterRate = value;
}

void setPolygonalApproxAccuracyRate(DetectorParametersHandle* params, double value) {
    if (params == nullptr) return;
    cv::Ptr<cv::aruco::DetectorParameters>* p = reinterpret_cast<cv::Ptr<cv::aruco::DetectorParameters>*>(params);
    (*p)->polygonalApproxAccuracyRate = value;
}

void setCornerRefinementMethod(DetectorParametersHandle* params, int method) {
    if (params == nullptr) return;
    cv::Ptr<cv::aruco::DetectorParameters>* p = reinterpret_cast<cv::Ptr<cv::aruco::DetectorParameters>*>(params);
    (*p)->cornerRefinementMethod = method;
}

void setCornerRefinementWinSize(DetectorParametersHandle* params, int value) {
    if (params == nullptr) return;
    cv::Ptr<cv::aruco::DetectorParameters>* p = reinterpret_cast<cv::Ptr<cv::aruco::DetectorParameters>*>(params);
    (*p)->cornerRefinementWinSize = value;
}

void setCornerRefinementMaxIterations(DetectorParametersHandle* params, int value) {
    if (params == nullptr) return;
    cv::Ptr<cv::aruco::DetectorParameters>* p = reinterpret_cast<cv::Ptr<cv::aruco::DetectorParameters>*>(params);
    (*p)->cornerRefinementMaxIterations = value;
}

void setMinDistanceToBorder(DetectorParametersHandle* params, int value) {
    if (params == nullptr) return;
    cv::Ptr<cv::aruco::DetectorParameters>* p = reinterpret_cast<cv::Ptr<cv::aruco::DetectorParameters>*>(params);
    (*p)->minDistanceToBorder = value;
}

void setMinOtsuStdDev(DetectorParametersHandle* params, double value) {
    if (params == nullptr) return;
    cv::Ptr<cv::aruco::DetectorParameters>* p = reinterpret_cast<cv::Ptr<cv::aruco::DetectorParameters>*>(params);
    (*p)->minOtsuStdDev = value;
}

void setPerspectiveRemoveIgnoredMarginPerCell(DetectorParametersHandle* params, double value) {
    if (params == nullptr) return;
    cv::Ptr<cv::aruco::DetectorParameters>* p = reinterpret_cast<cv::Ptr<cv::aruco::DetectorParameters>*>(params);
    (*p)->perspectiveRemoveIgnoredMarginPerCell = value;
}

DetectionResult* detectMarkersWithConfidence(ArucoDetectorHandle* detector, ImageHandle* image) {
    if (detector == nullptr || image == nullptr) return nullptr;
    
    ArucoDetectorState* state = reinterpret_cast<ArucoDetectorState*>(detector);
    cv::Mat* img = reinterpret_cast<cv::Mat*>(image);
    
    std::vector<int> markerIds;
    std::vector<std::vector<cv::Point2f>> markerCorners, rejectedCandidates;
    
    // Detect markers using OpenCV 4.6 API
    cv::aruco::detectMarkers(*img, state->dictionary, markerCorners, markerIds, state->parameters, rejectedCandidates);
    
    // Create result structure
    DetectionResult* result = new DetectionResult();
    result->count = markerIds.size();
    
    if (result->count > 0) {
        result->markers = new DetectedMarker[result->count];
        
        for (int i = 0; i < result->count; i++) {
            result->markers[i].id = markerIds[i];
            
            // Copy corner coordinates
            for (int j = 0; j < 4; j++) {
                result->markers[i].corners[j][0] = markerCorners[i][j].x;
                result->markers[i].corners[j][1] = markerCorners[i][j].y;
            }
            
            // Calculate confidence score based on corner quality
            // For now, we use a simple metric: marker perimeter vs expected perimeter
            // A better metric would consider image quality, corner sharpness, etc.
            float perimeter = 0.0f;
            for (int j = 0; j < 4; j++) {
                int next = (j + 1) % 4;
                float dx = markerCorners[i][next].x - markerCorners[i][j].x;
                float dy = markerCorners[i][next].y - markerCorners[i][j].y;
                perimeter += std::sqrt(dx * dx + dy * dy);
            }
            
            // Normalize confidence (larger markers = higher confidence, up to 1.0)
            // This is a simplified metric; adjust based on your needs
            result->markers[i].confidence = std::min(1.0f, perimeter / 400.0f);
        }
    } else {
        result->markers = nullptr;
    }
    
    return result;
}

void releaseDetectionResult(DetectionResult* result) {
    if (result != nullptr) {
        if (result->markers != nullptr) {
            delete[] result->markers;
        }
        delete result;
    }
}

ImageHandle* drawDetectedMarkers(ImageHandle* image, DetectionResult* result) {
    if (image == nullptr || result == nullptr) return nullptr;
    
    cv::Mat* src = reinterpret_cast<cv::Mat*>(image);
    cv::Mat* output = new cv::Mat();
    src->copyTo(*output);
    
    // Draw each detected marker
    for (int i = 0; i < result->count; i++) {
        DetectedMarker* marker = &result->markers[i];
        
        // Convert corners to cv::Point format
        std::vector<cv::Point2f> corners;
        for (int j = 0; j < 4; j++) {
            corners.push_back(cv::Point2f(marker->corners[j][0], marker->corners[j][1]));
        }
        
        // Draw marker outline
        for (int j = 0; j < 4; j++) {
            cv::line(*output, corners[j], corners[(j + 1) % 4], 
                    cv::Scalar(0, 255, 0), 2);
        }
        
        // Draw corner circles
        for (int j = 0; j < 4; j++) {
            cv::circle(*output, corners[j], 3, cv::Scalar(255, 0, 0), -1);
        }
        
        // Calculate center point
        cv::Point2f center(0, 0);
        for (int j = 0; j < 4; j++) {
            center.x += corners[j].x;
            center.y += corners[j].y;
        }
        center.x /= 4.0f;
        center.y /= 4.0f;
        
        // Draw marker ID
        char text[32];
        snprintf(text, sizeof(text), "ID:%d", marker->id);
        cv::putText(*output, text, cv::Point(center.x - 20, center.y - 10),
                   cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(0, 0, 255), 2);
        
        // Draw confidence score
        snprintf(text, sizeof(text), "%.2f", marker->confidence);
        cv::putText(*output, text, cv::Point(center.x - 20, center.y + 15),
                   cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255, 255, 0), 2);
    }
    
    return reinterpret_cast<ImageHandle*>(output);
}


// ===== Image Processing Functions =====

ImageHandle* convert_bgra_to_bgr(ImageHandle* handle) {
    if (handle == nullptr) return nullptr;
    
    cv::Mat* src = reinterpret_cast<cv::Mat*>(handle);
    cv::Mat* dst = new cv::Mat();
    
    cv::cvtColor(*src, *dst, cv::COLOR_BGRA2BGR);
    
    return reinterpret_cast<ImageHandle*>(dst);
}

ImageHandle* bitwise_and_mask(ImageHandle* image, ImageHandle* mask) {
    if (image == nullptr || mask == nullptr) return nullptr;
    
    cv::Mat* img = reinterpret_cast<cv::Mat*>(image);
    cv::Mat* msk = reinterpret_cast<cv::Mat*>(mask);
    cv::Mat* result = new cv::Mat();
    
    cv::bitwise_and(*img, *img, *result, *msk);
    
    return reinterpret_cast<ImageHandle*>(result);
}

ImageHandle* sharpen_image(ImageHandle* image) {
    if (image == nullptr) return nullptr;
    
    cv::Mat* src = reinterpret_cast<cv::Mat*>(image);
    cv::Mat* dst = new cv::Mat();
    
    // Sharpening kernel
    cv::Mat kernel = (cv::Mat_<float>(3, 3) << 
        -1, -1, -1,
        -1,  9, -1,
        -1, -1, -1);
    
    cv::filter2D(*src, *dst, -1, kernel);
    
    return reinterpret_cast<ImageHandle*>(dst);
}

ImageHandle* resize_image(ImageHandle* image, int new_width, int new_height) {
    if (image == nullptr) return nullptr;
    
    cv::Mat* src = reinterpret_cast<cv::Mat*>(image);
    cv::Mat* dst = new cv::Mat();
    
    cv::resize(*src, *dst, cv::Size(new_width, new_height));
    
    return reinterpret_cast<ImageHandle*>(dst);
}

// ===== Optimized versions with buffer reuse =====

ImageHandle* sharpen_image_reuse(ImageHandle* src, ImageHandle* dst) {
    if (src == nullptr) return nullptr;
    
    cv::Mat* src_mat = reinterpret_cast<cv::Mat*>(src);
    cv::Mat* dst_mat = nullptr;
    
    if (dst == nullptr) {
        // No buffer provided, allocate new one
        dst_mat = new cv::Mat();
    } else {
        // Reuse provided buffer
        dst_mat = reinterpret_cast<cv::Mat*>(dst);
        // Ensure buffer has correct dimensions
        if (dst_mat->rows != src_mat->rows || 
            dst_mat->cols != src_mat->cols || 
            dst_mat->type() != src_mat->type()) {
            // Reallocate if size mismatch
            *dst_mat = cv::Mat(src_mat->rows, src_mat->cols, src_mat->type());
        }
    }
    
    // Sharpening kernel
    cv::Mat kernel = (cv::Mat_<float>(3, 3) << 
        -1, -1, -1,
        -1,  9, -1,
        -1, -1, -1);
    
    cv::filter2D(*src_mat, *dst_mat, -1, kernel);
    
    return reinterpret_cast<ImageHandle*>(dst_mat);
}

ImageHandle* resize_image_reuse(ImageHandle* src, int new_width, int new_height, ImageHandle* dst) {
    if (src == nullptr) return nullptr;
    
    cv::Mat* src_mat = reinterpret_cast<cv::Mat*>(src);
    cv::Mat* dst_mat = nullptr;
    
    if (dst == nullptr) {
        // No buffer provided, allocate new one
        dst_mat = new cv::Mat();
    } else {
        // Reuse provided buffer
        dst_mat = reinterpret_cast<cv::Mat*>(dst);
        // Ensure buffer has correct dimensions
        if (dst_mat->rows != new_height || 
            dst_mat->cols != new_width || 
            dst_mat->type() != src_mat->type()) {
            // Reallocate if size mismatch
            *dst_mat = cv::Mat(new_height, new_width, src_mat->type());
        }
    }
    
    cv::resize(*src_mat, *dst_mat, cv::Size(new_width, new_height));
    
    return reinterpret_cast<ImageHandle*>(dst_mat);
}

ImageHandle* bitwise_and_mask_reuse(ImageHandle* src, ImageHandle* mask, ImageHandle* dst) {
    if (src == nullptr || mask == nullptr) return nullptr;
    
    cv::Mat* src_mat = reinterpret_cast<cv::Mat*>(src);
    cv::Mat* mask_mat = reinterpret_cast<cv::Mat*>(mask);
    cv::Mat* dst_mat = nullptr;
    
    if (dst == nullptr) {
        // No buffer provided, allocate new one
        dst_mat = new cv::Mat();
    } else {
        // Reuse provided buffer
        dst_mat = reinterpret_cast<cv::Mat*>(dst);
        // Ensure buffer has correct dimensions
        if (dst_mat->rows != src_mat->rows || 
            dst_mat->cols != src_mat->cols || 
            dst_mat->type() != src_mat->type()) {
            // Reallocate if size mismatch
            *dst_mat = cv::Mat(src_mat->rows, src_mat->cols, src_mat->type());
        }
    }
    
    cv::bitwise_and(*src_mat, *src_mat, *dst_mat, *mask_mat);
    
    return reinterpret_cast<ImageHandle*>(dst_mat);
}

// ===== Drawing Functions =====

void put_text(ImageHandle* image, const char* text, int x, int y, 
              double font_scale, Color color, int thickness) {
    if (image == nullptr || text == nullptr) return;
    
    cv::Mat* img = reinterpret_cast<cv::Mat*>(image);
    cv::Scalar cv_color(color.b, color.g, color.r);
    
    cv::putText(*img, text, cv::Point(x, y), 
                cv::FONT_HERSHEY_SIMPLEX, font_scale, cv_color, thickness);
}

void draw_polyline(ImageHandle* image, float corners[4][2], Color color, int thickness) {
    if (image == nullptr || corners == nullptr) return;
    
    cv::Mat* img = reinterpret_cast<cv::Mat*>(image);
    cv::Scalar cv_color(color.b, color.g, color.r);
    
    // Convert corners to cv::Point format
    std::vector<cv::Point> points;
    for (int i = 0; i < 4; i++) {
        points.push_back(cv::Point(static_cast<int>(corners[i][0]), 
                                   static_cast<int>(corners[i][1])));
    }
    
    // Draw lines between consecutive corners
    for (int i = 0; i < 4; i++) {
        cv::line(*img, points[i], points[(i + 1) % 4], cv_color, thickness);
    }
}

ImageHandle* fill_poly(ImageHandle* image, float* points, int num_points, Color color) {
    if (image == nullptr || points == nullptr) return nullptr;
    
    cv::Mat* img = reinterpret_cast<cv::Mat*>(image);
    cv::Mat* result = new cv::Mat();
    img->copyTo(*result);
    
    // Convert points to OpenCV format
    std::vector<cv::Point> poly_points;
    for (int i = 0; i < num_points; i++) {
        poly_points.push_back(cv::Point(
            static_cast<int>(points[i * 2]),
            static_cast<int>(points[i * 2 + 1])
        ));
    }
    
    std::vector<std::vector<cv::Point>> polys = {poly_points};
    cv::Scalar cv_color(color.b, color.g, color.r);
    cv::fillPoly(*result, polys, cv_color);
    
    return reinterpret_cast<ImageHandle*>(result);
}

ImageHandle* create_empty_image(int width, int height, int channels) {
    cv::Mat* img = new cv::Mat(height, width, 
                               channels == 1 ? CV_8UC1 : CV_8UC3, 
                               cv::Scalar(0));
    return reinterpret_cast<ImageHandle*>(img);
}

// ===== Geometric Transformation Functions =====

Point2f* fisheye_undistort_points(Point2f* points, int num_points,
                                   float* camera_matrix,
                                   float* dist_coeffs,
                                   float* output_camera_matrix) {
    if (points == nullptr || camera_matrix == nullptr || dist_coeffs == nullptr) {
        return nullptr;
    }
    
    // Convert input points to cv::Mat in CV_32FC2 format (required by fisheye::undistortPoints)
    std::vector<cv::Point2f> input_points;
    for (int i = 0; i < num_points; i++) {
        input_points.push_back(cv::Point2f(points[i].x, points[i].y));
    }
    cv::Mat points_mat(input_points);  // This creates a 1xN CV_32FC2 matrix
    // Ensure it's in the right format (Nx1 or 1xN with 2 channels)
    if (points_mat.channels() != 2) {
        points_mat = points_mat.reshape(2);  // Ensure 2 channels
    }
    
    // Create camera matrix (3x3)
    cv::Mat K = cv::Mat(3, 3, CV_32F, camera_matrix).clone();
    K.convertTo(K, CV_64F);
    
    // Create distortion coefficients (4 coefficients for fisheye)
    cv::Mat D = cv::Mat(1, 4, CV_32F, dist_coeffs).clone();
    D.convertTo(D, CV_64F);
    
    // Output camera matrix (use input K if not specified)
    cv::Mat P = K.clone();
    if (output_camera_matrix != nullptr) {
        P = cv::Mat(3, 3, CV_32F, output_camera_matrix).clone();
        P.convertTo(P, CV_64F);
    }
    
    // Undistort points
    cv::Mat undistorted;
    cv::fisheye::undistortPoints(points_mat, undistorted, K, D, cv::noArray(), P);
    
    // Convert output to Point2f array
    // The undistorted matrix is in CV_64FC2 format (or CV_32FC2)
    Point2f* result = new Point2f[num_points];
    
    if (undistorted.type() == CV_64FC2) {
        // 64-bit double, 2 channels
        for (int i = 0; i < num_points; i++) {
            cv::Point2d pt = undistorted.at<cv::Point2d>(i);
            result[i].x = static_cast<float>(pt.x);
            result[i].y = static_cast<float>(pt.y);
        }
    } else if (undistorted.type() == CV_32FC2) {
        // 32-bit float, 2 channels
        for (int i = 0; i < num_points; i++) {
            cv::Point2f pt = undistorted.at<cv::Point2f>(i);
            result[i].x = pt.x;
            result[i].y = pt.y;
        }
    } else {
        // Fallback: try as Nx2 matrix
        for (int i = 0; i < num_points; i++) {
            result[i].x = undistorted.at<float>(i, 0);
            result[i].y = undistorted.at<float>(i, 1);
        }
    }
    
    return result;
}

float* find_homography(Point2f* src_points, Point2f* dst_points, int num_points) {
    if (src_points == nullptr || dst_points == nullptr || num_points < 4) {
        return nullptr;
    }
    
    // Convert to OpenCV format
    std::vector<cv::Point2f> src, dst;
    for (int i = 0; i < num_points; i++) {
        src.push_back(cv::Point2f(src_points[i].x, src_points[i].y));
        dst.push_back(cv::Point2f(dst_points[i].x, dst_points[i].y));
    }
    
    // Find homography
    cv::Mat H = cv::findHomography(src, dst);
    
    if (H.empty()) return nullptr;
    
    // Convert to float array (3x3 = 9 elements)
    float* result = new float[9];
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            result[i * 3 + j] = static_cast<float>(H.at<double>(i, j));
        }
    }
    
    return result;
}

Point2f* perspective_transform(Point2f* points, int num_points, float* homography) {
    if (points == nullptr || homography == nullptr) return nullptr;
    
    // Convert input points
    std::vector<cv::Point2f> input_points;
    for (int i = 0; i < num_points; i++) {
        input_points.push_back(cv::Point2f(points[i].x, points[i].y));
    }
    
    // Create homography matrix
    cv::Mat H(3, 3, CV_32F, homography);
    H.convertTo(H, CV_64F);
    
    // Transform points
    std::vector<cv::Point2f> output_points;
    cv::perspectiveTransform(input_points, output_points, H);
    
    // Convert to Point2f array
    Point2f* result = new Point2f[num_points];
    for (int i = 0; i < num_points; i++) {
        result[i].x = output_points[i].x;
        result[i].y = output_points[i].y;
    }
    
    return result;
}

// ===== Pose Estimation =====

PnPResult solve_pnp(Point3f* object_points, Point2f* image_points, int num_points,
                    float* camera_matrix, float* dist_coeffs) {
    PnPResult result = {{0, 0, 0}, {0, 0, 0}, 0};
    
    if (object_points == nullptr || image_points == nullptr || 
        camera_matrix == nullptr || dist_coeffs == nullptr || num_points < 4) {
        return result;
    }
    
    // Convert points
    std::vector<cv::Point3f> obj_pts;
    std::vector<cv::Point2f> img_pts;
    
    for (int i = 0; i < num_points; i++) {
        obj_pts.push_back(cv::Point3f(object_points[i].x, object_points[i].y, object_points[i].z));
        img_pts.push_back(cv::Point2f(image_points[i].x, image_points[i].y));
    }
    
    // Create camera matrix
    cv::Mat K = cv::Mat(3, 3, CV_32F, camera_matrix).clone();
    K.convertTo(K, CV_64F);
    
    // Create distortion coefficients
    cv::Mat D = cv::Mat(1, 4, CV_32F, dist_coeffs).clone();
    D.convertTo(D, CV_64F);
    
    // Solve PnP
    cv::Mat rvec, tvec;
    bool success = cv::solvePnP(obj_pts, img_pts, K, D, rvec, tvec, 
                                 false, cv::SOLVEPNP_IPPE_SQUARE);
    
    if (success) {
        result.success = 1;
        result.rvec[0] = static_cast<float>(rvec.at<double>(0));
        result.rvec[1] = static_cast<float>(rvec.at<double>(1));
        result.rvec[2] = static_cast<float>(rvec.at<double>(2));
        result.tvec[0] = static_cast<float>(tvec.at<double>(0));
        result.tvec[1] = static_cast<float>(tvec.at<double>(1));
        result.tvec[2] = static_cast<float>(tvec.at<double>(2));
    }
    
    return result;
}

// ===== Memory Management =====

void free_points_2f(Point2f* points) {
    if (points != nullptr) {
        delete[] points;
    }
}

void free_points_3f(Point3f* points) {
    if (points != nullptr) {
        delete[] points;
    }
}

void free_matrix(float* matrix) {
    if (matrix != nullptr) {
        delete[] matrix;
    }
}


} // extern "C"


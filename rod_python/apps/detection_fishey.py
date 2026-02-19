import cv2
import numpy as np

# =============================
# FISHEYE CAMERA PARAMETERS
# =============================

# Intrinsic matrix (calibration values)
K = np.array(
    [
        [2.49362477e03, 0.00000000e00, 1.97718701e03],
        [0.00000000e00, 2.49311358e03, 2.03491176e03],
        [0.00000000e00, 0.00000000e00, 1.00000000e00],
    ]
)

# Fisheye distortion coefficients (k1, k2, k3, k4)
D = np.array([-0.1203345, 0.06802544, -0.13779641, 0.08243704])
# =============================
# IMAGE LOADING
# =============================

image_path = "img1.jpg"
img = cv2.imread(image_path)

if img is None:
    raise IOError("Cannot load the image")

h, w = img.shape[:2]

# =============================
# FISHEYE UNDISTORTION
# =============================

balance = 0.4
scale = 1.5

# Calculate new camera matrix
new_K = cv2.fisheye.estimateNewCameraMatrixForUndistortRectify(
    K, D, (w, h), np.eye(3), balance=balance
)

map1, map2 = cv2.fisheye.initUndistortRectifyMap(
    K, D, np.eye(3), new_K, (w, h), cv2.CV_16SC2
)

img = cv2.remap(img, map1, map2, interpolation=cv2.INTER_LINEAR)
# img = cv2.resize(img, None, fx=scale, fy=scale, interpolation=cv2.INTER_CUBIC)
# img = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)
# img = cv2.equalizeHist(img)

# =============================
# ARUCO DETECTION
# =============================

# ArUco dictionary (e.g.: 4x4_50)
aruco_dict = cv2.aruco.getPredefinedDictionary(cv2.aruco.DICT_4X4_50)

parameters = cv2.aruco.DetectorParameters()

params = cv2.aruco.DetectorParameters()

# # OpenCV tests multiple window sizes to threshold the image locally.
# params.adaptiveThreshWinSizeMin = 3

# # OpenCV tests multiple window sizes to threshold the image locally.
# params.adaptiveThreshWinSizeMax = 63

# # Step size between tested window sizes.
# params.adaptiveThreshWinSizeStep = 4

# params.minMarkerPerimeterRate = 0.015
# params.maxMarkerPerimeterRate = 4.0

# params.polygonalApproxAccuracyRate = 0.08
# params.minCornerDistanceRate = 0.01

detector = cv2.aruco.ArucoDetector(aruco_dict, parameters)

corners, ids, rejected = detector.detectMarkers(img)

# =============================
# DISPLAY RESULTS
# =============================

output = img.copy()

if ids is not None:
    cv2.aruco.drawDetectedMarkers(output, corners, ids)
    print("\n======")
    print(f"Detected tags: {ids.flatten()}")
    print(f"Number of detected tags: {len(ids)}")
    print(f"balance: {balance}")
    print(f"params.minMarkerPerimeterRate : {params.minMarkerPerimeterRate}")
    print(f"params.maxMarkerPerimeterRate : {params.maxMarkerPerimeterRate}")
    print(f"params.polygonalApproxAccuracyRate : {params.polygonalApproxAccuracyRate}")
    print(f"params.minCornerDistanceRate : {params.minCornerDistanceRate}")
    print("======\n")

else:
    print("No tag detected")

output_resized = cv2.resize(output, (1080, 1080))
cv2.imshow("Aruco detection", output_resized)
cv2.imwrite("aruco_detection.jpg", output)
cv2.waitKey(0)
cv2.destroyAllWindows()

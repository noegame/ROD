import cv2
import numpy as np
import glob
import os

# =============================
# CHECKERBOARD PARAMETERS
# =============================

CHECKERBOARD = (6, 9)  # inner corners (width, height)
square_size = 1.0  # square size (arbitrary unit)

# =============================
# IMAGE FOLDER
# =============================

images_path = "2026-01-14-calibration-4000x4000/*.jpg"
images = glob.glob(images_path)

if len(images) == 0:
    raise IOError("No images found")

# =============================
# 3D OBJECT PREPARATION
# =============================

objp = np.zeros((1, CHECKERBOARD[0] * CHECKERBOARD[1], 3), np.float32)
objp[0, :, :2] = np.mgrid[0 : CHECKERBOARD[0], 0 : CHECKERBOARD[1]].T.reshape(-1, 2)
objp *= square_size

objpoints = []  # 3D points
imgpoints = []  # 2D points

img_shape = None
valid_images = 0

# =============================
# CHECKERBOARD DETECTION
# =============================

for fname in images:
    img = cv2.imread(fname)
    if img is None:
        continue

    gray = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)

    if img_shape is None:
        img_shape = gray.shape[::-1]

    ret, corners = cv2.findChessboardCorners(
        gray,
        CHECKERBOARD,
        cv2.CALIB_CB_ADAPTIVE_THRESH
        + cv2.CALIB_CB_FAST_CHECK
        + cv2.CALIB_CB_NORMALIZE_IMAGE,
    )

    if ret:
        corners = cv2.cornerSubPix(
            gray,
            corners,
            (3, 3),
            (-1, -1),
            criteria=(cv2.TERM_CRITERIA_EPS + cv2.TERM_CRITERIA_MAX_ITER, 30, 1e-6),
        )

        objpoints.append(objp)
        imgpoints.append(corners)
        valid_images += 1

        cv2.drawChessboardCorners(img, CHECKERBOARD, corners, ret)
        img_resized = cv2.resize(img, (800, 800))
        cv2.imshow("Calibration", img_resized)
        cv2.waitKey(100)

cv2.destroyAllWindows()

if valid_images < 10:
    raise RuntimeError("Not enough valid images for reliable calibration")

# =============================
# FISHEYE CALIBRATION
# =============================

K = np.zeros((3, 3))
D = np.zeros((4, 1))

rvecs = []
tvecs = []

flags = (
    cv2.fisheye.CALIB_RECOMPUTE_EXTRINSIC
    + cv2.fisheye.CALIB_CHECK_COND
    + cv2.fisheye.CALIB_FIX_SKEW
)

rms, _, _, _, _ = cv2.fisheye.calibrate(
    objpoints,
    imgpoints,
    img_shape,
    K,
    D,
    rvecs,
    tvecs,
    flags,
    criteria=(cv2.TERM_CRITERIA_EPS + cv2.TERM_CRITERIA_MAX_ITER, 100, 1e-6),
)

# =============================
# RESULTS
# =============================

print("\n=== FISHEYE CALIBRATION RESULT ===")
print(f"RMS error: {rms:.6f}\n")

print("Intrinsic matrix K:")
print(K)

print("\nDistortion coefficients D (k1, k2, k3, k4):")
print(D.flatten())

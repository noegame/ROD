#!/usr/bin/env python3

"""
config.py
Configuration module for tests and main application.
Defines constants and getter functions for camera settings,
logging, feature toggles, and directory paths.
"""

# ---------------------------------------------------------------------------
# Imports
# ---------------------------------------------------------------------------

from pathlib import Path
from vision_python.src.aruco import aruco

# ---------------------------------------------------------------------------
# Camera Configuration
# ---------------------------------------------------------------------------

# Camera modes: 'picamera', 'webcam', 'emulated'
CAMERA_WIDTH = 4056
CAMERA_HEIGHT = 3040
CAMERA = "picamera"

# ---------------------------------------------------------------------------
# Logging Configuration
# ---------------------------------------------------------------------------

LOG_LEVEL = "INFO"

# ---------------------------------------------------------------------------
# Flask Server Configuration
# ---------------------------------------------------------------------------

FLASK_HOST = "0.0.0.0"
FLASK_PORT = 5000

# ---------------------------------------------------------------------------
# Directory Paths
# ---------------------------------------------------------------------------

PROJECT_DIR = Path(__file__).resolve().parents[2]
PICTURES_DIR = PROJECT_DIR / "pictures"
CAMERA_DIR = PICTURES_DIR / "camera"
LOG_DIR = PROJECT_DIR / "logs"
VISION_DIR = PROJECT_DIR / "vision_python"
CALIBRATION_DIR = VISION_DIR / "config" / "calibrations"
CALIBRATION_FILE = CALIBRATION_DIR / "80_lens" / "camera_calibration_4056x3040.npz"
EMULATED_CAM_DIR = CAMERA_DIR / "2026-01-09-playground-ready"


# ---------------------------------------------------------------------------
# Fixed Aruco marker positions
# ---------------------------------------------------------------------------


# ---------------------------------------------------------------------------
# Getter Functions
# ---------------------------------------------------------------------------


def is_aruco_detection_enabled():
    """Returns whether ArUco detection is enabled."""
    return True


def is_ui_enabled():
    """Returns whether the UI is enabled."""
    return True


def get_camera_resolution():
    """Returns the camera resolution as a tuple (width, height)."""
    return CAMERA_WIDTH, CAMERA_HEIGHT


def get_camera_width():
    """Returns the camera width."""
    return CAMERA_WIDTH


def get_camera_height():
    """Returns the camera height."""
    return CAMERA_HEIGHT


def get_camera_mode():
    """Returns the current camera mode."""
    return CAMERA


def get_flask_server_config():
    """Returns the Flask server configuration as a tuple (host, port)."""
    return FLASK_HOST, FLASK_PORT


def get_logging_level():
    """Returns the logging level."""
    return LOG_LEVEL


def get_camera_params():
    return {
        "calibration_file": get_camera_calibration_file(),
        "img_width": get_camera_width(),
        "img_height": get_camera_height(),
    }


def get_fixed_aruco_positions():
    """Returns a list of fixed ArUco marker positions."""
    return [A1, B1, C1, D1]


# ---------------------------------------------------------------------------
# Directory Getter Functions
# ---------------------------------------------------------------------------


def get_camera_directory():
    """Returns the camera output directory path."""
    return CAMERA_DIR


def get_debug_directory():
    """Returns the debug output directory path."""
    return PICTURES_DIR / "debug"


def get_vision_directory():
    """Returns the vision_python directory path."""
    return VISION_DIR


def get_project_directory():
    """Returns the project root directory path."""
    return PROJECT_DIR


def get_pictures_directory():
    """Returns the pictures directory path."""
    return PICTURES_DIR


def get_log_directory():
    """Returns the log directory path."""
    return LOG_DIR


def get_camera_calibration_directory():
    """Returns the camera calibration directory path."""
    return CALIBRATION_DIR


def get_camera_calibration_file():
    """Returns the camera calibration file path."""
    return CALIBRATION_FILE


def get_emulated_cam_directory():
    """Returns the emulated images directory path."""
    return EMULATED_CAM_DIR

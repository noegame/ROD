#!/usr/bin/env python3
"""
Simple script to capture images for camera calibration.

This script initializes the camera in still mode (high quality),
and captures an image each time the user presses [Enter].

Images are saved in '/var/roboteseo/pictures/calibration' with the date and resolution in the filename.

Press Ctrl+C to stop the script.
"""

# ---------------------------------------------------------------------------
# Imports
# ---------------------------------------------------------------------------

import logging
import sys
from datetime import datetime
from pathlib import Path

try:
    from picamera2 import Picamera2
    import cv2
    import numpy as np
except ImportError as e:
    print(f"Error: Required library not found: {e}")
    print("Please install: pip install picamera2 opencv-python numpy")
    sys.exit(1)

# ANSI color codes
COLOR_GREEN = '\033[0;32m'
COLOR_RED = '\033[0;31m'
COLOR_RESET = '\033[0m'


def setup_logging():
    """Set up logging configuration."""
    logging.basicConfig(
        level=logging.INFO,
        format="%(asctime)s - %(name)s - %(levelname)s - %(message)s",
        stream=sys.stdout,
    )


def main():
    """Main function of the capture script."""
    setup_logging()
    logger = logging.getLogger("capture_for_calibration")

    cam = None
    try:
        # Configuration
        image_width, image_height = 4056, 3040  # Full resolution for calibration
        output_path = Path("/var/roboteseo/pictures/calibration") / datetime.now().strftime('%Y-%m-%d')

        # Create the output directory if it doesn't exist
        output_path.mkdir(parents=True, exist_ok=True)
        logger.info(f"Image output directory: {output_path}")

        # Initialize the camera in still mode (high quality)
        logger.info(
            f"Initializing the camera with a resolution of {image_width}x{image_height}..."
        )
        cam = Picamera2()
        config_cam = cam.create_still_configuration(
            main={"size": (image_width, image_height), "format": "RGB888"},
        )
        cam.configure(config_cam)
        cam.start()
        logger.info(f"{COLOR_GREEN}Camera initialized in STILL mode (high quality){COLOR_RESET}")

        logger.info("=" * 60)
        logger.info("Press [Enter] to capture an image")
        logger.info("Press Ctrl+C to quit")
        logger.info("=" * 60)

        # Loop to wait for user input
        capture_count = 0
        while True:
            input()  # Wait for the user to press Enter
            capture_count += 1
            logger.info(f"Capture #{capture_count} in progress...")

            try:
                # Create the timestamp and the filename with the resolution
                timestamp = datetime.now().strftime("%Y%m%d_%H%M%S_%f")[:-3]
                filename = f"{timestamp}_{image_width}x{image_height}_capture.jpg"
                filepath = output_path / filename

                # Capture the image
                image_array = cam.capture_array()

                # Save the image manually (convert RGB to BGR for OpenCV)
                cv2.imwrite(
                    str(filepath),
                    cv2.cvtColor(image_array, cv2.COLOR_RGB2BGR),
                    [cv2.IMWRITE_JPEG_QUALITY, 100],
                )

                logger.info(f"{COLOR_GREEN}Image captured: {filename}{COLOR_RESET}")

            except Exception as e:
                logger.error(f"{COLOR_RED}Capture error: {e}{COLOR_RESET}")

    except KeyboardInterrupt:
        logger.info("\nScript stopped by user.")
    except Exception as e:
        logger.error(f"A fatal error occurred: {e}")
    finally:
        if cam is not None:
            try:
                cam.close()
            except Exception as e:
                logger.debug(f"Error while stopping the camera: {e}")
        logger.info("Camera stopped.")


if __name__ == "__main__":
    main()

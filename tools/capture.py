#!/usr/bin/env python3

# ---------------------------------------------------------------------------
# Imports
# ---------------------------------------------------------------------------

from datetime import datetime
import logging
import sys

from rod_python.src.config import config
from rod_python.src.camera.camera_factory import get_camera

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

    cam = None  # has to be initialized before the try block
    camera = (
        config.get_camera_mode()
    )  # type of camera from config(emulated, raspberry, computer)
    try:
        image_width, image_height = config.get_camera_resolution()
        output_path = config.get_output_directory() / "camera"

        # Initialize the camera in still mode (high quality)
        logger.info(
            f"Initializing the camera with a resolution of {image_width}x{image_height}..."
        )
        cam = get_camera(
            w=image_width,
            h=image_height,
            camera=camera,
        )
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
                filename = (
                    f"{timestamp}_{image_width}x{image_height}_capture_debut_match.jpg"
                )
                filepath = output_path / filename

                # Capture the image
                import cv2

                image_array = cam.take_picture()

                # Save the image manually
                output_path.mkdir(parents=True, exist_ok=True)
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

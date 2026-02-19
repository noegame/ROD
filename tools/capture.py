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

        # Initialiser la caméra en mode still (haute qualité)
        logger.info(
            f"Initialisation de la caméra avec une résolution de {image_width}x{image_height}..."
        )
        cam = get_camera(
            w=image_width,
            h=image_height,
            camera=camera,
        )
        logger.info(f"{COLOR_GREEN}Camera initialized in STILL mode (high quality){COLOR_RESET}")

        logger.info("=" * 60)
        logger.info("Appuyez sur [Entrée] pour capturer une image")
        logger.info("Appuyez sur Ctrl+C pour quitter")
        logger.info("=" * 60)

        # Boucle pour attendre l'input de l'utilisateur
        capture_count = 0
        while True:
            input()  # Attend que l'utilisateur appuie sur Entrée
            capture_count += 1
            logger.info(f"Capture #{capture_count} en cours...")

            try:
                # Créer le timestamp et le nom de fichier avec la résolution
                timestamp = datetime.now().strftime("%Y%m%d_%H%M%S_%f")[:-3]
                filename = (
                    f"{timestamp}_{image_width}x{image_height}_capture_debut_match.jpg"
                )
                filepath = output_path / filename

                # Capturer l'image
                import cv2

                image_array = cam.take_picture()

                # Sauvegarder manuellement l'image
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
        logger.info("\nArrêt du script par l'utilisateur.")
    except Exception as e:
        logger.error(f"Une erreur fatale est survenue : {e}")
    finally:
        if cam is not None:
            try:
                cam.close()
            except Exception as e:
                logger.debug(f"Erreur lors de l'arrêt de la caméra : {e}")
        logger.info("Caméra arrêtée.")


if __name__ == "__main__":
    main()

#!/usr/bin/env python3
"""
Module pour la caméra émulée (simulation).
Lit des images depuis un dossier pour simuler une caméra.
"""

# ---------------------------------------------------------------------------
# Imports
# ---------------------------------------------------------------------------

from pathlib import Path
import numpy as np
import cv2
import logging
from typing import Dict, Any

from .camera import Camera

# ---------------------------------------------------------------------------
# Classe
# ---------------------------------------------------------------------------

logger = logging.getLogger("emulated_camera")


class EmulatedCamera(Camera):
    """
    Implémentation de Camera pour la simulation/test.

    Lit des images depuis un dossier spécifié pour simuler une caméra,
    permettant le test et le débogage sans matériel réel.
    """

    def __init__(self):
        """
        Initialise la caméra émulée.

        Note: Les paramètres (width, height, image_folder) doivent être configurés
        via set_parameters() avant d'appeler init().
        """
        self.width = None
        self.height = None
        self.image_folder = None
        self.image_files = []
        self.current_image_index = 0

    def init(self) -> None:
        """Initialise la caméra émulée en chargeant la liste des images."""
        if self.image_folder is None:
            raise ValueError(
                "image_folder doit être configuré via set_parameters() avant l'initialisation"
            )

        logger.info(
            f"Initialisation de la caméra émulée depuis le dossier: {self.image_folder}"
        )

        self.image_files = sorted(
            [
                p
                for p in self.image_folder.glob("*")
                if p.suffix.lower() in [".jpg", ".png", ".jpeg"]
            ]
        )

        if not self.image_files:
            raise FileNotFoundError(
                f"Aucune image trouvée dans le dossier: {self.image_folder}"
            )

        self.current_image_index = 0
        logger.info(f"Caméra émulée initialisée avec {len(self.image_files)} images.")

    def start(self) -> None:
        """
        Démarre le flux vidéo de la caméra émulée.

        Note: Pour la caméra émulée, cette opération ne fait rien de particulier.
        """
        logger.info("Flux vidéo de la caméra émulée 'démarré' (simulation).")

    def stop(self) -> None:
        """Arrête le flux vidéo et libère les ressources."""
        logger.info("Caméra émulée fermée.")
        self.current_image_index = 0

    def set_parameters(self, parameters: Dict[str, Any]) -> None:
        """
        Configure les paramètres de la caméra émulée.

        :param parameters: Dictionnaire de paramètres
                          Paramètres supportés:
                          - width (int): Largeur de l'image
                          - height (int): Hauteur de l'image
                          - image_folder (Path): Dossier contenant les images à utiliser
        """
        if "width" in parameters:
            self.width = parameters["width"]
            logger.info(f"Largeur configurée: {self.width}")

        if "height" in parameters:
            self.height = parameters["height"]
            logger.info(f"Hauteur configurée: {self.height}")

        if "image_folder" in parameters:
            self.image_folder = parameters["image_folder"]
            if not isinstance(self.image_folder, Path):
                self.image_folder = Path(self.image_folder)
            logger.info(f"Image folder configured: {self.image_folder}")

        # Log unrecognized parameters
        known_params = {"width", "height", "image_folder"}
        unknown_params = set(parameters.keys()) - known_params
        if unknown_params:
            logger.warning(f"Unrecognized parameters ignored: {unknown_params}")

    def take_picture(self) -> np.ndarray:
        """
        'Capture' une image en lisant le fichier suivant du dossier.

        :return: Image capturée en tant que np.ndarray (format RGB)
        """
        try:
            if not self.image_files:
                raise Exception("No images available in the configured folder")

            source_path = self.image_files[self.current_image_index]
            image_array = cv2.imread(str(source_path))  # cv2 reads in BGR

            if image_array is None:
                raise Exception(f"Cannot read the image: {source_path}")

            # Convert BGR to RGB for consistency with other cameras
            image_array_rgb = cv2.cvtColor(image_array, cv2.COLOR_BGR2RGB)

            # Move to next image (circular loop)
            self.current_image_index = (self.current_image_index + 1) % len(
                self.image_files
            )
            logger.info(f"Image 'captured' from: {source_path.name}")

            return image_array_rgb
        except Exception as e:
            logger.error(f"Error during simulated capture: {e}")
            raise

    def close(self):
        """Ferme et nettoie la caméra émulée (alias pour stop())."""
        self.stop()

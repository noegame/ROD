#!/usr/bin/env python3
"""
Module pour la gestion des webcams USB standard.
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

logger = logging.getLogger("webcam")


class Webcam(Camera):
    """
    Implémentation de Camera pour les webcams USB standard.

    Utilise OpenCV pour capturer des images depuis une webcam USB.
    """

    def __init__(self):
        """
        Initialise la webcam.

        Note: Les paramètres (width, height, device_id) doivent être configurés
        via set_parameters() avant d'appeler init().
        """
        self.width = None
        self.height = None
        self.device_id = 0  # Valeur par défaut
        self.capture = None

    def init(self) -> None:
        """Initialise la webcam USB."""
        if self.width is None or self.height is None:
            raise ValueError(
                "width et height doivent être configurés via set_parameters() avant l'initialisation"
            )

        try:
            logger.info(f"Initialisation de la webcam (device {self.device_id})...")
            self.capture = cv2.VideoCapture(self.device_id)

            if not self.capture.isOpened():
                raise Exception(
                    f"Impossible d'ouvrir la webcam avec l'ID {self.device_id}"
                )

            # Configuration de la résolution
            self.capture.set(cv2.CAP_PROP_FRAME_WIDTH, self.width)
            self.capture.set(cv2.CAP_PROP_FRAME_HEIGHT, self.height)

            # Vérification de la résolution effective
            actual_width = int(self.capture.get(cv2.CAP_PROP_FRAME_WIDTH))
            actual_height = int(self.capture.get(cv2.CAP_PROP_FRAME_HEIGHT))

            if actual_width != self.width or actual_height != self.height:
                logger.warning(
                    f"Résolution demandée ({self.width}x{self.height}) "
                    f"différente de la résolution obtenue ({actual_width}x{actual_height})"
                )

            logger.info(
                f"Webcam initialisée avec succès (résolution: {actual_width}x{actual_height})."
            )
        except Exception as e:
            logger.error(f"Erreur lors de l'initialisation de la webcam: {e}")
            raise

    def start(self) -> None:
        """
        Démarre le flux vidéo de la webcam.

        Note: Pour OpenCV VideoCapture, le flux est déjà actif après init().
        """
        if self.capture is None or not self.capture.isOpened():
            logger.warning("La webcam n'est pas initialisée. Réinitialisation...")
            self.init()
        else:
            logger.info("Flux vidéo de la webcam déjà actif.")

    def stop(self) -> None:
        """Arrête le flux vidéo et libère les ressources."""
        try:
            if self.capture is not None:
                self.capture.release()
                self.capture = None
                logger.info("Webcam fermée correctement.")
            else:
                logger.warning("Webcam n'était pas initialisée, rien à fermer.")
        except Exception as e:
            logger.warning(f"Erreur lors de la fermeture de la webcam: {e}")

    def set_parameters(self, parameters: Dict[str, Any]) -> None:
        """
        Configure les paramètres de la webcam.

        :param parameters: Dictionnaire de paramètres
                          Paramètres de configuration:
                          - width (int): Largeur de l'image
                          - height (int): Hauteur de l'image
                          - device_id (int): ID du périphérique de la webcam

                          Propriétés OpenCV (si la webcam est déjà initialisée):
                          - brightness, contrast, saturation, exposure, fps, gain, auto_exposure
        """
        # Paramètres de configuration (avant init)
        config_params = {"width", "height", "device_id"}
        opencv_params = {}

        for key, value in parameters.items():
            if key == "width":
                self.width = value
                logger.info(f"Largeur configurée: {self.width}")
            elif key == "height":
                self.height = value
                logger.info(f"Hauteur configurée: {self.height}")
            elif key == "device_id":
                self.device_id = value
                logger.info(f"Device ID configured: {self.device_id}")
            else:
                # OpenCV properties
                opencv_params[key] = value

        # Apply OpenCV properties if the webcam is initialized
        if opencv_params:
            try:
                if self.capture is None or not self.capture.isOpened():
                    logger.warning(
                        f"Webcam not initialized, ignored properties: {opencv_params}"
                    )
                    return

                # Mapping of readable parameter names to OpenCV properties
                param_mapping = {
                    "brightness": cv2.CAP_PROP_BRIGHTNESS,
                    "contrast": cv2.CAP_PROP_CONTRAST,
                    "saturation": cv2.CAP_PROP_SATURATION,
                    "exposure": cv2.CAP_PROP_EXPOSURE,
                    "fps": cv2.CAP_PROP_FPS,
                    "gain": cv2.CAP_PROP_GAIN,
                    "auto_exposure": cv2.CAP_PROP_AUTO_EXPOSURE,
                }

                for param_name, value in opencv_params.items():
                    if param_name in param_mapping:
                        prop_id = param_mapping[param_name]
                        success = self.capture.set(prop_id, value)
                        if success:
                            logger.info(f"Parameter '{param_name}' configured to {value}")
                        else:
                            logger.warning(
                                f"Unable to configure parameter '{param_name}'"
                            )
                    else:
                        logger.warning(f"Unknown parameter: '{param_name}'")

            except Exception as e:
                logger.error(f"Error while configuring parameters: {e}")
                raise

    def take_picture(self) -> np.ndarray:
        """
        Capture an image from the webcam.

        :return: Captured image as np.ndarray (RGB format)
        """
        try:
            if self.capture is None:
                raise Exception("The webcam is not initialized.")

            ret, frame = self.capture.read()

            if not ret:
                raise Exception("Failed to capture image from the webcam")

            # OpenCV reads images in BGR, we convert to RGB
            frame_rgb = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
            logger.debug(f"Image capturée: {frame_rgb.shape}")

            return frame_rgb

        except Exception as e:
            logger.error(f"Error while capturing image: {e}")
            raise

    def close(self):
        """Close and cleanup the webcam properly (alias for stop())."""
        self.stop()

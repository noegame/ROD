# Comment calibrer la caméra

Chaques caméra a des caractéristiques optiques qui lui sont propres. Ces caractéristiques peuvent introduire des distorsions dans les images capturées.
Pour compenser ces distorsions et obtenir des mesures précises, il est essentiel de calibrer la caméra. La calibration permet de déterminer les paramètres intrinsèques et extrinsèques de la caméra, ainsi que les coefficients de distorsion.
## Étapes de calibration
1. **Préparation du matériel** :
   - Utilisez une grille de calibration (par exemple, un échiquier imprimé) avec des dimensions connues comme ce [modèle de calibration](chessboard.pdf).
   - Assurez-vous que la grille est bien éclairée et que la caméra est stable.
2. **Capture des images** :
    - Prenez plusieurs images de la grille de calibration sous différents angles et positions.
    - Assurez-vous que la grille occupe une grande partie de l'image et qu'elle est bien visible.
3. **Détection des points de la grille** :
    - Utilisez une bibliothèque de vision par ordinateur (comme OpenCV) pour détecter les coins de la grille dans chaque image capturée.
    - Stockez les coordonnées des points détectés ainsi que leurs correspondances dans le monde réel.

4. **Calcul des paramètres de la caméra** :
    - Utilisez les points détectés pour calculer les paramètres intrinsèques (matrice de la caméra) et extrinsèques (rotation et translation) de la caméra.
    - Calculez également les coefficients de distorsion (radiale et tangentielles).

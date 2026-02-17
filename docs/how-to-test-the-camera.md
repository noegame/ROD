# How to test the camera quickly

On the raspberry pi, you can't use the librairie `libcamera` directly, you need to use the `rpicam` tools that are built on top of it. These tools are available in the `rpicam-tools` package.

Check first if the camera is connected.
``` shell
# Lister les caméras
rpicam-hello --list-cameras
# Test rapide d'affichage (5 secondes)
rpicam-hello --timeout 5000
# Test de capture d'image
rpicam-still --output test.jpg --timeout 2000
ls -la test.jpg
# Si l'image est créée, l'afficher pour vérifier
file test.jpg
```
#!/bin/bash
# Prend une photo toutes les 5 secondes et la sauvegarde dans le dossier picam/pictures/

# Configuration
INTERVAL=5
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
OUTPUT_DIR="$SCRIPT_DIR/../../output/camera"

# Color codes
GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m' # No Color

# Créer le répertoire s'il n'existe pas
mkdir -p "$OUTPUT_DIR"

echo "======================================"
echo "PiCam - Capture d'images en continu"
echo "======================================"
echo "Intervalle: ${INTERVAL} secondes"
echo "Dossier: ${OUTPUT_DIR}/"
echo "Appuyez sur Ctrl+C pour arrêter"
echo "--------------------------------------"

# Fonction de nettoyage
cleanup() {
    echo
    echo "Arrêt de la capture..."
    exit 0
}

# Capturer le signal Ctrl+C
trap cleanup SIGINT

# Compteur d'images
count=0

# Boucle principale
while true; do
    # Générer le nom de fichier avec timestamp
    timestamp=$(date '+%Y%m%d_%H%M%S')
    filename="${timestamp}_capture_shell.jpg"
    filepath="${OUTPUT_DIR}/${filename}"
    
    # Essayer rpicam-still (Raspberry Pi OS Bookworm)
    if command -v rpicam-still &> /dev/null; then
        if rpicam-still --width 1920 --height 1080 --output "$filepath" --timeout 1000 --nopreview &>/dev/null; then
            ((count++))
            echo -e "[$(date '+%H:%M:%S')] ${GREEN}[OK]${NC} Image $count: $filename"
        else
            echo -e "[$(date '+%H:%M:%S')] ${RED}[FAIL]${NC} Capture failed"
        fi

    # Essayer libcamera-still (Raspberry Pi OS Bookworm)
    elif command -v libcamera-still &> /dev/null; then
        if libcamera-still --width 1920 --height 1080 --output "$filepath" --timeout 1000 --nopreview &>/dev/null; then
            ((count++))
            echo -e "[$(date '+%H:%M:%S')] ${GREEN}[OK]${NC} Image $count: $filename"
        else
            echo -e "[$(date '+%H:%M:%S')] ${RED}[FAIL]${NC} Capture failed"
        fi
    
    # Essayer raspistill (Raspberry Pi OS Bullseye)
    elif command -v raspistill &> /dev/null; then
        if raspistill -w 1920 -h 1080 -o "$filepath" -t 1000 -n &>/dev/null; then
            ((count++))
            echo -e "[$(date '+%H:%M:%S')] ${GREEN}[OK]${NC} Image $count: $filename"
        else
            echo -e "[$(date '+%H:%M:%S')] ${RED}[FAIL]${NC} Capture failed"
        fi
    
    # Fallback avec fswebcam
    elif command -v fswebcam &> /dev/null; then
        if fswebcam -r 1920x1080 --no-banner --save "$filepath" &>/dev/null; then
            ((count++))
            echo -e "[$(date '+%H:%M:%S')] ${GREEN}[OK]${NC} Image $count: $filename (webcam)"
        else
            echo -e "[$(date '+%H:%M:%S')] ${RED}[FAIL]${NC} Capture failed"
        fi
    
    else
        echo -e "${RED}[FAIL]${NC} No capture tool found!"
        echo "Install: sudo apt install libcamera-apps"
        exit 1
    fi
    
    # Attendre 5 secondes
    sleep $INTERVAL
done
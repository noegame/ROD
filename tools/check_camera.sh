#!/bin/bash
# Check the Raspberry Pi camera

# Configuration
TIMESTAMP=$(date '+%Y%m%d_%H%M%S')
TIMEOUT=2000  # 2 secondes
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
OUTPUT_DIR="$SCRIPT_DIR/../../output/camera"
PHOTO_NAME="$OUTPUT_DIR/${TIMESTAMP}_camera_check.jpg"

# Couleurs pour l'affichage
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

print_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[OK]${NC} $1"
}

print_error() {
    echo -e "${RED}[FAIL]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

echo "$(printf '=%.0s' {1..60})"
echo "Raspberry Pi Camera Quick Test (Shell)"
echo "$(printf '=%.0s' {1..60})"

# Fonction pour prendre la photo
take_photo() {
    print_info "Test de la caméra..."
    
    # Vérifier si rpicam-still est disponible (Bookworm)
    if command -v rpicam-still &> /dev/null; then
        print_info "Utilisation de rpicam-still (Raspberry Pi OS Bookworm)"
        
        if rpicam-still --output "$PHOTO_NAME" --timeout $TIMEOUT --width 1920 --height 1080 --nopreview 2>/dev/null; then
            print_success "Photo prise avec rpicam-still: $PHOTO_NAME"
            return 0
        else
            print_error "Échec avec rpicam-still"
            return 1
        fi
        
    # Vérifier si libcamera-still est disponible
    elif command -v libcamera-still &> /dev/null; then
        print_info "Utilisation de libcamera-still"
        
        if libcamera-still --output "$PHOTO_NAME" --timeout $TIMEOUT --width 1920 --height 1080 --nopreview 2>/dev/null; then
            print_success "Photo prise avec libcamera-still: $PHOTO_NAME"
            return 0
        else
            print_error "Échec avec libcamera-still"
            return 1
        fi
        
    # Vérifier si raspistill est disponible (legacy)
    elif command -v raspistill &> /dev/null; then
        print_info "Utilisation de raspistill (legacy)"
        
        if raspistill -o "$PHOTO_NAME" -t $TIMEOUT -w 1920 -h 1080 -n 2>/dev/null; then
            print_success "Photo prise avec raspistill: $PHOTO_NAME"
            return 0
        else
            print_error "Échec avec raspistill"
            return 1
        fi
        
    else
        print_error "Aucun outil de capture trouvé!"
        print_error "Installez: sudo apt install libcamera-apps"
        return 1
    fi
}

# Script principal
main() {
    # Prendre la photo
    if ! take_photo; then
        print_error "Test échoué - Problème avec la caméra"
        exit 1
    fi
    
    print_success "Test caméra réussi!"
    print_info "Fichier local: $PHOTO_NAME"
}

# Vérification des permissions
if [ ! -w "." ]; then
    print_error "Pas de permission d'écriture dans le répertoire courant"
    exit 1
fi

# Lancer le script principal
main "$@"
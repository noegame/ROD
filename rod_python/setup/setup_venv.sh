#!/bin/bash

# Script pour créer l'environnement virtuel Python et installer les dépendances
# Usage: ./setup_venv.sh

set -e  # Arrêter le script en cas d'erreur

# Couleurs
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Se déplacer vers le répertoire racine du projet (rod_python/)
cd "$(dirname "$0")/.."

echo "Configuration de l'environnement virtuel Python..."

# Vérifier si Python3 est installé
if ! command -v python3 &> /dev/null; then
    echo -e "${RED}Erreur: Python3 n'est pas installé${NC}"
    echo "Installez Python3 d'abord: sudo apt install python3 python3-venv python3-pip"
    exit 1
fi

# Afficher la version de Python
PYTHON_VERSION=$(python3 --version)
echo -e "${GREEN}$PYTHON_VERSION détecté${NC}"

# Créer l'environnement virtuel s'il n'existe pas déjà
if [ -d ".venv" ]; then
    echo -e "${YELLOW}Le dossier .venv existe déjà${NC}"
    read -p "Voulez-vous le supprimer et le recréer? (o/N): " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Oo]$ ]]; then
        echo "Suppression de l'ancien .venv..."
        rm -rf .venv
    else
        echo "Conservation du .venv existant"
    fi
fi

if [ ! -d ".venv" ]; then
    echo "Création de l'environnement virtuel..."
    python3 -m venv .venv
    echo -e "${GREEN}Environnement virtuel créé dans $(pwd)/.venv${NC}"
else
    echo "Utilisation du .venv existant"
fi

# Activer l'environnement virtuel
echo "Activation de l'environnement virtuel..."
source .venv/bin/activate

# Mettre à jour pip
echo "Mise à jour de pip..."
pip install --upgrade pip

# Installer les dépendances depuis requirements.txt
echo "Installation des dépendances..."
if [ -f "requirements.txt" ]; then
    pip install -r requirements.txt
    echo -e "${GREEN}Dépendances installées avec succès${NC}"
else
    echo -e "${RED}Erreur: fichier requirements.txt non trouvé${NC}"
    exit 1
fi

echo ""
echo -e "${GREEN}Configuration terminée!${NC}"
echo ""
echo "Pour activer l'environnement virtuel:"
echo "  source .venv/bin/activate"
echo ""
echo "Pour désactiver l'environnement virtuel:"
echo "  deactivate"

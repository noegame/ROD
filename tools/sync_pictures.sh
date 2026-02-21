#!/bin/bash

# Sync pictures from Raspberry Pi to local PC
# Downloads /var/roboteseo/pictures/YYYY_MM_DD from Pi to local pictures/camera/

set -e

# Configuration
PI_USER="roboteseo"
PI_HOST="tdc.local"
PI_PICTURES_DIR="/var/roboteseo/pictures/camera"
LOCAL_PICTURES_DIR="pictures/camera"

# Get current date in YYYY_MM_DD format
CURRENT_DATE=$(date +%Y_%m_%d)

# Source and destination paths
SOURCE="${PI_USER}@${PI_HOST}:${PI_PICTURES_DIR}/${CURRENT_DATE}/"
DESTINATION="${LOCAL_PICTURES_DIR}/${CURRENT_DATE}/"

echo "Syncing pictures from Pi..."
echo "Source: ${SOURCE}"
echo "Destination: ${DESTINATION}"

# Create local destination directory if it doesn't exist
mkdir -p "${DESTINATION}"

# Use rsync to download only new/modified files
# -a: archive mode (preserves permissions, timestamps, etc.)
# -v: verbose output
# -z: compress data during transfer
# --progress: show progress during transfer
# --update: skip files that are newer on the receiver
rsync -avz --progress --update "${SOURCE}" "${DESTINATION}"

echo "Sync complete!"
echo "Pictures saved to: ${DESTINATION}"

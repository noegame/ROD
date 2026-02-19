#!/bin/bash
# Installation script for ROD systemd services
# This script installs the ROD services to systemd

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Get the directory where the script is located
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

echo -e "${GREEN}=== ROD Systemd Services Installation ===${NC}"
echo ""

# Check if running as root
if [ "$EUID" -ne 0 ]; then 
    echo -e "${RED}Error: This script must be run as root (use sudo)${NC}"
    exit 1
fi

# Check if executables exist
if [ ! -f "$SCRIPT_DIR/../build/rod_detection" ]; then
    echo -e "${RED}Error: rod_detection executable not found${NC}"
    echo "Please build the project first: cd .. && ./build.sh"
    exit 1
fi

if [ ! -f "$SCRIPT_DIR/../build/rod_communication" ]; then
    echo -e "${RED}Error: rod_communication executable not found${NC}"
    echo "Please build the project first: cd .. && ./build.sh"
    exit 1
fi

# Create required directories
echo "Creating application directories..."
mkdir -p /var/roboteseo/pictures
mkdir -p /var/roboteseo/pictures/debug
mkdir -p /var/roboteseo/pictures/camera_tests

# Set permissions (allow current user and service to write)
chown -R $SUDO_USER:$SUDO_USER /var/roboteseo
chmod -R 755 /var/roboteseo

echo -e "${GREEN}✓${NC} Directories created in /var/roboteseo/"

# Copy service files
echo "Installing service files..."
cp "$SCRIPT_DIR/rod-detection.service" /etc/systemd/system/
cp "$SCRIPT_DIR/rod-communication.service" /etc/systemd/system/
cp "$SCRIPT_DIR/rod.target" /etc/systemd/system/

echo -e "${GREEN}✓${NC} Service files copied to /etc/systemd/system/"

# Reload systemd
echo "Reloading systemd daemon..."
systemctl daemon-reload
echo -e "${GREEN}✓${NC} Systemd daemon reloaded"

echo ""
echo -e "${GREEN}Installation complete!${NC}"
echo ""
echo "Next steps:"
echo "  1. Start services:     ${YELLOW}sudo systemctl start rod.target${NC}"
echo "  2. Check status:       ${YELLOW}systemctl status rod.target${NC}"
echo "  3. View logs:          ${YELLOW}sudo journalctl -u rod-detection.service -f${NC}"
echo "  4. Enable at boot:     ${YELLOW}sudo systemctl enable rod.target${NC}"
echo ""

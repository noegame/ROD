#!/bin/bash
# Uninstallation script for ROD systemd services

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${YELLOW}=== ROD Systemd Services Uninstallation ===${NC}"
echo ""

# Check if running as root
if [ "$EUID" -ne 0 ]; then 
    echo -e "${RED}Error: This script must be run as root (use sudo)${NC}"
    exit 1
fi

# Stop services if running
echo "Stopping services..."
systemctl stop rod.target 2>/dev/null || true
systemctl stop rod_detection.service 2>/dev/null || true
systemctl stop rod_communication.service 2>/dev/null || true
echo -e "${GREEN}✓${NC} Services stopped"

# Disable services
echo "Disabling services..."
systemctl disable rod.target 2>/dev/null || true
systemctl disable rod_detection.service 2>/dev/null || true
systemctl disable rod_communication.service 2>/dev/null || true
echo -e "${GREEN}✓${NC} Services disabled"

# Remove service files
echo "Removing service files..."
rm -f /etc/systemd/system/rod_detection.service
rm -f /etc/systemd/system/rod_communication.service
rm -f /etc/systemd/system/rod.target
echo -e "${GREEN}✓${NC} Service files removed"

# Clean up socket file
rm -f /tmp/rod_detection.sock

# Ask about data directories
echo ""
read -p "Remove application directories (/var/roboteseo/pictures)? [y/N] " -n 1 -r
echo ""
if [[ $REPLY =~ ^[Yy]$ ]]; then
    rm -rf /var/roboteseo
    echo -e "${GREEN}✓${NC} Application directories removed"
else
    echo -e "${YELLOW}→${NC} Application directories preserved"
fi

# Reload systemd
echo "Reloading systemd daemon..."
systemctl daemon-reload
echo -e "${GREEN}✓${NC} Systemd daemon reloaded"

echo ""
echo -e "${GREEN}Uninstallation complete!${NC}"
echo ""

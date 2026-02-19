#!/bin/bash

# Script to show all registered WiFi networks with known passwords

# Color codes
GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m' # No Color

echo "=== Registered WiFi Networks ==="
echo ""

# Check if nmcli is available
if ! command -v nmcli &> /dev/null; then
    echo "Error: nmcli command not found. Please install NetworkManager."
    exit 1
fi

# List all configured WiFi connections from /etc/NetworkManager/system-connections
if [ -d "/etc/NetworkManager/system-connections" ]; then
    found_any=0
    for file in /etc/NetworkManager/system-connections/*.nmconnection; do
        if [ -f "$file" ]; then
            # Extract SSID from the file
            ssid=$(grep "^ssid=" "$file" | head -1 | cut -d'=' -f2-)
            
            if [ -n "$ssid" ]; then
                # Check if connection has a password configured
                has_psk=$(grep -c "^psk=" "$file")
                
                if [ "$has_psk" -gt 0 ]; then
                    echo -e "${GREEN}[OK]${NC} $ssid"
                    found_any=1
                fi
            fi
        fi
    done
    
    if [ "$found_any" -eq 0 ]; then
        echo -e "${RED}[FAIL]${NC} No WiFi networks with passwords found."
    fi
else
    echo -e "${RED}[FAIL]${NC} NetworkManager system connections directory not found."
    echo "Try using: nmcli device wifi list"
    exit 1
fi

#!/bin/bash

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${GREEN}Building Vaani - Indian English Speech Assistant...${NC}"

# Check if Vosk library is extracted
VOSK_ZIP="vosk-linux-aarch64-0.3.45.zip"
VOSK_DIR="vosk-linux-aarch64-0.3.45"

if [ ! -d "$VOSK_DIR" ]; then
    if [ -f "$VOSK_ZIP" ]; then
        echo -e "${YELLOW}Vosk library not found. Extracting from $VOSK_ZIP...${NC}"
        unzip -q "$VOSK_ZIP"
        if [ $? -eq 0 ]; then
            echo -e "${GREEN}Vosk library extracted successfully!${NC}"
        else
            echo -e "${RED}Failed to extract Vosk library!${NC}"
            exit 1
        fi
    else
        echo -e "${RED}Error: Vosk library zip file not found: $VOSK_ZIP${NC}"
        echo -e "${YELLOW}Please download the Vosk library for your platform.${NC}"
        exit 1
    fi
else
    echo -e "${BLUE}Vosk library already available.${NC}"
fi

# Clean previous build
make clean

# Build the program
if make; then
    mkdir -p ~/.config/systemd/user
    cp vaani.service ~/.config/systemd/user/vaani.service
    systemctl --user daemon-reload
    systemctl --user enable vaani.service
    loginctl enable-linger $USER
    systemctl --user start vaani.service
    echo -e "\n${GREEN}Build successful!${NC}"
    exit 0
else
    echo -e "\n${RED}Build failed! Please check the errors above.${NC}"
    exit 1
fi 
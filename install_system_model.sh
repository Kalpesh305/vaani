#!/bin/bash

# System-wide Vosk Model Installation Script
# This script specifically installs the model system-wide

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Model information
MODEL_NAME="vosk-model-en-in-0.5"
MODEL_URL="https://alphacephei.com/vosk/models/${MODEL_NAME}.zip"
SYSTEM_MODEL_DIR="/usr/local/share/vosk-models"

echo -e "${BLUE}=== System-wide Vosk Model Installation ===${NC}"
echo

# Check if already installed system-wide
if [ -d "${SYSTEM_MODEL_DIR}/${MODEL_NAME}" ]; then
    echo -e "${GREEN}Model already installed system-wide at: ${SYSTEM_MODEL_DIR}/${MODEL_NAME}${NC}"
    exit 0
fi

# Check for sudo privileges
if ! sudo -n true 2>/dev/null; then
    echo -e "${RED}This script requires sudo privileges for system-wide installation.${NC}"
    echo -e "${YELLOW}Please run with sudo or ensure you have sudo access.${NC}"
    exit 1
fi

# Create system directory
echo -e "${BLUE}Creating system directories...${NC}"
sudo mkdir -p "$SYSTEM_MODEL_DIR"
if [ $? -ne 0 ]; then
    echo -e "${RED}Failed to create system directory${NC}"
    exit 1
fi

# Create temporary directory
TEMP_DIR="/tmp/vosk_model_install_$$"
mkdir -p "$TEMP_DIR"
cd "$TEMP_DIR"

# Download model
echo -e "${YELLOW}Downloading ${MODEL_NAME}.zip (this may take a while)...${NC}"
wget --progress=bar:force "$MODEL_URL"
if [ $? -ne 0 ]; then
    echo -e "${RED}Failed to download model${NC}"
    cd - > /dev/null
    rm -rf "$TEMP_DIR"
    exit 1
fi

# Extract model
echo -e "${YELLOW}Extracting model...${NC}"
unzip -q "${MODEL_NAME}.zip"
if [ $? -ne 0 ]; then
    echo -e "${RED}Failed to extract model${NC}"
    cd - > /dev/null
    rm -rf "$TEMP_DIR"
    exit 1
fi

# Install to system location
echo -e "${YELLOW}Installing to system location...${NC}"
sudo mv "$MODEL_NAME" "${SYSTEM_MODEL_DIR}/"
if [ $? -eq 0 ]; then
    echo -e "${GREEN}✓ Model installed successfully to: ${SYSTEM_MODEL_DIR}/${MODEL_NAME}${NC}"
    echo -e "${BLUE}The model is now available system-wide for all users${NC}"
    echo -e "${BLUE}You can now safely delete any local vosk-model directories${NC}"
else
    echo -e "${RED}Failed to install to system location${NC}"
    cd - > /dev/null
    rm -rf "$TEMP_DIR"
    exit 1
fi

# Cleanup
cd - > /dev/null
rm -rf "$TEMP_DIR"

echo
echo -e "${GREEN}=== System Installation Complete ===${NC}"
echo -e "${YELLOW}To remove local model (optional): rm -rf ./vosk-model${NC}" 
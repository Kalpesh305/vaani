#!/bin/bash

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
LOCAL_MODEL_DIR="./vosk-model"

# Function to check if directory exists
directory_exists() {
    [ -d "$1" ]
}

# Function to check if user has sudo privileges
has_sudo() {
    sudo -n true 2>/dev/null
}

# Function to create system directories with proper permissions
create_system_dirs() {
    echo -e "${BLUE}Creating system directories...${NC}"
    if has_sudo; then
        sudo mkdir -p "$SYSTEM_MODEL_DIR"
        if [ $? -eq 0 ]; then
            echo -e "${GREEN}Created system directory: $SYSTEM_MODEL_DIR${NC}"
            return 0
        else
            echo -e "${RED}Failed to create system directory${NC}"
            return 1
        fi
    else
        echo -e "${YELLOW}No sudo privileges. Will install locally.${NC}"
        return 1
    fi
}

# Function to install model to system location
install_system_model() {
    local temp_dir="/tmp/vosk_model_$$"
    local system_path="${SYSTEM_MODEL_DIR}/${MODEL_NAME}"
    
    echo -e "${BLUE}Installing Vosk model to system location...${NC}"
    
    # Create temporary directory
    mkdir -p "$temp_dir"
    cd "$temp_dir"
    
    # Download model
    echo -e "${YELLOW}Downloading ${MODEL_NAME}.zip...${NC}"
    wget -q --show-progress "$MODEL_URL"
    if [ $? -ne 0 ]; then
        echo -e "${RED}Failed to download model${NC}"
        cd - > /dev/null
        rm -rf "$temp_dir"
        return 1
    fi
    
    # Extract model
    echo -e "${YELLOW}Extracting model...${NC}"
    unzip -q "${MODEL_NAME}.zip"
    if [ $? -ne 0 ]; then
        echo -e "${RED}Failed to extract model${NC}"
        cd - > /dev/null
        rm -rf "$temp_dir"
        return 1
    fi
    
    # Install to system location
    echo -e "${YELLOW}Installing to system location...${NC}"
    sudo mv "$MODEL_NAME" "$system_path"
    if [ $? -eq 0 ]; then
        echo -e "${GREEN}Model installed successfully to: $system_path${NC}"
        cd - > /dev/null
        rm -rf "$temp_dir"
        return 0
    else
        echo -e "${RED}Failed to install to system location${NC}"
        cd - > /dev/null
        rm -rf "$temp_dir"
        return 1
    fi
}

# Function to install model locally
install_local_model() {
    echo -e "${BLUE}Installing Vosk model locally...${NC}"
    
    # Check if local model already exists
    if directory_exists "$LOCAL_MODEL_DIR"; then
        echo -e "${GREEN}Local model already exists at: $LOCAL_MODEL_DIR${NC}"
        return 0
    fi
    
    # Download model
    echo -e "${YELLOW}Downloading ${MODEL_NAME}.zip...${NC}"
    wget -q --show-progress "$MODEL_URL"
    if [ $? -ne 0 ]; then
        echo -e "${RED}Failed to download model${NC}"
        return 1
    fi
    
    # Extract model
    echo -e "${YELLOW}Extracting model...${NC}"
    unzip -q "${MODEL_NAME}.zip"
    if [ $? -ne 0 ]; then
        echo -e "${RED}Failed to extract model${NC}"
        rm -f "${MODEL_NAME}.zip"
        return 1
    fi
    
    # Rename to expected directory name
    mv "$MODEL_NAME" "$LOCAL_MODEL_DIR"
    if [ $? -eq 0 ]; then
        echo -e "${GREEN}Model installed successfully to: $LOCAL_MODEL_DIR${NC}"
        rm -f "${MODEL_NAME}.zip"
        return 0
    else
        echo -e "${RED}Failed to rename model directory${NC}"
        rm -f "${MODEL_NAME}.zip"
        rm -rf "$MODEL_NAME"
        return 1
    fi
}

# Function to check if model exists in any location
check_existing_model() {
    local locations=(
        "/usr/local/share/vosk-models/vosk-model-en-in-0.5"
        "/opt/vosk-models/vosk-model-en-in-0.5"
        "/usr/share/vosk-models/vosk-model-en-in-0.5"
        "./vosk-model"
    )
    
    for location in "${locations[@]}"; do
        if directory_exists "$location"; then
            echo -e "${GREEN}Vosk model already exists at: $location${NC}"
            return 0
        fi
    done
    
    return 1
}

# Main execution
main() {
    echo -e "${BLUE}=== Vosk Model Setup Script ===${NC}"
    echo
    
    # Check if model already exists
    if check_existing_model; then
        echo -e "${GREEN}Vosk model is already available. No action needed.${NC}"
        exit 0
    fi
    
    echo -e "${YELLOW}Vosk model not found. Setting up...${NC}"
    echo
    
    # Try to install to system location first
    if create_system_dirs && install_system_model; then
        echo
        echo -e "${GREEN}✓ Model installed successfully to system location${NC}"
        echo -e "${BLUE}The model is now available system-wide and won't need to be included in your repository${NC}"
    else
        echo
        echo -e "${YELLOW}System installation failed. Installing locally...${NC}"
        if install_local_model; then
            echo
            echo -e "${GREEN}✓ Model installed successfully to local directory${NC}"
            echo -e "${YELLOW}Note: Add 'vosk-model/' to your .gitignore to avoid committing the model${NC}"
        else
            echo
            echo -e "${RED}✗ Failed to install model${NC}"
            echo -e "${RED}Please check your internet connection and try again${NC}"
            exit 1
        fi
    fi
    
    echo
    echo -e "${GREEN}=== Setup Complete ===${NC}"
}

# Run main function
main "$@" 
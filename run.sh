#!/bin/bash

# Colors for output
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Check if the program exists
if [ ! -f "./vaani" ]; then
    echo -e "${RED}Error: Vaani program not found. Please run ./build.sh first.${NC}"
    exit 1
fi

# Check if the program is executable
if [ ! -x "./vaani" ]; then
    echo -e "${RED}Error: Vaani program is not executable.${NC}"
    exit 1
fi

# Function to check if model exists in any location
check_model_exists() {
    local locations=(
        "/usr/local/share/vosk-models/vosk-model-en-in-0.5"
        "/opt/vosk-models/vosk-model-en-in-0.5"
        "/usr/share/vosk-models/vosk-model-en-in-0.5"
        "./vosk-model"
    )
    
    for location in "${locations[@]}"; do
        if [ -d "$location" ]; then
            return 0
        fi
    done
    
    return 1
}

# Check if the model exists in any location
if ! check_model_exists; then
    echo -e "${YELLOW}Vosk model not found. Setting up automatically...${NC}"
    ./setup_vosk_model.sh
    if [ $? -ne 0 ]; then
        echo -e "${RED}Failed to setup Vosk model. Please run ./setup_vosk_model.sh manually.${NC}"
        exit 1
    fi
fi

echo -e "${GREEN}Starting Vaani - Indian English Speech Assistant...${NC}\n"
./vaani 
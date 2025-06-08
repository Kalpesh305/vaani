# Vaani - Indian English Speech Assistant

Vaani (वाणी, meaning "voice/speech" in Sanskrit) is an intelligent speech processing system optimized for Indian English. It provides Speech-to-Text (STT), Text-to-Speech (TTS), and speech-based Q&A capabilities using an intent-matching system.

## Features

- **Speech-to-Text** with Indian English accent recognition using Vosk
- **Text-to-Speech** using Festival with customized speech rate
- **Speech-based Q&A System** with CSV-based intent matching
- **Smart Model Management** with system-wide installation support
- Real-time audio processing with:
  - Audio normalization
  - DC offset removal
  - Silence detection
  - Chunk-based processing
  - Buffer overrun protection

## Prerequisites

- GCC compiler
- ALSA development libraries (`libasound2-dev`)
- Vosk library and Indian English model
- Festival Text-to-Speech system
- wget and unzip utilities

## Installation

1. Install required packages:
   ```bash
   sudo apt-get update
   sudo apt-get install gcc libasound2-dev festival wget unzip
   ```

2. Install Vosk library:
   ```bash
   sudo pip3 install vosk
   ```

3. **Automatic Model Setup** (Recommended):
   ```bash
   # Run the setup script - it will handle everything automatically
   ./setup_vosk_model.sh
   ```
   
   The setup script will:
   - Check if a Vosk model already exists (system-wide or locally)
   - Try to install the model system-wide (`/usr/local/share/vosk-models/`) if you have sudo privileges
   - Fall back to local installation if system-wide installation fails
   - Skip download if model already exists

4. **Manual Model Setup** (Alternative):
   ```bash
   # Download and extract the Indian English model locally
   wget https://alphacephei.com/vosk/models/vosk-model-en-in-0.5.zip
   unzip vosk-model-en-in-0.5.zip
   mv vosk-model-en-in-0.5 vosk-model
   ```

## Building and Running

1. **Quick Start** (Recommended):
   ```bash
   ./build.sh  # Build the project (automatically extracts Vosk library if needed)
   ./run.sh    # Run with automatic model setup
   ```
   
   The build script will automatically:
   - Extract the Vosk library from the included zip file if not already extracted
   - Build all components and create the executable

2. **Manual Build**:
   ```bash
   # Extract Vosk library manually if needed
   unzip -q vosk-linux-aarch64-0.3.45.zip
   
   # Build the project
   make clean
   make
   ./vaani
   ```

## Usage

The program provides an interactive menu with the following options:
1. **Ask a Question (Speech Q&A)** - Complete STT→Intent Matching→TTS pipeline
2. **Speech to Text (STT)** - Convert speech to text only
3. **Text to Speech (TTS)** - Convert text to speech only
4. **Exit**

### Speech Q&A System
- Choose option 1 and ask questions in natural language
- The system matches your speech against a CSV database of intents
- Responds with relevant answers using TTS
- Shows confidence scores for matches

### Intent Database
Questions and answers are stored in `data/intents.csv`. You can:
- Add new Q&A pairs
- Modify existing responses
- The system uses word-based similarity matching with 80% threshold

## Model Management

The system intelligently looks for Vosk models in the following order:
1. `/usr/local/share/vosk-models/vosk-model-en-in-0.5` (System-wide, preferred)
2. `/opt/vosk-models/vosk-model-en-in-0.5` (Alternative system location)
3. `/usr/share/vosk-models/vosk-model-en-in-0.5` (Another system location)
4. `./vosk-model` (Local directory)

**Benefits of system-wide installation:**
- No need to include large model files in your Git repository
- Shared across multiple projects
- Automatic setup with the provided script

## Project Structure

```
vaani/
├── include/
│   ├── speech_processor.h      # Speech processing declarations
│   └── intent_processor.h      # Intent matching declarations
├── src/
│   ├── main.c                  # Main program and menu system
│   ├── audio/
│   │   └── audio_processor.c   # Audio processing functions
│   └── speech/
│       ├── speech_processor.c  # STT and TTS functions
│       └── intent_processor.c  # Intent matching and CSV parsing
├── data/
│   └── intents.csv            # Q&A database
├── vosk-linux-aarch64-0.3.45.zip  # Vosk library (auto-extracted during build)
├── setup_vosk_model.sh        # Automatic model setup script
├── install_system_model.sh    # System-wide model installation script
├── build.sh                   # Build script with auto-extraction
├── run.sh                     # Run script with auto-setup
├── Makefile                   # Build system
├── SETUP_GUIDE.md             # Detailed setup instructions
└── .gitignore                 # Excludes build artifacts and large files
```

## License

This project is open source and available under the MIT License.

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request. 
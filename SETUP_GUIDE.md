# Vaani Setup Guide

This guide explains how to set up the Vosk model for Vaani in different ways.

## Quick Setup (Recommended)

The easiest way to get started:

```bash
# 1. Build the project
./build.sh

# 2. Run with automatic setup
./run.sh
```

The `run.sh` script will automatically detect if you need the Vosk model and run the setup script for you.

## Model Installation Options

### Option 1: Automatic Setup Script (Recommended)

```bash
./setup_vosk_model.sh
```

This script will:
- Check if a model already exists in any location
- Try to install system-wide (requires sudo)
- Fall back to local installation if needed
- Skip download if model already exists

### Option 2: System-wide Installation Only

```bash
sudo ./install_system_model.sh
```

This script specifically installs the model system-wide to `/usr/local/share/vosk-models/`.

### Option 3: Manual Local Installation

```bash
wget https://alphacephei.com/vosk/models/vosk-model-en-in-0.5.zip
unzip vosk-model-en-in-0.5.zip
mv vosk-model-en-in-0.5 vosk-model
```

## Model Search Order

Vaani looks for the Vosk model in this order:

1. `/usr/local/share/vosk-models/vosk-model-en-in-0.5` (System-wide, preferred)
2. `/opt/vosk-models/vosk-model-en-in-0.5` (Alternative system location)
3. `/usr/share/vosk-models/vosk-model-en-in-0.5` (Another system location)
4. `./vosk-model` (Local directory)

## Benefits of System-wide Installation

✅ **Advantages:**
- Model is shared across all projects
- No need to include large files in Git repositories
- Cleaner project structure
- One-time download for multiple projects

❌ **Disadvantages:**
- Requires sudo privileges
- May need coordination in multi-user systems

## Benefits of Local Installation

✅ **Advantages:**
- No sudo privileges required
- Project-specific model versions
- Works in restricted environments

❌ **Disadvantages:**
- Larger repository size (if not using .gitignore)
- Need to download for each project
- Takes up more disk space

## Troubleshooting

### Model Not Found Error
If you see "Could not find Vosk model", try:
1. Run `./setup_vosk_model.sh`
2. Check your internet connection
3. Verify wget and unzip are installed

### Permission Denied
If system installation fails:
1. Check if you have sudo privileges
2. Use local installation instead
3. Ask your system administrator

### Download Fails
If download fails:
1. Check internet connection
2. Try manual download with a web browser
3. Use a different mirror if available

## File Structure After Setup

### System-wide Installation:
```
/usr/local/share/vosk-models/
└── vosk-model-en-in-0.5/
    ├── am/
    ├── conf/
    ├── graph/
    └── ivector/
```

### Local Installation:
```
your-project/
├── vosk-model/          # ← Model files here
│   ├── am/
│   ├── conf/
│   ├── graph/
│   └── ivector/
└── src/
```

## Removing Models

### Remove Local Model:
```bash
rm -rf ./vosk-model
```

### Remove System Model:
```bash
sudo rm -rf /usr/local/share/vosk-models/vosk-model-en-in-0.5
```

## Advanced Configuration

You can modify the model search paths by editing the `find_vosk_model_path()` function in `src/speech/speech_processor.c` if you need custom locations. 
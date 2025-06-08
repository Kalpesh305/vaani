#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <vosk_api.h>
#include "../../include/speech_processor.h"

// Global Vosk model instance
VoskModel *g_vosk_model = NULL;

// Buffer to store the last recognized text
static char g_last_recognized_text[MAX_TEXT_LENGTH] = {0};

// Helper function to escape special characters for Festival
static char* escape_text_for_festival(const char* text, char* buffer, size_t buffer_size) {
    size_t i = 0, j = 0;
    
    // Leave room for null terminator
    buffer_size--;
    
    while (text[i] && j < buffer_size) {
        // Handle special characters that need escaping for Festival
        if (text[i] == '"' || text[i] == '\\' || text[i] == '(' || text[i] == ')' || 
            text[i] == '[' || text[i] == ']' || text[i] == '{' || text[i] == '}' ||
            text[i] == '/' || text[i] == '|' || text[i] == '*' || text[i] == '+' ||
            text[i] == '=' || text[i] == '&' || text[i] == '$' || text[i] == '%' ||
            text[i] == '#' || text[i] == '@' || text[i] == '!' || text[i] == '~') {
            if (j + 1 >= buffer_size) break;
            buffer[j++] = '\\';
            buffer[j++] = text[i];
        }
        // Replace non-ASCII characters with their closest ASCII equivalents
        else if ((unsigned char)text[i] >= 0x80) {
            // Handle common non-ASCII characters
            if (text[i] == '-' || text[i] == '-') {
                if (j + 1 >= buffer_size) break;
                buffer[j++] = '-';
            }
            else if (text[i] == '"' || text[i] == '"') {
                if (j + 1 >= buffer_size) break;
                buffer[j++] = '"';
            }
            else if (text[i] == '\'' || text[i] == '\'') {
                if (j + 1 >= buffer_size) break;
                buffer[j++] = '\'';
            }
            else {
                // Skip other non-ASCII characters
                i++;
                continue;
            }
        }
        else {
            buffer[j++] = text[i];
        }
        i++;
    }
    
    buffer[j] = '\0';
    return buffer;
}

// Helper function to check if a directory exists
static int directory_exists(const char *path) {
    struct stat st;
    return (stat(path, &st) == 0 && S_ISDIR(st.st_mode));
}

// Helper function to find the Vosk model in various locations
static const char* find_vosk_model_path(void) {
    // List of potential model locations in order of preference
    const char* model_paths[] = {
        "/usr/local/share/vosk-models/vosk-model-en-in-0.5",   // System-wide preferred
        "/opt/vosk-models/vosk-model-en-in-0.5",               // Alternative system location
        "/usr/share/vosk-models/vosk-model-en-in-0.5",         // Another system location
        "vosk-model",                                           // Local directory (current setup)
        "./vosk-model",                                         // Explicit local path
        NULL
    };
    
    for (int i = 0; model_paths[i] != NULL; i++) {
        if (directory_exists(model_paths[i])) {
            printf("Found Vosk model at: %s\n", model_paths[i]);
            return model_paths[i];
        }
    }
    
    return NULL;
}

int initialize_vosk_model(void) {
    printf("Initializing speech recognition with Indian English model...\n");
    
    // Find the model in system or local directories
    const char* model_path = find_vosk_model_path();
    if (!model_path) {
        fprintf(stderr, "Could not find Vosk model in any of the following locations:\n");
        fprintf(stderr, "  - /usr/local/share/vosk-models/vosk-model-en-in-0.5\n");
        fprintf(stderr, "  - /opt/vosk-models/vosk-model-en-in-0.5\n");
        fprintf(stderr, "  - /usr/share/vosk-models/vosk-model-en-in-0.5\n");
        fprintf(stderr, "  - vosk-model (local directory)\n");
        fprintf(stderr, "\nPlease run the setup script: ./setup_vosk_model.sh\n");
        return 0;
    }
    
    // Initialize Vosk with found model path
    g_vosk_model = vosk_model_new(model_path);
    if (!g_vosk_model) {
        fprintf(stderr, "Could not load model from %s\n", model_path);
        fprintf(stderr, "Please ensure the model directory contains the required files\n");
        return 0;
    }
    
    printf("Speech recognition model loaded successfully from: %s\n", model_path);
    return 1;
}

void cleanup_vosk_model(void) {
    if (g_vosk_model) {
        vosk_model_free(g_vosk_model);
        g_vosk_model = NULL;
    }
}

void text_to_speech(const char *text) {
    char escaped_text[1024];
    char command[2048];
    
    // Escape special characters in the text
    escape_text_for_festival(text, escaped_text, sizeof(escaped_text));
    
    // Use Festival's --tts mode which is more reliable
    snprintf(command, sizeof(command), "echo \"%s\" | festival --tts", escaped_text);
    system(command);
}

const char* speech_to_text(void) {
    VoskRecognizer *recognizer;
    size_t nsamps;
    int16_t *audio_buffer;

    // Clear previous result
    g_last_recognized_text[0] = '\0';

    // Create recognizer with improved settings
    recognizer = vosk_recognizer_new(g_vosk_model, SAMPLE_RATE);
    if (!recognizer) {
        fprintf(stderr, "Could not create recognizer\n");
        return NULL;
    }

    // Enable words with times
    vosk_recognizer_set_words(recognizer, 1);

    printf("\nRecording... Speak clearly.\n");

    // Record audio using ALSA
    audio_buffer = record_audio(&nsamps);
    if (!audio_buffer) {
        fprintf(stderr, "Failed to record audio\n");
        vosk_recognizer_free(recognizer);
        return NULL;
    }

    printf("Processing speech...\n");

    // Process the recorded audio in smaller chunks for better accuracy
    const size_t chunk_size = SAMPLE_RATE / 2; // Process 0.5 second chunks
    size_t processed = 0;

    while (processed < nsamps) {
        size_t current_chunk = (nsamps - processed < chunk_size) ? 
                             (nsamps - processed) : chunk_size;
        
        vosk_recognizer_accept_waveform(recognizer, 
                                      (const char *)(audio_buffer + processed),
                                      current_chunk * sizeof(int16_t));
        processed += current_chunk;
    }

    // Get and show final result
    const char *final_result = vosk_recognizer_final_result(recognizer);
    
    // Simple parsing to extract just the text from JSON
    const char *text_start = strstr(final_result, "\"text\" : \"");
    if (text_start) {
        text_start += 10; // Skip past "\"text\" : \""
        const char *text_end = strchr(text_start, '\"');
        if (text_end && text_end > text_start) {
            size_t text_len = text_end - text_start;
            if (text_len > 0) {
                // Store the recognized text
                size_t copy_len = text_len < MAX_TEXT_LENGTH - 1 ? text_len : MAX_TEXT_LENGTH - 1;
                strncpy(g_last_recognized_text, text_start, copy_len);
                g_last_recognized_text[copy_len] = '\0';
            }
        }
    }

    // Cleanup
    free(audio_buffer);
    vosk_recognizer_free(recognizer);

    return g_last_recognized_text[0] != '\0' ? g_last_recognized_text : NULL;
} 
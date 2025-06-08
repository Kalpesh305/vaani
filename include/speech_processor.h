#ifndef SPEECH_PROCESSOR_H
#define SPEECH_PROCESSOR_H

#include <stddef.h>
#include <stdint.h>
#include <vosk_api.h>

// Constants
#define SAMPLE_RATE 16000
#define RECORDING_TIME_SEC 5
#define AUDIO_DEVICE "plughw:3,0"
#define FRAME_SIZE 8000
#define BUFFER_SIZE (SAMPLE_RATE * RECORDING_TIME_SEC)
#define SILENCE_THRESHOLD 500
#define MAX_TEXT_LENGTH 1024

// TTS Voice Configuration
// Available female voices (recommended for clarity):
// - "voice_rab_diphone" : Rachel voice (clear, natural)
// - "voice_kal_diphone" : Kal voice (clear female)
// - "voice_us2_mbrola"  : US2 MBROLA (high quality female)
// - "voice_us3_mbrola"  : US3 MBROLA (alternative female)
// - "voice_cmu_us_slt_arctic_hts" : CMU SLT Arctic (very clear, natural female)
#define TTS_VOICE "voice_cmu_us_slt_arctic_hts"

// Global Vosk model
extern VoskModel *g_vosk_model;

// Function declarations for model initialization
int initialize_vosk_model(void);
void cleanup_vosk_model(void);

// Function declarations for text-to-speech
void text_to_speech(const char *text);

// Function declarations for speech-to-text
// Returns the recognized text or NULL if recognition failed
const char* speech_to_text(void);

// Function declarations for audio processing
int16_t *record_audio(size_t *out_nsamps);
void normalize_audio(int16_t *buffer, size_t samples);
void remove_dc_offset(int16_t *buffer, size_t samples);
int is_silence(const int16_t *buffer, size_t samples);

// Menu functions
void clear_input_buffer(void);
void show_menu(void);

#endif // SPEECH_PROCESSOR_H 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <alsa/asoundlib.h>
#include "../../include/speech_processor.h"

// Function to find USB audio device automatically
char* find_usb_audio_device(void) {
    FILE *cards_file;
    char line[256];
    int card_num = -1;
    static char device_name[32];
    
    // Read /proc/asound/cards to find USB audio devices
    cards_file = fopen("/proc/asound/cards", "r");
    if (!cards_file) {
        fprintf(stderr, "Cannot open /proc/asound/cards\n");
        return NULL;
    }
    
    printf("Searching for USB audio devices...\n");
    
    while (fgets(line, sizeof(line), cards_file)) {
        // Look for lines containing card numbers and USB-Audio driver
        if (strstr(line, "USB-Audio") != NULL) {
            // Extract card number from the beginning of the line
            if (sscanf(line, " %d [", &card_num) == 1) {
                fclose(cards_file);
                snprintf(device_name, sizeof(device_name), "plughw:%d,0", card_num);
                printf("Found USB audio device: %s\n", device_name);
                return device_name;
            }
        }
    }
    
    fclose(cards_file);
    
    // If no USB audio device found, try to find any capture device
    printf("No USB audio device found, searching for any capture device...\n");
    
    snd_ctl_t *handle;
    snd_ctl_card_info_t *info;
    snd_pcm_info_t *pcminfo;
    char card_id[32];
    
    snd_ctl_card_info_alloca(&info);
    snd_pcm_info_alloca(&pcminfo);
    
    // Try each card
    for (int card = 0; card < 8; card++) {
        snprintf(card_id, sizeof(card_id), "hw:%d", card);
        
        if (snd_ctl_open(&handle, card_id, 0) < 0) {
            continue;
        }
        
        if (snd_ctl_card_info(handle, info) < 0) {
            snd_ctl_close(handle);
            continue;
        }
        
        // Check if card has capture capability
        int device = 0;
        snd_pcm_info_set_device(pcminfo, device);
        snd_pcm_info_set_subdevice(pcminfo, 0);
        snd_pcm_info_set_stream(pcminfo, SND_PCM_STREAM_CAPTURE);
        
        if (snd_ctl_pcm_info(handle, pcminfo) >= 0) {
            snd_ctl_close(handle);
            snprintf(device_name, sizeof(device_name), "plughw:%d,0", card);
            printf("Found capture device: %s (%s)\n", device_name, snd_ctl_card_info_get_name(info));
            return device_name;
        }
        
        snd_ctl_close(handle);
    }
    
    fprintf(stderr, "No suitable audio capture device found\n");
    return NULL;
}

void normalize_audio(int16_t *buffer, size_t samples) {
    // Find maximum amplitude
    int16_t max_amp = 0;
    for (size_t i = 0; i < samples; i++) {
        int16_t abs_amp = abs(buffer[i]);
        if (abs_amp > max_amp) {
            max_amp = abs_amp;
        }
    }

    // Normalize if the maximum amplitude is too low
    if (max_amp > 0 && max_amp < 16384) {  // 16384 is half of INT16_MAX
        float scale = 16384.0f / max_amp;
        for (size_t i = 0; i < samples; i++) {
            buffer[i] = (int16_t)(buffer[i] * scale);
        }
    }
}

void remove_dc_offset(int16_t *buffer, size_t samples) {
    // Calculate mean (DC offset)
    long long sum = 0;
    for (size_t i = 0; i < samples; i++) {
        sum += buffer[i];
    }
    int16_t dc_offset = (int16_t)(sum / samples);

    // Remove DC offset
    for (size_t i = 0; i < samples; i++) {
        buffer[i] -= dc_offset;
    }
}

int is_silence(const int16_t *buffer, size_t samples) {
    long sum = 0;
    for (size_t i = 0; i < samples; i++) {
        sum += abs(buffer[i]);
    }
    return (sum / samples) < SILENCE_THRESHOLD;
}

int16_t *record_audio(size_t *out_nsamps) {
    snd_pcm_t *capture_handle;
    snd_pcm_hw_params_t *hw_params;
    int err;
    size_t nsamples = BUFFER_SIZE;
    int16_t *buffer = malloc(nsamples * sizeof(int16_t));
    
    if (!buffer) {
        fprintf(stderr, "Failed to allocate memory for audio buffer\n");
        return NULL;
    }

    // Automatically find USB audio device
    char* audio_device = find_usb_audio_device();
    if (!audio_device) {
        fprintf(stderr, "No suitable audio device found\n");
        free(buffer);
        return NULL;
    }

    // Open the audio device
    if ((err = snd_pcm_open(&capture_handle, audio_device, SND_PCM_STREAM_CAPTURE, 0)) < 0) {
        fprintf(stderr, "Cannot open audio device %s: %s\n", audio_device, snd_strerror(err));
        free(buffer);
        return NULL;
    }

    // Allocate hardware parameters object
    snd_pcm_hw_params_alloca(&hw_params);

    // Fill with default values
    if ((err = snd_pcm_hw_params_any(capture_handle, hw_params)) < 0) {
        fprintf(stderr, "Cannot initialize hardware parameter structure: %s\n", snd_strerror(err));
        goto cleanup;
    }

    // Set access type
    if ((err = snd_pcm_hw_params_set_access(capture_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
        fprintf(stderr, "Cannot set access type: %s\n", snd_strerror(err));
        goto cleanup;
    }

    // Set sample format
    if ((err = snd_pcm_hw_params_set_format(capture_handle, hw_params, SND_PCM_FORMAT_S16_LE)) < 0) {
        fprintf(stderr, "Cannot set sample format: %s\n", snd_strerror(err));
        goto cleanup;
    }

    // Set sample rate
    unsigned int actual_rate = SAMPLE_RATE;
    if ((err = snd_pcm_hw_params_set_rate_near(capture_handle, hw_params, &actual_rate, 0)) < 0) {
        fprintf(stderr, "Cannot set sample rate: %s\n", snd_strerror(err));
        goto cleanup;
    }

    // Set channels
    if ((err = snd_pcm_hw_params_set_channels(capture_handle, hw_params, 1)) < 0) {
        fprintf(stderr, "Cannot set channel count: %s\n", snd_strerror(err));
        goto cleanup;
    }

    // Set buffer size
    snd_pcm_uframes_t buffer_size = FRAME_SIZE;
    if ((err = snd_pcm_hw_params_set_buffer_size_near(capture_handle, hw_params, &buffer_size)) < 0) {
        fprintf(stderr, "Cannot set buffer size: %s\n", snd_strerror(err));
        goto cleanup;
    }

    // Apply hardware parameters
    if ((err = snd_pcm_hw_params(capture_handle, hw_params)) < 0) {
        fprintf(stderr, "Cannot set parameters: %s\n", snd_strerror(err));
        goto cleanup;
    }

    // Start recording
    if ((err = snd_pcm_prepare(capture_handle)) < 0) {
        fprintf(stderr, "Cannot prepare audio interface: %s\n", snd_strerror(err));
        goto cleanup;
    }

    printf("Starting to record...\n");
    
    // Read samples
    size_t frames_read = 0;
    int silence_count = 0;
    while (frames_read < nsamples) {
        snd_pcm_sframes_t rc = snd_pcm_readi(capture_handle, buffer + frames_read, 
                                            nsamples - frames_read);
        if (rc < 0) {
            if (rc == -EPIPE) {
                // Handle buffer overrun
                snd_pcm_prepare(capture_handle);
                continue;
            }
            fprintf(stderr, "Read error: %s\n", snd_strerror(rc));
            goto cleanup;
        }
        
        // Check for silence in the current chunk
        if (is_silence(buffer + frames_read, rc)) {
            silence_count++;
            if (silence_count > 3) { // More than 3 consecutive silent chunks
                printf("Detected extended silence, stopping recording...\n");
                nsamples = frames_read;
                break;
            }
        } else {
            silence_count = 0;
        }
        
        frames_read += rc;
    }

    // Process the recorded audio
    remove_dc_offset(buffer, frames_read);
    normalize_audio(buffer, frames_read);

    *out_nsamps = frames_read;
    snd_pcm_close(capture_handle);
    return buffer;

cleanup:
    snd_pcm_close(capture_handle);
    free(buffer);
    return NULL;
} 
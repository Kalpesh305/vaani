#include <stdio.h>
#include <stdlib.h>
#include <alsa/asoundlib.h>
#include "../../include/speech_processor.h"

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

    // Open the audio device
    if ((err = snd_pcm_open(&capture_handle, AUDIO_DEVICE, SND_PCM_STREAM_CAPTURE, 0)) < 0) {
        fprintf(stderr, "Cannot open audio device %s: %s\n", AUDIO_DEVICE, snd_strerror(err));
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
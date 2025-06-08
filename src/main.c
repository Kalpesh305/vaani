#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "../include/speech_processor.h"
#include "../include/intent_processor.h"

void clear_input_buffer(void) {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

void show_menu(void) {
    printf("\n=== Vaani Speech Processing Menu ===\n");
    printf("1. Ask a Question (Speech Q&A)\n");
    printf("2. Speech to Text (STT only)\n");
    printf("3. Text to Speech (TTS only)\n");
    printf("4. Exit\n");
    printf("Enter your choice (1-4): ");
}

int main(void) {
    int choice = 1;
    char text_input[MAX_TEXT_LENGTH];
    
    // Initialize Vosk model at program start
    if (!initialize_vosk_model()) {
        fprintf(stderr, "Failed to initialize speech recognition. Exiting.\n");
        return 1;
    }

    // Initialize intent processor
    if (!initialize_intent_processor()) {
        fprintf(stderr, "Failed to initialize intent processor. Exiting.\n");
        cleanup_vosk_model();
        return 1;
    }
    text_to_speech("device has been started");
    sleep(3);
    while (1) {
        // show_menu();
        
        // if (scanf("%d", &choice) != 1) {
        //     printf("Invalid input. Please enter a number.\n");
        //     clear_input_buffer();
        //     continue;
        // }
        // clear_input_buffer();
        
        switch (choice) {
            case 1: {
                printf("\n=== Ask a Question Mode ===\n");
                printf("Speak your question clearly when recording starts...\n");
                text_to_speech("Please ask your question");
                const char* recognized_text = speech_to_text();
                if (recognized_text && strlen(recognized_text) > 0) {
                    printf("\nYour question: %s\n", recognized_text);
                    
                    // Try to find a matching answer
                    const char* answer = find_matching_answer(recognized_text);
                    if (answer) {
                        printf("Found answer! Speaking response...\n");
                        text_to_speech(answer);
                    } else {
                        text_to_speech("I did not get it please ask again");
                        printf("Sorry, I don't have an answer for that question.\n");
                        printf("Please try asking something about road safety, traffic rules, or emergency procedures.\n");
                    }
                }
                else
                {
                    text_to_speech("I did not get it please ask again");
                }
                break;
            }
            
            case 2: {
                printf("\n=== Speech to Text Mode ===\n");
                const char* recognized_text = speech_to_text();
                if (recognized_text && strlen(recognized_text) > 0) {
                    printf("\nRecognized text: %s\n", recognized_text);
                }
                break;
            }
                
            case 3:
                printf("\n=== Text to Speech Mode ===\n");
                printf("Enter text to speak (max %d characters):\n", MAX_TEXT_LENGTH - 1);
                if (fgets(text_input, MAX_TEXT_LENGTH, stdin) != NULL) {
                    // Remove trailing newline if present
                    size_t len = strlen(text_input);
                    if (len > 0 && text_input[len-1] == '\n') {
                        text_input[len-1] = '\0';
                    }
                    if (strlen(text_input) > 0) {
                        text_to_speech(text_input);
                    }
                }
                break;
                
            case 4:
                printf("\nExiting program. Goodbye!\n");
                cleanup_vosk_model();
                cleanup_intent_processor();
                return 0;
                
            default:
                printf("\nInvalid choice. Please select 1-4.\n");
                break;
        }
        
        // printf("\nPress Enter to continue...");
        // getchar();
        sleep(2);
    }

    return 0;
} 
#ifndef INTENT_PROCESSOR_H
#define INTENT_PROCESSOR_H

#include <stdbool.h>

#define MAX_QUESTION_LENGTH 1024
#define MAX_ANSWER_LENGTH 2048
#define MAX_INTENT_LENGTH 256

// Structure to hold a single intent entry
typedef struct {
    char question[MAX_QUESTION_LENGTH];
    char answer[MAX_ANSWER_LENGTH];
    char intent[MAX_INTENT_LENGTH];
} IntentEntry;

// Initialize the intent processor with data from CSV
bool initialize_intent_processor(void);

// Clean up intent processor resources
void cleanup_intent_processor(void);

// Find matching answer for a given text
// Returns NULL if no match found
const char* find_matching_answer(const char* text);

#endif // INTENT_PROCESSOR_H 
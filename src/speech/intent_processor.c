#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <math.h>
#include "../../include/intent_processor.h"

#define MAX_INTENTS 300
#define SIMILARITY_THRESHOLD 0.7  // 70% similarity threshold
#define SIMILARITY_THRESHOLD_MIN 0.5  // 50% similarity threshold
#define MIN_SIMILARITY_TO_SHOW 0.3 // Show matches above 30% for debugging
#define MAX_VOCABULARY_SIZE 2000   // Maximum unique words in corpus
#define MAX_WORDS_PER_QUESTION 50  // Maximum words per question

static IntentEntry* g_intents = NULL;
static size_t g_intent_count = 0;

// Vocabulary structure for TF-IDF
typedef struct {
    char word[64];
    int document_frequency;  // Number of documents containing this word
} VocabularyEntry;

// TF-IDF vector structure
typedef struct {
    float* tfidf_values;    // TF-IDF values for each word in vocabulary
    int num_words;          // Number of words in this document
} TFIDFVector;

// Global vocabulary and TF-IDF data
static VocabularyEntry* g_vocabulary = NULL;
static size_t g_vocabulary_size = 0;
static TFIDFVector* g_question_vectors = NULL;

// Helper function to trim whitespace
static char* trim(char* str) {
    char* end;
    while(isspace((unsigned char)*str)) str++;
    if(*str == 0) return str;
    end = str + strlen(str) - 1;
    while(end > str && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
    return str;
}

// Helper function to convert text to lowercase for comparison
static void to_lower(char* str) {
    for(int i = 0; str[i]; i++) {
        str[i] = tolower(str[i]);
    }
}

// Helper function to normalize text and remove stopwords
static bool is_stopword(const char* word) {
    const char* stopwords[] = {
        "a", "an", "and", "are", "as", "at", "be", "by", "for", "from",
        "has", "he", "in", "is", "it", "its", "of", "on", "that", "the",
        "to", "was", "will", "with", "what", "how", "when", "where", "why",
        "should", "would", "could", "can", "do", "does", "did", "have", 
        "had", "you", "your", "we", "us", "our", "i", "me", "my", NULL
    };
    
    for (const char** stop = stopwords; *stop != NULL; stop++) {
        if (strcmp(word, *stop) == 0) {
            return true;
        }
    }
    return false;
}

// Helper function to tokenize text into words (removes punctuation and stopwords)
static int tokenize_text(const char* text, char words[][64], int max_words) {
    char* temp = strdup(text);
    char* orig_temp = temp;
    int word_count = 0;
    
    // Convert to lowercase
    to_lower(temp);
    
    // Tokenize by spaces and punctuation
    char* token = strtok(temp, " \t\n.,?!;:\"'()[]{}/-");
    while (token && word_count < max_words) {
        // Skip very short words and stopwords
        if (strlen(token) > 1 && !is_stopword(token)) {
            strncpy(words[word_count], token, 63);
            words[word_count][63] = '\0';
            word_count++;
        }
        token = strtok(NULL, " \t\n.,?!;:\"'()[]{}/-");
    }
    
    free(orig_temp);
    return word_count;
}

// Build vocabulary from all questions
static void build_vocabulary(void) {
    g_vocabulary = malloc(sizeof(VocabularyEntry) * MAX_VOCABULARY_SIZE);
    g_vocabulary_size = 0;
    
    // Process each question to build vocabulary
    for (size_t i = 0; i < g_intent_count; i++) {
        char words[MAX_WORDS_PER_QUESTION][64];
        int word_count = tokenize_text(g_intents[i].question, words, MAX_WORDS_PER_QUESTION);
        
        // For each word in this question
        for (int j = 0; j < word_count; j++) {
            // Check if word already exists in vocabulary
            bool found = false;
            for (size_t k = 0; k < g_vocabulary_size; k++) {
                if (strcmp(g_vocabulary[k].word, words[j]) == 0) {
                    // Word exists, increment document frequency
                    found = true;
                    break;
                }
            }
            
            // If word doesn't exist and we have space, add it
            if (!found && g_vocabulary_size < MAX_VOCABULARY_SIZE) {
                strncpy(g_vocabulary[g_vocabulary_size].word, words[j], 63);
                g_vocabulary[g_vocabulary_size].word[63] = '\0';
                g_vocabulary[g_vocabulary_size].document_frequency = 0;
                g_vocabulary_size++;
            }
        }
    }
    
    // Count document frequencies
    for (size_t i = 0; i < g_intent_count; i++) {
        char words[MAX_WORDS_PER_QUESTION][64];
        int word_count = tokenize_text(g_intents[i].question, words, MAX_WORDS_PER_QUESTION);
        
        // Mark which vocabulary words appear in this document
        bool word_seen[MAX_VOCABULARY_SIZE] = {false};
        
        for (int j = 0; j < word_count; j++) {
            for (size_t k = 0; k < g_vocabulary_size; k++) {
                if (strcmp(g_vocabulary[k].word, words[j]) == 0 && !word_seen[k]) {
                    g_vocabulary[k].document_frequency++;
                    word_seen[k] = true;
                    break;
                }
            }
        }
    }
    
    printf("Built vocabulary with %zu unique words\n", g_vocabulary_size);
}

// Calculate TF-IDF vector for a given text
static TFIDFVector calculate_tfidf_vector(const char* text) {
    TFIDFVector vector;
    vector.tfidf_values = calloc(g_vocabulary_size, sizeof(float));
    vector.num_words = 0;
    
    char words[MAX_WORDS_PER_QUESTION][64];
    int word_count = tokenize_text(text, words, MAX_WORDS_PER_QUESTION);
    vector.num_words = word_count;
    
    if (word_count == 0) {
        return vector;
    }
    
    // Count term frequencies in this document
    int term_frequencies[MAX_VOCABULARY_SIZE] = {0};
    
    for (int i = 0; i < word_count; i++) {
        for (size_t j = 0; j < g_vocabulary_size; j++) {
            if (strcmp(g_vocabulary[j].word, words[i]) == 0) {
                term_frequencies[j]++;
                break;
            }
        }
    }
    
    // Calculate TF-IDF values
    for (size_t i = 0; i < g_vocabulary_size; i++) {
        if (term_frequencies[i] > 0) {
            // TF = (frequency of term in document) / (total number of terms in document)
            float tf = (float)term_frequencies[i] / (float)word_count;
            
            // IDF = log(total number of documents / number of documents containing term)
            float idf = logf((float)g_intent_count / (float)g_vocabulary[i].document_frequency);
            
            // TF-IDF = TF * IDF
            vector.tfidf_values[i] = tf * idf;
        }
    }
    
    return vector;
}

// Calculate cosine similarity between two TF-IDF vectors
static float calculate_cosine_similarity(const TFIDFVector* vec1, const TFIDFVector* vec2) {
    float dot_product = 0.0f;
    float magnitude1 = 0.0f;
    float magnitude2 = 0.0f;
    
    // Calculate dot product and magnitudes
    for (size_t i = 0; i < g_vocabulary_size; i++) {
        dot_product += vec1->tfidf_values[i] * vec2->tfidf_values[i];
        magnitude1 += vec1->tfidf_values[i] * vec1->tfidf_values[i];
        magnitude2 += vec2->tfidf_values[i] * vec2->tfidf_values[i];
    }
    
    magnitude1 = sqrtf(magnitude1);
    magnitude2 = sqrtf(magnitude2);
    
    // Avoid division by zero
    if (magnitude1 == 0.0f || magnitude2 == 0.0f) {
        return 0.0f;
    }
    
    return dot_product / (magnitude1 * magnitude2);
}

// Precompute TF-IDF vectors for all questions
static void precompute_question_vectors(void) {
    g_question_vectors = malloc(sizeof(TFIDFVector) * g_intent_count);
    
    for (size_t i = 0; i < g_intent_count; i++) {
        g_question_vectors[i] = calculate_tfidf_vector(g_intents[i].question);
    }
    
    printf("Precomputed TF-IDF vectors for %zu questions\n", g_intent_count);
}

// New similarity function using Cosine similarity with TF-IDF
static float calculate_similarity(const char* str1, const char* str2) {
    // Check for exact string match first (case-insensitive)
    char* temp1 = strdup(str1);
    char* temp2 = strdup(str2);
    to_lower(temp1);
    to_lower(temp2);
    
    bool exact_match = (strcmp(temp1, temp2) == 0);
    free(temp1);
    free(temp2);
    
    if (exact_match) {
        return 1.0f; // 100% match for exact strings
    }
    
    // Calculate TF-IDF vector for input text
    TFIDFVector input_vector = calculate_tfidf_vector(str1);
    
    // Find the corresponding question vector
    TFIDFVector question_vector;
    question_vector.tfidf_values = NULL;
    question_vector.num_words = 0;
    
    // Find which question we're comparing against
    for (size_t i = 0; i < g_intent_count; i++) {
        if (strcmp(g_intents[i].question, str2) == 0) {
            question_vector = g_question_vectors[i];
            break;
        }
    }
    
    // If we couldn't find precomputed vector, calculate it
    if (question_vector.tfidf_values == NULL) {
        question_vector = calculate_tfidf_vector(str2);
    }
    
    // Calculate cosine similarity
    float similarity = calculate_cosine_similarity(&input_vector, &question_vector);
    
    // Clean up input vector
    if (input_vector.tfidf_values) {
        free(input_vector.tfidf_values);
    }
    
    // If we calculated question vector on the fly, clean it up
    if (question_vector.tfidf_values != NULL) {
        bool is_precomputed = false;
        for (size_t i = 0; i < g_intent_count; i++) {
            if (question_vector.tfidf_values == g_question_vectors[i].tfidf_values) {
                is_precomputed = true;
                break;
            }
        }
        if (!is_precomputed) {
            free(question_vector.tfidf_values);
        }
    }
    
    return similarity;
}

// Helper function to parse a CSV line properly handling quoted fields
static bool parse_csv_line(char* line, char* fields[], int max_fields, int* num_fields) {
    enum State { FIELD_START, IN_FIELD, IN_QUOTED_FIELD, QUOTE_IN_QUOTED_FIELD } state = FIELD_START;
    char* p = line;
    char* field_start = p;
    *num_fields = 0;
    
    while (*p && *num_fields < max_fields) {
        switch (state) {
            case FIELD_START:
                if (*p == '"') {
                    field_start = p + 1;
                    state = IN_QUOTED_FIELD;
                } else if (*p == ',') {
                    *p = '\0';
                    fields[(*num_fields)++] = field_start;
                    field_start = p + 1;
                } else {
                    state = IN_FIELD;
                }
                break;
                
            case IN_FIELD:
                if (*p == ',') {
                    *p = '\0';
                    fields[(*num_fields)++] = field_start;
                    field_start = p + 1;
                    state = FIELD_START;
                }
                break;
                
            case IN_QUOTED_FIELD:
                if (*p == '"') {
                    state = QUOTE_IN_QUOTED_FIELD;
                }
                break;
                
            case QUOTE_IN_QUOTED_FIELD:
                if (*p == '"') {
                    // Double quote - copy one quote and stay in quoted field
                    memmove(p - 1, p, strlen(p) + 1);
                    state = IN_QUOTED_FIELD;
                } else if (*p == ',') {
                    *(p - 1) = '\0';  // End at the quote
                    fields[(*num_fields)++] = field_start;
                    field_start = p + 1;
                    state = FIELD_START;
                } else {
                    // Invalid format - treat rest as part of field
                    state = IN_FIELD;
                }
                break;
        }
        p++;
    }
    
    // Handle last field
    if (field_start < p) {
        if (state == QUOTE_IN_QUOTED_FIELD) {
            *(p - 1) = '\0';  // Remove last quote
        }
        fields[(*num_fields)++] = field_start;
    }
    
    return *num_fields > 0;
}

bool initialize_intent_processor(void) {
    FILE* file = fopen("data/Intents.csv", "r");
    if (!file) {
        fprintf(stderr, "Failed to open Intents.csv\n");
        return false;
    }

    // Allocate memory for intents
    g_intents = malloc(sizeof(IntentEntry) * MAX_INTENTS);
    if (!g_intents) {
        fclose(file);
        return false;
    }

    char line[MAX_QUESTION_LENGTH + MAX_ANSWER_LENGTH + MAX_INTENT_LENGTH + 3];
    char* fields[3];  // For question, answer, and intent
    int num_fields;
    
    // Skip header line
    fgets(line, sizeof(line), file);
    
    // Read entries
    while (fgets(line, sizeof(line), file) && g_intent_count < MAX_INTENTS) {
        // Remove newline if present
        size_t len = strlen(line);
        if (len > 0 && line[len-1] == '\n') {
            line[len-1] = '\0';
        }
        
        if (parse_csv_line(line, fields, 3, &num_fields) && num_fields == 3) {
            strncpy(g_intents[g_intent_count].question, trim(fields[0]), MAX_QUESTION_LENGTH - 1);
            strncpy(g_intents[g_intent_count].answer, trim(fields[1]), MAX_ANSWER_LENGTH - 1);
            strncpy(g_intents[g_intent_count].intent, trim(fields[2]), MAX_INTENT_LENGTH - 1);
            
            // Note: We don't convert to lowercase here since TF-IDF handles case normalization
            g_intent_count++;
        }
    }

    fclose(file);
    
    // Build vocabulary and precompute TF-IDF vectors
    printf("Initializing Cosine similarity with TF-IDF...\n");
    build_vocabulary();
    precompute_question_vectors();
    printf("Intent processor initialized with %zu questions\n", g_intent_count);
    
    return true;
}

void cleanup_intent_processor(void) {
    if (g_intents) {
        free(g_intents);
        g_intents = NULL;
    }
    
    if (g_vocabulary) {
        free(g_vocabulary);
        g_vocabulary = NULL;
    }
    
    if (g_question_vectors) {
        for (size_t i = 0; i < g_intent_count; i++) {
            if (g_question_vectors[i].tfidf_values) {
                free(g_question_vectors[i].tfidf_values);
            }
        }
        free(g_question_vectors);
        g_question_vectors = NULL;
    }
    
    g_intent_count = 0;
    g_vocabulary_size = 0;
}

const char* find_matching_answer(const char* text) {
    if (!text || !g_intents) return NULL;
    
    float best_similarity = 0;
    size_t best_match_index = 0;
    bool found_match = false;
    bool found_exact_match = false;
    
    printf("\nAnalyzing matches using Cosine similarity...\n");
    printf("Your question: \"%s\"\n", text);
    printf("\nTop matching questions:\n");
    printf("----------------------------------------\n");
    
    // Store top 3 matches
    struct {
        float similarity;
        size_t index;
    } top_matches[3] = {{0,0}, {0,0}, {0,0}};
    
    // First pass: look for exact matches (ignoring case)
    char* lower_text = strdup(text);
    to_lower(lower_text);
    
    for (size_t i = 0; i < g_intent_count; i++) {
        char* lower_question = strdup(g_intents[i].question);
        to_lower(lower_question);
        
        if (strcmp(lower_text, lower_question) == 0) {
            best_similarity = 1.0;
            best_match_index = i;
            found_match = true;
            found_exact_match = true;
            free(lower_question);
            break;
        }
        free(lower_question);
    }
    
    free(lower_text);
    
    // If no exact match, do cosine similarity matching
    if (!found_exact_match) {
        for (size_t i = 0; i < g_intent_count; i++) {
            float similarity = calculate_similarity(text, g_intents[i].question);
            
            // Update best overall match
            if (similarity > best_similarity) {
                best_similarity = similarity;
                best_match_index = i;
                found_match = true;
            }

            // Update top 3 matches
            for (int j = 0; j < 3; j++) {
                if (similarity > top_matches[j].similarity) {
                    // Shift lower matches down
                    for (int k = 2; k > j; k--) {
                        top_matches[k] = top_matches[k-1];
                    }
                    top_matches[j].similarity = similarity;
                    top_matches[j].index = i;
                    break;
                }
            }
        }
    }
    
    // Show top 3 matches if they're above minimum threshold
    for (int i = 0; i < 3; i++) {
        if (top_matches[i].similarity >= MIN_SIMILARITY_TO_SHOW) {
            printf("%d. \"%s\"\n", i+1, g_intents[top_matches[i].index].question);
            printf("   Cosine Similarity: %.1f%%\n", top_matches[i].similarity * 100);
        }
    }
    printf("----------------------------------------\n");
    
    // Return answer if similarity is above threshold or exact match
    if (found_exact_match) {
        printf("\nExact match found! (100%% confidence)\n");
        return g_intents[best_match_index].answer;
    }
    
    if (found_match && best_similarity >= SIMILARITY_THRESHOLD_MIN) {
        printf("\nFound matching answer! (%.1f%% cosine similarity)\n", best_similarity * 100);
        return g_intents[best_match_index].answer;
    }
    
    printf("\nNo answer found - required similarity: %.1f%%, best match: %.1f%%\n", 
           SIMILARITY_THRESHOLD_MIN * 100, best_similarity * 100);
    return NULL;
} 
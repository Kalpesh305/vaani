CUR_DIR = $(shell pwd)
VOSK_DIR = $(CUR_DIR)/vosk-linux-aarch64-0.3.45

CC = gcc
CFLAGS = -Wall -Wextra -I./include -I./ -I$(VOSK_DIR)
LDFLAGS = -L$(VOSK_DIR) -Wl,-rpath=$(VOSK_DIR) -lasound -lvosk -lm

SRC_DIR = src
BUILD_DIR = build
DIRS = $(BUILD_DIR) $(BUILD_DIR)/audio $(BUILD_DIR)/speech

SRCS = $(SRC_DIR)/main.c \
       $(SRC_DIR)/audio/audio_processor.c \
       $(SRC_DIR)/speech/speech_processor.c \
       $(SRC_DIR)/speech/intent_processor.c

OBJS = $(SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)
TARGET = vaani

.PHONY: all clean run

all: $(DIRS) $(TARGET)

$(DIRS):
	mkdir -p $@

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR) $(TARGET)

run: $(TARGET)
	./$(TARGET)
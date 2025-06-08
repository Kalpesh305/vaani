CC = gcc
CFLAGS = -Wall -Wextra -I./include
LDFLAGS = -lasound -lvosk -lm

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
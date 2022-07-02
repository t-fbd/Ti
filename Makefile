CC = gcc

CFLAGS = -Wall -Wextra -pedantic -std=c99

TARGET = kilot

all: $(TARGET)

$(TARGET): $(TARGET).c
	$(CC) $(TARGET).c -o $(TARGET) $(CFLAGS)

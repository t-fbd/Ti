CC = gcc

CFLAGS = -Wall -Wextra -pedantic -std=c99

TARGET = kilot

kilot: $(TARGET).c
	$(CC) $(TARGET).c -o $(TARGET) $(CFLAGS)

CC = gcc
CFLAGS = -g -Wall -Isrc/tokens -Isrc/scanner
TARGET = compilador
SRC_DIR = src
SCAN_DIR = src/scanner
SOURCES = $(SRC_DIR)/main.c $(SCAN_DIR)/scanner.c
OBJECTS = $(SOURCES:.c=.o)
DEPS = $(SRC_DIR)/scanner/scanner.h $(SRC_DIR)/tokens/tokens.h

INPUT = teste.cmm

all: $(TARGET)
	./$(TARGET) $(INPUT)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(TARGET)

%.o: %.c $(DEPS)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	del /Q src\main.o src\scanner\scanner.o compilador.exe

.PHONY: all clean

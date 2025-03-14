# Compiler and flags
CC = gcc
CFLAGS = 
LDFLAGS = -lm -pthread

# Target executable
TARGET = prod-cons

# Default rule
all: $(TARGET)

# Compile the program
$(TARGET): prod-cons.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Run the program with specified p and q values
run: $(TARGET)
	./$(TARGET) $(p) $(q)

# Clean up
clean:
	rm -f $(TARGET)

.PHONY: all run clean
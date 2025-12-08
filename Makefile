# Compiler and flags
CC      = gcc
CFLAGS  = -Wall -Wextra -std=c11 -g

# Target binary name
TARGET  = shell

# Source files
SRC     = shell.c

# Default target
all: $(TARGET)

# Build rule
$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $@ $^

# Convenience target to run the shell
run: $(TARGET)
	./$(TARGET)

# Clean up build artifacts
clean:
	rm -f $(TARGET)

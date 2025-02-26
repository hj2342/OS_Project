# Compiler
CC = gcc
CFLAGS = -Wall -Wextra -g

# Source files
SRC = main.c command_parser.c executor.c

# Object files
OBJ = $(SRC:.c=.o)

# Executable name
TARGET = shell

# Default rule: build the executable
all: $(TARGET)

# Compile the executable
$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ)

# Compile source files into object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean object files and executable
clean:
	rm -f $(OBJ) $(TARGET)

# Run the shell
run: $(TARGET)
	./$(TARGET)
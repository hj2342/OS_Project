CC = gcc
CFLAGS = -Wall -Wextra -g -pthread
LDFLAGS = -pthread

# Source files
SERVER_SRCS = main.c command_parser.c client_handler.c queue.c scheduler.c executor.c
CLIENT_SRCS = client.c

# Object files
SERVER_OBJS = $(SRCS:.c=.o)
CLIENT_OBJS = client.o

# Executables
SERVER = server
CLIENT = client

all: $(SERVER) $(CLIENT)

# Server build
$(SERVER): $(SERVER_SRCS)
	$(CC) $(CFLAGS) -o $@ $(SERVER_SRCS) $(LDFLAGS)

# Client build
$(CLIENT): $(CLIENT_SRCS)
	$(CC) $(CFLAGS) -o $@ $(CLIENT_SRCS)

# Clean rule
clean:
	rm -f $(SERVER) $(CLIENT) *.o
	rm -rf *.dSYM

# Run rules
run_server: $(SERVER)
	./$(SERVER)

run_client: $(CLIENT)
	./$(CLIENT)

# Development helper rules
format:
	clang-format -i *.c *.h

valgrind: $(SERVER)
	valgrind --leak-check=full --show-leak-kinds=all ./$(SERVER)

.PHONY: all clean run_server run_client format valgrind

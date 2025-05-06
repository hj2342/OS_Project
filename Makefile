# Compiler flags
CC = gcc
CFLAGS = -Wall -Wextra -g -I.
LDFLAGS = -lpthread

# Targets
all: server client demo

server: main.o command_parser.o executor.o client_handler.o task.o queue.o task_executor.o scheduler.o gantt.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

client: client.o
	$(CC) $(CFLAGS) -o $@ $^

demo: demo.c
	$(CC) $(CFLAGS) -o $@ $<

# Object file rules
main.o: main.c client_handler.h scheduler.h
	$(CC) $(CFLAGS) -c $<

command_parser.o: command_parser.c command_parser.h
	$(CC) $(CFLAGS) -c $<

executor.o: executor.c executor.h command_parser.h
	$(CC) $(CFLAGS) -c $<

client_handler.o: client_handler.c client_handler.h scheduler.h
	$(CC) $(CFLAGS) -c $<

# New modular files
task.o: task.c task.h
	$(CC) $(CFLAGS) -c $<

queue.o: queue.c queue.h task.h
	$(CC) $(CFLAGS) -c $<

task_executor.o: task_executor.c task_executor.h task.h
	$(CC) $(CFLAGS) -c $<

scheduler.o: scheduler.c scheduler.h task.h queue.h task_executor.h client_handler.h
	$(CC) $(CFLAGS) -c $<

gantt.o: gantt.c gantt.h task.h
	$(CC) $(CFLAGS) -c $<

client.o: client.c
	$(CC) $(CFLAGS) -c $<

# Utility targets
clean:
	rm -f *.o server client demo

run_server:
	./server

run_client:
	./client

.PHONY: all clean run_server run_client

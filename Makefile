all: server client

server: main.o command_parser.o executor.o client_handler.o
	gcc -Wall -Wextra -g -o server main.o command_parser.o executor.o client_handler.o -lpthread


client: client.o
	gcc -Wall -Wextra -g -o client client.o

main.o: main.c
	gcc -Wall -Wextra -g -c main.c

command_parser.o: command_parser.c
	gcc -Wall -Wextra -g -c command_parser.c

executor.o: executor.c
	gcc -Wall -Wextra -g -c executor.c

client.o: client.c
	gcc -Wall -Wextra -g -c client.c

client_handler.o: client_handler.c client_handler.h
	gcc -Wall -Wextra -g -c client_handler.c


clean:
	rm -f *.o server client

run_server:
	./server

run_client:
	./client

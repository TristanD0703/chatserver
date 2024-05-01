chatserver: client server

flags = -pthread -g3 -O0 -Wall -Wcast-qual -Wdisabled-optimization  -Wmissing-include-dirs -Wfloat-equal -Wredundant-decls -Wshadow -Wsign-conversion -Wstrict-overflow=2 -Wswitch-default -Wundef -Waggregate-return -Wmissing-declarations -Wcast-align -Wbad-function-cast -Wno-shift-negative-value -Wmissing-prototypes -Wstrict-prototypes -Wnested-externs -Wold-style-definition

client: client.o
	gcc $(flags) -o client client.o

server: server.o
	gcc $(flags) -o server server.o

client.o: client.c
	gcc $(flags) -c client.c

server.o: server.c
	gcc $(flags) -c server.c



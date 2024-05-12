CC = gcc
CFLAGS = -Iinclude
DEPS = include/HTTP_Server.h include/Routes.h include/Response.h include/Code_Run_Lib.h include/REST_Api_Handler.h include/Logger.h
OBJ = src/main.o include/HTTP_Server.o include/Routes.o include/Response.o include/Code_Run_Lib.o include/REST_Api_Handler.o include/Logger.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

server: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

.PHONY: clean

clean:
	rm -f $(OBJ) server

start: server
	./server

run: clean server start

rerun: server start
CC = gcc
CFLAGS = -Iinclude -Ilibs
DEPS = include/HTTP_Server.h include/Routes.h include/Response.h include/Code_Run_Lib.h include/Logger.h \
libs/cJSON.h libs/thpool.h libs/sqlite3.h
OBJ = src/main.o include/HTTP_Server.o include/Routes.o include/Response.o include/Code_Run_Lib.o include/Logger.o \
libs/cJSON.o libs/thpool.o libs/sqlite3.o
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
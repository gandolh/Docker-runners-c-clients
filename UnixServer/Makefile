CC = gcc
CFLAGS = -Iinclude -Ilibs
DEPS =  include/Logger.h include/Database.h libs/cJSON.h libs/sqlite3.h
OBJ = src/main.o include/Logger.o include/Database.o libs/cJSON.o libs/sqlite3.o
%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

main: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

.PHONY: clean

clean:
	rm -f $(OBJ) main

start: main
	./main

run: clean main start

rerun: main start
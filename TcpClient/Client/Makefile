CC = gcc
CFLAGS = -Iinclude -Ilibs
DEPS =  include/Logger.h libs/cJSON.h
OBJ = src/main.o include/Logger.o libs/cJSON.o
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
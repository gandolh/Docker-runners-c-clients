CC = gcc
CFLAGS = -Iinclude -Ilibs
DEPS =
OBJ = src/main.o
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
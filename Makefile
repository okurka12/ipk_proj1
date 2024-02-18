RESULT_BINARY=ipk24chat-client
CC = gcc
CFLAGS =
LDFLAGS =

MODULES = udpcl.o

ALL: $(RESULT_BINARY)

udpcl.o: udpcl.c
	$(CC) $(CFLAGS) -c -o $@ $^

$(RESULT_BINARY): $(MODULES)
	$(CC) $(LDFLAGS) -o $@ $^

.PHONY: clean
clean:
	rm -rf *.o $(RESULT_BINARY)
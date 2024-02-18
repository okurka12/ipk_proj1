##################
##  Vit Pavlik  ##
##   xpavli0a   ##
##    251301    ##
##              ##
##  Created:    ##
##  2024-02-18  ##
##              ##
##  Edited:     ##
##  2024-02-18  ##
##################

RESULT_BINARY=ipk24chat-client
CC=gcc
CFLAGS=-Wall -Wextra -pedantic
LDFLAGS=

MODULES = udpcl.o

ALL: $(RESULT_BINARY)

udpcl.o: udpcl.c
	$(CC) $(CFLAGS) -c -o $@ $^

$(RESULT_BINARY): $(MODULES)
	$(CC) $(LDFLAGS) -o $@ $^

.PHONY: clean
clean:
	rm -rf *.o $(RESULT_BINARY)

##################
##  Vit Pavlik  ##
##   xpavli0a   ##
##    251301    ##
##              ##
##  Created:    ##
##  2024-02-18  ##
##              ##
##  Edited:     ##
##  2024-02-19  ##
##################

# log level (DEBUG, INFO, WARNING, ERROR, FATAL)
LOGLEVEL=DEBUG

# uncomment this to disable all logging
# DNDEBUG=-DNDEBUG

RESULT_BINARY=ipk24chat-client
CC=gcc
CFLAGS=-Wall -Wextra -pedantic -DLOGLEVEL=$(LOGLEVEL) $(DNDEBUG)
LDFLAGS=

MODULES = udpcl.o rwmsgid.o

ALL: $(RESULT_BINARY)

udpcl.o: udpcl.c udpcl.h ipk24chat.h utils.h rwmsgid.h
	$(CC) $(CFLAGS) -c -o $@ $<

rwmsgid.o: rwmsgid.c rwmsgid.h
	$(CC) $(CFLAGS) -c -o $@ $<

$(RESULT_BINARY): $(MODULES)
	$(CC) $(LDFLAGS) -o $@ $^

.PHONY: clean
clean:
	rm -rf *.o $(RESULT_BINARY)

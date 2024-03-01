##################
##  Vit Pavlik  ##
##   xpavli0a   ##
##    251301    ##
##              ##
##  Created:    ##
##  2024-02-18  ##
##              ##
##  Edited:     ##
##  2024-02-25  ##
##################

# log level (DEBUG, INFO, WARNING, ERROR, FATAL)
LOGLEVEL=DEBUG

# uncomment this to disable all logging
# DNDEBUG=-DNDEBUG

# uncomment this to enable adress sanitizer
# ASAN=-fsanitize=address

RESULT_BINARY=ipk24chat-client

# the first one enables valgrind to read debug info
# CC=/usr/bin/gcc-10
CC=gcc

CFLAGS=-Wall -Wextra -pedantic -g -std=c11 -DLOGLEVEL=$(LOGLEVEL) \
$(DNDEBUG) $(ASAN)

LDFLAGS=$(ASAN) -lpthread

MODULES = udpcl.o rwmsgid.o main.o argparse.o gexit.o mmal.o udp_render.o \
udp_confirmer.o udp_listener.o sleep_ms.o udp_sender.o

ALL: $(RESULT_BINARY)

udpcl.o: udpcl.c udpcl.h ipk24chat.h utils.h rwmsgid.h gexit.h mmal.h \
udp_render.h
	$(CC) $(CFLAGS) -c -o $@ $<

rwmsgid.o: rwmsgid.c rwmsgid.h
	$(CC) $(CFLAGS) -c -o $@ $<

main.o: main.c ipk24chat.h udpcl.h utils.h argparse.h gexit.h udp_confirmer.h \
udp_listener.h udp_sender.h sleep_ms.h mmal.h
	$(CC) $(CFLAGS) -c -o $@ $<

argparse.o: argparse.c argparse.h ipk24chat.h utils.h mmal.h
	$(CC) $(CFLAGS) -c -o $@ $<

gexit.o: gexit.c ipk24chat.h gexit.h utils.h
	$(CC) $(CFLAGS) -c -o $@ $<

mmal.o: mmal.c mmal.h gexit.h
	$(CC) $(CFLAGS) -c -o $@ $<

udp_render.o: udp_render.c udp_render.h mmal.h ipk24chat.h rwmsgid.h utils.h
	$(CC) $(CFLAGS) -c -o $@ $<

udp_listener.o: udp_listener.c udp_listener.h ipk24chat.h udp_confirmer.h \
utils.h mmal.h rwmsgid.h udp_sender.h
	$(CC) $(CFLAGS) -c -o $@ $<

udp_confirmer.o: udp_confirmer.c ipk24chat.h udp_confirmer.h mmal.h utils.h
	$(CC) $(CFLAGS) -c -o $@ $<

sleep_ms.o: sleep_ms.c utils.h
	$(CC) $(CFLAGS) -c -o $@ $<

udp_sender.o: udp_sender.c udp_sender.h udp_render.h ipk24chat.h utils.h \
mmal.h sleep_ms.h udp_confirmer.h
	$(CC) $(CFLAGS) -c -o $@ $<

$(RESULT_BINARY): $(MODULES)
	$(CC) $(LDFLAGS) -o $@ $^

.PHONY: clean
clean:
	rm -rf *.o *.bin $(RESULT_BINARY)

.PHONY: remake
remake: clean $(RESULT_BINARY)

test_argparse.bin: test_argparse.c argparse.h argparse.o
	$(CC) $(CFLAGS) -o $@ $< argparse.o

test_sockopen.bin: test_sockopen.c utils.h
	$(CC) $(CFLAGS) -o $@ $<

test:
	./test.sh

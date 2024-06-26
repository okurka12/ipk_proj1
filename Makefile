##################
##  Vit Pavlik  ##
##   xpavli0a   ##
##    251301    ##
##              ##
##  Created:    ##
##  2024-02-18  ##
##################

# log level (DEBUG, INFO, WARNING, ERROR, FATAL)
# LOGLEVEL=-DLOGLEVEL=WARNING

# uncomment this to disable all logging
DNDEBUG=-DNDEBUG

# uncomment this to enable adress sanitizer
# ASAN=-fsanitize=address
# ASAN=-fsanitize=thread  # doesn't work?

RESULT_BINARY=ipk24chat-client

# the first one enables valgrind to read debug info
# CC=/usr/bin/gcc-10
CC=gcc

CFLAGS=-Wall -Wextra -pedantic -std=c11 -O3 $(LOGLEVEL) $(DNDEBUG) $(ASAN)

LDFLAGS=$(ASAN) -lpthread

MODULES = udpcl.o rwmsgid.o main.o argparse.o gexit.o mmal.o udp_render.o \
udp_confirmer.o udp_listener.o sleep_ms.o udp_sender.o shell.o msg.o \
udp_print_msg.o udp_marker.o tcpcl.o tcp_render.o tcp_parse.o

ALL: $(RESULT_BINARY)

udpcl.o: udpcl.c udpcl.h ipk24chat.h utils.h rwmsgid.h gexit.h mmal.h \
udp_render.h udp_confirmer.h udp_listener.h msg.h shell.h udp_sender.h \
udp_print_msg.h sleep_ms.h
	$(CC) $(CFLAGS) -c -o $@ $<

rwmsgid.o: rwmsgid.c rwmsgid.h
	$(CC) $(CFLAGS) -c -o $@ $<

main.o: main.c ipk24chat.h udpcl.h utils.h argparse.h gexit.h udp_confirmer.h \
udp_listener.h udp_sender.h sleep_ms.h shell.h mmal.h udp_marker.h \
udp_print_msg.h tcpcl.h
	$(CC) $(CFLAGS) -c -o $@ $<

argparse.o: argparse.c argparse.h ipk24chat.h utils.h mmal.h
	$(CC) $(CFLAGS) -c -o $@ $<

gexit.o: gexit.c ipk24chat.h gexit.h utils.h msg.h udp_sender.h \
udp_confirmer.h tcpcl.h
	$(CC) $(CFLAGS) -c -o $@ $<

mmal.o: mmal.c mmal.h gexit.h
	$(CC) $(CFLAGS) -c -o $@ $<

udp_render.o: udp_render.c udp_render.h mmal.h ipk24chat.h rwmsgid.h utils.h \
msg.h
	$(CC) $(CFLAGS) -c -o $@ $<

udp_listener.o: udp_listener.c udp_listener.h ipk24chat.h udp_confirmer.h \
utils.h mmal.h rwmsgid.h udp_sender.h udp_print_msg.h udp_marker.h
	$(CC) $(CFLAGS) -c -o $@ $<

udp_confirmer.o: udp_confirmer.c ipk24chat.h udp_confirmer.h mmal.h utils.h
	$(CC) $(CFLAGS) -c -o $@ $<

sleep_ms.o: sleep_ms.c utils.h
	$(CC) $(CFLAGS) -c -o $@ $<

shell.o: shell.c mmal.h ipk24chat.h shell.h utils.h msg.h gexit.h \
udp_listener.h udp_confirmer.h udp_sender.h udp_print_msg.h
	$(CC) $(CFLAGS) -c -o $@ $<

udp_sender.o: udp_sender.c udp_sender.h udp_render.h ipk24chat.h utils.h \
mmal.h sleep_ms.h udp_confirmer.h msg.h
	$(CC) $(CFLAGS) -c -o $@ $<

msg.o: msg.c msg.h mmal.h utils.h
	$(CC) $(CFLAGS) -c -o $@ $<

udp_print_msg.o: udp_print_msg.c ipk24chat.h udp_print_msg.h utils.h mmal.h
	$(CC) $(CFLAGS) -c -o $@ $<

udp_marker.o: udp_marker.c mmal.h udp_marker.h utils.h
	$(CC) $(CFLAGS) -c -o $@ $<

tcpcl.o: tcpcl.c ipk24chat.h tcpcl.h msg.h utils.h shell.h mmal.h \
tcp_render.h gexit.h tcp_parse.h
	$(CC) $(CFLAGS) -c -o $@ $<

tcp_render.o: tcp_render.c ipk24chat.h mmal.h msg.h utils.h
	$(CC) $(CFLAGS) -c -o $@ $<

tcp_parse.o: tcp_parse.c tcp_parse.h ipk24chat.h utils.h
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

# -Wno-unused-value for comma operator in assert
test_tcp_parse.bin: test_tcp_parse.c tcp_parse.o
	$(CC) $(CFLAGS) -Wno-unused-value -o $@ $< tcp_parse.o

test: test_argparse.bin test_tcp_parse.bin
	./test.sh
	./test_tcp_parse.bin

# IPK24CHAT client

This is a client application for the IPK24CHAT protocol. It came into being
as my solution to the first assignment of the IPK course.

Author: Vit Pavlik (`xpavli0a`)

Date: March 31, 2024

## Build instructions

This program is written in the C programming language and is specific for
linux-based systems, because it utilizes the `epoll` I/O notification
facility.

 ### Prerequisites:

- GNU Make (version `4.4.1` works)
- GCC compiler (version `12.3.0` works)
- ISO C11 Concurrency support library on the system (i.e. `threads.h`, glibc
`2.38` works)
- support for `epoll` (linux `6.5.0-17-generic` with glibc `2.38` works)

### Build process:

- running `make` builds `ipk24chat-client` binary

## Usage

The client can be executed with following parameters:

`ipk24chat-client -t tcp|udp -s ADDRESS [-p PORT] [-d TIMEOUT]
[-r RETRIES] [-h]`

- `-t` - the desired transport protocol: this program can do bot TCP and UDP
- `-s` - the address of an IPK24CHAT server
  - this can be a number (e. g. `2481260788`), a dotted-decimal address (e. g.
`147.229.8.244`) or a domain name (e. g. `anton5.fit.vutbr.cz`)
- `-p` - port to use (the default is `4567`)
  - values between `0` and `65535`
- `-d` - number of miliseconds the client will wait before retransmitting
messages if they were not confirmed as delivered by server (only for UDP)
  - value needs to fit into `unsigned int` C data type
  - on a linux machine with gcc compiler, this will most likely mean
a value range between `0` and `4294967296`
- `-r` - how many times will the client transmit the messages if they are
not confirmed as delivered by server (only for UDP)
  - after 1 transmission and n - 1 retransmissions (where n is the value given
to `-r` option), the client consider further effort to send the message
hopeless and print an error that the message couldn't be sent
  - value range is the same as for `-d`
- `-h` - print usage information and exit


*Note that arguments are parsed using the POSIX `getopt` library, therefore,
if an option is specified more than once, the last one is used.*
- *Example: `ipk24chat-client -t udp -s localhost -r 20 -r 5` will mean
the program takes `-r 5`.*

## Theoretical background

This project solves a network communication of many users. The users (clients)
communicate through a server: IPK24CHAT is a client-server architecture.

The goals of this client application are following:
- establish 1 to 1 connection with the server
- provide user interface for the user
- output data sent by server for the user
- send user's data to the server

The user interface is set up via standard streams
(https://www.bell-labs.com/usr/dmr/www/st.html) - standard output, standard
input and standard error stream.

### Standard output and standard error

There's little to worry about regarding the
output - the output is line buffered, so each time a message is outputted,
the user sees it right away, assuming he has the standard output connected
to an interactive device (ie. a terminal).
(https://www.gnu.org/software/libc/manual/html_node/Buffering-Concepts.html)

I'm assuming that if the user redirects the output to a file, he won't be
examining it's contents right away, even though that's not impossible:
someone could `tail -f` the file, for example.

### Standard input

Standard input poses a significant problem for the application, because,
assuming it's not a file, it contents are not available right away and can
come at any time. Whether standard input stream is connected to a terminal or
it's another stream (a pipe), the application needs to be notified about
when data on the standard input stream becomes ready. Otherwise, it would
block on the `read` operation and couldn't process data from the server in the
meantime.

It's worth noting that we should also allow the standard input to be a file.
This way, someone can for example build another application on top of
IPK24CHAT, like file transfer.

### Network

On an application layer, we don't know nor care about how data gets to the
other side, like what route it takes or what hardware network interface we use,
we just need to make sure the data gets there sometime somehow.

In case of TCP, this is as simple as checking the return value of `send`. In
TCP, `send` is most often successful, unless the connection was reset or
something. In that case, the connection needs to be re-established so only
thing to do end the application, so the user can run it again if he wants.

In UDP, we have no way of knowing if the data made its way to the destination
host (server), so the server needs to actively reply another application data
that confirm the server got our data. This is addressed in the IPK24CHAT
protocol. In case we don't receive a confirmation to our data in a given amount
of time, we send them again. We try to do that only a given number of times
until we receive the confirmation, and if we don't, we consider the connection
finished. This poses a problem that we need to somehow *wait* for the
confirmation data.

Receiving data from the poses the same problem as standard input, they can
come at any unspecified point in time. Trying to read data from the network
before they come would again result in blocking.

## Activity diagram, interesting source code sections

Implementation for the TCP and UDP variant is quite different, because
they solve different problems and UDP variant spawns a listener thread,
while TCP variant is single-threaded.

![Activity diagram for the TCP variant](./doc_tcp_activity_diag.drawio.svg "Activity diagram for the TCP variant")

Activity diagram for the TCP variant

While the UDP variant is implemented differently



/*****************
**  Vit Pavlik  **
**   xpavli0a   **
**    251301    **
**              **
**   Created:   **
**  2024-02-18  **
**              **
**    Edited:   **
**  2024-02-18  **
*****************/

/**
 *
 * udpcl.c - UDP client
 * standalone for now
 *
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>  // close
#include <stdlib.h>  // malloc
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>  // struct sockaddr_in
#include <arpa/inet.h>  // htons

#include "udpcl.h"
#include "ipk24chat.h"
#include "utils.h"

/* addres struct for sendto */
#define SSA struct sockaddr

/* size of the addres structure (`struct sockaddr_in`) */
#define AS_SIZE sizeof(struct sockaddr_in)

/**
 * Private: fill `struct sockaddr_in` from `addr`
 * used in `udp_send_data`
 * @return a pointer to `struct sockaddr` (SSA) but it actually
 * points to `struct sockaddr_in` of size AS_SIZE
 * @note dynamically allocated, needs to be freed
*/
SSA *udp_get_addrstruct(addr_t *addr/*, struct sockaddr_in *s*/) {

    /* allocate + initialize to zero, initialize domain */
    struct sockaddr_in *s = calloc(sizeof(struct sockaddr_in), 1);
    s->sin_family = AF_INET;

    /* convert IP addres from text to binary */
    if (inet_pton(AF_INET, addr->addr, &s->sin_addr) <= 0) {
        perror("inet_pton error (invalid address?)");
        logf(ERROR, "inet_pton error (invalid address '%s' ?)", addr->addr);
        return NULL;
    }

    /* convert from host byte order to network byte order */
    s->sin_port = htons(addr->port);

    return (SSA *)s;
}


/**
 * Private: On an open AF_INET SOCK_DGRAM socket, sends `data`
 * @param sockfd socket file descriptor
 * @param sa server address struct
 * @param data
 * @param length
 * @return 0 on success else 1
*/
int udp_send(int sockfd, SSA *sa, const char *data, unsigned int length) {

    ssize_t result = sendto(sockfd, data, length, 0, sa, AS_SIZE);
    if (result == -1) {
        perror("sendto failed");
        log(ERROR, "sendto failed");
        return 1;
    }
    return 0;
}


/**
 * Private: create AF_INET SOCK_DGRAM socket (UDP)
 * @return socket file descriptor on succes, else -1
 * @note needs to be closed with `close()` from `unistd.h`
*/
int udp_create_socket() {
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd == -1) {
        perror("socket creation failed");
        log(ERROR, "socket creation failed");
        return -1;
    }
    return sockfd;
}


int udp_send_data(addr_t *addr, const char *data,
                  unsigned int length) {

    SSA *sa = udp_get_addrstruct(addr);
    int sockfd = udp_create_socket();

    /* send the packet */
    udp_send(sockfd, sa, data, length);

    close(sockfd);
    free(sa);
    return 0;
}


int main() {
    char *msg = "Hello, I am client.\r\n";
    char *addr = "127.0.0.1";
    u_int16_t port = 4567;
    udp_send_data(&(addr_t){.addr=addr, .port=port}, msg, strlen(msg));
}

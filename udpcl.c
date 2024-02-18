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

#include <assert.h>  // static_assert
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
 * used in `udp_send_msg`
 * @return a pointer to `struct sockaddr` (SSA) (but it actually
 * points to `struct sockaddr_in` of size AS_SIZE) on success, else NULL
 * @note dynamically allocated, needs to be freed
*/
SSA *udp_get_addrstruct(addr_t *addr/*, struct sockaddr_in *s*/) {

    /* allocate + initialize to zero, initialize domain */
    struct sockaddr_in *s = calloc(sizeof(struct sockaddr_in), 1);
    if (s == NULL) {
        perror(MEMFAIL_MSG);
        log(ERROR, MEMFAIL_MSG);
        return NULL;
    }
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


/**
 * Private: returns a pointer to memory where the entire msg contained
 * in `msg` lies, returns length via `length`
 * @note dynamically allocated, needs to be freed
*/
char *udp_render_message(msg_t *msg, unsigned int *length) {

    /*        header  content                crlf */
    *length = 1 + 2 + strlen(msg->content) + 2;
    char *output = malloc(*length);
    if (output == NULL) {
        perror(MEMFAIL_MSG);
        log(ERROR, MEMFAIL_MSG);
    }

    /* write msg type*/
    output[0] = msg->type;

    /* write msg id (gcc specific macros) */
    if (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__) {
        output[1] = ((char *)(&msg->id))[0];
        output[2] = ((char *)(&msg->id))[1];
    } else {
        output[1] = ((char *)(&msg->id))[1];
        output[2] = ((char *)(&msg->id))[0];
    }

    /* write msg content */
    strcpy(output + 3, msg->content);

    /* write crlf at the end */
    strcpy(output + *length - 2, CRLF);

    return output;
}


int udp_send_msg(addr_t *addr, msg_t *msg) {

    SSA *sa = udp_get_addrstruct(addr);
    int sockfd = udp_create_socket();
    unsigned int length = 0;
    char *data = udp_render_message(msg, &length);

    /* send the packet */
    udp_send(sockfd, sa, data, length);

    close(sockfd);
    free(sa);
    free(data);
    return 0;
}


int main() {

    char *msg_text = "Hello, I am client.";

    char *ip = "127.0.0.1";
    u_int16_t port = 4567;

    addr_t addr = { .addr = ip, .port = port };
    msg_t msg = { .type = MSG, .id = 1, .content = msg_text };

    udp_send_msg(&addr, &msg);
}

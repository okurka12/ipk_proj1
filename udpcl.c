/*****************
**  Vit Pavlik  **
**   xpavli0a   **
**    251301    **
**              **
**   Created:   **
**  2024-02-18  **
**              **
**    Edited:   **
**  2024-02-19  **
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
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>  // struct timeval
#include <netinet/in.h>  // struct sockaddr_in
#include <arpa/inet.h>  // htons

#include "udpcl.h"
#include "ipk24chat.h"
#include "utils.h"
#include "rwmsgid.h"

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
int udp_create_socket(udp_conf_t *conf) {
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd == -1) {
        perror("socket creation failed");
        log(ERROR, "socket creation failed");
        return -1;
    }
    struct timeval t = { .tv_usec = conf->t * 1000 };
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &t, sizeof(t));
    return sockfd;
}


/**
 * Private: returns a pointer to memory where the entire msg contained
 * in `msg` lies, returns length via `length`
 * @note dynamically allocated, needs to be freed
 * @return pointer to rendered `msg` or NULL on failure
*/
char *udp_render_message(msg_t *msg, unsigned int *length) {

    /* note: strlen(CRLF) is 2 */

    /*        header, content,               crlf */
    *length = 1 + 2 + strlen(msg->content) + strlen(CRLF);
    char *output = malloc(*length);
    if (output == NULL) {
        perror(MEMFAIL_MSG);
        log(ERROR, MEMFAIL_MSG);
        return NULL;
    }

    /* write msg type*/
    output[0] = msg->type;

    /* write msg id */
    write_msgid(output + 1, msg->id);

    /* write msg content */
    strcpy(output + 3, msg->content);

    /* write crlf at the end */
    memcpy(output + *length - strlen(CRLF), CRLF, strlen(CRLF));

    return output;
}


/**
 * Private: waits for confirmation of `msg`
 * @return 1 if message was confirmed, 0 if it was not (timed out)
*/
int udp_wait_for_confirm(int sockfd, msg_t *msg) {
    char *reply = (char *)malloc(CONFIRM_BUFSIZE);
    if (reply == NULL) {
        perror(MEMFAIL_MSG);
        log(ERROR, MEMFAIL_MSG);
        return 0;
    }

    /* todo: what if this catches other udp packet? */
    int received_bytes = recv(sockfd, reply, CONFIRM_BUFSIZE, 0);
    if (received_bytes == -1) {
        logf(DEBUG, "timed out (errno %d)", errno);  // EAGAIN - socket(7)
        free(reply);
        return 0;
    }
    logf(DEBUG, "received %d bytes", received_bytes);
    uint8_t reply_type = reply[0];
    uint16_t reply_msgid = read_msgid(reply + 1);

    free(reply); reply = NULL;

    if (reply_msgid != msg->id || reply_type != CONFIRM) {
        logf(DEBUG, "message type=%x, id=%hu ignored", reply_type, reply_msgid);
        return 0;
    }
    return 1;
}


/**
 * Private: bind the socket to 0.0.0.0 port `port`
 * @return 0 on success else 1
*/
int udp_bind(int sockfd, uint16_t port) {
    addr_t addr = { .addr = "0.0.0.0", .port = port };
    SSA *localhost = udp_get_addrstruct(&addr);
    if (bind(sockfd, localhost, AS_SIZE) != 0) {
        perror("couldn't bind");
        log(ERROR, "couldn't bind");
        return 1;
    }
    return 0;
}



int udp_send_msg(addr_t *addr, msg_t *msg, udp_conf_t *conf) {

    logf(DEBUG, "sending 0x%02hhx:%hu:'%s' to %s:%hu", msg->type, msg->id,
         msg->content, addr->addr, addr->port);

    /* process address */
    SSA *sa = udp_get_addrstruct(addr);
    if (sa == NULL) return 1;

    /* create socket */
    int sockfd = udp_create_socket(conf);
    if (sockfd == -1) return 1;

    /* bind socket */
    // if (udp_bind(sockfd, addr->port)) return 1;

    /* render message */
    unsigned int length = 0;
    char *data = udp_render_message(msg, &length);

    /* send the packet */
    udp_send(sockfd, sa, data, length);

    int confirmed = udp_wait_for_confirm(sockfd, msg);
    if (not confirmed) {
        logf(WARNING, "msg id %hu not confirmed", msg->id);
        return 1;
    } else {
        logf(INFO, "msg id %hu confirmed", msg->id);
    }

    close(sockfd);
    free(sa);
    free(data);
    return 0;
}


int main() {

    char *msg_text = "Hello, I am client.";

    char *ip = "127.0.0.1";  // localhost
    // char *ip = "192.168.1.73";  // oslavany debian12vita local
    u_int16_t port = 4567;

    addr_t addr = { .addr = ip, .port = port };
    msg_t msg = { .type = MSG, .id = 1, .content = msg_text };
    udp_conf_t conf = { .r = 3, .t = 250 };

    udp_send_msg(&addr, &msg, &conf);
}

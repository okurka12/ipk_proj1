/*****************
**  Vit Pavlik  **
**   xpavli0a   **
**    251301    **
**              **
**   Created:   **
**  2024-02-18  **
**              **
**    Edited:   **
**  2024-02-25  **
*****************/

/**
 *
 * udpcl.c - UDP client
 * functions necessary to perform communication on the UDP transport protocol
 *
 */

#include <assert.h>
#include <stdio.h>  // perror
#include <string.h>  // strcpy, memcpy
#include <unistd.h>  // close
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
#include "gexit.h"
#include "mmal.h"
#include "udp_render.h"

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
SSA *udp_get_addrstruct(char *addr, uint16_t port) {

    /* allocate + initialize to zero, initialize domain */
    struct sockaddr_in *s = mcal(sizeof(struct sockaddr_in), 1);
    if (s == NULL) {
        perror(MEMFAIL_MSG);
        log(ERROR, MEMFAIL_MSG);
        return NULL;
    }
    s->sin_family = AF_INET;

    /* convert IP addres from text to binary */
    if (inet_pton(AF_INET, addr, &s->sin_addr) <= 0) {
        perror("inet_pton error (invalid address?)");
        logf(ERROR, "inet_pton error (invalid address '%s' ?)", addr);
        return NULL;
    }

    /* convert from host byte order to network byte order */
    s->sin_port = htons(port);

    return (SSA *)s;
}


int udp_set_rcvtimeo(conf_t *conf, unsigned int ms) {
    int rc = 0;
    unsigned int sec = ms / 1000;
    unsigned int msec = ms % 1000;
    struct timeval t = {.tv_sec = sec , .tv_usec = msec * 1000 };
    rc = setsockopt(conf->sockfd, SOL_SOCKET, SO_RCVTIMEO, &t, sizeof(t));
    if (rc == -1) {
        rc = 1;
        perror("Couldn't setsockopt SO_RCVTIMEO");
        log(ERROR, "couldn't set a timeout on socket");
    }
    return rc;
}


int udp_create_socket(conf_t *conf) {

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd == -1) {
        perror("socket creation failed");
        log(ERROR, "socket creation failed");
        return 1;
    }
    conf->sockfd = sockfd;

    return 0;
}


/** todo: remove this?
 * Private: waits for confirmation of `msg`, saves the source port of the
 * incoming message to `resp_port` (if it isn't NULL)
 * @param sockfd
 * @param msg
 * @param resp_port
 * @return true if message was confirmed, false if it was not (timed out or
 * invalid)
*/
// bool udp_wait_for_confirm(int sockfd, msg_t *msg, uint16_t *resp_port) {

//     /* buffer for incoming data */
//     char *reply = (char *)mmal(RESPONSE_BUFSIZE);
//     if (reply == NULL) {
//         perror(MEMFAIL_MSG);
//         log(ERROR, MEMFAIL_MSG);
//         return 0;
//     }

//     /* bsd socket api stuff */
//     struct sockaddr_in respaddr;
//     socklen_t respaddr_len = AS_SIZE;

//     /* recvfrom */
//     int received_bytes = recvfrom(sockfd, reply, RESPONSE_BUFSIZE, 0,
//         (SSA *)&respaddr, &respaddr_len);
//     if (received_bytes == -1) {
//         logf(DEBUG, "timed out (errno %d)", errno);  // EAGAIN - socket(7)
//         mfree(reply);
//         return 0;
//     }

//     /* extract the address and portl */
//     char *respaddr_str = inet_ntoa(respaddr.sin_addr);
//     if (resp_port != NULL) { *resp_port = ntohs(respaddr.sin_port); }

//     logf(DEBUG, "received %d bytes from %s:%hu", received_bytes, respaddr_str,
//         ntohs(respaddr.sin_port));

//     /* extract IPK24 message header */
//     uint8_t reply_type = reply[0];
//     uint16_t reply_msgid = read_msgid(reply + 1);

//     /* get rid of data (all is extracted) */
//     mfree(reply); reply = NULL;

//     /* was it the confirm we were looking for? */
//     if (reply_msgid != msg->id || reply_type != MTYPE_CONFIRM) {
//         logf(DEBUG, "message type=%s, id=%hu ignored", mtype_str(reply_type),
//             reply_msgid);
//         return 0;
//     }

//     /* message successfully confirmed */
//     return 1;
// }


int udp_send_msg(msg_t *msg, conf_t *conf) {

    assert(conf->sockfd != -1);

    logf(INFO, "sending %s id=%hu to %s:%hu", mtype_str(msg->type),
         msg->id, conf->addr, conf->port);

    /* process address */
    SSA *sa = udp_get_addrstruct(conf->addr, conf->port);
    if (sa == NULL) return 1;

    /* render message */
    unsigned int length = 0;
    char *data = udp_render_message(msg, &length);
    if (data == NULL) { log(ERROR, "couldn't render message"); return 1; }

    /* send message */
    ssize_t result = sendto(conf->sockfd, data, length, 0, sa, AS_SIZE);
    if (result == -1) {
        perror("sendto failed");
        log(ERROR, "sendto failed");
        return 1;
    }
    return 0;

    /* cleanup */
    mfree(sa);
    mfree(data);

    return 0;
}

/* todo: get rid of this */
// int udp_send_and_confirm(msg_t *msg, conf_t *conf) {


//     logf(INFO, "sending %s id=%hu to %s:%hu", mtype_str(msg->type),
//          msg->id, conf->addr, conf->port);

//     /* process address */
//     SSA *sa = udp_get_addrstruct(conf->addr, conf->port);
//     if (sa == NULL) return 1;

//     /* create socket */
//     int sockfd = udp_create_socket(conf);
//     if (sockfd == -1) return 1;
//     gexit(GE_SET_FD, &sockfd);

//     /* render message */
//     unsigned int length = 0;
//     char *data = udp_render_message(msg, &length);
//     if (data == NULL) { log(ERROR, "couldn't render message"); return 1; }

//     bool confirmed = false;

//     unsigned int i;
//     for (i = 1; i < conf->retries + 1; i++) {
//         logf(DEBUG, "sending msg id %hu (attempt: %u)", msg->id, i);

//         /* send the packet */
//         udp_send(sockfd, sa, data, length);

//         /* wait for CONFIRM (no need to bind) */
//         confirmed = udp_wait_for_confirm(sockfd, msg, &conf->port);
//         if (confirmed) {
//             break;
//         }
//     }

//     /* cleanup */
//     gexit(GE_UNSET_FD, &sockfd);
//     close(sockfd);
//     mfree(sa);
//     mfree(data);

//     if (confirmed) {
//         logf(INFO, "confirmed in %u attempts", i);
//         return 0;
//     } else {
//         logf(WARNING, "couldn't confirm msg %hu in %u attempts", msg->id, --i);
//         return ERR_NOTCONF;
//     }
// }


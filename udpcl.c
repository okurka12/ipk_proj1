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
#include "udp_confirmer.h"  // udp_cnfm_t
#include "udp_listener.h"  // LISTENER_TIMEOUT
#include "udp_sender.h"  // udp_send
#include "msg.h"  // msg_t
#include "shell.h"


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


int udp_main(conf_t *conf) {

    int rc = 0;

    /* crate socket + set timeout*/
    rc = udp_create_socket(conf);
    if (rc != 0) {
        log(ERROR, "couldn't create socket");
        return ERR_INTERNAL;
    }
    gexit(GE_SET_FD, &(conf->sockfd));
    rc = udp_set_rcvtimeo(conf, LISTENER_TIMEOUT);
    if (rc != 0) {
        log(ERROR, "couldn't set timeout on socket");
        return ERR_INTERNAL;
    }

    udp_shell(conf);

    gexit(GE_UNSET_FD, NULL);
    close(conf->sockfd);
    log(INFO, "udp client done");
    return rc;
}

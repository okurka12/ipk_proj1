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
    udp_cnfm_data_t cnfm_data = { .arr = NULL, .len = 0 };

    /* hard coded message data */
    msg_t first_msg = { .type = MTYPE_AUTH, .id = 2, .username = "xpavli0a",
        .dname = "vita", .secret = "a38c9ccc-9934-4603-afc8-33d9db47c66c" };
    msg_t msg2 = { .type = MTYPE_JOIN, .id = 3, .chid = "discord.general",
        .dname = "vita" };
    msg_t last_msg = { .type = MTYPE_BYE, .id = 4 };
    (void)first_msg;
    (void)msg2;
    (void)last_msg;

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

    /* start listener thread (only to return immediately after receiving
    a CONFIRM message for `first_msg`, the lock isn't unlocked by listener) */
    log(DEBUG, "MAIN: starting listener");
    thrd_t listener_thread_id;
    mtx_t listener_mtx;
    mtx_init(&listener_mtx, mtx_plain);
    mtx_lock(&listener_mtx);
    listener_args_t listener_args = {
        .conf = conf,
        .cnfm_data = &cnfm_data,
        .mtx = &listener_mtx,
        .save_port = true,
        .auth_msg_id = first_msg.id
    };
    rc = thrd_create(&listener_thread_id, udp_listener, &listener_args);
    if (rc != thrd_success) {
        log(ERROR, "couldnt create listener thread");
        return ERR_INTERNAL;
    }
    gexit(GE_SET_LISTHR, &listener_thread_id);
    gexit(GE_SET_LISMTX, &listener_mtx);

    /* send first AUTH message */
    rc = udp_sender_send(&first_msg, conf, &cnfm_data);
    if (rc != 0) { log(ERROR, "couldn't send"); goto cleanup; }


    /* wait for the listener thread */
    log(DEBUG, "waiting for listener thread");
    thrd_join(listener_thread_id, NULL);
    gexit(GE_UNSET_LISTNR, NULL);

    logf(INFO, "REPLY came from port %hu", conf->port);

    /* start listener again, but now for real */
    listener_args.save_port = false;
    rc = thrd_create(&listener_thread_id, udp_listener, &listener_args);
    if (rc != thrd_success) {
        log(ERROR, "couldn't create listener thread");
        return ERR_INTERNAL;
    }
    gexit(GE_SET_LISTHR, &listener_thread_id);
    gexit(GE_SET_LISMTX, &listener_mtx);

    /* sleep before sending msg2 */
    getchar();

    /* now that listener is started and will be confirming messages,
    we can send messages */
    rc = udp_sender_send(&msg2, conf, &cnfm_data);
    if (rc != 0) { log(ERROR, "couldn't send"); rc = 1; goto cleanup; }
    // sleep_ms(400);
    rc = udp_sender_send(&last_msg, conf, &cnfm_data);
    if (rc != 0) { log(ERROR, "couldn't send"); rc = 1; goto cleanup; }

    cleanup:

    /* let listener finish */
    mtx_unlock(&listener_mtx);

    /* wait for it to finish */
    log(DEBUG, "waiting for listener thread");
    thrd_join(listener_thread_id, NULL);
    gexit(GE_UNSET_LISTNR, NULL);

    /* cleanup */
    mtx_destroy(&listener_mtx);
    mfree(cnfm_data.arr);
    gexit(GE_UNSET_FD, NULL);
    close(conf->sockfd);

    log(INFO, "udp client done");
    return rc;
}

/*****************
**  Vit Pavlik  **
**   xpavli0a   **
**    251301    **
**              **
**   Created:   **
**  2024-02-26  **
**              **
**    Edited:   **
**  2024-02-26  **
*****************/

/**
 * implementation of the UDP Listener module - see udp_listener.h
*/

#include <assert.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>  // struct sockaddr_in
#include <arpa/inet.h>  // htons
#include <ctype.h>
#include <stdio.h>

#include "udp_listener.h"
#include "ipk24chat.h"  // conf_t
#include "udp_confirmer.h"
#include "utils.h"
#include "mmal.h"
#include "rwmsgid.h"
#include "udp_sender.h"


/* addres struct for sendto */
#define SSA struct sockaddr

/* size of the addres structure (`struct sockaddr_in`) */
#define AS_SIZE sizeof(struct sockaddr_in)


void udp_listener_print(char *msg, unsigned int len) {
    if (len > 0) {
        printf("\n%s ", mtype_str(msg[0]));
    }
    if (len > 2) {
        printf("id=%hu: ", read_msgid(msg + 1));
    }
    printf("b\"");
    for (unsigned int i = 0; i < len; i++) {
        if (isprint(msg[i])) {
            putchar(msg[i]);
        } else {
            printf("\\x%02hhu", (unsigned char)msg[i]);
        }
    }
    printf("\"\n");
}


int udp_listener(void *args) {

    /* extract the arguments from the `args` strcture */
    conf_t *conf =               ((listener_args_t *)args)->conf;
    udp_cnfm_data_t *cnfm_data = ((listener_args_t *)args)->cnfm_data;
    mtx_t *mtx =                 ((listener_args_t *)args)->mtx;
    bool save_port =             ((listener_args_t *)args)->save_port;
    uint16_t auth_msgid =        ((listener_args_t *)args)->auth_msg_id;

    log(DEBUG, "listener starting");

    assert(conf->sockfd != -1);

    /* buffer for incoming data */
    char *buf = (char *)mmal(RESPONSE_BUFSIZE);
    if (buf == NULL) {
        perror(MEMFAIL_MSG);
        log(ERROR, MEMFAIL_MSG);
        return 0;
    }

    /* bsd socket api stuff */
    struct sockaddr_in respaddr;
    socklen_t respaddr_len = AS_SIZE;
    SSA *sa = udp_get_addrstruct(conf->addr, conf->port);  // for sending conf.

    /* loop */
    while (true) {
    log(DEBUG, "listener looping");

    /* return if main thread wants us to return */
    int mtx_rc = mtx_trylock(mtx);
    if (mtx_rc == thrd_success) {
        mtx_unlock(mtx);
        break;
    }

    /* recvfrom */
    int received_bytes = recvfrom(conf->sockfd, buf, RESPONSE_BUFSIZE, 0,
        (SSA *)&respaddr, &respaddr_len);


    /* if recvfrom timed out */
    if (received_bytes == -1) {
        continue;
    }

    udp_listener_print(buf, received_bytes);

    /* extract the address and port */
    char *respaddr_str = inet_ntoa(respaddr.sin_addr);
    uint16_t respaddr_port = ntohs(respaddr.sin_port);
    logf(DEBUG, "received %d bytes from %s:%hu", received_bytes, respaddr_str,
        respaddr_port);

    if (received_bytes < 3) {
        logf(INFO, "got only %d byte(s), ignoring", received_bytes);
        continue;
    }

    /* extract IPK24 message header */
    uint8_t resp_mtype = buf[0];
    uint16_t resp_id = read_msgid(buf + 1);  // id (or ref_id for CONFIRM)

    /* special case: listener only called to obtain the source port of
    a CONFIRM message to the first AUTH message*/
    if (save_port and resp_mtype == MTYPE_CONFIRM and resp_id == auth_msgid) {
        udp_cnfm_confirm(auth_msgid, cnfm_data);
        logf(DEBUG, "AUTH msg id=%hu, was confirmed from port %hu, "
            "changing conf->port", auth_msgid, respaddr_port);
        conf->port = respaddr_port;
        break;
    }

    if (resp_mtype == MTYPE_CONFIRM) {
        logf(DEBUG, "outgoing message id=%hu confirmed", resp_id);
        udp_cnfm_confirm(resp_id, cnfm_data);
    } else {
        logf(DEBUG, "sending CONFIRM for id %hu", resp_id);
        char data[3] = { MTYPE_CONFIRM, 0, 0 };
        write_msgid(data + 1, resp_id);
        ssize_t result = sendto(conf->sockfd, data, 3, 0, sa, AS_SIZE);
        if (result == -1) {
            perror("sendto failed");
            log(ERROR, "sendto failed");
            return 1;
        }
    }
    if (resp_mtype == MTYPE_BYE) {
        break;
    }

    /* todo: keep track of seen messages */

    }  // while true

    /* get rid of data (all is extracted) */
    mfree(buf); buf = NULL;
    mfree(sa);

    log(DEBUG, "listener done...");
    return 0;

}

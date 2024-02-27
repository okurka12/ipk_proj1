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

#include "udp_listener.h"
#include "ipk24chat.h"  // conf_t
#include "udp_confirmer.h"
#include "utils.h"
#include "mmal.h"
#include "rwmsgid.h"


/* addres struct for sendto */
#define SSA struct sockaddr

/* size of the addres structure (`struct sockaddr_in`) */
#define AS_SIZE sizeof(struct sockaddr_in)


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

    /* extract the address and port */
    char *respaddr_str = inet_ntoa(respaddr.sin_addr);
    uint16_t respaddr_port = ntohs(respaddr.sin_port);
    logf(DEBUG, "received %d bytes from %s:%hu", received_bytes, respaddr_str,
        respaddr_port);

    /* extract IPK24 message header */
    uint8_t resp_mtype = buf[0];
    uint16_t resp_id = read_msgid(buf + 1);  // id (or ref_id for CONFIRM)

    /* special case: listener only called to obtain the source port of
    a CONFIRM message to the first AUTH message*/
    if (save_port and resp_mtype == MTYPE_CONFIRM and resp_id == auth_msgid) {
        logf(DEBUG, "AUTH msg id=%hu, was confirmed from port %hu, "
            "changing conf->port", auth_msgid, respaddr_port);
        conf->port = respaddr_port;
        break;
    }

    /* todo: break on BYE messages and also send confirms */
    /* case: message is a CONFIRM message */
    if (resp_mtype == MTYPE_CONFIRM && received_bytes >= 3) {
        logf(DEBUG, "confirming id=%hu", resp_id);
        udp_cnfm_confirm(resp_id, cnfm_data);
    }

    /* todo: print the messages and keep track of seen messages */

    }  // while true

    /* get rid of data (all is extracted) */
    mfree(buf); buf = NULL;

    log(DEBUG, "listener done...");
    return 0;

}
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
    conf_t *conf =               ((listener_args_t *)args)->conf;
    udp_cnfm_data_t *cnfm_data = ((listener_args_t *)args)->cnfm_data;
    mtx_t *mtx =                 ((listener_args_t *)args)->mtx;

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

    int mtx_rc = mtx_trylock(mtx);
    if (mtx_rc == thrd_success) {
        mtx_unlock(mtx);
        log(DEBUG, "listener stopping");
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
    uint8_t reply_type = buf[0];
    uint16_t reply_msgid = read_msgid(buf + 1);


    /* case: message is a CONFIRM message */
    if (reply_type == MTYPE_CONFIRM && received_bytes >= 3) {
        logf(DEBUG, "confirming id=%hu", reply_msgid);
        udp_cnfm_confirm(reply_msgid, cnfm_data);
    }

    }  // while true

    /* get rid of data (all is extracted) */
    mfree(buf); buf = NULL;

    return 0;

}
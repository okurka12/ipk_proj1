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
 * implementation of UDP Sender - see udp_sender.h
*/

#include <assert.h>
#include <netinet/in.h>  // struct sockaddr_in
#include <arpa/inet.h>  // htons

#include "udp_sender.h"
#include "udp_render.h"
#include "ipk24chat.h"
#include "utils.h"
#include "mmal.h"
#include "sleep_ms.h"
#include "udp_confirmer.h"
#include "msg.h"  // msg_t

/* confirm check interval in miliseconds (after sending, periodically check if
a message was confirmed, basically active waiting) */
#define CCHECK_INT 10


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


int udp_sender_send(const msg_t *msg, const conf_t *conf,
    udp_cnfm_data_t *cnfm_data) {
    logf(DEBUG, "udp_sender started (msg id %hu)", msg->id);

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

    /* send message in n retries, break if confirmed */
    int confirmed = false;
    unsigned int i;
    udp_cnfm_reg(msg->id, cnfm_data);
    for (i = 0; i < conf->retries; i++) {
        ssize_t result = sendto(conf->sockfd, data, length, 0, sa, AS_SIZE);
        if (result == -1) {
            perror("sendto failed");
            log(ERROR, "sendto failed");
            return 1;
        }

        /* wait for at least CCHECK_INT, at most for conf->timeout */
        for (unsigned int j = 0; j * CCHECK_INT < conf->timeout; j++) {
            sleep_ms(CCHECK_INT);
            confirmed = udp_cnfm_was_confirmed(msg->id, cnfm_data);
            if (confirmed) goto after_outer_loop;  // double break
        }

        logf(DEBUG, "msg id=%hu not confirmed after %u ms, senging again",
            msg->id, conf->timeout);
    }
    after_outer_loop:

    /* cleanup */
    mfree(sa);
    mfree(data);

    if (not confirmed) {
        logf(ERROR, "message id=%hu was not confirmed in %u retries", msg->id,
        conf->retries);
        return 1;
    } else {
        logf(DEBUG, "udp_sender ended successfully (msg id %hu, confirmed "
            "on the %u. attempt)", msg->id, i + 1);
        return 0;
    }
}


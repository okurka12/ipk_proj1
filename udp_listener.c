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
#include "udp_print_msg.h"


/* addres struct for sendto */
#define SSA struct sockaddr

/* size of the addres structure (`struct sockaddr_in`) */
#define AS_SIZE sizeof(struct sockaddr_in)

/**
 * Prints `msg` TYPE + ID + the rest of the message in binary to stderr,
 * but only if `NDEBUG` is NOT defined
*/
void print_raw(char *msg, unsigned int len) {
    #ifndef NDEBUG

    if (len > 0) {
        fprintf(stderr, "\n%s ", mtype_str(msg[0]));
    }
    if (len > 2) {
        fprintf(stderr, "id=%hu: ", read_msgid(msg + 1));
    }
    fprintf(stderr, "b\"");
    for (unsigned int i = 0; i < len; i++) {
        if (isprint(msg[i])) {
            putchar(msg[i]);
        } else if (msg[i] == '\\') {
            fprintf(stderr, "\\\\");
        } else {
            fprintf(stderr, "\\x%02hhx", msg[i]);
        }
    }
    fprintf(stderr, "\"\n");

    #else  // ifndef NDEBUG

    (void)msg;
    (void)len;

    #endif  // ifndef NDEBUG
}


/**
 * listener has a lot of parameters so its good to check if the caller
 * didn't forget to initialize all of them
*/
static inline void check_listener_args(listener_args_t *args) {
    assert(args->conf != NULL);
    assert(args->cnfm_data != NULL);
    assert(args->mtx != NULL);
    assert(args->done_flag != NULL);
    assert(args->stop_flag != NULL);
    assert(args->server_sent_bye != NULL);
}


int udp_listener(void *args) {

    check_listener_args(args);

    /* extract the arguments from the `args` strcture */
    conf_t *conf =               ((listener_args_t *)args)->conf;
    udp_cnfm_data_t *cnfm_data = ((listener_args_t *)args)->cnfm_data;
    mtx_t *mtx =                 ((listener_args_t *)args)->mtx;
    bool save_port =             ((listener_args_t *)args)->save_port;
    uint16_t auth_msgid =        ((listener_args_t *)args)->auth_msg_id;
    bool *stop_flag =            ((listener_args_t *)args)->stop_flag;
    bool *done_flag =            ((listener_args_t *)args)->done_flag;
    bool *server_sent_bye =      ((listener_args_t *)args)->server_sent_bye;

    log(DEBUG, "listener starting");

    /* on the special case when `udp_listener` is called only for obtaining
    the source port of a response + result of the REPLY, these flags indicate
    listener to stop looping */
    bool auth_msg_confirmed = false;
    bool got_reply = false;

    assert(conf->sockfd != -1);

    /* buffer for the received data */
    char *buf = (char *)mmal(RESPONSE_BUFSIZE);
    if (buf == NULL) {
        perror(MEMFAIL_MSG);
        log(ERROR, MEMFAIL_MSG);
        return 0;
    }

    int rc = 0;

    /* bsd socket api stuff */
    struct sockaddr_in respaddr;
    socklen_t respaddr_len = AS_SIZE;
    SSA *sa = udp_get_addrstruct(conf->addr, conf->port);  // for sending conf.

    /* loop */
    while (true) {

    /* return if main thread wants us to return */
    mtx_lock(mtx);
    if (*stop_flag) {
        mtx_unlock(mtx);
        rc = 0;
        break;
    }
    mtx_unlock(mtx);

    /* call recvfrom and start the loop again if it timed out*/
    int received_bytes = recvfrom(conf->sockfd, buf, RESPONSE_BUFSIZE, 0,
        (SSA *)&respaddr, &respaddr_len);
    if (received_bytes == -1) {
        continue;
    }

    /* for debugging purposes, print the raw contents of a message */
    print_raw(buf, received_bytes);

    /* extract the address and port */
    char *respaddr_str = inet_ntoa(respaddr.sin_addr);
    uint16_t respaddr_port = ntohs(respaddr.sin_port);
    logf(DEBUG, "received %d bytes from %s:%hu", received_bytes, respaddr_str,
        respaddr_port);
    (void)respaddr_str;  // suppress warning if NDEBUG is defined

    if (received_bytes < 3) {
        logf(INFO, "got only %d byte(s), ignoring", received_bytes);
        continue;
    }

    /* extract IPK24 message header */
    uint8_t resp_mtype = buf[0];
    uint16_t resp_id = read_msgid(buf + 1);  // id (or ref_id for CONFIRM)

    /* mark outgoing message as confirmed or send CONFIRM */
    if (resp_mtype == MTYPE_CONFIRM) {
        logf(DEBUG, "outgoing message id=%hu confirmed", resp_id);
        udp_cnfm_confirm(resp_id, cnfm_data);
    } else {
        logf(DEBUG, "sending CONFIRM for id %hu", resp_id);
        char response_data[3] = { MTYPE_CONFIRM, 0, 0 };
        write_msgid(response_data + 1, resp_id);
        ssize_t sendto_result =
            sendto(conf->sockfd, response_data, 3, 0, sa, AS_SIZE);
        if (sendto_result == -1) {
            perror("sendto failed");
            log(ERROR, "sendto failed");
            rc = ERR_INTERNAL;
            break;
        }
    }

    /* try to print the message or indicate a bad format */
    rc = udp_print_msg(buf, received_bytes);
    if (rc == ERR_INTERNAL) break;
    if (rc == UPM_BADFMT) {
        log(WARNING, "invalid format of incoming message");
        /* todo: a flag for invalid format, so that main thread sends ERR */
    }

    /* special case: save_port */
    if (save_port and resp_mtype == MTYPE_CONFIRM and resp_id == auth_msgid) {
        logf(DEBUG, "AUTH msg id=%hu, was confirmed from port %hu, "
            "changing conf->port", auth_msgid, respaddr_port);
        conf->port = respaddr_port;
        auth_msg_confirmed = true;
    }
    if (save_port and resp_mtype == MTYPE_REPLY) {
        /* todo: check ref_msgid of the reply */
        assert(received_bytes >= 7);
        logf(DEBUG, "got REPLY with ref_msgid=%hu", read_msgid(buf + 4));
        rc = buf[3] == 1 ? 0 : 1;  // success/failure
        got_reply = true;
    }
    if(save_port and auth_msg_confirmed and got_reply) {
        /* todo: document what happens when AUTH message is confirmed but no REPLY comes */
        break;
    }

    /* case: server sent BYE */
    if (resp_mtype == MTYPE_BYE) {
        /* todo: confirm all unconfirmed messages, so that the sender thread(s) stop spamming (listener got BYE so it wont be listening for confirms anymore) */
        log(DEBUG, "listener received BYE, so it's stopping...");
        mtx_lock(mtx);
        *server_sent_bye = true;
        mtx_unlock(mtx);
        break;
    }

    /* case: server sent ERR */
    if (resp_mtype == MTYPE_ERR) {
        /* todo: send bye when received ERR */
        break;
    }

    /* todo: keep track of seen messages */

    }  // while true

    /* get rid of data (all is extracted) */
    mfree(buf); buf = NULL;
    mfree(sa);

    /* set the flag so main thread knows listener is finished */
    mtx_lock(mtx);
    *done_flag = true;
    mtx_unlock(mtx);

    log(DEBUG, "listener done...");

    return rc;

}

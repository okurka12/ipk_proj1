/*****************
**  Vit Pavlik  **
**   xpavli0a   **
**    251301    **
**              **
**   Created:   **
**  2024-02-20  **
**              **
**    Edited:   **
**  2024-02-20  **
*****************/

/**
 * main module for ipk24chat-client
*/

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>  // free, srand
#include <signal.h>  // register interrupt handler
#include <time.h>  // something to put in srand
#include <unistd.h>  // close


#include "ipk24chat.h"
#include "udpcl.h"
#include "utils.h"
#include "argparse.h"
#include "gexit.h"
#include "udp_confirmer.h"
#include "udp_listener.h"
#include "udp_sender.h"
#include "sleep_ms.h"

mtx_t gcl;

void handle_interrupt(int sig) {
    (void)sig;
    gexit(GE_TERMINATE, NULL);
}

char *mtype_str(uint8_t mtype) {
    switch (mtype) {
        case 0x00: return "CONFIRM";
        case 0x01: return "REPLY";
        case 0x02: return "AUTH";
        case 0x03: return "JOIN";
        case 0x04: return "MSG";
        case 0xFE: return "ERR";
        case 0xFF: return "BYE";
    }
    return "unknown";
}

/* send message and check return code (put it into `int *rcp`) */
#define smchrc(msgp, confp, rcp) \
do { \
    *(rcp) = udp_send_msg(msgp, confp); \
    if(*(rcp) != 0) { \
        log(ERROR, "couldn't send message"); \
    } \
} while (0)


/* todo: move udp_main to udpcl.c
calls functions from `udpcl` module */
int main_udp(conf_t *conf) {

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
    if (rc != 0) { log(ERROR, "couldn't create socket"); return 1; }
    rc = udp_set_rcvtimeo(conf, LISTENER_TIMEOUT);
    if (rc != 0) { log(ERROR, "couldn't set timeout on socket"); return 1; }

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
        return 1;
    }

    /* send first AUTH message */
    rc = udp_sender_send(&first_msg, conf, &cnfm_data);
    if (rc != 0) { log(ERROR, "couldn't send"); return 1; }


    /* wait for the listener thread */
    log(DEBUG, "waiting for listener thread");
    thrd_join(listener_thread_id, NULL);

    logf(INFO, "REPLY came from port %hu", conf->port);

    /* start listener again, but now for real */
    listener_args.save_port = false;
    rc = thrd_create(&listener_thread_id, udp_listener, &listener_args);
    if (rc != thrd_success) {
        log(ERROR, "couldn't create listener thread");
        return 1;
    }

    /* sleep before sending msg2 */
    // sleep_ms(400);

    /* now that listener is started and will be confirming messages,
    we can send messages */
    // rc = udp_sender_send(&msg2, conf, &cnfm_data);
    // if (rc != 0) { log(ERROR, "couldn't send"); return 1; }
    // sleep_ms(400);
    rc = udp_sender_send(&last_msg, conf, &cnfm_data);
    if (rc != 0) { log(ERROR, "couldn't send"); return 1; }



    /* let listener finish */
    mtx_unlock(&listener_mtx);

    /* wait for it to finish */
    log(DEBUG, "waiting for listener thread");
    thrd_join(listener_thread_id, NULL);

    /* cleanup */
    mtx_destroy(&listener_mtx);

    log(INFO, "udp client done");
    return rc;
}


int main(int argc, char *argv[]) {

    if (mtx_init(&gcl, mtx_plain) == thrd_error) {
        log(FATAL, "couldnt initialize global lock");
        return 1;
    }

    /* needed for testing (see `mmal.c`), normally it has no effect */
    srand(time(NULL));
    logf(DEBUG, "random number: %d", rand());

    /* register the interrupt handler */
    signal(SIGINT, handle_interrupt);

    /* return code */
    int rc = 0;

    conf_t conf = { .addr = NULL, .sockfd = -1 };

    if (not args_ok(argc, argv, &conf)) {
        log(ERROR, "bad arguments (or no memory?)");
        free(conf.addr);
        return ERR_BAD_ARG;
    }

    if (conf.should_print_help) {
        printf(USAGE LF);
        printf(HELP_TXT LF);
        free(conf.addr);
        return 0;
    }

    if (conf.tp == UDP) {
        rc = main_udp(&conf);
    } else {
        log(FATAL, "tcp version not implemented yet");
    }

    /* cleanup */
    gexit(GE_UNSET_FD, NULL);
    close(conf.sockfd);
    gexit(GE_FREE_RES, NULL);
    free(conf.addr);

    return rc;
}
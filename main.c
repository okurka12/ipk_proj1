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


#include "ipk24chat.h"
#include "udpcl.h"
#include "utils.h"
#include "argparse.h"
#include "gexit.h"

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
    return "";
}

/* send message and check return code (put it into `int *rcp`) */
#define smchrc(msgp, confp, rcp) \
do { \
    *(rcp) = udp_send_msg(&msg, &conf); \
    if(*(rcp) != 0) { \
        log(ERROR, "couldn't send message"); \
    } \
} while (0)


int main(int argc, char *argv[]) {

    /* needed for testing (see `mmal.c`) otherwise it has no effect */
    srand(time(NULL));
    logf(DEBUG, "random number: %d\n", rand());

    /* register the interrupt handler */
    signal(SIGINT, handle_interrupt);

    /* return code */
    int rc = 0;

    conf_t conf;

    if (not args_ok(argc, argv, &conf)) {
        free(conf.addr);
        return ERR_BAD_ARG;
    }
    if (conf.should_print_help) {
        printf(USAGE LF);
        printf(HELP_TXT LF);
        free(conf.addr);
        return 0;
    }

    msg_t msg = {
        .type = MTYPE_MSG, .id = 1,
        .dname = "vita",
        .content = "Hello, I am client."
    };
    if (conf.tp == UDP) {
        smchrc(&msg, &conf, &rc);
    } else {
        log(FATAL, "tcp version not implemented yet");
    }

    /* cleanup */
    gexit(GE_FREE_RES, NULL);
    free(conf.addr);

    return rc;
}
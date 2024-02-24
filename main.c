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
#include <stdlib.h>  // free
#include <signal.h>  // register interrupt handler


#include "ipk24chat.h"
#include "udpcl.h"
#include "utils.h"
#include "argparse.h"
#include "gexit.h"

void handle_interrupt(int sig) {
    (void)sig;
    gexit(GE_TERMINATE, NULL);
}

int main(int argc, char *argv[]) {

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

    char *msg_text = "Hello, I am client.";
    msg_t msg = { .type = MSG, .id = 1, .content = msg_text };

    if (conf.tp == UDP) {
        rc = udp_send_msg(&msg, &conf);
        if(rc != 0) {
            log(ERROR, "couldn't send message");
        }
    } else {
        log(FATAL, "tcp version not implemented yet");
    }

    /* cleanup */
    gexit(GE_FREE_RES, NULL);
    free(conf.addr);

    return rc;
}
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


#include "ipk24chat.h"
#include "udpcl.h"
#include "utils.h"
#include "argparse.h"

int main(int argc, char *argv[]) {

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


    // char *ip = "127.0.0.1";  // localhost
    // char *ip = "192.168.1.73";  // oslavany debian12vita local

    char *msg_text = "Hello, I am client.";
    msg_t msg = { .type = MSG, .id = 1, .content = msg_text };
    udp_send_msg(&msg, &conf);

    free(conf.addr);
}
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


#include "ipk24chat.h"
#include "udpcl.h"
#include "utils.h"

bool is_opt(char *s) {
    return s[0] == '-';
}


/**
 * Checks and parses args (initializes `conf`)
 * @param argc argument count
 * @param argv arguments
 * @param conf configuration structure to initialize
 * @return 1 if args are ok and 0 if they're not
*/
bool args_ok(int argc, char *argv[], conf_t *conf) {

    /* default values */
    conf->tp = NOT_SPECIFIED;
    conf->addr = NULL;
    conf->port = DEFAULT_PORT;
    conf->timeout = 250;
    conf->retries = DEFAULT_RETRIES;
    conf->should_print_help = false;
    
    if (argc < 2) {
        fprintf(stderr, "Too few arguments\n");
        fprintf(stderr, USAGE LF);
        return false;
    }
}

int main(int argc, char *argv[]) {

    conf_t conf;

    if (not args_ok(argc, argv, &conf)) {
        return ERR_BAD_ARG;
    }
    if (conf.should_print_help) {
        printf(USAGE LF);
        printf(HELP_TXT LF);
        return 0;
    }

    char *msg_text = "Hello, I am client.";

    char *ip = "127.0.0.1";  // localhost
    // char *ip = "192.168.1.73";  // oslavany debian12vita local
    uint16_t port = 4567;

    addr_t addr = { .addr = conf.addr, .port = conf.port };
    addr.addr = ip;
    addr.port = port;
    msg_t msg = { .type = MSG, .id = 1, .content = msg_text };

    udp_send_msg(&addr, &msg, &conf);
}
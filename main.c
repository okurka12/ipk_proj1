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
 * Checks and parses args (initializes `addr`)
 * @param argc argument count
 * @param argv arguments
 * @param addr address struct to initialize
 * @param help - set to true if there was -h option
 * @return 1 if args are ok and 0 if they're not
*/
bool args_ok(int argc, char *argv[], addr_t *addr, bool *help) {
    if (argc < 2) {
        fprintf(stderr, "Too few arguments\n");
        fprintf(stderr, USAGE LF);
        return false;
    }
}

int main(int argc, char *argv[]) {

    addr_t addr = { .addr = NULL, .port = 0 };
    bool should_print_help = false;

    if (not args_ok(argc, argv, &addr, &should_print_help)) {
        return ERR_BAD_ARG;
    }
    if (should_print_help) {
        printf(USAGE LF);
        printf(HELP_TXT LF);
    }

    char *msg_text = "Hello, I am client.";

    char *ip = "127.0.0.1";  // localhost
    // char *ip = "192.168.1.73";  // oslavany debian12vita local
    uint16_t port = 4567;

    addr.addr = ip;
    addr.port = port;
    msg_t msg = { .type = MSG, .id = 1, .content = msg_text };
    udp_conf_t conf = { .r = 3, .t = 250 };

    udp_send_msg(&addr, &msg, &conf);
}
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
 * argparse: module to check + parse arguments
*/

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "argparse.h"
#include "ipk24chat.h"
#include "utils.h"

/**
 * Private: states for the argument parsing
*/
enum parse_state { INIT, OPTION_T, OPTION_S, OPTION_D, OPTION_R, OPTION_H };

/**
 * returns a position in `s` where the first non-whitespace character is found
 * or NULL if there's no non-whitespace character
*/
char *next_nonws(char *s) {
    if (not isspace(*s)) return s;

    unsigned int i = 0;
    for (i = 0; s[i] != '\0'; i++) {
        if (not isspace(s[i])) {
            break;
        }
    }
    if (i != strlen(s)) {
        return s + i;
    } else {
        return NULL;
    }
}

bool is_opt(char *s) {
    return s[0] == '-';
}

void addr_from_conf(addr_t *addr, const conf_t *conf) {
    addr->addr = conf->addr;
    addr->port = conf->port;
}


/**
 * processes single arg
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

    (void)argv;
    log(ERROR, "not implemented yet");
    return false;
}
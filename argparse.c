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
#include <stdlib.h>

#include <unistd.h>  // getopt
#include <getopt.h>
extern char *optarg;

#include "argparse.h"
#include "ipk24chat.h"
#include "utils.h"

char *strdup(const char *s) {
    unsigned int length = strlen(s) + 1;
    char *output = malloc(length);
    if (output == NULL) {
        log(ERROR, MEMFAIL_MSG);
        return NULL;
    }
    strcpy(output, s);
    return output;
}


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

    bool t_specified = false;
    bool s_specified = false;
    bool p_specified = false;
    bool d_specified = false;
    bool r_specified = false;
    bool h_specified = false;

    int c;
    while ((c = getopt(argc, argv, "t:s:p:d:r:h")) != -1) {
        switch (c) {

        case 't':
            t_specified = true;
            if (not are_equal(optarg, "tcp") and not are_equal(optarg, "udp")) {
                fprintf(stderr, "invalid transport protocol: %s\n", optarg);
                return false;
            }
            conf->tp = are_equal(optarg, "udp") ? UDP : TCP;
            logf(INFO, "parsed transport protocol: %s\n", optarg);
            break;

        case 's':
            s_specified = true;
            conf->addr = strdup(optarg);
            logf(INFO, "parsed address: %s", conf->addr);
            break;

        case 'p':
            p_specified = true;
            if (sscanf(optarg, "%hu", &conf->port) != 1) {
                fprintf(stderr, "invalid port '%s'\n", optarg);
                return false;
            }
            logf(INFO, "parsed port: %hu", conf->port);
            break;

        case 'd':
            d_specified = true;
            if (sscanf(optarg, "%u", &conf->timeout) != 1) {
                fprintf(stderr, "invalid timeout: %s\n", optarg);
            }
            logf(INFO, "parsed timeout: %s\n", optarg);
            break;
        case 'r':
            r_specified = true;
            if (sscanf(optarg, "%u", &conf->retries) != 1) {
                fprintf(stderr, "invalid number of retries: %s\n", optarg);
            }
            logf(INFO, "parsed number of retries: %u", conf->retries);
            break;

        case 'h':
            h_specified = true;
            conf->should_print_help = true;
            log(INFO, "parsed -h option");
            break;

        default:
            break;
        }
    }

    (void)p_specified;
    (void)d_specified;
    (void)r_specified;
    (void)h_specified;

    if (not t_specified or not s_specified) {
        log(ERROR, "server or transport protocol not specified");
        return false;
    }

    return true;
}
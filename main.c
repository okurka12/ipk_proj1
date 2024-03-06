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

/* for getaddrinfo */
#define _POSIX_C_SOURCE 200112L

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>  // free, srand
#include <signal.h>  // register interrupt handler
#include <time.h>  // something to put in srand
#include <unistd.h>  // close
#include <netdb.h>  // getaddrinfo
#include <netinet/in.h>  // struct sockaddr_in
#include <arpa/inet.h>  // inet_ntoa


#include "ipk24chat.h"
#include "udpcl.h"
#include "utils.h"
#include "argparse.h"
#include "gexit.h"
#include "udp_confirmer.h"
#include "udp_listener.h"
#include "udp_sender.h"
#include "sleep_ms.h"
#include "shell.h"
#include "mmal.h"

mtx_t gcl;

/* changes conf->addr from hostname to address */
int resolve_hostname(conf_t *conf) {

    /* getaddrinfo stuff */
    struct sockaddr_in s;
    struct addrinfo hints, *result;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;  // IPv4
    hints.ai_socktype = SOCK_DGRAM;  // UDP
    if (getaddrinfo(conf->addr, NULL, &hints, &result) != 0) {
        perror("getaddrinfo");
        logf(ERROR, "getaddrinfo couldn't resolve hostname '%s'", conf->addr);
        return 1;
    }

    /* use the first address (result is a linked list) */
    memcpy(&s, result->ai_addr, result->ai_addrlen);

    /* ptr to statically allocated address from inet_ntoa */
    char *addrp = inet_ntoa(s.sin_addr);

    /* copy it and overwrite conf->addr */
    char *new_addr = mmal(strlen(addrp) + 1);
    strcpy(new_addr, addrp);
    freeaddrinfo(result);
    logf(DEBUG, "converted %s to %s", conf->addr, new_addr);
    mfree(conf->addr);
    conf->addr = new_addr;

    return 0;
}

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


int main(int argc, char *argv[]) {

    /* initialize the global lock */
    if (mtx_init(&gcl, mtx_recursive) == thrd_error) {
        log(FATAL, "couldnt initialize global lock");
        return 1;
    }

    /* register the interrupt handler */
    if (signal(SIGINT, handle_interrupt) == SIG_ERR) {
        log(FATAL, "couldnt register signal handler");
        return 1;
    }

    /* needed for testing (see `mmal.c`), normally it has no effect */
    srand(time(NULL));
    logf(DEBUG, "random number: %d", rand());

    /* return code */
    int rc = 0;

    /* important: initialize configuration structure */
    conf_t conf = { .addr = NULL, .sockfd = -1, .dname = NULL };

    if (not args_ok(argc, argv, &conf)) {
        log(ERROR, "bad arguments (or no memory?)");
        rc = ERR_BAD_ARG;
        goto cleanup;
    }
    if (conf.should_print_help) {
        printf(USAGE LF);
        printf(HELP_TXT LF);
        free(conf.addr);
        return 0;
    }

    if (resolve_hostname(&conf) != 0) {
        logf(ERROR, "couldn't resolve host %s", conf.addr);
        rc = ERR_HOSTNAME;
        goto cleanup;
    }

    if (conf.tp == UDP) {
        rc = udp_main(&conf);
    } else {
        log(FATAL, "tcp version not implemented yet");
    }

    cleanup:
    log(DEBUG, "cleaning up");
    gexit(GE_FREE_RES, NULL);
    free(conf.addr);

    return rc;
}
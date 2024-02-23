/*****************
**  Vit Pavlik  **
**   xpavli0a   **
**    251301    **
**              **
**   Created:   **
**  2024-02-18  **
**              **
**    Edited:   **
**  2024-02-19  **
*****************/

/**
 *
 * implementation of the `gexit` - gracefully exit module
 * see gexit.h
 *
 */

#include <unistd.h>  // close
#include <stdlib.h>  // free
#include <stdbool.h>


// these wont be necessary ig
// #include <sys/types.h>
// #include <sys/socket.h>
// #include <sys/time.h>  // struct timeval
// #include <netinet/in.h>  // struct sockaddr_in
// #include <arpa/inet.h>  // htons

#include "ipk24chat.h"
#include "gexit.h"
#include "utils.h"

void gexit(enum gexit_statement statement, void *p) {
    static int sockfd = -1;
    static conf_t *confp = NULL;

    /* todo: remove this */
    (void)sockfd;
    (void)confp;

    switch (statement) {

    case GE_SET_CONFP:
        confp = (conf_t *)p;
        break;

    case GE_TERMINATE:
        log(INFO, "the program was interrupted");
        break;

    /* todo: all the other cases */

    default:
        /* todo*/
        break;
    }
}

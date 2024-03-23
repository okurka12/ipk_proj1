/*****************
**  Vit Pavlik  **
**   xpavli0a   **
**    251301    **
**              **
**   Created:   **
**  2024-03-18  **
**              **
**    Edited:   **
**  2024-03-18  **
*****************/

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "ipk24chat.h"
#include "mmal.h"
#include "msg.h"
#include "utils.h"

#define TCP_RENDER_BUFSIZE 8192

/* check the snprintf return value */
#define warn(snprintf_rc, bufr_len) \
if (snprintf_rc == bufr_len) { \
    log(WARNING, "message was truncated"); \
}

char *tcp_render(const msg_t *msg) {
    int rc = 0;

    char *output = mmal(TCP_RENDER_BUFSIZE);
    if (output == NULL) {
        return NULL;
    }

    switch (msg->type) {
        case MTYPE_AUTH:
            assert(msg->username != NULL);
            assert(msg->dname != NULL);
            assert(msg->secret != NULL);
            rc = snprintf(output, TCP_RENDER_BUFSIZE,
                "aUtH %s aS %s uSinG %s\r\n",
                msg->username, msg->dname, msg->secret);
            warn(rc, TCP_RENDER_BUFSIZE);

            break;

        case MTYPE_JOIN:
            assert(msg->dname != NULL);
            assert(msg->chid != NULL);
            rc = snprintf(output, TCP_RENDER_BUFSIZE, "jOIn %s aS %s\r\n",
                msg->chid, msg->dname);
            warn(rc, TCP_RENDER_BUFSIZE);
            break;

        case MTYPE_MSG:
            assert(msg->content != NULL);
            assert(msg->dname != NULL);
            rc = snprintf(output, TCP_RENDER_BUFSIZE, "msG FRoM %s iS %s\r\n",
                msg->dname, msg->content);
            warn(rc, TCP_RENDER_BUFSIZE);
            break;

        case MTYPE_BYE:
            strcpy(output, "bYe\r\n");
            break;

        default:
            logf(WARNING, "unhandled type %s", mtype_str(msg->type));
            break;
    }


    return output;
}
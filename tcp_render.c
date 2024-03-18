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

#include <stdio.h>
#include <string.h>
#include "ipk24chat.h"
#include "mmal.h"
#include "msg.h"
#include "utils.h"

#define TCP_RENDER_BUFSIZE 8192

char *tcp_render(const msg_t *msg) {
    int rc = 0;

    char *output = mmal(TCP_RENDER_BUFSIZE);
    if (output == NULL) {
        return NULL;
    }

    switch (msg->type) {
        case MTYPE_AUTH:
            rc = snprintf(output, TCP_RENDER_BUFSIZE,
            "AUTH %s AS %s USING %s\r\n",
            msg->username, msg->dname, msg->secret);
            if (rc == TCP_RENDER_BUFSIZE) {
                log(WARNING, "message was truncated");
            }
            break;
    }


    return output;
}
/*****************
**  Vit Pavlik  **
**   xpavli0a   **
**    251301    **
**              **
**   Created:   **
**  2024-02-25  **
**              **
**    Edited:   **
**  2024-02-25  **
*****************/

#include <string.h>

#include "udp_render.h"
#include "mmal.h"
#include "ipk24chat.h"  // msg_t
#include "rwmsgid.h"  // write_msgid
#include "utils.h"


char *udp_render_message(msg_t *msg, unsigned int *length) {

    char *output = NULL;
    *length = 0;

    switch (msg->type)
    {
    case MTYPE_MSG:

        *length =
            1 +                          // message type
            2 +                          // message id
            strlen(msg->dname) + 1 +     // display name
            strlen(msg->content) + 1;    // message content

        output = mmal(*length);
        if (output == NULL) {
            perror(MEMFAIL_MSG);
            log(ERROR, MEMFAIL_MSG);
            return NULL;
        }

        /* write msg type*/
        output[0] = msg->type;

        /* write msg id */
        write_msgid(output + 1, msg->id);

        /* write dname */
        strcpy(output + 3, msg->dname);

        /* write msg content */
        strcpy(output + 3 + strlen(msg->dname) + 1, msg->content);

        break;

    default:
        logf(ERROR, "unhandled message type %hhx", msg->type);
        break;
    }

    return output;
}
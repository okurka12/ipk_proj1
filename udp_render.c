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

/* allocates `length` to `iden` and checks for NULL */
#define allocate(iden, length) \
do { \
    iden = mmal(length); \
    if (iden == NULL) { \
        perror(MEMFAIL_MSG); \
        log(ERROR, MEMFAIL_MSG); \
        return NULL; \
        } \
} while (0)


char *udp_render_message(msg_t *msg, unsigned int *length) {

    char *output = NULL;
    *length = 0;

    switch (msg->type) {

    case MTYPE_MSG: case MTYPE_ERR:
        *length = 1 + 2 + strlen(msg->dname) + 1 +strlen(msg->content) + 1;
        allocate(output, *length);
        output[0] = msg->type;
        write_msgid(output + 1, msg->id);
        strcpy(output + 3, msg->dname);
        strcpy(output + 3 + strlen(msg->dname) + 1, msg->content);

        break;

    case MTYPE_CONFIRM:
        *length = 1 + 2;
        allocate(output, *length);
        output[0] = msg->type;
        write_msgid(output + 1, msg->ref_msgid);
        break;

    case MTYPE_REPLY:
        *length = 1 + 2 + 1 + 2 + strlen(msg->content) + 1;
        allocate(output, *length);
        output[0] = msg->type;
        write_msgid(output + 1, msg->id);
        output[3] = msg->result;
        write_msgid(output + 4, msg->ref_msgid);
        strcpy(output + 6, msg->content);
        break;

    case MTYPE_AUTH:
        *length = 1 + 2 + strlen(msg->username) + 1 + strlen(msg->dname) + 1 +
            strlen(msg->secret) + 1;
        allocate(output, *length);
        output[0] = msg->type;
        write_msgid(output + 1, msg->id);
        strcpy(output + 3, msg->username);
        strcpy(output + 3 + strlen(msg->username) + 1, msg->dname);
        strcpy(output + 3 + strlen(msg->username) + 1 + strlen(msg->dname) + 1,
            msg->secret);
        break;

    case MTYPE_JOIN:
        *length = 1 + 2 + strlen(msg->chid) + 1 + strlen(msg->dname) + 1;
        allocate(output, *length);
        output[0] = msg->type;
        write_msgid(output + 1, msg->id);
        strcpy(output + 3, msg->chid);
        strcpy(output + 3 + strlen(msg->chid) + 1, msg->dname);
        break;

    case MTYPE_BYE:
        *length = 1 + 2;
        allocate(output, *length);
        output[0] = msg->type;
        write_msgid(output + 1, msg->id);
        break;

    default:
        logf(ERROR, "unhandled message type %hhx", msg->type);
        break;
    }

    return output;
}
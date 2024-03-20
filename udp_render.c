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

#include <assert.h>
#include <string.h>

#include "udp_render.h"
#include "mmal.h"
#include "ipk24chat.h"  // message types 
#include "rwmsgid.h"  // write_msgid
#include "utils.h"
#include "msg.h"  // msg_t

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


char *udp_render_message(const msg_t *msg, unsigned int *length) {

    char *output = NULL;
    *length = 0;

    switch (msg->type) {

    case MTYPE_MSG: case MTYPE_ERR:
        assert(msg->dname != NULL);
        assert(msg->content != NULL);

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

    /* client won't be sending REPLY, but here it is anyways */
    case MTYPE_REPLY:
        log(WARNING, "client sending reply? :hmm:");
        assert(msg->content != NULL);

        *length = 1 + 2 + 1 + 2 + strlen(msg->content) + 1;
        allocate(output, *length);
        output[0] = msg->type;
        write_msgid(output + 1, msg->id);
        output[3] = msg->result;
        write_msgid(output + 4, msg->ref_msgid);
        strcpy(output + 6, msg->content);
        break;

    case MTYPE_AUTH:
        assert(msg->username != NULL);
        assert(msg->secret != NULL);
        assert(msg->dname != NULL);

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
        assert(msg->chid != NULL);
        assert(msg->dname !=  NULL);

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
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

    /* note: strlen(CRLF) is 2 */

    /*        header, content,               null byte */
    *length = 1 + 2 + strlen(msg->content) + 1;
    char *output = mmal(*length);
    if (output == NULL) {
        perror(MEMFAIL_MSG);
        log(ERROR, MEMFAIL_MSG);
        return NULL;
    }

    /* write msg type*/
    output[0] = msg->type;

    /* write msg id */
    write_msgid(output + 1, msg->id);

    /* write msg content */
    strcpy(output + 3, msg->content);

    /* write null byte at the end */
    output[*length - 1] = '\0';

    return output;
}
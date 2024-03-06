/*****************
**  Vit Pavlik  **
**   xpavli0a   **
**    251301    **
**              **
**   Created:   **
**  2024-03-04  **
**              **
**    Edited:   **
**  2024-03-04  **
*****************/

#include "msg.h"
#include "mmal.h"
#include "utils.h"

msg_t *msg_ctor(void) {
    msg_t *p = mmal(sizeof(msg_t));
    if (p == NULL) { log(ERROR, MEMFAIL_MSG); return NULL; }

    p->id = 0;
    p->ref_msgid = 0;
    p->result = 0;
    p->type = 0;

    /* char * */
    p->secret = NULL;
    p->username = NULL;
    p->chid = NULL;
    p->content = NULL;
    p->dname = NULL;

    return p;
}


void msg_dtor(msg_t *p) {
    if (p->secret != NULL) mfree(p->secret);
    if (p->username != NULL) mfree(p->username);
    if (p->chid != NULL) mfree(p->chid);
    if (p->content != NULL) mfree(p->content);
    if (p->dname != NULL) mfree(p->dname);
    mfree(p);
}

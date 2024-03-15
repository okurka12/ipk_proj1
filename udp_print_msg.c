/*****************
**  Vit Pavlik  **
**   xpavli0a   **
**    251301    **
**              **
**   Created:   **
**  2024-03-10  **
**              **
**    Edited:   **
**  2024-03-10  **
*****************/

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdbool.h>

#include "ipk24chat.h"
#include "udp_print_msg.h"
#include "utils.h"
#include "mmal.h"

/* a buffer size for processing individual message fields */
static const unsigned int upm_bufsize = 70000;

/**
 * checks if a string contains only printable characters
 * @return `false` if `s` contains unprintable characters else `true`
 * @note uses `ctype.h`'s `isprint`
*/
bool str_isprint(const char *s) {
    for (unsigned int i = 0; s[i] != '\0'; i++) {
        if (not isprint(s[i])) {
            return false;
        }
    }
    return true;
}

/**
 * same as str_isprint, but exclude space character
*/
static bool str_isprint_nosp(char *s) {
    for (unsigned int i = 0; s[i] != '\0'; i++) {
        if (not isprint(s[i]) or s[i] == ' ') {
            return false;
        }
    }
    return true;
}


static int print_reply(char *msg, unsigned int msglen) {

    /* 7 is the minimal length of REPLY message according to specification */
    if (msglen < 7) return UPM_BADFMT;
    if (msglen > upm_bufsize) {
        logf(ERROR, "called with suspiciously large msglen=%u", msglen);
        return ERR_INTERNAL;
    }

    /* check the reply result */
    uint8_t reply_res = msg[3];
    if (reply_res > 1) return UPM_BADFMT;

    /* allocate zeroed buffer */
    char *buf = mcal(upm_bufsize, 1);
    if (buf == NULL) { log(ERROR, MEMFAIL_MSG); return ERR_INTERNAL; }

    /* 6 is the start of the messagecontents field */
    strncpy(buf, msg + 6, msglen - 6);

    /* now `buf` contains a safe string that is definitely null terminated */

    /* check for non-printable characters */
    if (not str_isprint(buf)) {
        mfree(buf);
        return UPM_BADFMT;
    }

    fprintf(stderr, "%s: %s\n", reply_res ? "Success" : "Failure", buf);

    mfree(buf);
    return 0;
}

static int print_msg_err(char *msg, unsigned int msglen) {

    if (msglen == 0) return 0;
    if (msglen < 5) return UPM_BADFMT;
    if (msglen > upm_bufsize) {
        logf(ERROR, "called with suspiciously large msglen=%u", msglen);
        return ERR_INTERNAL;
    }

    /* allocate buffers */
    char *dname = mcal(upm_bufsize, 1);
    if (dname == NULL) {
        log(ERROR, MEMFAIL_MSG);
        return ERR_INTERNAL;
    }
    char *content = mcal(upm_bufsize, 1);
    if (content == NULL) {
        mfree(dname);
        log(ERROR, MEMFAIL_MSG);
        return ERR_INTERNAL;
    }

    /* copy fields safely */
    strncpy(dname, msg + 3, msglen - 3);
    unsigned int content_pos = 3 + strlen(dname) + 1;
    if (content_pos > msglen) {
        mfree(dname);
        mfree(content);
        return UPM_BADFMT;
    }
    strncpy(content, msg + content_pos, msglen - content_pos);

    /* check characters */
    bool dname_ok = false;
    bool content_ok = false;
    dname_ok = str_isprint_nosp(dname);
    content_ok = str_isprint(content);
    if (not dname_ok or not content_ok) {
        mfree(dname);
        mfree(content);
        return UPM_BADFMT;
    }

    /* print */
    if (msg[0] == MTYPE_MSG) {
        printf("%s: %s\n", dname, content);
    } else {
        fprintf(stderr, "ERR FROM %s: %s", dname, content);
    }

    mfree(dname);
    mfree(content);
    return 0;
}

int udp_print_msg(char *msg, unsigned int msglen) {

    static_assert(ERR_INTERNAL != UPM_BADFMT, "bad values, couldn't "
        "distinguish bad format and internal error");

    if (msglen == 0) return 0;

    uint8_t mtype = msg[0];

    switch (mtype) {

    case MTYPE_REPLY:
        return print_reply(msg, msglen);

    case MTYPE_MSG: case MTYPE_ERR:
        return print_msg_err(msg, msglen);

    case MTYPE_AUTH: case MTYPE_JOIN: case MTYPE_BYE: case MTYPE_CONFIRM:
        return 0;

    }

    /* if we got here means the message was of an unknown type */
    return UPM_BADFMT;
}

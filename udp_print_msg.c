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
static const unsigned int upm_bufsize = 65600;

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

/* todo TODO: a function that has a static variable and returns a pointer to the buffer or frees it, ez, no 20000 allocs */

/**
 * if `freebuffer` is false, returns a pointer to a zeroed static buffer
 * of size `upm_bufsize` if it is true, frees the buffer and returns null
 *
 * it supports having two buffers at a time, numbered 0 and 1, chosen by
 * `bufid`, if `bufid` is out of bounds, returns null
*/
static char *upm_get_buffer(bool freebuffer, uint8_t bufid) {

    static char *bufs[] = { NULL, NULL };

    if (freebuffer) {
        mfree(bufs[0]);
        mfree(bufs[1]);
        bufs[0] = NULL;
        bufs[1] = NULL;
        return NULL;
    }

    if (bufid > 1) {
        log(WARNING, "invalid argumend `bufid`");
        return NULL;
    }

    /* zero the buffer and reurn it (allocate it) */
    if (bufs[bufid] != NULL) {
        memset(bufs[bufid], 0, upm_bufsize);
        return bufs[bufid];
    } else {
        bufs[bufid] = mmal(upm_bufsize);
        if (bufs[bufid] == NULL) {
            return NULL;
        }
        memset(bufs[bufid], 0, upm_bufsize);
        return bufs[bufid];
    }
}

void udp_free_printer_resources() {
    upm_get_buffer(true, 0);
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
    char *buf = upm_get_buffer(false, 0);
    if (buf == NULL) { log(ERROR, MEMFAIL_MSG); return ERR_INTERNAL; }

    /* 6 is the start of the messagecontents field */
    strncpy(buf, msg + 6, msglen - 6);

    /* now `buf` contains a safe string that is definitely null terminated */

    /* check for non-printable characters */
    if (not str_isprint(buf)) {
        return UPM_BADFMT;
    }

    fprintf(stderr, "%s: %s\n", reply_res ? "Success" : "Failure", buf);

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
    char *dname = upm_get_buffer(false, 0);
    if (dname == NULL) {
        log(ERROR, MEMFAIL_MSG);
        return ERR_INTERNAL;
    }
    char *content = upm_get_buffer(false, 1);
    if (content == NULL) {
        log(ERROR, MEMFAIL_MSG);
        return ERR_INTERNAL;
    }

    /* copy fields safely */
    strncpy(dname, msg + 3, msglen - 3);
    unsigned int content_pos = 3 + strlen(dname) + 1;
    if (content_pos > msglen) {
        return UPM_BADFMT;
    }
    strncpy(content, msg + content_pos, msglen - content_pos);

    /* check characters */
    bool dname_ok = false;
    bool content_ok = false;
    dname_ok = str_isprint_nosp(dname);
    content_ok = str_isprint(content);
    if (not dname_ok or not content_ok) {
        return UPM_BADFMT;
    }

    /* print */
    if (msg[0] == MTYPE_MSG) {
        printf("%s: %s\n", dname, content);
    } else {
        fprintf(stderr, "ERR FROM %s: %s", dname, content);
    }

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

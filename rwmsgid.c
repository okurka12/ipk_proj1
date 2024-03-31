/*****************
**  Vit Pavlik  **
**   xpavli0a   **
**    251301    **
**              **
**   Created:   **
**  2024-02-19  **
*****************/

/**
 *
 * rwmsgid.c - read/write message id
 * uses GCC-specific macros __BYTE_ORDER__ and __ORDER_BIG_ENDIAN__
 *
 */
#include <stdint.h>
#include "rwmsgid.h"

void write_msgid(char *dst, uint16_t msgid) {

    if (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__) {
            dst[0] = ((char *)(&msgid))[0];
            dst[1] = ((char *)(&msgid))[1];
        } else {
            dst[0] = ((char *)(&msgid))[1];
            dst[1] = ((char *)(&msgid))[0];
    }
}

uint16_t read_msgid(char *src) {
    uint16_t output = 0;
    if (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__) {
        ((char *)&output)[0] = src[0];
        ((char *)&output)[1] = src[1];
    } else {
        ((char *)&output)[1] = src[0];
        ((char *)&output)[0] = src[1];
    }
    return output;
}
/*****************
**  Vit Pavlik  **
**   xpavli0a   **
**    251301    **
**              **
**   Created:   **
**  2024-02-19  **
**              **
**    Edited:   **
**  2024-02-19  **
*****************/

/**
 *
 * rwmsgid.c - read/write message id module API
 *
 */
#include <stdint.h>

#ifndef _R_W_M_S_G_I_D_
#define _R_W_M_S_G_I_D_

/**
 * Writes `msgid` in network byte order to `dst`
*/
void write_msgid(char *dst, uint16_t msgid);

/**
 * Reads 2 bytes from `src` in network byte order and returns integer
*/
uint16_t read_msgid(char *src);

#endif  // ifndef _R_W_M_S_G_I_D_
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

#include "ipk24chat.h"
#include "msg.h"  // msg_t

#ifndef _U_D_P_R_E_N_D_E_R_H_
#define _U_D_P_R_E_N_D_E_R_H_

/**
 * Returns a pointer to memory where the entire msg contained
 * in `msg` lies, returns length via `length`
 * @note dynamically allocated, needs to be freed
 * @return pointer to rendered `msg` or NULL on failure
*/
char *udp_render_message(const msg_t *msg, unsigned int *length);

#endif  // ifndef _U_D_P_R_E_N_D_E_R_H_

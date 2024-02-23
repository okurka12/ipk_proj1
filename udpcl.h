/*****************
**  Vit Pavlik  **
**   xpavli0a   **
**    251301    **
**              **
**   Created:   **
**  2024-02-18  **
**              **
**    Edited:   **
**  2024-02-19  **
*****************/

/* API for the UDP client udpcl.c */

#include <stdint.h>
#include "ipk24chat.h"

#ifndef _U_D_P_C_L_H_
#define _U_D_P_C_L_H_

/* buffer size for incoming reply (CONFIRM message) */
#define CONFIRM_BUFSIZE 512


/**
 * Sends AF_INET SOCK_DGRAM (IPv4 UDP) packet to `addr`
 * @param addr
 * @param msg
 * @param conf
 * @return 0 on success else 1
*/
int udp_send_msg(msg_t *msg, conf_t *conf);

#endif  // ifndef _U_D_P_C_L_H_

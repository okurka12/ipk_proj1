/*****************
**  Vit Pavlik  **
**   xpavli0a   **
**    251301    **
**              **
**   Created:   **
**  2024-02-18  **
**              **
**    Edited:   **
**  2024-02-18  **
*****************/

/* API for the UDP client udpcl.c */

#include <stdint.h>
#include "ipk24chat.h"

#ifndef _U_D_P_C_L_H_
#define _U_D_P_C_L_H_

/**
 * Sends AF_INET SOCK_DGRAM (IPv4 UDP) packet to `addr`
 * @param addr IPv4 address (eg. 127.0.0.1)
 * @param port port
 * @param data payload
 * @param length lengh of the data in bytes
 * @return 0 on success else 1
*/
int udp_send_msg(addr_t *addr, msg_t *msg);

#endif  // ifndef _U_D_P_C_L_H_
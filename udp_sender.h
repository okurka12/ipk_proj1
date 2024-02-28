/*****************
**  Vit Pavlik  **
**   xpavli0a   **
**    251301    **
**              **
**   Created:   **
**  2024-02-26  **
**              **
**    Edited:   **
**  2024-02-26  **
*****************/

/**
 * UDP Sender - spawn a thread that sends a message and waits for its
 * confirmation
*/

#include "ipk24chat.h"
#include "udp_confirmer.h"

#ifndef _U_D_P_S_E_N_D_E_R_H_
#define _U_D_P_S_E_N_D_E_R_H_

/* addres struct for sendto */
#define SSA struct sockaddr

/* size of the addres structure (`struct sockaddr_in`) */
#define AS_SIZE sizeof(struct sockaddr_in)

/**
 * Sends a message and waits for its confirmation
 * @return 0 if the message was successfully delivered or 1
*/
int udp_sender_send(const msg_t *msg, const conf_t *conf,
    udp_cnfm_data_t *cnfm_data);

/**
 * fill `struct sockaddr_in` from `addr`
 * used in `udp_send_msg`
 * @return a pointer to `struct sockaddr` (SSA) (but it actually
 * points to `struct sockaddr_in` of size AS_SIZE) on success, else NULL
 * @note dynamically allocated, needs to be freed
*/
SSA *udp_get_addrstruct(char *addr, uint16_t port);

#endif  // _U_D_P_S_E_N_D_E_R_H_

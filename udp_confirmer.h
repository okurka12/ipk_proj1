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
 * UDP confirmer - keeps track of unconfirmed messages and can confirm them
 *
 * The use of this module is that:
 * - UDP Sender thread logs the unconfirmed message via this module
 * - UDP Listener thread confirms the messages via this module
 * - UDP Sender then checks (via this module) if a given message was confirmed
 *   to indicate if the send operation was successful
 *
 * Functions in this module are to be prefixed with `udp_cnfm_`
 *
 * @note allocates resources, those can be freed with `udp_conf_free()`
 *
*/
#include <stdint.h>
#include <stdbool.h>

#ifndef _U_D_P_C_O_N_F_I_R_M_E_R_H_
#define _U_D_P_C_O_N_F_I_R_M_E_R_H_

/* base array length to initialize udp confirmer data */
#define UDP_CNFM_BASE_ARRLEN 1024

/**
 * a struct to be allocated on stack in main and passed as a referece to
 * function in module `udp_confirmer`
*/
typedef struct {
    uint16_t *arr;
    unsigned int len;
} udp_cnfm_data_t;

/**
 * Register a message to be confirmed
 * @return 0 on succes, else 1
*/
int udp_cnfm_reg(uint16_t id, udp_cnfm_data_t *data);

/**
 * confirm a message
*/
void udp_cnfm_confirm(uint16_t id, udp_cnfm_data_t *data);

/**
 * returns if a message was confirmed
*/
bool udp_cnfm_was_confirmed(uint16_t id, udp_cnfm_data_t *data);

#endif  // ifndef _U_D_P_C_O_N_F_I_R_M_E_R_H_

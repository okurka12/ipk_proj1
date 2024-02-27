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
 * UDP listener - a listener thread that prints incoming messages and
 * confirms messages via `udp_confirmer`
*/

#include <threads.h>
#include <stdbool.h>
#include "ipk24chat.h"
#include "udp_confirmer.h"

#ifndef _U_D_P_L_I_S_T_E_N_E_R_H_
#define _U_D_P_L_I_S_T_E_N_E_R_H_

/* timeout in ms */
#define LISTENER_TIMEOUT 150

/* buffer size for incoming messages */
#define RESPONSE_BUFSIZE 65535

typedef struct {
    conf_t *conf;
    udp_cnfm_data_t *cnfm_data;
    mtx_t *mtx;

    /* if this flag is specified, waits for the CONFIRM message for
    AUTH message of ID `auth_msg_id`, and when it receives that,
    returns immediately */
    bool save_port;
    uint16_t auth_msg_id;

} listener_args_t;

/**
 * Listens on a socket in `conf` (`conf->sockfd`),
 * confirms messages via `udp_confirmer`,
 * prints messages on stderr, stdout
 *
 * Cycles until it can lock `mtx`, until then it is being blocked
 * LISTENER_TIMEOUT ms at a time
 * @param args pointer to `listener_args_t`
 * @return 0 on succes 1 on error
*/
int udp_listener(void *args);

#endif  // ifndef _U_D_P_L_I_S_T_E_N_E_R_H_

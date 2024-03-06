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
#include "udp_confirmer.h"  // udp_cnfm_data_t

#ifndef _U_D_P_L_I_S_T_E_N_E_R_H_
#define _U_D_P_L_I_S_T_E_N_E_R_H_

/* timeout in ms */
#define LISTENER_TIMEOUT 150

/* buffer size for incoming messages */
#define RESPONSE_BUFSIZE 65535

typedef struct {

    /* todo: document when and what parts of `conf` does listener write to */
    conf_t *conf;

    udp_cnfm_data_t *cnfm_data;

    /* lock for `stop_flag` and `listener_done` */
    mtx_t *mtx;

    /* setting this flag means listener can stop looping and return */
    bool *stop_flag;

    /* listener sets this flag when its finished */
    bool *done_flag;

    /* if this flag is true, waits for the CONFIRM and REPLY, saves
    source port of the response and returns (`mtx` is not unlocked in this
    case) */
    bool save_port;
    uint16_t auth_msg_id;

} listener_args_t;

/**
 * Listens on a socket in `conf` (`conf->sockfd`),
 * confirms messages via `udp_confirmer`,
 * prints messages on stderr, stdout
 *
 * Cycles until the flag `stop_flag` is set, until then it is being blocked
 * LISTENER_TIMEOUT ms at a time
 *
 * Also stops looping when it receives BYE or ERR, in that case it sets
 * `done_flag`
 *
 * @param args pointer to `listener_args_t`
 * @return 0 on succes 1 on error
*/
int udp_listener(void *args);

#endif  // ifndef _U_D_P_L_I_S_T_E_N_E_R_H_

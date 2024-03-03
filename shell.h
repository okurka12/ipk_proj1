/*****************
**  Vit Pavlik  **
**   xpavli0a   **
**    251301    **
**              **
**   Created:   **
**  2024-03-03  **
**              **
**    Edited:   **
**  2024-03-03  **
*****************/

/**
 * a module that processes stdin
*/
#include "ipk24chat.h"  // conf_t

#ifndef _S_H_E_L_L_H_
#define _S_H_E_L_L_H_

/* shell FSM states */
enum sstate { SS_START, SS_AUTH, SS_OPEN, SS_ERROR, SS_END };

/* returns string repr. of the shell FSM state, (char *) */
#define sstate_str(iden) (\
(iden) == SS_START ? "SS_START" : \
(iden) == SS_AUTH ? "SS_AUTH" : \
(iden) == SS_OPEN ? "SS_OPEN" : \
(iden) == SS_ERROR ? "SS_ERROR" : \
(iden) == SS_END ? "SS_END" : "unknown")

int udp_shell(conf_t *conf);

#endif  // ifndef _S_H_E_L_L_H_
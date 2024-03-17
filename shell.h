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

/* while blocking for stdin, check if listener hasn't finished every this many
miliseconds */
#define SH_STDIN_TIMEOUT_MS 10  // todo: document this constant

/**
 * Starts a "shell". The shell starts a listener, sends messages. Returns
 * when reaching the end of user input or upon receiving BYE.
 * @return 0 on success else non-zero
*/
int udp_shell(conf_t *conf);

#endif  // ifndef _S_H_E_L_L_H_
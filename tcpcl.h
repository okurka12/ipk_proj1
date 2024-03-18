/*****************
**  Vit Pavlik  **
**   xpavli0a   **
**    251301    **
**              **
**   Created:   **
**  2024-03-17  **
**              **
**    Edited:   **
**  2024-03-17  **
*****************/

#include "ipk24chat.h"
#include "msg.h"

#ifndef _T_C_P_C_L_H_
#define _T_C_P_C_L_H_

int tcp_main(conf_t *conf);

int tcp_send(conf_t *conf, const msg_t *msg);


#endif  // ifndef _T_C_P_C_L_H_

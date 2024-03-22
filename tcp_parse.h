/*****************
**  Vit Pavlik  **
**   xpavli0a   **
**    251301    **
**              **
**   Created:   **
**  2024-03-21  **
**              **
**    Edited:   **
**  2024-03-21  **
*****************/

#include <stdbool.h>

#ifndef _T_C_P_P_A_R_S_E_H_
#define _T_C_P_P_A_R_S_E_H_

/**
 * Parses reply
 * @return reply success, sets `content` to the position in `data` to where
 * the content starts, if the message is invalid, sets
 * `content` to NULL and returns false, if an internal error occurs, sets
 * `content` to NULL, returns false and sets `err` to true
 * @note it also changes the CR character in `data` to NULL
*/
bool tcp_parse_reply(char *data, char **content, bool *err);

#endif  // ifndef _T_C_P_P_A_R_S_E_H_

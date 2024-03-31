/*****************
**  Vit Pavlik  **
**   xpavli0a   **
**    251301    **
**              **
**   Created:   **
**  2024-03-16  **
*****************/

/**
 * UDP marker - mark messages as seen, so if the come twice we can discard
 * the second one
*/

#include <stdint.h>
#include <stdbool.h>

#ifndef _U_D_P_M_A_R_K_E_R_H_
#define _U_D_P_M_A_R_K_E_R_H_

/* there can be no more unique msgids than this */
#define UDPM_MAX_SEEN 65536


/* UDP marker statement */
enum udpm_st {
    UDPM_MARK
};

/**
 * mark message as seen
 * @return 0 on success else non-zero
 * @note if this function is called, `udpm_free_res` needs to be called at the
 * end of the program
*/
int udpm_mark(uint16_t msgid);

/**
 * returns if a message was seen, note that it returns false if
 * udpm_mark was never successfully called
*/
bool udpm_was_seen(uint16_t msgid);

/**
 * free internal resources of UDP Marker module
 * @note UDP Marker uses `mmal` module, so this function needs to be called
 * before `gexit(GE_FREE_RES, ...)`
*/
void udpm_free_res();



#endif  // ifndef _U_D_P_M_A_R_K_E_R_H_

/*****************
**  Vit Pavlik  **
**   xpavli0a   **
**    251301    **
**              **
**   Created:   **
**  2024-02-20  **
**              **
**    Edited:   **
**  2024-02-20  **
*****************/

/**
 * API for argparse: module to check + parse arguments
*/

#include "ipk24chat.h"

#ifndef _A_R_G_P_A_R_S_E_H_
#define _A_R_G_P_A_R_S_E_H_

/**
 * Checks and parses args (initializes `conf`)
 * @param argc argument count
 * @param argv arguments
 * @param conf configuration structure to initialize
 * @return true if args are ok and false if they're not
 * @note allocates memory for conf->addr, that needs to be freed
*/
bool args_ok(int argc, char *argv[], conf_t *conf);

/**
 * initializes `addr` from `conf`
*/
void addr_from_conf(addr_t *addr, const conf_t *conf);

#endif  // ifndef _A_R_G_P_A_R_S_E_H_
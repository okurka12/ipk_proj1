/*****************
**  Vit Pavlik  **
**   xpavli0a   **
**    251301    **
**              **
**   Created:   **
**  2024-02-23  **
**              **
**    Edited:   **
**  2024-02-24  **
*****************/

/**
 *
 * API for the `gexit` - gracefully exit module
 * `gexit` is meant to handle the program interrupt
 *
 */

#ifndef _G_E_X_I_T_H_
#define _G_E_X_I_T_H_

/* statements that `gexit` can be called with */
enum gexit_statement {

    /* set sockfd to be closed if the program is interrupted */
    GE_SET_FD,

    /* unset sockfd to be closed if the program is interrupted */
    GE_UNSET_FD,

    /* set a pointer to the configuration structure */
    GE_SET_CONFP,

    /* unset a pointer to the */
    GE_UNSET_CONFP,

    /* call this statement when the program is terminated */
    GE_TERMINATE
};

/**
 * gracefully exit
 *
 * this function keeps static variables for the config structure and possibly
 * a socket file descriptor that is currently open, so that allocated resources
 * (address string, open sockets) can be freed upon termination with `C-c`
 *
 * Call this function with `statement` and a pointer `p` pointing to the
 * desired property you want to set (sockfd, confp, etc..)
 *
 * @note
 * this function contains a few `log` statements from `utils.h` which may
 * result in an undefined behavior (see `man 7 signal-safety`). eliminate
 * this by defining NDEBUG
*/
void gexit(enum gexit_statement statement, void *p);

#endif  // ifndef _G_E_X_I_T_H_

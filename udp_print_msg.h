/*****************
**  Vit Pavlik  **
**   xpavli0a   **
**    251301    **
**              **
**   Created:   **
**  2024-03-10  **
**              **
**    Edited:   **
**  2024-03-10  **
*****************/

/**
 * Module that prints incoming UDP messages
*/

#ifndef _U_D_P_P_R_I_N_T_M_S_G_H_
#define _U_D_P_P_R_I_N_T_M_S_G_H_

/* return code for bad message format */
#define UPM_BADFMT 999

/**
 * Prints a message according to its type and specified format in the
 * assignment. This function tries to be buffer-overflow-safe and only
 * prints printable characters
 *
 * @return 0 on succes or ERR_INTERNAL (defined in ipk24chat.h) or UPM_BADFMT
 * (defined in udp_print_msg.h)
 *
 * @note allocates internal buffers, to free them, call
 * `udp_free_printer_resources`
 *
*/
int udp_print_msg(char *msg, unsigned int msglen);

/**
 * checks if a string contains only printable characters
 * @return `false` if `s` contains unprintable characters else `true`
 * @note uses `ctype.h`'s `isprint`
*/
bool str_isprint(const char *s);

/**
 * Frees internal buffer of `udp_print_msg`
 * @note `udp_print_msg` uses `mmal` module, this needs to be called
 * before `gexit(GE_FREE_RES, ...)`
*/
void udp_free_printer_resources();

#endif  // ifndef _U_D_P_P_R_I_N_T_M_S_G_H_

/*****************
**  Vit Pavlik  **
**   xpavli0a   **
**    251301    **
**              **
**   Created:   **
**  2024-02-18  **
**              **
**    Edited:   **
**  2024-02-18  **
*****************/

/* Constants and stuff for the IPK24CHAT protocol */

#include <stdint.h>

#ifndef _I_P_K_2_4_C_H_A_T_H_
#define _I_P_K_2_4_C_H_A_T_H_

#define CONFIRM	0x00
#define REPLY	0x01
#define AUTH	0x02
#define JOIN	0x03
#define MSG	    0x04
#define ERR	    0xFE
#define BYE	    0xFF

#define CRLF "\r\n"

/* address type - contains address and port */
typedef struct {

    /* text form of an address (eg. 127.0.0.1) */
    char *addr;

    uint16_t port;

} addr_t;

#endif  // ifndef _I_P_K_2_4_C_H_A_T_H_

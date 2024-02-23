/*****************
**  Vit Pavlik  **
**   xpavli0a   **
**    251301    **
**              **
**   Created:   **
**  2024-02-18  **
**              **
**    Edited:   **
**  2024-02-20  **
*****************/

/**
 * Constants and stuff for the IPK24CHAT protocol
 * (message type constants, structures, error codes)
*/

#include <stdint.h>
#include <stdbool.h>

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
#define LF "\n"

#define ERR_BAD_ARG 13

#define DEFAULT_PORT 4567
#define DEFAULT_TO 250
#define DEFAULT_RETRIES 3

#define STRSTR(x) #x
#define STR(x) STRSTR(x)

#define USAGE "Usage: ipk24chat-client -t tcp|udp -s ADDRESS [-p PORT] " \
"[-d TIMEOUT] [-r RETRIES] [-h]"

#define HELP_TXT "Options:" LF \
"    -t           Transport protocol used" LF \
"    -s           Server IP or hostname" LF \
"    -p           Port (default is " STR(DEFAULT_PORT) ")" LF \
"    -d           UDP confirmation timeout (ns) (default is " \
                  STR(DEFAULT_TO) ")" LF \
"    -r           Maximum number of UDP retransmissions (default is " \
                  STR(DEFAULT_RETRIES) ")" LF \
"    -h           Print help and exit"

/* message struct */
typedef struct {

    /* message type (CONFIRM, REPLY, AUTH, JOIN, MSG, ERR, BYE) */
    uint8_t type;

    /* message id */
    uint16_t id;

    /* message itself */
    char *content;
} msg_t;

/* chosen transport protocol */
enum tp { TCP, UDP, NOT_SPECIFIED };

/* configuration of current's execution */
typedef struct {

    /* transport protocol */
    enum tp tp;

    /* ip address or hostname */
    char *addr;

    uint16_t port;
    unsigned int timeout;
    unsigned int retries;

    /* presence of -h option */
    bool should_print_help;

} conf_t;

#endif  // ifndef _I_P_K_2_4_C_H_A_T_H_

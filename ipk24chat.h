/*****************
**  Vit Pavlik  **
**   xpavli0a   **
**    251301    **
**              **
**   Created:   **
**  2024-02-18  **
*****************/

/**
 * Constants and stuff for the IPK24CHAT protocol
 * (message type constants, structures, error codes)
*/

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <threads.h>


#ifndef _I_P_K_2_4_C_H_A_T_H_
#define _I_P_K_2_4_C_H_A_T_H_

#define MTYPE_CONFIRM 0x00
#define MTYPE_REPLY	  0x01
#define MTYPE_AUTH	  0x02
#define MTYPE_JOIN	  0x03
#define MTYPE_MSG	  0x04
#define MTYPE_ERR	  0xFE
#define MTYPE_BYE	  0xFF

#define CRLF "\r\n"
#define LF "\n"

/* bad arguments to the program */
#define ERR_BAD_ARG 13

/* couldn't resolve the hostname */
#define ERR_HOSTNAME 14

/* internal error (couldn't allocate memory, open socket etc.) */
#define ERR_INTERNAL 99

#define DEFAULT_PORT 4567
#define DEFAULT_TO 250
#define DEFAULT_RETRIES 3

/* msgid for the first message sent by client */
#define START_MSGID 69

/* msgid for the last BYE message sent by client */
#define LAST_MSGID 0xffff

/* maximal length of field `Username` */
#define MAX_UNAME_LEN 20

/* maximal length of field `ChannelID` */
#define MAX_CHID_LEN 20

/* maximal length of field `Secret` */
#define MAX_SECRET_LEN 128

/* maximal length of field `DisplayName` */
#define MAX_DNAME_LEN 20

/* maximal length of field `MessageContent` */
#define MAX_MSGCONT_LEN 1400

/* prefix of an internall client error message */
#define ERRPRE "ERR: "

/* suffix of an internal client error message (includes LF) */
#define ERRSUF "\n"

/* print internal client error (with `ERRPRE` and `ERRSUF`) */
#define pinerror(s) fprintf(stderr, ERRPRE "%s" ERRSUF, (s))

/* for STR() */
#define STRSTR(x) #x

/* stringize the expanded macro (for example if we have `#define x 5` then
`STR(x)` will be `"5"`) */
#define STR(x) STRSTR(x)

/* author to print in help messages (if you want to print no author, set this
to empty string) */
#define AUTHOR LF "Author: Vit Pavlik, 2024" LF

/* this needs to be manually changed but still better than not knowing to
whom i sent what*/
#define VERSION "Version: 663550c9d1db476e5889dda1935c38c895528475" LF

#define USAGE "Usage: ipk24chat-client -t tcp|udp -s ADDRESS [-p PORT] " \
"[-d TIMEOUT] [-r RETRIES] [-h]"

#define HELP_TXT "Options:" LF \
"  -t    Transport protocol used" LF \
"  -s    Server IP or hostname" LF \
"  -p    Port (default is " STR(DEFAULT_PORT) ")" LF \
"  -d    UDP confirmation timeout (ns) (default is " \
         STR(DEFAULT_TO) ")" LF \
"  -r    Maximum number of UDP retransmissions (default is " \
         STR(DEFAULT_RETRIES) ")" LF \
"  -h    Print help and exit" AUTHOR VERSION

#define CMD_HELP_TXT "Available commands:" LF \
"  /auth username secret displayname -- authenticate if not authenticated " \
"yet" LF \
"  /join channel_id -- join a new channel" LF \
"  /rename displayname -- change the display name for future messages" LF \
"Parameter length restrictions:" LF \
"  username:    " STR(MAX_UNAME_LEN) " characters" LF \
"  secret:      " STR(MAX_SECRET_LEN) " characters" LF \
"  displayname: " STR(MAX_DNAME_LEN) " characters" LF \
"  channel_id:  " STR(MAX_CHID_LEN) " characters" LF \
"Other inputs that don't represent any known command will be " LF \
"interpreted as a message and a message can be no longer than " \
STR(MAX_MSGCONT_LEN) " characters" LF AUTHOR


/* global client lock (for accessing `conf`, `cnfm_data`, or `gexit`) */
extern mtx_t gcl;

/* chosen transport protocol */
enum tp { TCP, UDP, NOT_SPECIFIED };

/* configuration of current's execution */
typedef struct {

    /* transport protocol */
    enum tp tp;

    /* ip address or hostname */
    char *addr;

    /* displayname from /auth or /join command */
    char *dname;

    /* outgoing message counter */
    uint16_t cnt;

    uint16_t port;
    unsigned int timeout;
    unsigned int retries;

    /* presence of -h option */
    bool should_print_help;

    /* socket to send and receive messages from */
    int sockfd;

} conf_t;

/**
 * Returns a string representing the message type if it exists, else an
 * empty string
 *
 * Example: `mtype_str(0x04)` returns `"MSG"`
 *
 * @note implemented in `main.c`
*/
char *mtype_str(uint8_t mtype);

#endif  // ifndef _I_P_K_2_4_C_H_A_T_H_

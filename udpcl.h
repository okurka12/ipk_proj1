/*****************
**  Vit Pavlik  **
**   xpavli0a   **
**    251301    **
**              **
**   Created:   **
**  2024-02-18  **
*****************/

/* API for the UDP client udpcl.c */

#include <stdint.h>
#include "ipk24chat.h"
#include "msg.h"  // msg_t

#ifndef _U_D_P_C_L_H_
#define _U_D_P_C_L_H_

/* buffer size for incoming messages */
#define RESPONSE_BUFSIZE 65535

/**
 * Sends `msg` (as a AF_INET SOCK_DGRAM - IPv4 UDP packet) to `addr`
 * @param addr
 * @param msg
 * @param conf
 * @return 0 on success else 1
*/
int udp_send_msg(msg_t *msg, conf_t *conf);

/**
 * Sends first UDP message and waits for the CONFIRM itself.
 * Also saves the source port of the incoming CONFIRM message to `conf`.
 * @return 0 on success else 1
 * @note only use this function to send the AUTH message (first message)
 * and the BYE message (the last message) since this function itself
 * listens for CONFIRM response messages and discards any other
 * messages/packets
*/
int udp_send_and_confirm(msg_t *msg, conf_t *conf);

/**
 * Create AF_INET SOCK_DGRAM socket (IPv4 UDP) from `conf->addr` and
 * `conf->port`. * Resulting socket file descriptor will be in `conf->sockfd`
 * @return 0 on success, else 1
 * @note needs to be closed with `close()` from `unistd.h`
*/
int udp_create_socket(conf_t *conf);

/**
 * Sets a `ms` timeout on `conf->sockfd`
 * @return 0 on success else 1
 */
int udp_set_rcvtimeo(conf_t *conf, unsigned int ms);

/**
 * handles all the stuf for the UDP variant
*/
int udp_main(conf_t *conf);

#endif  // ifndef _U_D_P_C_L_H_

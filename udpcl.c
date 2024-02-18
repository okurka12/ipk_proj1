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

/**
 *
 * udpcl.c - UDP client
 * standalone for now
 *
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>  // close
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>  // struct sockaddr_in
#include <arpa/inet.h>  // htons

#include "ipk24chat.h"

#define CSSA const struct sockaddr *


int udp_fill_addrstruct(const char *addr, uint16_t port, struct sockaddr_in *s) {

    /* initialize to zero, initialize domain */
    memset(s, 0, sizeof(struct sockaddr_in));
    s->sin_family = AF_INET;

    /* convert IP addres from text to binary */
    if (inet_pton(AF_INET, addr, &s->sin_addr) <= 0) {
        perror("inet_pton error (invalid address?)");
        return 1;
    }

    /* convert from host byte order to network byte order */
    s->sin_port = htons(port);
}


int udp_send_data(const char *addr, uint16_t port, const char *data,
                  unsigned int length) {

    struct sockaddr_in server_addr;
    udp_fill_addrstruct(addr, port, &server_addr);


    /* create socket */
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd == -1) {
        perror("socket creation failed");
        return 1;
    }

    /* server address but correct type */
    CSSA sa = (CSSA)&server_addr;

    /* send the packet */
    ssize_t result = sendto(sockfd, data, length, 0, sa, sizeof(server_addr));
    if (result == -1) {
        perror("sendto failed");
        return 1;
    }

    close(sockfd);
    return 0;
}


int main() {
    char *msg = "Hello, I am client.\r\n";
    char *addr = "127.0.0.1";
    u_int16_t port = 4567;
    udp_send_data(addr, port, msg, strlen(msg));
}

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


/**
 * Sends AF_INET SOCK_DGRAM (IPv4 UDP) packet to `addr`
 * @param addr IPv4 address (eg. 127.0.0.1)
 * @param port port
 * @param data payload
 * @param length lengh of the data in bytes
 * @return 0 on success else 1
*/
int udp_send_msg(const char *addr, uint16_t port, const char *data,
                 unsigned int length) {

    /* create struct sockaddr_in, initialize to zero, initialize domain */
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;

    /* convert IP addres from text to binary */
    if (inet_pton(AF_INET, addr, &server_addr.sin_addr) <= 0) {
        perror("inet_pton error (invalid address?)");
        return 1;
    }

    /* convert from host byte order to network byte order */
    server_addr.sin_port = htons(port);


    /* create socket */
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd == -1) {
        perror("socket creation failed");
        return 1;
    }

    /* server address but correct type */
    const struct sockaddr *sa = (const struct sockaddr *)&server_addr;

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
    udp_send_msg(addr, port, msg, strlen(msg));
}

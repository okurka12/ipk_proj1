/*****************
**  Vit Pavlik  **
**   xpavli0a   **
**    251301    **
**              **
**   Created:   **
**  2024-03-17  **
**              **
**    Edited:   **
**  2024-03-17  **
*****************/

#include <assert.h>
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>  // inet_pton, htons
#include <netinet/in.h>  // struct sockaddr_in
#include <unistd.h>  // close

#include "ipk24chat.h"
#include "tcpcl.h"
#include "msg.h"  // msg_t
#include "utils.h"

/**
 * create socket and save it to `conf`
 * @return 0 on success else non-zero
*/
static int tcp_create_socket(conf_t *conf) {
    conf->sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (conf->sockfd == -1) {
        pinerror("couldn't create socket");
        perror(ERRPRE "socket");
        log(FATAL, "couldn't create socket");
        return ERR_INTERNAL;
    }
    return 0;
}

/**
 * connect the TCP socket in `conf`, return 0 on success else non-zero
*/
static int tcp_connect(conf_t *conf) {
    int rc;

    struct sockaddr_in s;
    s.sin_family = AF_INET;
    s.sin_port = htons(conf->port);

    /* we could use inet_addr here but nah */
    rc = inet_pton(AF_INET, conf->addr, &s.sin_addr);
    if (rc <= 0) {
        pinerror("couldn't convert address");
        perror(ERRPRE "inet_pton");
        return ERR_INTERNAL;
    }

    rc = connect(conf->sockfd, (struct sockaddr *)(&s), sizeof(s));
    if (rc == -1) {
        pinerror("coudlnt connect");
        perror(ERRPRE "connect");
        log(FATAL, "couldn't connect");
        return ERR_INTERNAL;
    }

    return 0;
}

int tcp_send(conf_t *conf, const msg_t *msg) { /* todo */
    log(DEBUG, "sending");
    (void)msg;
    assert(conf->sockfd != -1);
    char msgg[] = "eyo";

    ssize_t result = send(conf->sockfd, msgg, sizeof(msgg), 0);
    if (result == -1) {
        pinerror("couldn't send");
        perror(ERRPRE "sendto");
        return ERR_INTERNAL;
    }
    return 0;
}

int tcp_main(conf_t *conf) {

    int rc;

    rc = tcp_create_socket(conf);
    if (rc != 0) {
        return rc;
    }

    rc = tcp_connect(conf);
    if (rc != 0) {
        return rc;
    }

    while (true) {

        rc = tcp_send(conf, NULL);
        if (rc != 0) {
            log(ERROR, "couldn't send");
        }
        getchar();

    }
    close(conf->sockfd);
    conf->sockfd = -1;

}

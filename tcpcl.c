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
#include <sys/epoll.h>
#include <errno.h>

#include "ipk24chat.h"
#include "tcpcl.h"
#include "msg.h"  // msg_t
#include "utils.h"
#include "shell.h"
#include "mmal.h"
#include "tcp_render.h"

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

    char *rendered = tcp_render(msg);

    ssize_t result = send(conf->sockfd, rendered, strlen(rendered), 0);
    mfree(rendered);
    if (result == -1) {
        pinerror("couldn't send");
        perror(ERRPRE "sendto");
        return ERR_INTERNAL;
    }
    return 0;
}

/**
 * creates an epoll instance and events EPOLLIN for the socket
 * and stdin if stdin is not a file
 * @return epoll fd on success else -1
*/
/* i dont know what i was trying to do here */
// int tcp_epoll(conf_t *conf) {
//     int rc;
//     if (isatty(0)) {
//     }

//     /* create epoll instance */
//     int epoll_fd = epoll_create1(0);
//     if (epoll_fd == -1) {
//         pinerror("couldn't create epoll instance");
//         perror(ERRPRE "epoll_create1");
//         log(FATAL, "couldn't create epoll instance");
//         return -1;
//     }

//     struct epoll_event sock_event = {
//         .data.fd = conf->sockfd,
//         .events = EPOLLIN
//     };

//     rval = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sock_fd, &sock_event);
//     assert(rval == 0);

// }

int tcp_auth_loop(conf_t *conf ) {

    size_t line_length = INIT_LINE_BUFSIZE;
    char *line = mmal(INIT_LINE_BUFSIZE);
    if (line == NULL) {
        pinerror(MEMFAIL_MSG);
        log(ERROR, MEMFAIL_MSG);
        return ERR_INTERNAL;
    }

    bool done = false;
    msg_t *msg = NULL;

    while (not done) {

        ssize_t getlinerc = mgetline(&line, &line_length, stdin);
        if (getlinerc == -1 and errno == ENOMEM) {
            pinerror(MEMFAIL_MSG);
            log(ERROR, MEMFAIL_MSG);
            return ERR_INTERNAL;
        }
        if (getlinerc == -1) {
            log(DEBUG, "EOF reached, stopping auth loop");
            break;
        }

        bool error_occured = false;
        msg = parse_auth(line, &error_occured);
        if (msg == NULL and error_occured) {
            pinerror("internal error during auth parsing");
            log(FATAL, "error in parse_auth");
            return ERR_INTERNAL;
        }
        if (msg == NULL) {  // invalid /auth command
            continue;
        }

        /* tcp send */
        tcp_send(conf, msg);
        /* tcp recv */
        /* copy dname */

        msg_dtor(msg);
        msg = NULL;

    }

    mfree(line);
    line = NULL;

    return 0;

}

int tcp_main(conf_t *conf) {

    int rc = 0;

    rc = tcp_create_socket(conf);
    if (rc != 0) {
        return ERR_INTERNAL;
    }

    rc = tcp_connect(conf);
    if (rc != 0) {
        close(conf->sockfd);
        conf->sockfd = -1;
        return ERR_INTERNAL;
    }

    tcp_auth_loop(conf);


    close(conf->sockfd);
    conf->sockfd = -1;
    return rc;

}

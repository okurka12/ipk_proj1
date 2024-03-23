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
#include <ctype.h>  // isprint
#include <arpa/inet.h>  // inet_pton, htons
#include <netinet/in.h>  // struct sockaddr_in
#include <unistd.h>  // close
#include <sys/epoll.h>
#include <errno.h>
#include <signal.h>

#include "ipk24chat.h"
#include "tcpcl.h"
#include "msg.h"  // msg_t
#include "utils.h"
#include "shell.h"
#include "mmal.h"
#include "tcp_render.h"
#include "gexit.h"  // GE_SET_FD
#include "tcp_parse.h"

#define TCP_READBUF_SIZE 128000  /* todo: document this constant */

#define TCP_LOOP_MS 10 /* todo: document this constant */

/* reads data up to CRLF, returns dynamically allcoated buffer */
static char *tcp_myrecv(conf_t *conf) {

    char *buf = mcal(TCP_READBUF_SIZE, 1);
    size_t write_idx = 0;
    if (buf == NULL) {
        pinerror(MEMFAIL_MSG);
        log(ERROR, MEMFAIL_MSG);
        return NULL;
    }

    enum { S_CHAR, S_CR } state = S_CHAR;

    ssize_t rc = 0;
    bool done = false;
    while (not done) {
        char c;
        rc = recv(conf->sockfd, &c, 1, 0);
        if (rc == -1) {
            pinerror("couldn't recv");
            perror(ERRPRE "recv");
            mfree(buf);
            return NULL;
        }

        /* hard core logging (but could be useful) */
        // logf(DEBUG, "got char: %c (0x%02hhx) state=%d",
        //     isprint(c) ? c : '.', c, state);

        buf[write_idx] = c;
        write_idx++;

        /* state transition */
        switch (state) {
        case S_CHAR:
            if (c == '\r') state = S_CR;
            break;

        case S_CR:
            if (c == '\n') done = true;
            break;
        }
    }

    return buf;
}

/* does nothing */
void handle_sigpipe(int sig) {
    log(DEBUG, "program received SIGPIPE");
    (void)sig;
}

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
        pinerror("coudln't connect to host");
        perror(ERRPRE "connect");
        log(FATAL, "couldn't connect");
        return ERR_INTERNAL;
    }

    return 0;
}

/**
 * send msg to conf->sockfd
 * return 0 on success
*/
int tcp_send(conf_t *conf, const msg_t *msg) { /* todo */
    logf(DEBUG, "sending %s msg id %hu", mtype_str(msg->id), msg->id);
    (void)msg;
    assert(conf->sockfd != -1);

    char *rendered = tcp_render(msg);

    ssize_t result = send(conf->sockfd, rendered, strlen(rendered), 0);
    mfree(rendered);
    logf(DEBUG, "send returned %ld", result);
    if (result == -1) {
        pinerror("couldn't send");
        perror(ERRPRE "sendto");
        return ERR_INTERNAL;
    }
    log(DEBUG, "successfully sent");
    return 0;
}

/**
 * loops over stdin, receives data (todo: make it able to print them)
 * at the end it DOESN'T send BYE message...
 * @return 0 on success else non-zero
*/
static int tcp_loop(conf_t *conf) {
    int rc = 0;

    /* buffer for a line */
    size_t line_length = INIT_LINE_BUFSIZE;
    char *line = mmal(INIT_LINE_BUFSIZE);
    if (line == NULL) {
        pinerror(MEMFAIL_MSG);
        log(ERROR, MEMFAIL_MSG);
        return ERR_INTERNAL;
    }

    /* epoll stuff */
    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        pinerror("couldnn't create epoll instance");
        perror(ERRPRE "epoll_create1");
        log(ERROR, "epoll_create failed");
        return ERR_INTERNAL;
    }
    gexit(GE_SET_EPOLLFD, &epoll_fd);
    struct epoll_event sock_event = {
        .data.fd = conf->sockfd,
        .events = EPOLLIN
    };
    rc = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, conf->sockfd, &sock_event);
    if (rc != 0) {
        pinerror("epoll_ctl failed");
        perror(ERRPRE "epoll_ctl");
        log(ERROR, "epoll_ctl failed");
        return ERR_INTERNAL;
    }

    /* either block on both stdin and the socket, or set a short timeout */
    int timeout = -1;
    struct epoll_event stdin_event = { .data.fd = 0, .events = EPOLLIN };
    if (isatty(0)) {
        timeout = -1;
        rc = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, 0, &stdin_event);
        if (rc != 0) {
            pinerror("epoll_ctl failed");
            perror(ERRPRE "epoll_ctl");
            log(ERROR, "epoll_ctl failed");
            return ERR_INTERNAL;
        }
    } else {  // stdin is non-blocking
        timeout = TCP_LOOP_MS;
    }

    bool done = false;
    struct epoll_event events[1];
    msg_t *msg = NULL;
    while (not done) {

        /* wait for an event or just wait a bit for network data */
        rc = epoll_wait(epoll_fd, events, 1, timeout);
        if (rc == -1) {
            pinerror("epoll_wait failed");
            perror(ERRPRE "epoll_wait");
            log(ERROR, "epoll_wait failed");
            return ERR_INTERNAL;
        }

        /* data came from network */
        if (rc == 1 and events[0].data.fd == conf->sockfd) {

            log(DEBUG, "data came from network, todo: call recv");
            /* todo: call recv, tcp_print */

            /* either block again if stdin is blocking or don't if it's not */
            if (isatty(0)) continue;

        }  // if data came from network

        /* read one line */
        ssize_t getlinerc = mgetline(&line, &line_length, stdin);
        if (getlinerc == -1 and errno == ENOMEM) {
            pinerror(MEMFAIL_MSG);
            log(ERROR, MEMFAIL_MSG);
            return ERR_INTERNAL;
        }
        if (getlinerc < 1) {
            log(INFO, "EOF reached");
            break;
        }
        rstriplf(line);
        logf(DEBUG, "read %ld characters: '%s' + LF", getlinerc, line);

        if (is_auth(line)) {
            pinerror("already authenticated");
            continue;

        } else if (is_join(line)) {

            /* parse /join command */
            bool error_occurred = false;
            msg = parse_join(line, &error_occurred);

            /* /join command was invalid */
            if (msg == NULL and not error_occurred) {
                pinerror("invalid /join command");
                continue;
            }

            /* internal error occured while parsing */
            if (msg == NULL and error_occurred) {
                pinerror(MEMFAIL_MSG);
                continue;
            }

            msg->dname = conf->dname;
            rc = tcp_send(conf, msg);
            msg->dname = NULL;
            msg_dtor(msg);
            msg = NULL;
            if (rc != 0) {
                pinerror("couldn't send");
                continue;
            }

        } else if (is_rename(line)) {

            /* parse /rename command */
            bool error_occurred = false;
            char *dname = parse_rename(line, &error_occurred);

            /* couldnt allocate buffer */
            if (dname == NULL and error_occurred) {
                fprintf(stderr, ERRPRE MEMFAIL_MSG ERRSUF);
                log(ERROR, MEMFAIL_MSG);
                continue;
            }

            /* invalid display name */
            if (dname == NULL and not error_occurred) {
                pinerror("invalid /rename command");
                continue;
            }

            /* replace conf->dname */
            mfree(conf->dname);
            conf->dname = dname;

            } else if (is_help(line)) {
            printf(CMD_HELP_TXT);

        /* else: inputted line is a message */
        } else {

            if (not message_valid(line)) continue;

            msg = msg_ctor();
            msg->type = MTYPE_MSG;
            msg->content = line;
            msg->dname = conf->dname;

            rc = tcp_send(conf, msg);
            msg->dname = NULL;
            msg->content = NULL;
            msg_dtor(msg);
            msg = NULL;
            if (rc != 0) {
                pinerror("couldn't send");
                continue;
            }

        }

    }  // while not done

    mfree(line);
    close(epoll_fd);
    gexit(GE_UNSET_EPOLLFD, NULL);

    return 0;
}

static int tcp_auth_loop(conf_t *conf ) {

    /* buffer for a line */
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

        if (is_help(line)) {
            printf(CMD_HELP_TXT);
            continue;
        }

        /* try to parse /auth command */
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

        /* send AUTH message */
        tcp_send(conf, msg);

        /* wait for REPLY message */
        char *reply_data = tcp_myrecv(conf);
        logf(DEBUG, "yo server replied: %s", reply_data);

        /* parse reply (reply is printed by the parse fn) */
        enum parse_result pr = tcp_parse_any(reply_data);
        if (pr == ERR_INTERNAL) {
            pinerror("error parsing REPLY");
            log(ERROR, "error parsing REPLY");
            return ERR_INTERNAL;
        } else if (pr != PR_REPLY_NOK and pr != PR_REPLY_OK) {
            pinerror("server sent unexpected message type");
            log(WARNING, "server sent unexpected message type");
        } else {
            done = pr == PR_REPLY_OK;  // if auth successful, end loop
        }

        /* copy dname */
        if (conf->dname != NULL) mfree(conf->dname);
        conf->dname = msg->dname;
        msg->dname = NULL;  // so it's not wiped by msg_dtor

        mfree(reply_data);
        reply_data = NULL;
        msg_dtor(msg);
    }

    mfree(line);
    line = NULL;
    return 0;
}

int tcp_main(conf_t *conf) {

    int rc = 0;

    signal(SIGPIPE, handle_sigpipe);

    rc = tcp_create_socket(conf);
    if (rc != 0) {
        return ERR_INTERNAL;
    }
    gexit(GE_SET_FD, &conf->sockfd);

    rc = tcp_connect(conf);
    if (rc != 0) {
        close(conf->sockfd);
        conf->sockfd = -1;
        return ERR_INTERNAL;
    }

    rc = tcp_auth_loop(conf);
    if (rc != 0) {
        log(ERROR, "auth process went not well");
        return rc;
    }

    rc = tcp_loop(conf);

    close(conf->sockfd);
    conf->sockfd = -1;
    return rc;

}

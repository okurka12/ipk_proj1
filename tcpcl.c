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

/* what to return in auth loop if connection ended */
#define TCPCONN_ENDED 7

/* what to return if ctrl d was pressed in auth loop */
#define TCP_EOF_AUTH 11

/**
 * reads data up to CRLF, returns dynamically allcoated buffer
 * it returns NULL either when no data could be read (the connection ended)
 * or when error occured (in that case, sets `err_occured` to true)
 */
static char *tcp_myrecv(conf_t *conf, bool *err_occured) {

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
        if (rc == -1) {  // error occured
            pinerror("couldn't recv");
            perror(ERRPRE "recv");
            mfree(buf);
            *err_occured = true;
            return NULL;
        }
        if (rc == 0) {  // connection ended
            pinerror("connection ended abruptly");
            log(DEBUG, "connection ended abruptly");
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
    logf(DEBUG, "sending %s msg id %hu", mtype_str(msg->type), msg->id);
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
    log(DEBUG, "normal loop started");
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
    bool should_send_bye = true;
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
            bool error_occurred = false;
            char *reply_data = tcp_myrecv(conf, &error_occurred);
            if (reply_data == NULL and error_occurred) {
                pinerror("something went wrong");
                log(ERROR, "something went wront with recv");
                return ERR_INTERNAL;
            }

            /* this is what happens when connection ends abruptly */
            if (reply_data == NULL and not error_occurred) {
                should_send_bye = false;
                break;
            }

            enum parse_result pr = PR_UNKNOWN;
            pr = tcp_parse_any(reply_data);
            mfree(reply_data);
            if (pr == PR_BYE) {
                should_send_bye = false;
                break;
            } else if (pr == PR_UNKNOWN) {
                pinerror("server sent invalid data");
            }

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

    if (should_send_bye) {
        msg_t bye_msg = { .type = MTYPE_BYE };
        rc = tcp_send(conf, &bye_msg);
        if (rc != 0) log(ERROR, "couldn't send bye");
    }

    mfree(line);
    close(epoll_fd);
    gexit(GE_UNSET_EPOLLFD, NULL);

    log(DEBUG, "normal loop finished");
    return rc;
}

static int tcp_auth_loop(conf_t *conf ) {
    log(DEBUG, "auth loop started");
    int rc = 0;
    static_assert(TCPCONN_ENDED != ERR_INTERNAL, "constant collision");
    static_assert(TCP_EOF_AUTH != ERR_INTERNAL, "constant collision");

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
            rc = TCP_EOF_AUTH;
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
        error_occured = false;
        char *reply_data = tcp_myrecv(conf, &error_occured);
        if (reply_data == NULL and error_occured) {
            pinerror("internal error");
            log(ERROR, "something went wrong");
            return ERR_INTERNAL;
        }

        /* if connection ended abruptly */
        if (reply_data == NULL and not error_occured) {
            msg_dtor(msg);
            rc = 1;
            break;
        }


        /* parse reply (reply is printed by the parse fn) */
        enum parse_result pr = tcp_parse_any(reply_data);
        if (pr == PR_ERR_INTERNAL) {
            pinerror("error parsing REPLY");
            log(ERROR, "error parsing REPLY");
            return ERR_INTERNAL;

        } else if (pr == PR_BYE) {
            done = true;
            rc = TCPCONN_ENDED;
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
        msg = NULL;
    }

    mfree(line);
    line = NULL;
    log(DEBUG, "auth loop finished");
    return rc;
}

int tcp_main(conf_t *conf) {

    int rc = 0;

    signal(SIGPIPE, handle_sigpipe);
    log(DEBUG, "SIGPIPE handler registered");

    rc = tcp_create_socket(conf);
    if (rc != 0) {
        return ERR_INTERNAL;
    }
    gexit(GE_SET_FD, &conf->sockfd);
    log(DEBUG, "socket created");

    rc = tcp_connect(conf);
    if (rc != 0) {
        close(conf->sockfd);
        conf->sockfd = -1;
        return ERR_INTERNAL;
    }
    log(DEBUG, "connected to remote host");

    rc = tcp_auth_loop(conf);
    if (rc == ERR_INTERNAL) {
        log(ERROR, "auth process went not well");
        return rc;
    }

    if (rc == 0) {
        rc = tcp_loop(conf);  // no need to check
    }

    shutdown(conf->sockfd, SHUT_RDWR);
    close(conf->sockfd);
    gexit(GE_UNSET_FD, NULL);
    conf->sockfd = -1;
    return rc;

}

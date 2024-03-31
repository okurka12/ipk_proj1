/*****************
**  Vit Pavlik  **
**   xpavli0a   **
**    251301    **
**              **
**   Created:   **
**  2024-02-18  **
*****************/

/**
 *
 * udpcl.c - UDP client
 * functions necessary to perform communication on the UDP transport protocol
 *
 */

#include <assert.h>
#include <stdio.h>  // perror
#include <string.h>  // strcpy, memcpy
#include <unistd.h>  // close
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>  // struct timeval
#include <netinet/in.h>  // struct sockaddr_in
#include <arpa/inet.h>  // htons
#include <sys/epoll.h>

#include "udpcl.h"
#include "ipk24chat.h"
#include "utils.h"
#include "gexit.h"
#include "mmal.h"
#include "udp_confirmer.h"  // cnfm_data_t
#include "udp_listener.h"  // LISTENER_TIMEOUT
#include "udp_sender.h"  // udp_send
#include "msg.h"  // msg_t
#include "shell.h"
#include "udp_print_msg.h"  // str_isprint
#include "sleep_ms.h"

/* while blocking for stdin, check if listener hasn't finished every this many
miliseconds */
#define UDP_STDIN_TIMEOUT_MS 50

/* this is the interval how often will the the main sender thread check if
the JOIN message got a REPLY. it wil start checking only after the JOIN
message was confirmed (miliseconds) */
#define REPLY_WAIT_INTERVAL 15


int udp_set_rcvtimeo(conf_t *conf, unsigned int ms) {
    int rc = 0;
    unsigned int sec = ms / 1000;
    unsigned int msec = ms % 1000;
    struct timeval t = {.tv_sec = sec , .tv_usec = msec * 1000 };
    rc = setsockopt(conf->sockfd, SOL_SOCKET, SO_RCVTIMEO, &t, sizeof(t));
    if (rc == -1) {
        rc = 1;
        perror("Couldn't setsockopt SO_RCVTIMEO");
        log(ERROR, "couldn't set a timeout on socket");
    }
    return rc;
}


int udp_create_socket(conf_t *conf) {

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd == -1) {
        perror("socket creation failed");
        log(ERROR, "socket creation failed");
        return 1;
    }
    conf->sockfd = sockfd;

    return 0;
}


/**
 * UDP shell - loop endlessly:
 * starts listener, reads stdin, sends messages,
 * loops until listener is finished (incoming BYE) or until the end of user
 * input is reached or until the program is interrupted
 * @note run this function after the client is successfully authenticated
 * @return 0 on success else non-zero
*/
int udpsh_loop_endlessly(conf_t *conf, udp_cnfm_data_t *cnfm_data) {
    int rc = 0;

    /* start listener*/
    log(DEBUG, "MAIN: starting listener");
    thrd_t listener_thread_id;
    mtx_t listener_mtx;
    bool listener_stop_flag = false;
    bool listener_done_flag = false;
    bool listener_server_sent_bye = false;
    bool waiting_for_reply = false;
    uint16_t join_msgid = START_MSGID - 1;  // see listener_args_t definition
    if (mtx_init(&listener_mtx, mtx_plain) == thrd_error) {
        log(ERROR, "couldnt initialize lock");
        return ERR_INTERNAL;
    }
    listener_args_t listener_args = {
        .conf = conf,
        .cnfm_data = cnfm_data,
        .mtx = &listener_mtx,
        .save_port = false,
        .stop_flag = &listener_stop_flag,
        .done_flag = &listener_done_flag,
        .server_sent_bye = &listener_server_sent_bye,
        .waiting_for_reply = &waiting_for_reply,
        .join_msgid = &join_msgid
    };
    rc = thrd_create(&listener_thread_id, udp_listener, &listener_args);
    if (rc != thrd_success) {
        log(ERROR, "couldnt create listener thread");
        return ERR_INTERNAL;
    }
    gexit(GE_SET_LISTHR, &listener_thread_id);
    gexit(GE_SET_STPFLG, &listener_stop_flag);
    gexit(GE_SET_LISMTX, &listener_mtx);

    /**************************************************************************/
    /* from now on the listener is started */

    ssize_t read_chars = 0;
    char *line = mmal(INIT_LINE_BUFSIZE);
    if (line == NULL) { log(ERROR, MEMFAIL_MSG); return ERR_INTERNAL; }
    size_t line_length = INIT_LINE_BUFSIZE;
    msg_t *msg = NULL;

    bool should_send_bye = true;

    /* for parse_. calls */
    bool error_occurred = false;
    char *dname = NULL;

    /* epoll boilerplate, but if stdin is a file all this is skipped */
    /* epoll bp */ int epoll_fd = -1;
    /* epoll bp */ if (isatty(0)) {
    /* epoll bp */
    /* epoll bp */ epoll_fd = epoll_create1(0);
    /* epoll bp */ if (epoll_fd == -1) {
    /* epoll bp */     log(FATAL, "couldn't create epoll instance");
    /* epoll bp */     fprintf(stderr, ERRPRE "couldn't create epoll instance"
    /* epoll bp */         ERRSUF);
    /* epoll bp */     perror(ERRPRE "epoll_create1");
    /* epoll bp */     return ERR_INTERNAL;
    /* epoll bp */ }
    /* epoll bp */ gexit(GE_SET_EPOLLFD, &epoll_fd);
    /* epoll bp */ struct epoll_event stdin_event = {
    /* epoll bp */     .data.fd = 0,
    /* epoll bp */     .events = EPOLLIN
    /* epoll bp */ };
    /* epoll bp */ rc = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, 0, &stdin_event);
    /* epoll bp */ if (rc != 0) {
    /* epoll bp */     log(FATAL, "couldn't add epoll event");
    /* epoll bp */     fprintf(stderr, ERRPRE "couldn't add epoll event"
    /* epoll bp */         ERRSUF);
    /* epoll bp */     perror(ERRPRE "epoll_ctl");
    /* epoll bp */     return ERR_INTERNAL;
    /* epoll bp */ }
    /* epoll bp */ }  // if isatty
    /* epoll bp */ struct epoll_event events[1];

    bool done = false;
    while (not done) {

        /* while blocking for stdin, ocassionally check if listener hasnt
        finished yet (if stdin is a file, this is never performed) */
        bool should_wait = true;
        while (epoll_fd != -1 and should_wait) {
            int epr = epoll_wait(epoll_fd, events, 1, UDP_STDIN_TIMEOUT_MS);
            if (epr == 1) {  // stdin event
                should_wait = false;  // done flag for the inner while loop
            }
            mtx_lock(&listener_mtx);
            if (listener_done_flag) {
                should_send_bye = not listener_server_sent_bye;
                goto after_outer_loop;  // double break
            }
            mtx_unlock(&listener_mtx);
        }

        /* read one line (strip trailing LF) */
        read_chars = mgetline(&line, &line_length, stdin);
        if (read_chars < 1) {
            log(INFO, "EOF reached, stopping...");
            break;
        }
        rstriplf(line);
        logf(DEBUG, "read %ld chars: '%s' + LF", read_chars, line);

        /* check again whether listener is finished */
        mtx_lock(&listener_mtx);
        if (listener_done_flag) {
            should_send_bye = not listener_server_sent_bye;
            break;
        }
        mtx_unlock(&listener_mtx);

        /* /join, /rename, /help or send message*/
        if (is_auth(line)) {
            fprintf(stderr, ERRPRE "already authenticated" ERRSUF);
            continue;

        } else if (is_join(line)) {

            /* parse /join command */
            error_occurred = false;
            msg = parse_join(line, &error_occurred);

            /* /join command was invalid */
            if (msg == NULL and not error_occurred) {
                fprintf(stderr, ERRPRE "invalid /join command" ERRSUF);
                continue;
            }

            /* internal error occured while parsing */
            if (msg == NULL and error_occurred) {
                fprintf(stderr, ERRPRE MEMFAIL_MSG ERRSUF);
                continue;
            }

            /* fill additional parameters  */
            msg->id = conf->cnt; conf->cnt += 1;
            msg->dname = conf->dname;

            /* indicate we are waiting for REPLY */
            mtx_lock(&listener_mtx);
            join_msgid = msg->id;
            waiting_for_reply = true;
            mtx_unlock(&listener_mtx);

            /* send */
            rc = udp_sender_send(msg, conf, cnfm_data);

            /* destroy message */
            msg->dname = NULL;  // so conf->dname isnt wiped by msg_dtor
            msg_dtor(msg);

            /* was sending successfull? */
            if (rc != 0) {
                fprintf(stderr, ERRPRE "couldn't send" ERRSUF);
            }

            /* wait for reply (literally) */
            bool done_waiting_for_reply = false;
            while (not done_waiting_for_reply) {
                mtx_lock(&listener_mtx);
                done_waiting_for_reply = not waiting_for_reply;
                mtx_unlock(&listener_mtx);
                if (not done_waiting_for_reply) sleep_ms(REPLY_WAIT_INTERVAL);
            }

        } else if (is_rename(line)) {

            /* parse /rename command */
            error_occurred = false;
            dname = parse_rename(line, &error_occurred);

            /* couldnt allocate buffer */
            if (dname == NULL and error_occurred) {
                fprintf(stderr, ERRPRE MEMFAIL_MSG ERRSUF);
                log(ERROR, MEMFAIL_MSG);
                continue;
            }

            /* invalid display name */
            if (dname == NULL and not error_occurred) {
                fprintf(stderr, ERRPRE "invalid /rename command" ERRSUF);
                log(ERROR, "invalid /rename command");
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

            /* create message struct */
            msg = msg_ctor();
            if (msg == NULL) {
                fprintf(stderr, ERRPRE MEMFAIL_MSG ERRSUF);
                log(ERROR, MEMFAIL_MSG);
                continue;
            }
            msg->type = MTYPE_MSG;
            msg->id = conf->cnt;
            conf->cnt += 1;
            msg->dname = conf->dname;
            msg->content = line;

            /* send */
            rc = udp_sender_send(msg, conf, cnfm_data);
            if (rc != 0) {
                fprintf(stderr, ERRPRE "couldn't send" ERRSUF);
            }

            /* destroy message struct */
            msg->dname = NULL;    // keep conf->dname for further use
            msg->content = NULL;  // keep the buffer allocated
            msg_dtor(msg);
            msg = NULL;

            /* check if listener is finished */
            mtx_lock(&listener_mtx);
            if (listener_done_flag) {
                should_send_bye = not listener_server_sent_bye;
                break;
            }
            mtx_unlock(&listener_mtx);

        }  // line is a message

    }  // while not done

    after_outer_loop:

    if (should_send_bye) {
        msg_t bye = { .type = MTYPE_BYE, .id = LAST_MSGID };
        udp_sender_send(&bye, conf, cnfm_data);
    }

    mfree(line);
    /* from now we will let the listener finish */
    /**************************************************************************/

    /* let listener finish if it wasnt finished */
    if (not listener_done_flag) {
        mtx_lock(&listener_mtx);
        listener_stop_flag = true;
        mtx_unlock(&listener_mtx);
    }

    /* wait for the listener thread */
    log(DEBUG, "waiting for listener thread");
    int lrc;  // listener return code
    thrd_join(listener_thread_id, &lrc);
    gexit(GE_UNSET_LISTNR, NULL);
    mtx_destroy(&listener_mtx);
    logf(INFO, "listener finished with return code %d", lrc);

    close(epoll_fd);
    gexit(GE_UNSET_EPOLLFD, NULL);

    return rc;
}


/**
 * UDP shell - authenticate:
 * Sets up a listener and sends AUTH message `auth_msg`, waits for both
 * CONFIRM and REPLY
 * @return 0 on succes else 1
 * @note success means the authentication went well, that is
 * the REPLY result field was 1
*/
int udpsh_auth(conf_t *conf, msg_t *auth_msg, udp_cnfm_data_t *cnfm_data) {
    assert(auth_msg->username != NULL);
    assert(auth_msg->dname != NULL);
    assert(auth_msg->secret != NULL);
    int rc;

    /* start listener thread (only to return immediately after receiving
    both CONFIRM + REPLY for `auth_msg`, stops without the stop flag) */
    log(DEBUG, "MAIN: starting listener");
    thrd_t listener_thread_id;
    mtx_t listener_mtx;
    bool listener_stop_flag = false;
    bool listener_done_flag = false;
    bool listner_server_sent_bye = false;
    bool listener_waiting_for_reply = false;  // unused here
    uint16_t listener_join_msgid = 0;  // unused here
    if (mtx_init(&listener_mtx, mtx_plain) == thrd_error) {
        log(ERROR, "couldnt initialize lock");
        return ERR_INTERNAL;
    }
    listener_args_t listener_args = {
        .conf = conf,
        .cnfm_data = cnfm_data,
        .mtx = &listener_mtx,
        .save_port = true,
        .auth_msg_id = auth_msg->id,
        .stop_flag = &listener_stop_flag,
        .done_flag = &listener_done_flag,
        .server_sent_bye = &listner_server_sent_bye,
        .waiting_for_reply = &listener_waiting_for_reply,
        .join_msgid = &listener_join_msgid
    };
    rc = thrd_create(&listener_thread_id, udp_listener, &listener_args);
    if (rc != thrd_success) {
        log(ERROR, "couldnt create listener thread");
        return ERR_INTERNAL;
    }
    gexit(GE_SET_LISTHR, &listener_thread_id);
    gexit(GE_SET_STPFLG, &listener_stop_flag);
    gexit(GE_SET_LISMTX, &listener_mtx);

    /* send first AUTH message */
    rc = udp_sender_send(auth_msg, conf, cnfm_data);
    if (rc != 0) {
        fprintf(stderr, ERRPRE "AUTH message wasn't confirmed" ERRSUF);
        log(ERROR, "couldn't send");

        /* let listener finish (else it would wait for the REPLY) */
        mtx_lock(&listener_mtx);
        listener_stop_flag = true;
        mtx_unlock(&listener_mtx);
    }


    /* wait for the listener thread */
    log(DEBUG, "waiting for listener thread");
    int lrc;  // listener return code
    thrd_join(listener_thread_id, &lrc);
    gexit(GE_UNSET_LISTNR, NULL);
    mtx_destroy(&listener_mtx);

    logf(INFO, "REPLY came from port %hu", conf->port);
    logf(INFO, "authentication %s", lrc == 0 ? "successful" : "error");

    /* authentication success from listener + internal success */
    return lrc == 0 and rc == 0 ? 0 : 1;
}


/**
 * Starts a "shell" for authentication. The shell starts a listener, sends
 * messages. Returns when reaching the end of user input or upon receiving BYE.
 * @return 0 on success else non-zero
*/
static int udp_shell(conf_t *conf) {

    int rc = 0;
    ssize_t read_chars = 0;
    msg_t *msg = NULL;
    udp_cnfm_data_t cnfm_data = { .arr = NULL, .len = 0 };
    bool error_occured = false;

    size_t line_length = INIT_LINE_BUFSIZE;
    char *line = mmal(INIT_LINE_BUFSIZE);
    if (line == NULL) { log(ERROR, MEMFAIL_MSG); return ERR_INTERNAL; }

    bool authenticated = false;
    while (not authenticated) {
        log(DEBUG, "shell authentication looping");

        /* read one line (blocking) */
        read_chars = mgetline(&line, &line_length, stdin);
        if (read_chars < 1) {  // eof
            mfree(line);
            mfree(cnfm_data.arr);
            return 0;
        }
        rstriplf(line);
        logf(DEBUG, "read %ld chars: '%s' + LF", read_chars, line);

        if (is_help(line)) {
            printf(CMD_HELP_TXT);
            continue;
        }

        /* process the line */
        msg = parse_auth(line, &error_occured);
        if (error_occured) {
            log(FATAL, "internal error in parse_auth");
            return ERR_INTERNAL;
        }

        /* invalid /auth command, try again */
        if (msg == NULL) continue;

        /* edit the id and increment counter */
        msg->id = conf->cnt;
        conf->cnt += 1;

        /* try to authenticate with the server */
        rc = udpsh_auth(conf, msg, &cnfm_data);

        /* extract dname from msg and put a copy of it to conf */
        /* todo: remove this? */
        char *dname = mstrdup(msg->dname);
        if (dname == NULL) {
            log(ERROR, MEMFAIL_MSG);
            rc = ERR_INTERNAL;
            break;
        }

        /* set or change the display name */
        if (conf->dname != NULL) {
            mfree(conf->dname);
        }
        conf->dname = dname;

        msg_dtor(msg);
        msg = NULL;
        if (rc == 0) {
            log(INFO, "successfully authenticated");
            authenticated = true;
        } else {
            log(ERROR, "couldn't be authenticated");
        }
    }

    gexit(GE_SET_CNFMDP, &cnfm_data);
    udpsh_loop_endlessly(conf, &cnfm_data);
    gexit(GE_UNSET_CNFMDP, NULL);

    mfree(cnfm_data.arr);
    mfree(line);
    return rc;
}


int udp_main(conf_t *conf) {

    int rc = 0;

    /* crate socket + set timeout*/
    rc = udp_create_socket(conf);
    if (rc != 0) {
        log(ERROR, "couldn't create socket");
        return ERR_INTERNAL;
    }
    gexit(GE_SET_FD, &(conf->sockfd));
    rc = udp_set_rcvtimeo(conf, LISTENER_TIMEOUT);
    if (rc != 0) {
        log(ERROR, "couldn't set timeout on socket");
        return ERR_INTERNAL;
    }

    udp_shell(conf);

    gexit(GE_UNSET_FD, NULL);
    close(conf->sockfd);
    log(INFO, "udp client done");
    return rc;
}

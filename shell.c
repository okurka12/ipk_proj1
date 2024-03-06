/*****************
**  Vit Pavlik  **
**   xpavli0a   **
**    251301    **
**              **
**   Created:   **
**  2024-03-03  **
**              **
**    Edited:   **
**  2024-03-03  **
*****************/

/**
 * implementation of shell.h
*/

/**
 *                              @@@@@@
 *                            @@      @@
 *                    @       @        @       @
 *                   @       @          @       @
 *                           @          @
 *               @   @       @          @       @    @
 *              @    @       @          @       @     @
 *             @             @          @              @
 *            @@       @      @        @      @        @
 *          @   @       @     @        @     @       @   @
 *         @     @      @                    @      @     @
 *         @      @      @     @      @     @      @      @
 *         @              @    @      @    @              @
 *          @        @     @              @     @        @
 *            @        @    @   @    @@  @    @        @@
 *              @        @   @          @   @        @
 *                 @       @     @  @@    @       @
 *                    @@     @  @    @  @     @@
 *                       @@@  @@@@@@@@@@  @@@
 *                      @      @@@@@@@@      @
 *                      @@@@@@@@@@@@@@@@@@@@@@
 *
 * Shell - https://www.asciiart.eu/image-to-ascii
 *
*/

#define _POSIX_C_SOURCE 200809L  // getline
#include <assert.h>
#include <stdio.h>  // stdin
#include <string.h>  // strlen
#include "mmal.h"  // mgetline
#include "ipk24chat.h"  // conf_t
#include "shell.h"
#include "utils.h"
#include "msg.h"  // msg_t
#include "gexit.h"
#include "udp_listener.h"  // LISTENER_TIMEOUT
#include "udp_confirmer.h"  // cnfm_data_t
#include "udp_sender.h"

/* initial buffer for the line (if needed, getline reallocs, so this doesnt
matter all that much) */
#define INIT_LINE_BUFSIZE 1024

/* maximal field length including null byte */
#define MFL 8192

/* maximal field length as a string literal
(one less than MFL, for null byte) */
#define MFLS "8191"

/* limited in length string (for a buffer-overflow-safe scanf format) */
#define LLS "%" MFLS "s"

/* strips the trailing line feed of a string if there is one */
void rstriplf(char *s) {
    if (s == NULL) return;
    if (s[0] == '\0') return;
    unsigned int i = 0;
    while (s[i + 1] != '\0') i++;
    if (s[i] == '\n') s[i] = '\0';
}

/* returns if `s` starts with `prefix` */
bool startswith(char *s, char *prefix) {
    if (strlen(s) < strlen(prefix)) return false;
    return strncmp(s, prefix, strlen(prefix)) == 0;
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
        .stop_flag = &listener_stop_flag
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
        fprintf(stderr, "ERROR: authentication message wasn't confirmed");
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
 * @brief Parses /auth command and returns corresponding msg_t
 * if the command is invalid, returns NULL if there is an internal error,
 * returns NULL and sets `error_occured` to true
 * @note returned msg_t needs to be freed with `msg_dtor`
*/
msg_t *parse_auth(char *line, bool *error_occured) {
    int rc = 1;
    msg_t *output = NULL;

    /* allocate space for the fields */
    char *username = mmal(MFL);
    char *secret = mmal(MFL);
    char *dname = mmal(MFL);
    if (username == NULL or secret == NULL or dname == NULL) {
        fprintf(stderr, "%s\n", MEMFAIL_MSG);
        log(ERROR, MEMFAIL_MSG);
        *error_occured = true;
        return NULL;
    }

    /* parse line */
    rc = sscanf(line, "/auth " LLS " " LLS " " LLS,
        username, secret, dname);
    if (rc != 3) {
        fprintf(stderr, "ERROR: not a valid /auth command\n");
        log(WARNING, "not a valid /auth command");
        mfree(username); mfree(secret); mfree(dname);
        *error_occured = false;
        return NULL;
    }
    logf(DEBUG, "got username=%s secret=%s dname=%s",
        username, secret, dname);

    /* create msg_t */
    output = msg_ctor();
    output->type = MTYPE_AUTH;
    output->id = 1;
    output->username = username;
    output->secret = secret;
    output->dname = dname;
    return output;
}


int udp_shell(conf_t *conf) {
    int rc = 0;
    enum sstate state = SS_START;
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
        read_chars = getline(&line, &line_length, stdin);
        if (read_chars < 1) state = SS_END;
        rstriplf(line);
        logf(DEBUG, "read %ld chars: '%s' + LF", read_chars, line);

        /* process the line */
        msg = parse_auth(line, &error_occured);
        if (error_occured) {
            log(INFO, "internal error");
            return ERR_INTERNAL;
        }

        /* invalid /auth command, try again */
        if (msg == NULL) continue;

        /* try to authenticate with the server */
        rc = udpsh_auth(conf, msg, &cnfm_data);
        msg_dtor(msg);
        msg = NULL;
        if (rc == 0) {
            log(INFO, "successfully authenticated");
            authenticated = true;
        } else {
            log(ERROR, "couldn't be authenticated");
        }
        log(INFO, "authenticated successfuly, i am done...");
    }

    mfree(cnfm_data.arr);
    mfree(line);
    return rc;
}

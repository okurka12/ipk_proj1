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

int authenticate(conf_t *conf, msg_t *auth_msg, udp_cnfm_data_t *cnfm_data) {
    assert(auth_msg->username != NULL);
    assert(auth_msg->dname != NULL);
    assert(auth_msg->secret != NULL);
    int rc;


    /* start listener thread (only to return immediately after receiving
    a CONFIRM message for `first_msg`, the lock isn't unlocked by listener) */
    log(DEBUG, "MAIN: starting listener");
    thrd_t listener_thread_id;
    mtx_t listener_mtx;
    mtx_init(&listener_mtx, mtx_plain);
    mtx_lock(&listener_mtx);
    listener_args_t listener_args = {
        .conf = conf,
        .cnfm_data = cnfm_data,
        .mtx = &listener_mtx,
        .save_port = true,
        .auth_msg_id = auth_msg->id
    };
    rc = thrd_create(&listener_thread_id, udp_listener, &listener_args);
    if (rc != thrd_success) {
        log(ERROR, "couldnt create listener thread");
        return ERR_INTERNAL;
    }
    gexit(GE_SET_LISTHR, &listener_thread_id);
    gexit(GE_SET_LISMTX, &listener_mtx);

    /* send first AUTH message */
    rc = udp_sender_send(auth_msg, conf, cnfm_data);
    if (rc != 0) { log(ERROR, "couldn't send"); }


    /* wait for the listener thread */
    log(DEBUG, "waiting for listener thread");
    thrd_join(listener_thread_id, NULL);
    gexit(GE_UNSET_LISTNR, NULL);

    logf(INFO, "REPLY came from port %hu", conf->port);
    return 0;
}

/**
 * handle state, create message
 * should_exit is set to true on error
 * dont forget to check if state isnt ss_end
 */
msg_t *hscm(char *line, enum sstate *state, bool *should_exit) {
    int tmp;
    msg_t *output = NULL;
    (void)output;
    switch (*state) {
        case SS_START:;  // semicolon to suppress warning

            /* allocate space for the fields */
            char *username = mmal(MFL);
            char *secret = mmal(MFL);
            char *dname = mmal(MFL);
            if (username == NULL or secret == NULL or dname == NULL) {
                fprintf(stderr, "%s\n", MEMFAIL_MSG);
                log(ERROR, MEMFAIL_MSG);
                *should_exit = true;
                return NULL;
            }

            /* parse line */
            tmp = sscanf(line, "/auth " LLS " " LLS " " LLS,
                username, secret, dname);
            if (tmp != 3) {
                fprintf(stderr, "ERROR: not a valid /auth command");
                log(WARNING, "not a valid /auth command");
                *should_exit = false;
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

        break;  // case SS_START

        case SS_OPEN: case SS_AUTH: case SS_ERROR: break;

        case SS_END:
        log(DEBUG, "got to state END");
        /* todo todo todo (tady jsem skoncil idk) */

    }

    /* shouldnt get here */
    log(WARNING, "shouldn't be possible to get here");
    return NULL;
}


int udp_shell(conf_t *conf) {
    int rc = 0;
    enum sstate state = SS_START;
    ssize_t read_chars = 0;
    msg_t *msg = NULL;
    udp_cnfm_data_t cnfm_data = { .arr = NULL, .len = 0 };
    bool should_exit = false;

    size_t line_length = INIT_LINE_BUFSIZE;
    char *line = mmal(INIT_LINE_BUFSIZE);
    if (line == NULL) { log(ERROR, MEMFAIL_MSG); return ERR_INTERNAL; }

    while (true) {
        logf(DEBUG, "shell looping (state: %s)", sstate_str(state));

        /* read one line (blocking) */
        read_chars = getline(&line, &line_length, stdin);
        if (read_chars < 1) state = SS_END;
        rstriplf(line);
        logf(DEBUG, "read %ld chars: '%s' + LF", read_chars, line);

        msg = hscm(line, &state, &should_exit);
        if (should_exit) { log(INFO, "shell done..."); return ERR_INTERNAL; }
        if (msg == NULL) { log(WARNING, "no message?"); continue; }

        if (state == SS_START and msg->type == MTYPE_AUTH) {
            rc = authenticate(conf, msg, &cnfm_data);
            msg_dtor(msg);
            msg = NULL;
            if (rc != 0) {
                log(ERROR, "couldn't be authenticated");
            }
            state = SS_END;
        }




        if (state == SS_END) {
            log(DEBUG, "reached state END, breaking...");
            break;
        }
    }

    mfree(cnfm_data.arr);
    mfree(line);
    return rc;
}

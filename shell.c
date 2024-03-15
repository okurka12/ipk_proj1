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
#include "udp_print_msg.h"  // str_isprint

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
static int udpsh_auth(conf_t *conf, msg_t *auth_msg, udp_cnfm_data_t *cnfm_data) {
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
        .server_sent_bye = &listner_server_sent_bye
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
 * @brief Parses /auth command and returns corresponding msg_t
 * if the command is invalid, returns NULL, if there is an internal error,
 * returns NULL and sets `error_occured` to true. Returned msg_t has
 * `id` field set to 0 and is expected to be then edited by the caller
 * @note returned msg_t needs to be freed with `msg_dtor`
*/
static msg_t *parse_auth(const char *line, bool *error_occured) {
    int rc = 1;
    msg_t *output = NULL;

    /* allocate space for the fields */
    char *username = mmal(MFL);
    char *secret = mmal(MFL);
    char *dname = mmal(MFL);
    if (username == NULL or secret == NULL or dname == NULL) {
        mfree(username); mfree(secret); mfree(dname);
        fprintf(stderr, ERRPRE MEMFAIL_MSG ERRSUF);
        log(ERROR, MEMFAIL_MSG);
        *error_occured = true;
        return NULL;
    }

    /* parse line */
    rc = sscanf(line, "/auth " LLS " " LLS " " LLS,
        username, secret, dname);
    if (rc != 3) {
        fprintf(stderr, ERRPRE "not a valid /auth command" ERRSUF);
        log(WARNING, "not a valid /auth command");
        mfree(username); mfree(secret); mfree(dname);
        *error_occured = false;
        return NULL;
    }

    /* check length of the fields */
    bool uname_too_long = strlen(username) > MAX_UNAME_LEN;
    bool secret_too_long = strlen(secret) > MAX_SECRET_LEN;
    bool dname_too_long = strlen(dname) > MAX_DNAME_LEN;
    if (uname_too_long or secret_too_long or dname_too_long) {
        fprintf(stderr, ERRPRE "one of the fields is too long" ERRSUF);
        log(WARNING, "field too long");
        mfree(username); mfree(secret); mfree(dname);
        *error_occured = false;
        return NULL;
    }

    /* check printability of the stuff */
    bool uname_printable = str_isprint(username);
    bool secret_printable = str_isprint(secret);
    bool dname_printable = str_isprint(dname);
    if (not uname_printable or not secret_printable or not dname_printable) {
        fprintf(stderr, ERRPRE "non-printable characters" ERRSUF);
        log(WARNING, "non printable characters");
        mfree(username); mfree(secret); mfree(dname);
        *error_occured = false;
        return NULL;
    }

    /* all ok */
    logf(DEBUG, "got username=%s secret=%s dname=%s",
        username, secret, dname);

    /* create msg_t */
    output = msg_ctor();
    if (output == NULL) { log(ERROR, MEMFAIL_MSG); return NULL; }
    output->type = MTYPE_AUTH;
    output->id = 0;  // to be edited by caller!
    output->username = username;
    output->secret = secret;
    output->dname = dname;
    return output;
}

/**
 * same as parse_auth, but for /join
*/
static msg_t *parse_join(const char *line, bool *error_occured) {

    /* allocate chid buffer */
    char *chid = mmal(MFL);
    if (chid == NULL) {
        fprintf(stderr, ERRPRE MEMFAIL_MSG ERRSUF);
        log(ERROR, MEMFAIL_MSG);
        *error_occured = true;
        return NULL;
    }

    /* scan chid */
    int rc = sscanf(line, "/join " LLS, chid);
    if (rc != 1 or strlen(chid) > MAX_CHID_LEN or not str_isprint(chid)) {
        mfree(chid);
        *error_occured = false;
        return NULL;
    }

    /* allocate msg buffer */
    msg_t *output = msg_ctor();
    if (output == NULL) {
        mfree(chid);
        fprintf(stderr, ERRPRE MEMFAIL_MSG ERRSUF);
        log(ERROR, MEMFAIL_MSG);
        *error_occured = true;
        return NULL;
    }

    output->type = MTYPE_JOIN;
    output->chid = chid;
    return output;
}

static char *parse_rename(char *line, bool *error_occurred) {
    char *dname = mmal(MFL);
    if (dname == NULL) {
        log(ERROR, MEMFAIL_MSG);
        *error_occurred = true;
        return NULL;
    }
    /* todo: non prinable charcters (not here) */
    int rc = sscanf(line, "/rename " LLS, dname);
    if (rc != 1 or strlen (dname) > MAX_DNAME_LEN or not str_isprint(dname)) {
        mfree(dname);
        *error_occurred = false;
        return NULL;
    }
    return dname;
}

static inline bool is_join(char *s) {
    return startswith(s, "/join ");
}

static inline bool is_rename(char *s) {
    return startswith(s, "/rename ");
}

static inline bool is_help(char *s) {
    return startswith(s, "/help");
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
        .server_sent_bye = &listener_server_sent_bye
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

    bool done = false;
    while (not done) {

        /* check whether listener is finished */
        mtx_lock(&listener_mtx);
        if (listener_done_flag) {
            should_send_bye = not listener_server_sent_bye;
            break;
        }
        mtx_unlock(&listener_mtx);

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
        if (is_join(line)) {

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

            /* fill additional parameters and send*/
            msg->id = (conf->cnt);
            msg->dname = conf->dname;
            rc = udp_sender_send(msg, conf, cnfm_data);

            /* destroy message */
            msg->dname = NULL;  // so conf->dname isnt wiped by msg_dtor
            msg_dtor(msg);

            /* was sending successfull? */
            if (rc != 0) {
                fprintf(stderr, ERRPRE "couldn't send" ERRSUF);
                continue;
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
            /* todo: implement printing help */

        /* else: inputted line is a message */
        } else {

            /* check the message content length */
            if (strlen(line) > MAX_MSGCONT_LEN) {
                fprintf(stderr, ERRPRE "message content too long" ERRSUF);
                log(WARNING, "message too long, not sending");
                continue;
            }

            /* don't send empty message */
            if (strlen(line) == 0) {
                fprintf(stderr, ERRPRE "cannot send empty message" ERRSUF);
                log(WARNING, "message empty, not sending");
                continue;
            }

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

        }  // line is a message

    }  // while not done

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
    return rc;
}



int udp_shell(conf_t *conf) {

    /* check if the ipk24chat.h constants are ok with this file's constants */
    static_assert(MAX_UNAME_LEN < MFL, "buffer too small, increase its size");
    static_assert(MAX_CHID_LEN < MFL, "buffer too small, increase its size");
    static_assert(MAX_SECRET_LEN < MFL, "buffer too small, increase its size");
    static_assert(MAX_DNAME_LEN < MFL, "buffer too small, increase its size");

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
        if (read_chars < 1) return 0;  // eof
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

        /* edit the id and increment counter */
        msg->id = conf->cnt;
        conf->cnt += 1;

        /* try to authenticate with the server */
        rc = udpsh_auth(conf, msg, &cnfm_data);

        /* extract dname from msg and put a copy of it to conf */
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

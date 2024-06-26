/*****************
**  Vit Pavlik  **
**   xpavli0a   **
**    251301    **
**              **
**   Created:   **
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

#define _XOPEN_SOURCE  // isascii

#include <assert.h>
#include <stdio.h>  // stdin
#include <string.h>  // strlen
#include <ctype.h>  // isspace
#include <unistd.h>  // isatty, close
#include <sys/epoll.h>

#include "mmal.h"
#include "shell.h"
#include "utils.h"
#include "msg.h"  // msg_t
#include "udp_print_msg.h"  // str_isprint

/* maximal field length including null byte */
#define MFL 8192

/* maximal field length as a string literal
(one less than MFL, for null byte) */
#define MFLS "8191"

/* limited in length string (for a buffer-overflow-safe scanf format) */
#define LLS "%" MFLS "s"

static void check_constants() {
    /* check if the ipk24chat.h constants are ok with this file's constants */
    static_assert(MAX_UNAME_LEN < MFL, "buffer too small, increase its size");
    static_assert(MAX_CHID_LEN < MFL, "buffer too small, increase its size");
    static_assert(MAX_SECRET_LEN < MFL, "buffer too small, increase its size");
    static_assert(MAX_DNAME_LEN < MFL, "buffer too small, increase its size");
}

void rstriplf(char *s) {
    if (s == NULL) return;
    if (s[0] == '\0') return;
    unsigned int i = 0;
    while (s[i + 1] != '\0') i++;
    if (s[i] == '\n') s[i] = '\0';
}

bool startswith(const char *s, const char *prefix) {
    if (strlen(s) < strlen(prefix)) return false;
    return strncmp(s, prefix, strlen(prefix)) == 0;
}

/**
 * checks that `f` is true for all the characters in a string
*/
static bool shell_str_all(const char *str, bool f(const char)) {
    unsigned int i = 0;
    while (str[i] != '\0') {
        char c = str[i];
        if (not f(c)) return false;
        i++;
    }
    return true;
}

/**
 * checks if `c` is ascii alphanumeric or dash (`-`)
*/
static bool shell_isalnum_dash(char c) {
    if (not isascii(c)) return false;
    return isalnum(c) or c == '-';
}

/**
 * checks if `c` is a visible printable character
*/
static bool shell_isvchar(char c) {
    return 0x21 <= c and c <= 0x7e;
}

/**
 * checks if `c` is a visible printable character or space
*/
static bool shell_isvcharsp(char c) {
    return 0x20 <= c and c <= 0x7e;
}


/**
 * checks if display name consists of valid characters
 * @note doesnt check display name length
*/
static bool check_uname(const char *str) {
    return shell_str_all(str, shell_isalnum_dash);
}

/**
 * same as `check_uname` but for secret
*/
static bool check_secret(const char *str) {
    return shell_str_all(str, shell_isalnum_dash);
}

/**
 * same as `check_uname` but for display name
*/
static bool check_dname(const char *str) {
    return shell_str_all(str, shell_isvchar);
}

/**
 * same as `check_uname` but for content
*/
static bool check_content(const char *str) {
    return shell_str_all(str, shell_isvcharsp);
}

/**
 * same as `check_uname` but for channel id
*/
static bool check_chid(const char *str) {
    return shell_str_all(str, shell_isalnum_dash);
}

msg_t *parse_auth(const char *line, bool *error_occured) {
    check_constants();
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

    /* check characters of the stuff */
    bool uname_valid = check_uname(username);
    bool secret_valid = check_secret(secret);
    bool dname_valid = check_dname(dname);
    if (not uname_valid or not secret_valid or not dname_valid) {
        fprintf(stderr, ERRPRE "invalid characters" ERRSUF);
        log(WARNING, "invalid characters");
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

msg_t *parse_join(const char *line, bool *error_occured) {

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
    if (rc != 1 or strlen(chid) > MAX_CHID_LEN or not check_chid(chid)) {
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

char *parse_rename(char *line, bool *error_occurred) {
    char *dname = mmal(MFL);
    if (dname == NULL) {
        log(ERROR, MEMFAIL_MSG);
        *error_occurred = true;
        return NULL;
    }
    int rc = sscanf(line, "/rename " LLS, dname);
    if (rc != 1 or strlen (dname) > MAX_DNAME_LEN or not check_dname(dname)) {
        mfree(dname);
        *error_occurred = false;
        return NULL;
    }
    return dname;
}

static bool iscommand(const char *s, const char *prfx) {
    unsigned int i = strlen(prfx);
    return startswith(s, prfx) and (isspace(s[i]) or s[i] == '\0');
}

bool is_auth(const char *s) {
    return iscommand(s, "/auth");
}
bool is_join(const char *s) {
    return iscommand(s, "/join");
}
bool is_rename(const char *s) {
    return iscommand(s, "/rename");
}
bool is_help(const char *s) {
    return iscommand(s, "/help");
}

bool message_valid(const char *line) {
    assert(line != NULL);

    /* check the message content length */
    if (strlen(line) > MAX_MSGCONT_LEN) {
        fprintf(stderr, ERRPRE "message content too long" ERRSUF);
        log(WARNING, "message too long, not sending");
        return false;
    }

    /* don't send empty message */
    if (strlen(line) == 0) {
        fprintf(stderr, ERRPRE "cannot send empty message" ERRSUF);
        log(WARNING, "message empty, not sending");
        return false;
    }

    /* don't send non-printable characters */
    if (not check_content(line)) {
        fprintf(stderr, ERRPRE "non-printable characters" ERRSUF);
        log(WARNING, "bad characters, not sending");
        return false;
    }

    return true;
}



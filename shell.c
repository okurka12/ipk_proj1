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

#include <stdio.h>  // stdin
#include <string.h>  // strlen
#include "mmal.h"  // mgetline
#include "ipk24chat.h"  // conf_t
#include "shell.h"
#include "utils.h"

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


// int hscm


int udp_shell(conf_t *conf) {
    int rc = 0;
    int tmp = 0;
    enum sstate state = SS_START;
    char *line = NULL;
    size_t line_length = 0;
    ssize_t read_chars = 0;

    (void)state;
    (void)conf;

    while (true) {
    logf(DEBUG, "shell looping (state: %s)", sstate_str(state));

    /* read one line (blocking) */
    read_chars = mgetline(&line, &line_length, stdin);
    if (read_chars < 1) state = SS_END;
    rstriplf(line);
    logf(DEBUG, "read %ld chars: '%s' + LF", read_chars, line);

    switch (state) {
        case SS_START:;  // semicolon to suppress warning
        char *username = mmal(MFL);
        char *secret = mmal(MFL);
        char *dname = mmal(MFL);
        if (username == NULL or secret == NULL or dname == NULL) {
            fprintf(stderr, "%s\n", MEMFAIL_MSG);
            log(ERROR, MEMFAIL_MSG);
            rc = 1;
            goto outside_while;  // double break
        }
        tmp = sscanf(line, "/auth " LLS " " LLS " " LLS,
            username, secret, dname);
        if (tmp != 3) {
            fprintf(stderr, "ERROR: not a valid /auth command");
            log(WARNING, "not a valid /auth command");
            break;  // break just from the switch not the while
        }
        logf(DEBUG, "got username=%s secret=%s dname=%s", username, secret, dname);

        break;

        case SS_END:
        log(DEBUG, "got to state END, breaking");
        goto outside_while;  // double break
    }




    }  // while true
    outside_while:

    mfree(line);
    return rc;
}

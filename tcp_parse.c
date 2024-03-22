/*****************
**  Vit Pavlik  **
**   xpavli0a   **
**    251301    **
**              **
**   Created:   **
**  2024-03-21  **
**              **
**    Edited:   **
**  2024-03-21  **
*****************/

#include <regex.h>
#include "tcp_parse.h"
#include "ipk24chat.h"
#include "utils.h"

/* a buffer for errors by `regerror`, it will be on stack so keep it small */
#define ERRMSG_BUFSIZE 80

const int rflg = REG_ICASE | REG_EXTENDED;


bool tcp_parse_reply(char *data, char **content, bool *err) {
    int rc = 0;
    char errmsg[ERRMSG_BUFSIZE];

    /* compile the regex */
    regex_t pattern;
    regcomp(&pattern, "REPLY (N?OK) IS ([ -~]{1,1400})", rflg);
    if (rc != 0) {
        pinerror("couldn't compile regex");
        regerror(rc, &pattern, errmsg, ERRMSG_BUFSIZE);
        fprintf(stderr, ERRPRE "regexec: %s" ERRSUF, errmsg);
        log(ERROR, "couldn't compile regex");
        *err = true;
        *content = NULL;
        return false;
    }

    /* try to match the regex */
    size_t nmatch = 3;  // whole match, first group, second group
    regmatch_t regmatches[3];
    rc = regexec(&pattern, data, nmatch, regmatches, 0);
    if (rc != 0) {
        log(WARNING, "couldn't match REPLY");
        regerror(rc, &pattern, errmsg, ERRMSG_BUFSIZE);
        fprintf(stderr, ERRPRE "regexec: %s" ERRSUF, errmsg);
        *content = NULL;
        regfree(&pattern);
        return false;
    }

    data[regmatches[0].rm_eo] = '\0';
    *content = data + regmatches[2].rm_so;
    bool reply_ok = regmatches[1].rm_eo - regmatches[1].rm_so == 2;

    logf(DEBUG, "successfully matched, reply_success=%d, content='%s'", reply_ok, *content);

    regfree(&pattern);
    return reply_ok;
}





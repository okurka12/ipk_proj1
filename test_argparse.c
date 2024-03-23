/*****************
**  Vit Pavlik  **
**   xpavli0a   **
**    251301    **
**              **
**   Created:   **
**  2024-02-23  **
**              **
**    Edited:   **
**  2024-02-23  **
*****************/

/**
 *
 * this file utilizes the argparse module and returns 0 or 1 depending on
 * whether the arguments are correct - this code itself does not test argparse
 *
*/
#define _POSIX_C_SOURCE 200809L  // strdup
#include <string.h>
#include "argparse.h"

/* replace the `mstrdup` in mmal.c (that is dependend on many other modules) */
char *mstrdup(char *p) {
    return strdup(p);
}

int main(int argc, char *argv[]) {
    conf_t conf;
    return args_ok(argc, argv, &conf) ? 0 : 1;
}
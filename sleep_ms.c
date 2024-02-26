/*****************
**  Vit Pavlik  **
**   xpavli0a   **
**    251301    **
**              **
**   Created:   **
**  2024-02-26  **
**              **
**    Edited:   **
**  2024-02-26  **
*****************/

/**
 * implementation of the sleep_ms module
*/

#define _POSIX_C_SOURCE 199309L
#include <time.h>
#include "utils.h"

void sleep_ms(unsigned int ms) {
    // logf(DEBUG, "sleeping for %u ms", ms);
    unsigned int sec = ms / 1000;
    unsigned int msec = ms % 1000;
    unsigned int nsec = 1000 * 1000 * msec;
    const struct timespec t = { .tv_sec = sec, .tv_nsec = nsec };
    nanosleep(&t, NULL);
}

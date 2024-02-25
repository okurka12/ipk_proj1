/*****************
**  Vit Pavlik  **
**   xpavli0a   **
**    251301    **
**              **
**   Created:   **
**  2024-02-24  **
**              **
**    Edited:   **
**  2024-02-24  **
*****************/

/**
 * mmal - my malloc
 * implementatiom
 *
*/

#include <stdlib.h>
/**
 * following construct enables testing for when malloc returns NULL
 * malloc itself doesnt return NULL, but mmal does
 */
#define TEST_MALLOC_NULL
#ifdef TEST_MALLOC_NULL

#include <stdio.h>

/* a chance malloc returns NULL */
#define TRESHOLD 0.2

/* returns NULL or `iden` with probability `TRESHOLD` */
#define value(iden) \
(rand() < (int)(TRESHOLD*RAND_MAX) ? \
(fprintf(stderr, "MMAL RETURNING NULL\n"), NULL) : iden)

#else  // ifdef TEST_MALLOC_NULL

/* just returns `iden` */
#define value(iden) (iden)

#endif  // ifdef TEST_MALLOC_NULL

#include "mmal.h"
#include "gexit.h"

void *mmal(size_t size) {
    void *p = malloc(size);
    gexit(GE_REGISTER_PTR, p);
    return value(p);
}

void *mcal(size_t nmemb, size_t size) {
    void *p = calloc(nmemb, size);
    gexit(GE_REGISTER_PTR, p);
    return value(p);
}

void mfree(void *p) {
    gexit(GE_UNREG_PTR, p);
    free(p);
}

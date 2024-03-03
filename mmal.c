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
#define _POSIX_C_SOURCE 200809L  // getline
#include <assert.h>
#include <stdlib.h>
/**
 * following construct enables testing for when malloc returns NULL
 * malloc itself doesnt return NULL, but mmal does
 */
// #define TEST_MALLOC_NULL
#ifdef TEST_MALLOC_NULL

#include <stdio.h>

/* a chance malloc returns NULL */
#define TRESHOLD 0.2

/* returns NULL or `iden` with probability `TRESHOLD` */
#define value(iden) \
(rand() < (int)(TRESHOLD*RAND_MAX) ? \
(fprintf(stderr, "MMAL RETURNING NULL\n"), free(p), NULL) : iden)

#else  // ifdef TEST_MALLOC_NULL

/* just returns `iden` */
#define value(iden) (iden)

#endif  // ifdef TEST_MALLOC_NULL

#include "mmal.h"
#include "gexit.h"

void *mmal(size_t size) {
    void *p = malloc(size);
    p = value(p);
    gexit(GE_REGISTER_PTR, p);
    return p;
}

void *mcal(size_t nmemb, size_t size) {
    void *p = calloc(nmemb, size);
    p = value(p);
    gexit(GE_REGISTER_PTR, p);
    return p;
}

void mfree(void *p) {
    gexit(GE_UNREG_PTR, p);
    free(p);
}

ssize_t mgetline(char **lineptr, size_t *n, FILE *fp) {
    assert(lineptr != NULL);
    ssize_t rc = getline(lineptr, n, fp);
    gexit(GE_REGISTER_PTR, *lineptr);
    return rc;

    /**
     * note: lineptr needs to be feed even
     * if getline fails - see getline(3)
     *
     * also, this function behaves normally even when TEST_MALLOC_NULL
     * is defined, couldn't find a way to simulate getline failure
    */
}

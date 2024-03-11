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
#include <stdio.h>
#include <string.h>

/**
 * following construct enables testing for when malloc returns NULL
 * malloc itself doesnt return NULL, but mmal does
 */
// #define TEST_MALLOC_NULL
#ifdef TEST_MALLOC_NULL


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

char *mstrdup(const char *s) {
    unsigned int length = strlen(s) + 1;
    char *output = mmal(length);
    if (output == NULL) return NULL;
    strcpy(output, s);
    return output;
}

ssize_t mgetline(char **lineptr, size_t *n, FILE *stream) {
    void *original_ptr = *lineptr;
    ssize_t rc = getline(lineptr, n, stream);
    if (original_ptr != *lineptr) {
        gexit(GE_UNREG_PTR, original_ptr);
        gexit(GE_REGISTER_PTR, *lineptr);
    }
    return rc;
}

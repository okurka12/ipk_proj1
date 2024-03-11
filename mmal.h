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
 * module that calls malloc and also registers the returned pointer to the
 * gexit module - that way it gets freed even when the program is interrupted
 *
*/

#include <stddef.h>  // size_t
#include <sys/types.h>  // ssize_t
#include <stdio.h>  // FILE

#ifndef _M_M_A_L_H_
#define _M_M_A_L_H_

/* like malloc, but also registers the pointer in the gexit module */
void *mmal(size_t size);

/* like calloc, but also registers the pointer in the gexit module */
void *mcal(size_t nmemb, size_t size);

/* like free, but also unregisters the pointer in the gexit module */
void mfree(void *ptr);

/* like strdup, but also registers the pointer in the gexit module */
char *mstrdup(const char *s);

/* like getline, but registers the pointer */
ssize_t mgetline(char **lineptr, size_t *n, FILE *stream);

#endif  // ifndef _M_M_A_L_H_

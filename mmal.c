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
#include "mmal.h"
#include "gexit.h"

void *mmal(size_t size) {
    void *p = malloc(size);
    gexit(GE_REGISTER_PTR, p);
    return p;
}

void mfree(void *p) {
    gexit(GE_UNREG_PTR, p);
    free(p);
}

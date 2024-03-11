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
 * implementation of UDP Confirmer - see udp_confirmer.h
*/
#include "ipk24chat.h"  // gcl
#include "udp_confirmer.h"
#include "mmal.h"

/* disable logging just for this module */
#ifndef NDEBUG
#define NDEBUG
#endif  // ifndef NDEBUG
#include "utils.h"

/* unlocks `gcl` and returns `rval` */
#define unlock_return(val) mtx_unlock(&gcl); return val


int udp_cnfm_reg(uint16_t id, udp_cnfm_data_t *data) {
    mtx_lock(&gcl);
    logf(DEBUG, "registering %hu to confirm", id);

    /* case: first call to udp confirmer */
    if (data->arr == NULL) {

        data->arr = mcal(UDP_CNFM_BASE_ARRLEN * sizeof(uint16_t), 1);
        if (data->arr == NULL) {
            perror(MEMFAIL_MSG);
            log(ERROR, MEMFAIL_MSG);
            unlock_return(1);
        }

        data->len = UDP_CNFM_BASE_ARRLEN;
        data->arr[0] = id;
        logf(DEBUG, "registered %hu, (position in the array: %d)", id, 0);
        unlock_return(0);
    }

    /* else: go through the array and find next empty pos */
    for (unsigned int i = 0; i < data->len; i++) {
        if (data->arr[i] == 0) {
            data->arr[i] = id;
            logf(DEBUG, "registered %hu, (position in the array: %u)", id, i);
            unlock_return(0);
        }
    }

    /* no empty pos, expand the array? actually no i wont be doing this
    because it's not necessary because sending messages from client is
    synchronous in my implementation hence there won't be more than
    1 unconfirmed message at a time */
    log(FATAL, "too much unconfirmed messages...");
    unlock_return(1);
}

void udp_cnfm_confirm(uint16_t id, udp_cnfm_data_t *data) {
    mtx_lock(&gcl);
    logf(DEBUG, "confirming %hu", id);
    for (unsigned int i = 0; i < data->len; i++) {
        if (data->arr[i] == id) {
            data->arr[i] = 0;
            logf(DEBUG, "confirmed %hu (pos in the arr: %u)", id, i);
            mtx_unlock(&gcl);
            return;
        }
    }
    logf(DEBUG, "msg id=%hu already confirmed or not registered", id);
    mtx_unlock(&gcl);
}

bool udp_cnfm_was_confirmed(uint16_t id, udp_cnfm_data_t *data) {
    mtx_lock(&gcl);
    for (unsigned int i = 0; i < data->len; i++) {
        if (data->arr[i] == id) {
            logf(DEBUG, "%hu was not confirmed (pos in the arr: %u)", id, i);
            unlock_return(false);
        }
    }
    logf(DEBUG, "%hu was confirmed", id);
    unlock_return(true);
}

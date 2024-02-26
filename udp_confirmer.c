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
#include "utils.h"

/* unlocks `gcl` and returns `rval` */
#define unlock_return(val) mtx_unlock(&gcl); return val


int udp_cnfm_reg(uint16_t id, udp_cnfm_data_t *data) {
    mtx_lock(&gcl);

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
        unlock_return(0);
    }

    /* else: go through the array and find next empty pos */
    for (unsigned int i = 0; i < data->len; i++) {
        if (data->arr[i] == 0) {
            data->arr[i] = id;
            unlock_return(0);
        }
    }

    /* no empty pos, expand the array (todo) */
    log(FATAL, "too much unconfirmed messages...");
    unlock_return(1);
}

void udp_cnfm_confirm(uint16_t id, udp_cnfm_data_t *data) {
    mtx_lock(&gcl);
    for (unsigned int i = 0; i < data->len; i++) {
        if (data->arr[i] == id) {
            data->arr[i] = 0;
            mtx_unlock(&gcl);
            return;
        }
    }
    mtx_unlock(&gcl);
}

bool udp_cnfm_was_confirmed(uint16_t id, udp_cnfm_data_t *data) {
    mtx_lock(&gcl);
    for (unsigned int i = 0; i < data->len; i++) {
        if (data->arr[i] == id) {
            unlock_return(false);
        }
    }
    unlock_return(true);
}

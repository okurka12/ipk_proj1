/*****************
**  Vit Pavlik  **
**   xpavli0a   **
**    251301    **
**              **
**   Created:   **
**  2024-03-16  **
**              **
**    Edited:   **
**  2024-03-16  **
*****************/

#include <stdbool.h>

#include "mmal.h"
#include "udp_marker.h"
#include "utils.h"

#define UDPM_ZERO (1 << 17)

struct udpm_data {

    /* actual 0 marks an empty slot and UDPM_ZERO marks a message id 0,
    this is why we neeed a type larger than uint16_t */
    uint32_t *arr;

    uint64_t len;
};

/**
 * if `freedata` is true, frees data and returns null
 * else
 * returns a pointer to the data, if there is no data, allocates space for the
 * data, if that fails, returns null
*/
static struct udpm_data *udpm_get_data(bool freedata) {
    static struct udpm_data *datap = NULL;

    if (freedata) {
        if (datap != NULL) mfree(datap->arr);
        mfree(datap);
        datap = NULL;
        return NULL;
    }

    /* if there is no data, allocate */
    if (datap == NULL) {
        datap = mcal(sizeof(struct udpm_data), 1);
        if (datap == NULL) return NULL;
        datap->arr = mcal(sizeof(uint32_t) * UDPM_MAX_SEEN, 1);
        if (datap->arr == NULL) {
            mfree(datap);
            datap = NULL;
            return NULL;
        }
        datap->len = UDPM_MAX_SEEN;
    }

    return datap;
}

int udpm_mark(uint16_t msgid) {

    struct udpm_data *datap = udpm_get_data(false);
    if (datap == NULL) return 1;

    uint32_t msg_id = msgid == 0 ? UDPM_ZERO : msgid;

    for (uint64_t i = 0; i < UDPM_MAX_SEEN; i++) {
        if (datap->arr[i] == 0) {
            datap->arr[i] = msg_id;
            return 0;
        }

    }

    log(WARNING, "this should never happen");
    return 1;
}

bool udpm_was_seen(uint16_t msgid) {
    struct udpm_data *datap = udpm_get_data(false);
    if (datap == NULL) return false;

    uint32_t msg_id = msgid == 0 ? UDPM_ZERO : msgid;

    for (uint64_t i = 0; i < UDPM_MAX_SEEN; i++) {
        if (datap->arr[i] == msg_id) return true;
        if (datap->arr[i] == 0) return false;
    }

    log(WARNING, "this should never happen");
    return false;
}

void udpm_free_res() {
    udpm_get_data(true);
}

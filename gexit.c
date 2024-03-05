/*****************
**  Vit Pavlik  **
**   xpavli0a   **
**    251301    **
**              **
**   Created:   **
**  2024-02-23  **
**              **
**    Edited:   **
**  2024-02-24  **
*****************/

/**
 *
 * implementation of the `gexit` - gracefully exit module
 * see gexit.h
 *
 * @note malloc here is not null-checked because it doesn't affect the
 * program's ability to function correctly, it just doesn't free memory
 * (but only if it's being exited with SIGINT!)
 *
 */

#include <unistd.h>  // close
#include <stdlib.h>  // free
#include <stdbool.h>
#include <threads.h>  // types, unlock listener lock

extern mtx_t gcl;


// these wont be necessary ig
// #include <sys/types.h>
// #include <sys/socket.h>
// #include <sys/time.h>  // struct timeval
// #include <netinet/in.h>  // struct sockaddr_in
// #include <arpa/inet.h>  // htons

#include "ipk24chat.h"
#include "gexit.h"
#include "utils.h"

/* how long should the array of the pointers be */
#define NUM_PTRS 256

/**
 * Private: initialize all positions of `arr` to NULL
 * @param arr the array to initialize
 * @param len length of the array
*/
void gexit_init_arr(void **arr, unsigned int len) {
    for (unsigned int i = 0; i < len; i++) {
        arr[i] = NULL;
    }
}

/**
 * Private: register a pointer to be freed if exiting via gexit
 * @param ptr_arr pointer to array of pointers (void ***)
 * @param len pointer to the length of the array
 * @param p the pointer to register
 * @note can be called with `p` equal to NULL (in that case, it does nothing)
 * @note if allocation fails, does nothing, therefore the pointer is not
 * registered and won't be freed if exiting via gexit (which is a risk i am
 * willing to take)
*/
void gexit_regptr(void ***ptr_arr, unsigned int *len, void *p) {

    if (p == NULL) {
        return;
    }

    /* case: first call to gexit_regptr - allocate a ptr_arr */
    if (len == 0) {
        *ptr_arr = malloc(sizeof(void *) * NUM_PTRS);
        if (*ptr_arr == NULL) return;
        *len = NUM_PTRS;
        gexit_init_arr(*ptr_arr, *len);
        (*ptr_arr)[0] = p;
    }

    /* write `p` to the first empty position and return*/
    for (unsigned int i = 0; i < *len; i++) {
        if ((*ptr_arr)[i] == NULL) {
            (*ptr_arr)[i] = p;
            return;
        }
    }

    /* if we got here, that means the array is not long enough */
    *ptr_arr = realloc(*ptr_arr, sizeof(void *) * (*len + NUM_PTRS));
    if (*ptr_arr == NULL) {
        return;
    }

    /* init the new part of the array */
    gexit_init_arr(*ptr_arr + *len, NUM_PTRS);

    (*ptr_arr)[*len] = p;
    *len += NUM_PTRS;
}


/**
 * Private: unregister a pointer
 * @param ptr_arr pointer to array of pointers (void ***)
 * @param len pointer to the length of the array
 * @param p the pointer to unregister
 *
*/
void gexit_unregptr(void ***ptrs, unsigned int *len, void *p) {
    for (unsigned int i = 0; i < *len; i++) {
        if ((*ptrs)[i] == p) {
            (*ptrs)[i] = NULL;
        }
    }
}


/**
 * Private: free all the registered pointers
 * @param ptr_arr pointer to array of pointers (void ***)
 * @param len pointer to the length of the array
*/
void gexit_free_all(void ***ptrs, unsigned int *len) {
    for (unsigned int i = 0; i < *len; i++) {
        if ((*ptrs)[i] != NULL) {
            free((*ptrs)[i]);
        }
    }
    free(*ptrs);
}


void gexit(enum gexit_statement statement, void *p) {
    static int sockfd = -1;
    static conf_t *confp = NULL;

    /* array of the pointers to free */
    static void **ptrs = NULL;
    static unsigned int ptrs_len = 0;

    static thrd_t listener_thread_id = 0;
    static mtx_t *listener_lock = NULL;

    /* todo: remove this */
    (void)sockfd;
    (void)confp;

    switch (statement) {

    case GE_SET_CONFP:
        confp = (conf_t *)p;
        break;

    case GE_SET_FD:
        sockfd = *((int *)p);
        break;

    case GE_REGISTER_PTR:
        mtx_lock(&gcl);
        gexit_regptr(&ptrs, &ptrs_len, p);
        mtx_unlock(&gcl);
        break;

    case GE_SET_LISTHR:
        listener_thread_id = *((thrd_t *)p);
        break;

    case GE_SET_LISMTX:
        listener_lock = (mtx_t *)p;
        break;

    case GE_UNSET_LISTNR:
        listener_lock = NULL;
        listener_thread_id = thrd_error;
        break;

    case GE_UNREG_PTR:
        mtx_lock(&gcl);
        gexit_unregptr(&ptrs, &ptrs_len, p);
        mtx_unlock(&gcl);
        break;

    case GE_UNSET_FD:
        sockfd = -1;
        break;

    case GE_UNSET_CONFP:
        confp = NULL;
        break;

    case GE_TERMINATE:
        log(INFO, "the program was interrupted, exiting");
        int rc = 0;
        /*todo:cleanup (done ig?)*/
        if (sockfd != -1) close(sockfd);
        if (listener_lock != NULL) {
            mtx_unlock(listener_lock);
            log(DEBUG, "gexit: waiting for listener thread...");
            thrd_join(listener_thread_id, NULL);
        }
        gexit_free_all(&ptrs, &ptrs_len);
        exit(rc);

    case GE_FREE_RES:
        free(ptrs);
        ptrs = NULL;
        break;

    /* todo: all the other cases */

    /* shouldn't happen */
    default:
        logf(ERROR, "gexit called with invalid statement: %d", statement);
        break;
    }
}

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
#include <threads.h>  // thrd_join, mtx_unlock...
#include <sys/socket.h>  // shutdown

#include "ipk24chat.h"
#include "gexit.h"
#include "utils.h"
#include "msg.h"
#include "udp_sender.h"
#include "udp_confirmer.h"  // udp_cnfm_t
#include "tcpcl.h"  // tcp_send

extern mtx_t gcl;

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
    static int epollfd = -1;
    static conf_t *confp = NULL;
    static udp_cnfm_data_t *cnfmdp = NULL;

    /* array of the pointers to free */
    static void **ptrs = NULL;
    static unsigned int ptrs_len = 0;

    static thrd_t listener_thread_id = 0;
    static mtx_t *listener_lock = NULL;
    static bool *listener_stop_flag = NULL;

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
        listener_stop_flag = NULL;
        break;

    case GE_SET_STPFLG:
        listener_stop_flag = (bool *)p;
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

    case GE_SET_CNFMDP:
        cnfmdp = (udp_cnfm_data_t *)p;
        break;

    case GE_UNSET_CNFMDP:
        cnfmdp = NULL;
        break;

    case GE_SET_EPOLLFD:
        epollfd = *((int *)p);
        break;

    case GE_UNSET_EPOLLFD:
        epollfd = -1;
        break;

    case GE_TERMINATE:
        log(INFO, "the program was interrupted, exiting");
        int rc = 0;

        msg_t last_msg;
        last_msg.type = MTYPE_BYE;
        last_msg.id = LAST_MSGID;
        if (confp != NULL and cnfmdp != NULL) {
            udp_sender_send(&last_msg, confp, cnfmdp);
        }
        if (confp != NULL and confp->tp == TCP) {
            tcp_send(confp, &last_msg);  // success or not? idc, i tried...
            shutdown(sockfd, SHUT_RDWR);
        }

        bool c1 = listener_lock != NULL;
        bool c2 = listener_stop_flag != NULL;
        bool c3 = listener_thread_id != thrd_error;
        if (c1 and c2 and c3) {
            log(DEBUG, "gexit: letting listener finish");
            mtx_lock(listener_lock);
            *listener_stop_flag = true;
            mtx_unlock(listener_lock);
            log(DEBUG, "gexit: waiting for listener thread...");
            thrd_join(listener_thread_id, NULL);
        }

        if (sockfd != -1) close(sockfd);
        if (epollfd != -1) close(epollfd);
        gexit_free_all(&ptrs, &ptrs_len);
        exit(rc);

    case GE_FREE_RES:
        free(ptrs);
        ptrs = NULL;
        break;

    }
}

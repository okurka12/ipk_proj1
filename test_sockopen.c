/**
 * this file opens and closes udp socket n times, to see how long it takes
 *
*/

const unsigned int n = 1000000;

/* do we want to set a timeout to the socket? */
#define SETSOCKOPT 1
const unsigned int t = 250;  // ms

#include <unistd.h>  // close
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>  // struct timeval
#include <netinet/in.h>  // struct sockaddr_in
#include <arpa/inet.h>  // htons

#include "utils.h"

static inline void open_and_close() {
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd == -1) {
        perror("socket creation failed");
        log(ERROR, "socket creation failed");
        return;
    }

    #if SETSOCKOPT
    struct timeval timeval = { .tv_usec = t * 1000 };
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeval, sizeof(timeval));
    #endif  // if SETSOCKOPT

    close(sockfd);
}

int main() {
    for (unsigned int i = 0; i < n; i++) {
        // logf(DEBUG, "opening %u", i);
        open_and_close();
    }
}



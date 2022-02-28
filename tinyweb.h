#ifndef __TINY_H
#define __TINY_H

#include <sys/types.h>

typedef struct {
    char filename[512];
    off_t offset; /* for support Range */
    size_t end;
} http_request;

// Launch tinyweb server
int tinyweb_main(int argc, char **argv);

// Tinyweb accept connection
int tinyweb_accept(int listenfd);

// Process http request
void parse_request(int fd, http_request *req);

#endif /* __TINY_H */
#include "tinyweb.h"

#include <arpa/inet.h> /* inet_ntoa */
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define LISTENQ 1024 /* second argument to listen() */
#define MAXLINE 1024 /* max length of a line */
#define RIO_BUFSIZE 1024

#ifndef DEFAULT_PORT
#define DEFAULT_PORT 9999 /* use this port if none given as arg to main() */
#endif

#ifndef FORK_COUNT
#define FORK_COUNT 4
#endif

#ifndef NO_LOG_ACCESS
#define LOG_ACCESS
#endif

typedef struct {
    int rio_fd;                /* descriptor for this buf */
    int rio_cnt;               /* unread byte in this buf */
    char *rio_bufptr;          /* next unread byte in this buf */
    char rio_buf[RIO_BUFSIZE]; /* internal buffer */
} rio_t;

/* Simplifies calls to bind(), connect(), and accept() */
typedef struct sockaddr SA;

typedef struct {
    const char *extension;
    const char *mime_type;
} mime_map;

// https://developer.mozilla.org/en-US/docs/Web/HTTP/Basics_of_HTTP/MIME_types/Common_types

mime_map meme_types[] = {
    {".aac", "audio/aac"},
    {".abw", "application/x-abiword"},
    {".arc", "application/x-freearc"},
    {".avi", "video/x-msvideo"},
    {".azw", "application/vnd.amazon.ebook"},
    {".bin", "application/octet-stream"},
    {".bmp", "image/bmp"},
    {".bz", "application/x-bzip"},
    {".bz2", "application/x-bzip2"},
    {".csh", "application/x-csh"},
    {".css", "text/css"},
    {".csv", "text/csv"},
    {".doc", "application/msword"},
    {".docx",
     "application/vnd.openxmlformats-officedocument.wordprocessingml.document"},
    {".eot", "application/vnd.ms-fontobject"},
    {".epub", "application/epub+zip"},
    {".gz", "application/gzip"},
    {".gif", "image/gif"},
    {".htm", "text/html"},
    {".html", "text/html"},
    {".ico", "image/vnd.microsoft.icon"},
    {".ics", "text/calendar"},
    {".jar", "application/java-archive"},
    {".jpeg", "image/jpeg"},
    {".jpg", "image/jpeg"},
    {".js", "text/javascript"},
    {".json", "application/json"},
    {".jsonld", "application/ld+json"},
    {".mid", "audio/midi audio/x-midi"},
    {".midi", "audio/midi audio/x-midi"},
    {".mjs", "text/javascript"},
    {".mp3", "audio/mpeg"},
    {".mp4", "video/mp4"},
    {".mpeg", "video/mpeg"},
    {".mpkg", "application/vnd.apple.installer+xml"},
    {".odp", "application/vnd.oasis.opendocument.presentation"},
    {".ods", "application/vnd.oasis.opendocument.spreadsheet"},
    {".odt", "application/vnd.oasis.opendocument.text"},
    {".oga", "audio/ogg"},
    {".ogv", "video/ogg"},
    {".ogx", "application/ogg"},
    {".opus", "audio/opus"},
    {".otf", "font/otf"},
    {".png", "image/png"},
    {".pdf", "application/pdf"},
    {".php", "application/x-httpd-php"},
    {".ppt", "application/vnd.ms-powerpoint"},
    {".pptx",
     "application/"
     "vnd.openxmlformats-officedocument.presentationml.presentation"},
    {".rar", "application/vnd.rar"},
    {".rtf", "application/rtf"},
    {".sh", "application/x-sh"},
    {".svg", "image/svg+xml"},
    {".swf", "application/x-shockwave-flash"},
    {".tar", "application/x-tar"},
    {".tif", "image/tiff"},
    {".tiff", "image/tiff"},
    {".ts", "video/mp2t"},
    {".ttf", "font/ttf"},
    {".txt", "text/plain"},
    {".vsd", "application/vnd.visio"},
    {".wav", "audio/wav"},
    {".weba", "audio/webm"},
    {".webm", "video/webm"},
    {".webp", "image/webp"},
    {".woff", "font/woff"},
    {".woff2", "font/woff2"},
    {".xhtml", "application/xhtml+xml"},
    {".xls", "application/vnd.ms-excel"},
    {".xlsx",
     "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet"},
    {".xml", "text/xml"},
    {".xul", "application/vnd.mozilla.xul+xml"},
    {".zip", "application/zip"},
    {".3gp", "video/3gpp"},
    {".3g2", "video/3gpp2"},
    {".7z", "application/x-7z-compressed"},
    {NULL, NULL},
};

char *default_mime_type = "text/plain";

void rio_readinitb(rio_t *rp, int fd)
{
    rp->rio_fd = fd;
    rp->rio_cnt = 0;
    rp->rio_bufptr = rp->rio_buf;
}

/*
 * rio_read - This is a wrapper for the Unix read() function that
 *    transfers min(n, rio_cnt) bytes from an internal buffer to a user
 *    buffer, where n is the number of bytes requested by the user and
 *    rio_cnt is the number of unread bytes in the internal buffer. On
 *    entry, rio_read() refills the internal buffer via a call to
 *    read() if the internal buffer is empty.
 */
/* $begin rio_read */
static ssize_t rio_read(rio_t *rp, char *usrbuf, size_t n)
{
    int cnt;
    while (rp->rio_cnt <= 0) { /* refill if buf is empty */

        rp->rio_cnt = read(rp->rio_fd, rp->rio_buf, sizeof(rp->rio_buf));
        if (rp->rio_cnt < 0) {
            if (errno != EINTR) { /* interrupted by sig handler return */
                return -1;
            }
        } else if (rp->rio_cnt == 0) { /* EOF */
            return 0;
        } else
            rp->rio_bufptr = rp->rio_buf; /* reset buffer ptr */
    }

    /* Copy min(n, rp->rio_cnt) bytes from internal buf to user buf */
    cnt = n;
    if (rp->rio_cnt < n) {
        cnt = rp->rio_cnt;
    }
    memcpy(usrbuf, rp->rio_bufptr, cnt);
    rp->rio_bufptr += cnt;
    rp->rio_cnt -= cnt;
    return cnt;
}

/*
 * rio_readlineb - robustly read a text line (buffered)
 */
ssize_t rio_readlineb(rio_t *rp, void *usrbuf, size_t maxlen)
{
    int n;
    char c, *bufp = usrbuf;

    for (n = 1; n < maxlen; n++) {
        int rc;
        if ((rc = rio_read(rp, &c, 1)) == 1) {
            *bufp++ = c;
            if (c == '\n') {
                break;
            }
        } else if (rc == 0) {
            if (n == 1) {
                return 0; /* EOF, no data read */
            } else {
                break; /* EOF, some data was read */
            }
        } else {
            return -1; /* error */
        }
    }
    *bufp = 0;
    return n;
}

int open_listenfd(int port)
{
    int listenfd, optval = 1;
    struct sockaddr_in serveraddr;

    /* Create a socket descriptor */
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        return -1;
    }

    /* Eliminates "Address already in use" error from bind. */
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const void *) &optval,
                   sizeof(int)) < 0) {
        return -1;
    }

    // 6 is TCP's protocol number
    // enable this, much faster : 4000 req/s -> 17000 req/s
    if (setsockopt(listenfd, 6, TCP_CORK, (const void *) &optval, sizeof(int)) <
        0) {
        return -1;
    }

    /* Listenfd will be an endpoint for all requests to port
       on any IP address for this host */
    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons((unsigned short) port);
    if (bind(listenfd, (SA *) &serveraddr, sizeof(serveraddr)) < 0) {
        return -1;
    }

    /* Make it a listening socket ready to accept connection requests */
    if (listen(listenfd, LISTENQ) < 0) {
        return -1;
    }
    return listenfd;
}

void url_decode(char *src, char *dest, int max)
{
    char *p = src;
    char code[3] = {0};
    while (*p && --max) {
        if (*p == '%') {
            memcpy(code, ++p, 2);
            *dest++ = (char) strtoul(code, NULL, 16);
            p += 2;
        } else {
            *dest++ = *p++;
        }
    }
    *dest = '\0';
}

void parse_request(int fd, http_request *req)
{
    rio_t rio;
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE];
    req->offset = 0;
    req->end = 0; /* default */

    rio_readinitb(&rio, fd);
    rio_readlineb(&rio, buf, MAXLINE);
    sscanf(buf, "%1023s %1023s", method, uri); /* version is not cared */
    /* read all */
    while (buf[0] != '\n' && buf[1] != '\n') { /* \n || \r\n */
        rio_readlineb(&rio, buf, MAXLINE);
        if (buf[0] == 'R' && buf[1] == 'a' && buf[2] == 'n') {
            sscanf(buf, "Range: bytes=%zu-%zu", &req->offset, &req->end);
            // Range: [start, end]
            if (req->end != 0) {
                req->end++;
            }
        }
    }
    char *filename = uri;
    if (uri[0] == '/') {
        filename = uri + 1;
        int length = strlen(filename);
        if (length == 0) {
            filename = ".";
        } else {
            int i = 0;
            for (; i < length; ++i) {
                if (filename[i] == '?') {
                    filename[i] = '\0';
                    break;
                }
            }
        }
    }
    url_decode(filename, req->filename, MAXLINE);
}

void print_help()
{
    printf("TINY WEBSERVER HELP\n");
    printf("tiny            #use default port, serve current dir\n");
    printf("tiny /tmp       #use default port, serve given dir\n");
    printf("tiny 1234       #use given port, serve current dir\n");
    printf("tiny /tmp 1234  #use given port, serve given dir\n");
    printf("default port is %d.\n", DEFAULT_PORT);
}

// Launch tinyweb server
// Return accepted fd
// Return 0 if error or failed.
int tinyweb_main(int argc, char **argv)
{
    if (argc > 1 && (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help"))) {
        print_help();
        return 0;
    }

    int default_port = DEFAULT_PORT, listenfd;
    char buf[256];
    char *path = getcwd(buf, 256);
    if (argc == 2) {
        if (argv[1][0] >= '0' && argv[1][0] <= '9') {
            default_port = atoi(argv[1]);
        } else {
            path = argv[1];
            if (chdir(path) != 0) {
                perror(path);
                exit(1);
            }
        }
    } else if (argc == 3) {
        default_port = atoi(argv[2]);
        path = argv[1];
        if (chdir(path) != 0) {
            perror(path);
            exit(1);
        }
    }
    printf("serve directory '%s'\n", path);

    listenfd = open_listenfd(default_port);
    if (listenfd > 0) {
        printf("listen on port %d, fd is %d\n", default_port, listenfd);
    } else {
        perror("ERROR");
        return 0;
    }
    // Ignore SIGPIPE signal, so if browser cancels the request, it
    // won't kill the whole process.
    signal(SIGPIPE, SIG_IGN);
    return listenfd;
}

// Tinyweb accept connection
// Return connection fd
int tinyweb_accept(int listenfd)
{
    struct sockaddr_in clientaddr;
    socklen_t clientlen = sizeof clientaddr;
    return accept(listenfd, (SA *) &clientaddr, &clientlen);
}
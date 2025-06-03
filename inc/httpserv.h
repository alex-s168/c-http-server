#ifndef _HTTP_SERV_H
#define _HTTP_SERV_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

enum HttpMethod {
    GET,
    POST,
    UNKNOWN
};

typedef enum {
    HTTP_CONTENT_BYTES,

    /** compression is currently not supported for this */
    HTTP_CONTENT_ITER,

    /** compression is currently not supported for this */
    HTTP_CONTENT_FILE,
} HttpContentMode;

typedef struct {
    const char *content;
    bool free_after;
} HttpBytesContent;

typedef struct {
    /** fread()s in chunks until content_size is reached */
    FILE* fp;
    bool close_after;
} HttpFileContent;

typedef struct {
    bool free_ptr;
    char* ptr;
    size_t len; // or 0 to indicate end of data
    void* new_userptr;
} HttpIterRes;

typedef struct {
    void* userptr;
    HttpIterRes (*next)(void* userptr);
} HttpIterContent;

/** sets content_mode, content_size and content_val to represent the input string */
#define HTTP_RESPONSE_TEXT(str) \
    .content_val.bytes = (HttpBytesContent) { .content = (str), .free_after = false }, \
    .content_size = strlen(str), \
    .content_mode = HTTP_CONTENT_BYTES,

struct HttpResponse {
    int status;
    const char *status_msg;
    const char *content_type;

    size_t content_size;
    HttpContentMode content_mode;
    union {
        HttpBytesContent bytes;
        HttpIterContent iter;
        HttpFileContent file;
    } content_val;

    const char *_internal_optional_encoding;
};

struct HttpRequest {
    enum HttpMethod method;
    
    const char *path;

    struct Header *headers;
    size_t headers_size;

    const char **accepted_encodings;
    size_t accepted_encodings_size;

    char *body;
    size_t body_size;
};

const char *http_header_get(struct HttpRequest const* request, const char *name);

#ifdef DEF_LOG
# include <stdio.h>
# define LOGF(fmt, ...)  fprintf(stderr, "[DEBG] " fmt "\n" __VA_OPT__(,) __VA_ARGS__)
# define WARNF(fmt, ...) fprintf(stderr, "[WARN] " fmt "\n" __VA_OPT__(,)  __VA_ARGS__)
# define ERRF(fmt, ...)  fprintf(stderr, "[ERRR] " fmt "\n" __VA_OPT__(,) __VA_ARGS__)
#endif

typedef struct HttpResponse (*Http_fn)(struct HttpRequest request, void* userdata);

typedef struct {
    char const* bind;
    int         reuse;
    size_t      num_threads;
    size_t      con_sleep_us;
    size_t      max_enq_con;
    Http_fn     handler;
} HttpCfg;

typedef struct Http Http;

Http* http_open(HttpCfg cfg, void* userdata);

/** accept new connection in thread or wait [cfg.con_sleep_us] */
void http_tick(Http* server);

/** waits for all cons to be processed */
void http_close(Http* server);

void http_slowlyStop(Http* server);
bool http_isStopping(Http* server);

/** chechks the extension of path; if it can't figure it out, always defaults to text/plain */
const char* http_detectMime(const char* path, char const* default_mine);

size_t http_urldecode(char *dst, size_t dstlen, const char *src);

#endif

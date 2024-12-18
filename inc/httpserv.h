#ifndef _HTTP_SERV_H
#define _HTTP_SERV_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

enum HttpMethod {
    GET,
    POST,
    UNKNOWN
};

struct HttpResponse {
    int status;
    const char *status_msg;
    const char *content_type;
    const char *content;
    size_t content_size;
    bool free_content;

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
    uint16_t port;
    int      reuse;
    size_t   num_threads;
    size_t   con_sleep_us;
    size_t   max_enq_con;
    Http_fn  handler;
} HttpCfg;

typedef struct Http Http;

Http* http_open(HttpCfg cfg, void* userdata);

/** accept new connection in thread or wait [cfg.con_sleep_us] */
void http_tick(Http* server);

/** wait for all current connections to be processed */
void http_wait(Http* server);

void http_close(Http* server);

void http_slowlyStop(Http* server);
bool http_isStopping(Http* server);

#endif

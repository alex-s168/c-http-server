#define DEF_LOG
#include "../inc/httpserv.h"
#include "../thread-pool/threadpool.h"
#include <stdatomic.h>
#include <stdlib.h>

struct Header {
    const char *name;
    const char *value;
};

#define SPLITERATE(str,split,p) for (const char *p = strtok(str, split); p != NULL; p = strtok(NULL, split))

typedef struct {
    const char * name;
    char* (*compress)(const char* input, size_t inputSize, size_t *lenOut);
} CompressAlgo;

/** null terminated */
extern CompressAlgo http__compressAlgo[];

typedef struct {
    Http* server;
    int fd;
} ServerConnection;

struct Http {
    void* userdata;

    HttpCfg cfg;

    bool running;
    threadpool_t* pool;

    int server_fd;

    atomic_size_t enq_con;
};

int http__handle_connection(ServerConnection* server);

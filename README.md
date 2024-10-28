# C HTTP Server Library
Small, save, and easy to use HTTP server library for Unix systems.

Features:
- Multi-Threaded request handler
- Compression of result data using ZSTD and ZLIB

There are plans to add Windows support.

## Simple Example 
For a more advanced example, see `example.c`.

```c 
#define DEF_LOG
#include "inc/httpserv.h"

#include <string.h>
#include <stdlib.h>

static struct HttpResponse serve(struct HttpRequest request, void* userdata) {
    (void) userdata;

    struct HttpResponse r = (struct HttpResponse) {
        .status = 404,
        .status_msg = "Not Found",
        .content_type = "text/plain",
        .content = "Page not found!",
        .content_size = SIZE_MAX,
        .free_content = false,
    };

    if (memcmp(request.path, "/echo/", 6) == 0)
    {
        r.status = 200;
        r.status_msg = "OK";
        r.content_type = "text/plain";
        r.content = request.path + 6;
    }

    if (r.content_size == SIZE_MAX)
        r.content_size = strlen(r.content);

    return r;
}

int main(int argc, char **argv)
{
    HttpCfg cfg = (HttpCfg) {
        .port = 80,
        .reuse = 1,
        .num_threads = 4,
        .con_sleep_us = 1000 * (/*ms*/ 5),
        .max_enq_con = 128,
        .handler = serve,
    };
    Http* server = http_open(cfg, NULL);
    while (true)
        http_tick(server);
	return 0;
}
```

## Using in a project
The recommended way of doing that is to add this repository as GIT submodule,
and compile all the relevant source files: `src/*.c` and `C-Thread-Pool/thpool.c`.

It is recommended to enable support for ZLib or Zstd:
- define `HAVE_ZLIB` and link with Zlib (on most systems: `-lz`)
- define `HAVE_ZSTD` and link with Zstd (on most systems: `-lzstd`)

See `compile_example.sh` as example.

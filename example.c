#define DEF_LOG
#include "inc/httpserv.h"
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

static Http* server = NULL;

static struct HttpResponse serve(struct HttpRequest request, void* userdata) {
    (void) userdata;

    if (strcmp(request.path, "/") == 0)
    {
        return (struct HttpResponse) {
            .status = 200,
            .status_msg = "OK",
            .content_type = "text/plain",
            HTTP_RESPONSE_TEXT("<html><body><h1>Hello, World!</h1></body></html>")
        };
    }
    else if (memcmp(request.path, "/echo/", 6) == 0)
    {
        return (struct HttpResponse) {
            .status = 200,
            .status_msg = "OK",
            .content_type = "text/plain",
            HTTP_RESPONSE_TEXT(request.path + 6)
        };
    }
    else if (strcmp(request.path, "/user-agent") == 0)
    {
        const char *user_agent = http_header_get(&request, "User-Agent");

        return (struct HttpResponse) {
            .status = 200,
            .status_msg = "OK",
            .content_type = "text/plain",
            HTTP_RESPONSE_TEXT(user_agent ? user_agent : "User-Agent header not found")
        };
    }
    else
    {
        return (struct HttpResponse) {
            .status = 404,
            .status_msg = "Not Found",
            .content_type = "text/plain",
            HTTP_RESPONSE_TEXT("Page not found!")
        };
    }
}

static void sigint_handler(int sig) {
    if (server && !http_isStopping(server)) {
        http_slowlyStop(server);
    } else {
        exit(130);
    }
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

    server = http_open(cfg, NULL);

    signal(SIGINT, sigint_handler);

	LOGF("Waiting for clients to connect...");

    while (true)
    {
        http_tick(server);
        if (http_isStopping(server)) {
            LOGF("Stopping...");
            http_wait(server);
            break;
        }
    }

    http_close(server);
	return 0;
}

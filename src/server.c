#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <pthread.h>
#include <string.h>
#include <threads.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include "internal.h"

const char *http_header_get(struct HttpRequest const* request, const char *name) {
    for (size_t i = 0; i < request->headers_size; i++) {
        if (strcmp(request->headers[i].name, name) == 0) {
            return request->headers[i].value;
        }
    }
    return NULL;
}

static void handle_task(void* _con)
{
    ServerConnection* con = _con;
    con->server->enq_con --;
    http__handle_connection(con);
    free(con);
}

static int ipaddr(struct sockaddr_in* out, char const* addrIn)
{
    char addr[128];
    strncpy(addr, addrIn, 128);

    struct addrinfo hints = {0};
    struct addrinfo *res;

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    char* colon = strrchr(addr, ':');
    if (!colon) return 1;
    *colon = '\0';
    char const* port = colon + 1;

    if ( getaddrinfo(addr, port, &hints, &res) )
        return 1;

    for (struct addrinfo* r = res; r; r = r->ai_next)
    {
        if (r->ai_family == AF_INET)
        {
            struct sockaddr_in *saddr = (struct sockaddr_in *) r->ai_addr;
            memcpy(out, saddr, sizeof(*saddr));
            return 0;
        }
        freeaddrinfo(r);
    }

    return 1;
}

Http* http_open(HttpCfg cfg, void* userdata)
{
    if (cfg.bind == NULL) {
        ERRF("HttpCfg.bind is not set");
        return NULL;
    }

	struct sockaddr_in serv_addr;
    if ( ipaddr(&serv_addr, cfg.bind) ) {
        ERRF("couldn't lookup bind address (%s)", cfg.bind);
        return NULL;
    }

    Http* server = malloc(sizeof(Http));
    if (server == NULL) {
        ERRF("Out of memory (requested %zu bytes)", sizeof(Http));
        return NULL;
    }
    memset(server, 0, sizeof(Http));

    server->cfg = cfg;
    server->userdata = userdata;

    if (cfg.num_threads > MAX_THREADS)
        cfg.num_threads = MAX_THREADS;

    server->pool = threadpool_create(cfg.num_threads, cfg.max_enq_con, 0);
    if (server->pool == NULL) {
        ERRF("Could not create thread pool. Out of memory?");
        free(server);
        return NULL;
    }

    server->server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server->server_fd == -1) {
	    ERRF("Socket creation failed: %s...", strerror(errno));
        threadpool_destroy(server->pool, 0);
        free(server);
        return NULL;
	}

    int flags = fcntl(server->server_fd, F_GETFL);
    if (flags != -1)
        flags = fcntl(server->server_fd, F_SETFL, flags | O_NONBLOCK);
    if (flags == -1) {
        ERRF("Could not set socket to non-blocking: %s", strerror(errno));
        close(server->server_fd);
        threadpool_destroy(server->pool, 0);
        free(server);
        return NULL;
    }

	if (setsockopt(server->server_fd, SOL_SOCKET, SO_REUSEPORT, &cfg.reuse, sizeof(cfg.reuse)) < 0) {
        ERRF("SO_REUSEPORT failed: %s", strerror(errno));
        close(server->server_fd);
        threadpool_destroy(server->pool, 0);
        free(server);
        return NULL;
	}

	if (bind(server->server_fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) != 0) {
	    ERRF("Bind failed: %s", strerror(errno));
        close(server->server_fd);
        threadpool_destroy(server->pool, 0);
        free(server);
	    return NULL;
	}

	int connection_backlog = 10;
	if (listen(server->server_fd, connection_backlog) != 0) {
	    ERRF("Listen failed: %s", strerror(errno));
        close(server->server_fd);
        threadpool_destroy(server->pool, 0);
        free(server);
	    return NULL;
	}

    server->running = true;
    return server;
}

/** accept new connection in thread or wait [cfg.con_sleep_us] */
void http_tick(Http* server)
{
    if (server->enq_con >= server->cfg.max_enq_con) {
        usleep((useconds_t) server->cfg.con_sleep_us);
        return;
    }

    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    int connection_fd = accept(server->server_fd,
                                (struct sockaddr *) &client_addr,
                                &client_addr_len);
    if (connection_fd == -1) {
        if (errno == EWOULDBLOCK) {
            usleep((useconds_t) server->cfg.con_sleep_us);
        }
    }
    // TODO: extend thread pool to get queue fill amount and sleep if full-ish
    else {
        server->enq_con ++;
        ServerConnection* con = malloc(sizeof(ServerConnection));
        if (con == NULL) {
            close(connection_fd);
            ERRF("out of memory at begin of handling connection");
        }
        else {
            con->server = server;
            con->fd = connection_fd;
            threadpool_add(server->pool, handle_task, con, 0);
        }
    }
}

void http_close(Http* server)
{
    close(server->server_fd);
    threadpool_destroy(server->pool, 0);
    free(server);
}

void http_slowlyStop(Http* server)
{
    server->running = false;
}

bool http_isStopping(Http* server)
{
    return !server->running;
}

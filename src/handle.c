#include "internal.h"
#include <errno.h>
#include <string.h>

// from: https://github.com/alex-s168/minilibs/blob/main/filelib.h
static char *readLine(FILE *file)
{
    if (file == NULL)
        return NULL;
    
    size_t buf_size = 0;
    size_t buf_cap = 200;
    char *buf = malloc(buf_cap);
    if (buf == NULL)
        return NULL;
    buf[buf_cap - 1] = '\0';

    while (fgets(buf + buf_size, buf_cap - buf_size, file) != NULL) {
        const size_t len = strlen(buf + buf_size);
        if (len == 0)
            break;
        buf_size += len;
        if (buf[buf_size - 1] == '\n') {
            buf_size --;
            if (buf[buf_size - 1] == '\r')
                buf_size --;
            break;
        }
        if (buf_size == buf_cap - 1) {
            buf_cap += 200;
            char *new = realloc(buf, buf_cap);
            if (new == NULL) {
                free(buf);
                return NULL;
            }
            buf = new;
            buf[buf_cap - 1] = '\0';
        }
    }

    buf[buf_size] = '\0';

    if (feof(file) && buf_size == 0) {
        free(buf);
        buf = NULL;
    }

    return buf;
}

static bool server_has_encoding(struct HttpRequest request, const char *name) {
    for (size_t i = 0; i < request.accepted_encodings_size; i ++) {
        if (strcmp(request.accepted_encodings[i], name) == 0) {
            return true;
        }
    }
    return false;
}

int http__handle_connection(ServerConnection* con) {
    FILE *fp = fdopen(con->fd, "r+");
    if (fp == NULL) {
        ERRF("fdopen failed: %s", strerror(errno));
        return 1;
    }
    char *line0 = readLine(fp);
    if (line0 == NULL) {
        ERRF("in request: no line 0");
        fclose(fp);
        return 1;
    }

    char *path = strchr(line0, ' ');
    if (path == NULL) {
        ERRF("in request: invalid line 0");
        fclose(fp);
        return 1;
    }
    *path = '\0';
    path++;
    const char *method_str = line0;
    char *path_end = strrchr(path, ' ');
    if (path_end == NULL) {
        ERRF("in request: invalid line 0");
        fclose(fp);
        return 1;   
    }
    *path_end = '\0';

    enum HttpMethod method;
    if (strcmp(method_str, "GET") == 0) {
        method = GET;
    } else if (strcmp(method_str, "POST") == 0) {
        method = POST;
    } else {
        method = UNKNOWN;
    }

    struct Header *headers = NULL;
    size_t headers_size = 0;

    char *line;
    while ((line = readLine(fp)) != NULL) {
        if (line[0] == '\0' || feof(fp))
            break;
        headers_size++;
        headers = realloc(headers, headers_size * sizeof(struct Header));
        char *col = strchr(line, ':');
        *col = '\0';
        headers[headers_size - 1].name = line;
        headers[headers_size - 1].value = col + 2;
    }

    struct HttpRequest request = { method, path, headers, headers_size, NULL, 0, NULL, 0 };

    char* accepted_encodings_header; {
        const char *t = http_header_get(&request, "Accept-Encoding");
        accepted_encodings_header = t ? strdup(t) : NULL;
        if (accepted_encodings_header != NULL) {
            SPLITERATE(accepted_encodings_header, ",", encoding) {
                while (*encoding == ' ') encoding++;
                request.accepted_encodings = realloc(request.accepted_encodings, sizeof(const char *) * (request.accepted_encodings_size + 1));
                request.accepted_encodings[request.accepted_encodings_size ++] = encoding;
            }
        }
    }

    const char *content_length_header = http_header_get(&request, "Content-Length");
    if (content_length_header != NULL) {
        size_t content_length = atoi(content_length_header);

        char *content = malloc(content_length);
        fread(content, 1, content_length, fp);

        request.body = content;
        request.body_size = content_length;
    }

    struct HttpResponse response =
        con->server->cfg.handler(request, con->server->userdata);
    response._internal_optional_encoding = NULL;

    for (size_t i = 0; ; i ++) {
        CompressAlgo algo = http__compressAlgo[i];
        if (algo.name == NULL) break;

        if (server_has_encoding(request, algo.name))
        {
            size_t compuz;
            char* comp = algo.compress(response.content,
                                       response.content_size,
                                       &compuz);

            if (comp == NULL) {
                WARNF("not enough memory for compressing response");
                break;
            }

            response._internal_optional_encoding = algo.name;
            if (response.free_content)
                free((char*) response.content);
            response.content = comp;
            response.content_size = compuz;
            response.free_content = true;

            break;
        }
    }

    fprintf(fp, "HTTP/1.1 %i %s\r\n", response.status, response.status_msg);
    if (response._internal_optional_encoding != NULL)
        fprintf(fp, "Content-Encoding: %s\r\n", response._internal_optional_encoding);
    fprintf(fp, "Content-Type: %s; charset=UTF-8\r\n", response.content_type);
    fprintf(fp, "Content-Length: %zu\r\n\r\n", response.content_size);
    fwrite(response.content, 1, response.content_size, fp);

    fclose(fp);

    free(line0);

    if (response.free_content)
        free((char*) response.content);

    if (headers) {
        for (size_t i = 0; i < headers_size; i++)
            free((char *) headers[i].name);
        
        free(headers);
    }

    if (request.body)
        free(request.body);

    if (accepted_encodings_header)
        free(accepted_encodings_header);

    return 0;
}

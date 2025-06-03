#include "internal.h"
#include <errno.h>
#include <string.h>
#include <unistd.h>

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

#define FILE_CHUNK_SIZE (1028 * 8)

int http__handle_connection(ServerConnection* con) {
    if (con->server->cfg.verbose) {
        LOGF("handling connection");
    }

    FILE *fp = fdopen(con->fd, "r+");
    if (fp == NULL) {
        ERRF("fdopen failed: %s", strerror(errno));
        close(con->fd);
        return 1;
    }
    char* const line0 = readLine(fp);
    if (line0 == NULL) {
        ERRF("empty request");
        fclose(fp);
        return 1;
    }

    char *path = strchr(line0, ' ');
    if (path == NULL) {
        ERRF("in request: empty first line");
        free(line0);
        fclose(fp);
        return 1;
    }
    *path = '\0';
    path++;
    const char *method_str = line0;
    char *path_end = strrchr(path, ' ');
    if (path_end == NULL) {
        ERRF("in request: invalid line 0");
        free(line0);
        fclose(fp);
        return 1;   
    }
    *path_end = '\0';

    while (path[0] == '/' && path[1] == '/') {
        path ++;
    }

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

    {
        char *line;
        while ((line = readLine(fp)) != NULL) {
            if (line[0] == '\0' || feof(fp)) {
                free(line);
                break;
            }
            char *col = strchr(line, ':');
            if (!col) {
                free(line);
                break;
            }

            if (headers_size + 1 < headers_size) {
                ERRF("too many headers in request. probably malicisious");
                free(line);
                break;
            }
            headers_size++;
            void* newHeaders = realloc(headers, headers_size * sizeof(struct Header));
            if (!newHeaders) {
                free(headers);
                free(line);
                ERRF("not enough memory for request");
                fclose(fp);
                free(line0);
                return 1;
            }
            headers = newHeaders;
            *col = '\0';
            headers[headers_size - 1].name = line;
            headers[headers_size - 1].value = col + 2;
        }
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
        if (content) {
            fread(content, 1, content_length, fp);

            request.body = content;
            request.body_size = content_length;
        }
    }

    struct HttpResponse response =
        con->server->cfg.handler(request, con->server->userdata);
    response._internal_optional_encoding = NULL;

    if (response.content_mode == HTTP_CONTENT_BYTES) {
        for (size_t i = 0; ; i ++) {
            CompressAlgo algo = http__compressAlgo[i];
            if (algo.name == NULL) break;

            if (server_has_encoding(request, algo.name))
            {
                size_t compuz;
                char* comp = algo.compress(response.content_val.bytes.content,
                                           response.content_size,
                                           &compuz);

                if (comp == NULL) {
                    WARNF("not enough memory for compressing response");
                    break;
                }

                response._internal_optional_encoding = algo.name;
                if (response.content_val.bytes.free_after)
                    free((char*) response.content_val.bytes.content);
                response.content_val.bytes.content = comp;
                response.content_size = compuz;
                response.content_val.bytes.free_after = true;

                break;
            }
        }
    }

    fprintf(fp, "HTTP/1.1 %i %s\r\n", response.status, response.status_msg);
    if (response._internal_optional_encoding != NULL)
        fprintf(fp, "Content-Encoding: %s\r\n", response._internal_optional_encoding);
    fprintf(fp, "Content-Type: %s; charset=UTF-8\r\n", response.content_type);
    fprintf(fp, "Content-Length: %zu\r\n\r\n", response.content_size);

    switch (response.content_mode)
    {
        case HTTP_CONTENT_BYTES: {
            fwrite(response.content_val.bytes.content, 1, response.content_size, fp);
            if (response.content_val.bytes.free_after)
                free((void*) response.content_val.bytes.content);
        } break;

        case HTTP_CONTENT_ITER: {
            HttpIterRes it;
            do {
                it = response.content_val.iter.next(response.content_val.iter.userptr);
                fwrite(it.ptr, 1, it.len, fp);
                if (it.free_ptr && it.ptr)
                    free(it.ptr);
                response.content_val.iter.userptr = it.new_userptr;
            } while (it.len);
        } break;

        case HTTP_CONTENT_FILE: {
            char buf[FILE_CHUNK_SIZE];
            size_t rem = response.content_size;
            while (rem >= FILE_CHUNK_SIZE)
            {
                fread(buf, 1, FILE_CHUNK_SIZE, response.content_val.file.fp);
                fwrite(buf, 1, FILE_CHUNK_SIZE, fp);
                rem -= FILE_CHUNK_SIZE;
            }
            fread(buf, 1, rem, response.content_val.file.fp);
            fwrite(buf, 1, rem, fp);
            if (response.content_val.file.close_after)
                fclose(response.content_val.file.fp);
        } break;
    }

    fclose(fp);

    free(line0);

    if (headers) {
        for (size_t i = 0; i < headers_size; i++)
            free((char *) headers[i].name);
        
        free(headers);
    }

    if (request.body)
        free(request.body);

    if (accepted_encodings_header)
        free(accepted_encodings_header);

    if (con->server->cfg.verbose) {
        LOGF("finished handling connection");
    }

    return 0;
}

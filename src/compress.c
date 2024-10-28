#include "internal.h"
#include <stdlib.h>

#ifdef HAS_ZLIB
#define CHUNK 16384
#include <zlib.h>
static char* server_gzip_compress(const char* input, size_t inputSize, size_t *lenOut)
{
    z_stream zs;
    zs.zalloc = Z_NULL;
    zs.zfree = Z_NULL;
    zs.opaque = Z_NULL;
    zs.avail_in = (uInt)inputSize;
    zs.next_in = (Bytef *)input;

    size_t outputSize = 512;
    char* output = malloc(outputSize);
    if (output == NULL) return NULL;

    do {
        zs.avail_out = (uInt)outputSize;
        zs.next_out = (Bytef *)output;

        deflateInit2(&zs, Z_DEFAULT_COMPRESSION, Z_DEFLATED, 15 | 16, 8, Z_DEFAULT_STRATEGY);
        deflate(&zs, Z_FINISH);
        deflateEnd(&zs);

        if (zs.total_out == 0) {
            outputSize *= 2;
            void* new = realloc(output, outputSize);
            if (new == NULL) {
                free(output);
                return NULL;
            }
            output = new;
        }
    } while (zs.total_out == 0);

    *lenOut = zs.total_out;
    return output;
}
#endif

#ifdef HAS_ZSTD
#include <zstd.h>
static char* server_zstd_compress(const char* input, size_t inputSize, size_t* lenOut)
{
    size_t outputSize = 512;
    char* output = malloc(outputSize);
    if (output == NULL) return NULL;

    size_t actual = 0;
    do {
        actual = ZSTD_compress(output, outputSize, input, inputSize, ZSTD_CLEVEL_DEFAULT);
        if (ZSTD_isError(actual)) {
            outputSize *= 2;
            void* new = realloc(output, outputSize);
            if (new == NULL) {
                free(output);
                return NULL;
            }
            output = new;

            actual = 0;
        }
    } while (actual == 0);

    *lenOut = actual;
    return output;
}
#endif

CompressAlgo http__compressAlgo[] =
{
#ifdef HAS_ZLIB
    { .name = "gzip", .compress = server_gzip_compress },
#endif 

#ifdef HAS_ZSTD
    { .name = "zstd", .compress = server_zstd_compress },
#endif

    { 0 }
};

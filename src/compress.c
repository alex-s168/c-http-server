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

    if (deflateInit2(&zs, Z_DEFAULT_COMPRESSION, Z_DEFLATED, 15 | 16, 8, Z_DEFAULT_STRATEGY) != Z_OK) {
        return NULL;
    }

    zs.avail_in = (uInt)inputSize;
    zs.next_in = (Bytef*)input;

    size_t outputSize = CHUNK;
    char* output = malloc(outputSize);
    if (output == NULL) {
        deflateEnd(&zs);
        return NULL;
    }

    size_t totalOut = 0;

    do {
        zs.avail_out = (uInt)(outputSize - totalOut);
        zs.next_out = (Bytef*)(output + totalOut);

        int ret = deflate(&zs, Z_FINISH);
        if (ret != Z_OK && ret != Z_STREAM_END) {
            free(output);
            deflateEnd(&zs);
            return NULL;
        }

        totalOut = zs.total_out;

        if (zs.avail_out == 0) {
            outputSize *= 2;
            char* newOutput = realloc(output, outputSize);
            if (newOutput == NULL) {
                free(output);
                deflateEnd(&zs);
                return NULL;
            }
            output = newOutput;
        }
    } while (zs.avail_out == 0);

    deflateEnd(&zs);

    *lenOut = totalOut;
    return output;
}
#endif

#ifdef HAS_ZSTD
#include <zstd.h>
static char* server_zstd_compress(const char* input, size_t inputSize, size_t* lenOut) {
    if (input == NULL || lenOut == NULL) return NULL;

    size_t outputSize = 512;
    char* output = malloc(outputSize);
    if (output == NULL) return NULL;

    while (1) {
        size_t actual = ZSTD_compress(output, outputSize, input, inputSize, ZSTD_CLEVEL_DEFAULT);
        if (!ZSTD_isError(actual)) {
            *lenOut = actual;
            return output;
        }

        size_t newOutputSize = outputSize * 2;
        if (newOutputSize > inputSize * 2)
            return NULL;
        void* newOutput = realloc(output, newOutputSize);
        if (newOutput == NULL) {
            free(output);
            return NULL;
        }

        output = newOutput;
        outputSize = newOutputSize;
    }
}
#endif

CompressAlgo http__compressAlgo[] =
{
#ifdef HAS_ZSTD
    { .name = "zstd", .compress = server_zstd_compress },
#endif

#ifdef HAS_ZLIB
    { .name = "gzip", .compress = server_gzip_compress },
#endif 

    { 0 }
};

#include <string.h>
#include "../inc/httpserv.h"

const char* http_detectMime(const char* path, const char* default_mine)
{
    const char* lastSlash = strrchr(path, '/');
    if (lastSlash) path = lastSlash + 1;
    const char* ext = strrchr(path, '.');
    if (ext) {
        ext++;

        if (!strcmp(ext, "aac")) return "audio/aac";
        else if (!strcmp(ext, "abw")) return "application/x-abiword";
        else if (!strcmp(ext, "apng")) return "image/apng";
        else if (!strcmp(ext, "arc")) return "application/x-freearc";
        else if (!strcmp(ext, "avif")) return "image/avif";
        else if (!strcmp(ext, "avi")) return "video/x-msvideo";
        else if (!strcmp(ext, "azw")) return "application/vnd.amazon.ebook";
        else if (!strcmp(ext, "bin")) return "application/octet-stream";
        else if (!strcmp(ext, "bmp")) return "image/bmp";
        else if (!strcmp(ext, "bz")) return "application/x-bzip";
        else if (!strcmp(ext, "bz2")) return "application/x-bzip2";
        else if (!strcmp(ext, "cda")) return "application/x-cdf";
        else if (!strcmp(ext, "csh")) return "application/x-csh";
        else if (!strcmp(ext, "css")) return "text/css";
        else if (!strcmp(ext, "csv")) return "text/csv";
        else if (!strcmp(ext, "doc")) return "application/msword";
        else if (!strcmp(ext, "docx")) return "application/vnd.openxmlformats-officedocument.wordprocessingml.document";
        else if (!strcmp(ext, "eot")) return "application/vnd.ms-fontobject";
        else if (!strcmp(ext, "epub")) return "application/epub+zip";
        else if (!strcmp(ext, "gz")) return "application/x-gzip.";
        else if (!strcmp(ext, "gif")) return "image/gif";
        else if (!strcmp(ext, "html")) return "text/html";
        else if (!strcmp(ext, "ico")) return "image/vnd.microsoft.icon";
        else if (!strcmp(ext, "ics")) return "text/calendar";
        else if (!strcmp(ext, "jar")) return "application/java-archive";
        else if (!strcmp(ext, "jpeg")) return "image/jpeg";
        else if (!strcmp(ext, "js")) return "text/javascript";
        else if (!strcmp(ext, "json")) return "application/json";
        else if (!strcmp(ext, "mjs")) return "text/javascript";
        else if (!strcmp(ext, "mp3")) return "audio/mpeg";
        else if (!strcmp(ext, "mp4")) return "video/mp4";
        else if (!strcmp(ext, "mpeg")) return "video/mpeg";
        else if (!strcmp(ext, "mpkg")) return "application/vnd.apple.installer+xml";
        else if (!strcmp(ext, "odp")) return "application/vnd.oasis.opendocument.presentation";
        else if (!strcmp(ext, "ods")) return "application/vnd.oasis.opendocument.spreadsheet";
        else if (!strcmp(ext, "odt")) return "application/vnd.oasis.opendocument.text";
        else if (!strcmp(ext, "oga")) return "audio/ogg";
        else if (!strcmp(ext, "ogv")) return "video/ogg";
        else if (!strcmp(ext, "ogx")) return "application/ogg";
        else if (!strcmp(ext, "opus")) return "audio/ogg";
        else if (!strcmp(ext, "otf")) return "font/otf";
        else if (!strcmp(ext, "png")) return "image/png";
        else if (!strcmp(ext, "pdf")) return "application/pdf";
        else if (!strcmp(ext, "php")) return "application/x-httpd-php";
        else if (!strcmp(ext, "ppt")) return "application/vnd.ms-powerpoint";
        else if (!strcmp(ext, "pptx")) return "application/vnd.openxmlformats-officedocument.presentationml.presentation";
        else if (!strcmp(ext, "rar")) return "application/vnd.rar";
        else if (!strcmp(ext, "rtf")) return "application/rtf";
        else if (!strcmp(ext, "sh")) return "application/x-sh";
        else if (!strcmp(ext, "svg")) return "image/svg+xml";
        else if (!strcmp(ext, "tar")) return "application/x-tar";
        else if (!strcmp(ext, "tar.xz")) return "application/x-tar";
        else if (!strcmp(ext, "tiff")) return "image/tiff";
        else if (!strcmp(ext, "ts")) return "video/mp2t";
        else if (!strcmp(ext, "ttf")) return "font/ttf";
        else if (!strcmp(ext, "txt")) return "text/plain";
        else if (!strcmp(ext, "vsd")) return "application/vnd.visio";
        else if (!strcmp(ext, "wav")) return "audio/wav";
        else if (!strcmp(ext, "weba")) return "audio/webm";
        else if (!strcmp(ext, "webm")) return "video/webm";
        else if (!strcmp(ext, "webp")) return "image/webp";
        else if (!strcmp(ext, "woff")) return "font/woff";
        else if (!strcmp(ext, "woff2")) return "font/woff2";
        else if (!strcmp(ext, "xhtml")) return "application/xhtml+xml";
        else if (!strcmp(ext, "xls")) return "application/vnd.ms-excel";
        else if (!strcmp(ext, "xlsx")) return "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet";
        else if (!strcmp(ext, "xml")) return "application/xml";
        else if (!strcmp(ext, "zip")) return "application/zip";
        else if (!strcmp(ext, "7z")) return "application/x-7z-compressed";

    }
    return default_mine;
}

http_serv=""
http_serv+="src/*.c C-Thread-Pool/thpool.c"
http_serv+=" -DHAS_ZSTD -lzstd"
http_serv+=" -DHAS_ZLIB -lz"

: ${CC:="clang"}
$CC example.c $http_serv -o out -O0 -g -glldb

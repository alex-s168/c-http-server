#include "../inc/httpserv.h"
#include <ctype.h>

// modified from: https://gist.github.com/jmsaavedra/7964251
size_t http_urldecode(char *dst, size_t dstlen, const char *src)
{
  size_t olddlen = dstlen;
  if (dstlen == 0) return 0;
  char a, b;
  while (*src) {
    if (!dstlen) break;
    if ((*src == '%') &&
      ((a = src[1]) && (b = src[2])) &&
      (isxdigit(a) && isxdigit(b))) {
      if (a >= 'a')
        a -= 'a'-'A';
      if (a >= 'A')
        a -= ('A' - 10);
      else
        a -= '0';
      if (b >= 'a')
        b -= 'a'-'A';
      if (b >= 'A')
        b -= ('A' - 10);
      else
        b -= '0';
      *dst++ = 16*a+b;
      dstlen--;
      src+=3;
    } 
    else if (*src == '+') {
        src++;
        *dst++ = ' ';
        dstlen--;
    }
    else {
      *dst++ = *src++;
      dstlen--;
    }
  }
  if (dstlen) {
    *dst++ = '\0';
  } else {
    dst --;
    *dst = '\0';
  }
  dstlen --;
  return olddlen - dstlen;
}


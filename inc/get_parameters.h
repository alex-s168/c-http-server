/*
Example usage:
```
char *url;
char *params[2] = { "name", "mail" };
urlGETAll(url, params, 2);
printf("name: %s\n", params[0]);
printf("mail: %s\n", params[1]);
```
 */

#ifndef HTTP_GET_PARAMS
#define HTTP_GET_PARAMS

#include <string.h>

// "/a/b?x=y" -> "x=y"
static const char *urlGETStart(const char *path) {
  while (*path != '\0') {
    if (*path == '?')
      return path + 1;

    path ++;
  }
  return path;
}

#define INTERNAL /* internal: */

struct urlGETParam {
  char *k;
  char *v;

INTERNAL
  char *next;
};

static void urlGETPrepare(struct urlGETParam *ptr) {
  ptr->next = NULL;
}

// Returns 1 if was success
// should not be called again after 0 was returned
static int urlGETNext(struct urlGETParam *ptr, char *start) {
  char *r = strtok(ptr->next ? NULL : start, "&");
  if (r == NULL)
    return 0;

  ptr->next = r;

  ptr->k = r;
  char *v = strchr(r, '=');
  *v = '\0';
  ptr->v = v + 1;

  return 1;
}

// "temp" needs to be the size of "len" and initialized with zeros
static void urlGETAll(char *path, char **dest, int len, int *temp) {
  char *start = (char*) urlGETStart(path);
  struct urlGETParam param;
  urlGETPrepare(&param);
  while (urlGETNext(&param, start)) {
    for (int i = 0; i < len; i ++) {
      if (temp[i])
        continue;

      if (!strcmp(dest[i], param.k)) {
        dest[i] = param.v;
        temp[i] = 1;
      }
    }
  }
  for (int i = 0; i < len; i ++)
    if (!temp[i])
      dest[i] = NULL;
}

#define urlGETAll(path, dest, count) \
  urlGETAll(path, dest, count, (int[count]) { 0 });

#endif//HTTP_GET_PARAMS

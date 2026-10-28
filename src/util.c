/* util.c
**
** (c) Matthew Bloch 2000
**
** See http://riscose.sourceforge.net/ for terms of distribution, and to
** pick up a later version of the software.
*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>
#include <dirent.h>
#include <strings.h>

#include <monty/mem.h>

#include "util.h"

/* FIXME:  poor name.  strncpy doesn't malloc, this does.  strncpy
 * doesn't always terminate the new string, this does. */

char *xstrncpy(char *src, int n)
{
    char *dst;

    dst = emalloc(n + 1);
    strncpy(dst, src, n);
    dst[n] = '\0';

    return dst;
}

int file_objecttype(char *name)
{
  struct stat file_info;
  if (stat(name, &file_info) == -1)
    return OBJECT_NOTFOUND;
  if ((file_info.st_mode & S_IFDIR) != 0)
    return OBJECT_DIR;
  return OBJECT_FILE;
}

size_t file_size(char *name)
{
  struct stat file_info;
  if (stat(name, &file_info) == -1)
    return -1;
  return file_info.st_size;
}

int file_loadat(char *name, void *addr)
{
  FILE *fh = fopen(name, "r");
  if (fh == NULL)
    return -1;
  size_t s = file_size(name);
  size_t r = fread(addr, 1, s, fh);
  assert(s == r);
  fclose(fh);
  return 0;
}

char *ro_path_from_host_path(const char *hostpath)
{
  const char *h;
  char *out, *p;

  p = out = malloc(1 + strlen(hostpath) + 1);
  if (out == NULL)
    return NULL;

  /* If host path is absolute, make RISC OS path so, too */
  if (hostpath[0] == '/')
    *p++ = '$';

  for (h = hostpath; *h; h++) {
    switch (h[0]) {
      case '/':
        *p++ = '.';
        break;
      case '.':
	switch (h[1]) {
          case '.':
            /* .. -> ^ */
            *p++ = '^';
            h++;
            break;
          case '/':
            /* . -> drop segment */
            h++;
            break;
          default:
            *p++ = '/';
            break;
        }
        break;
      default:
        *p++ = h[0];
        break;
    }
  }
  *p = '\0';

  return out;
}

/* RISC OS's filing systems ignore case; the host's needn't.  If a path
 * doesn't exist as given, each component of it that doesn't is looked for
 * in its directory ignoring case, as far as one is found.  A name found
 * this way is as long as the one asked for, so the path is edited in
 * place; a component not found (a file about to be created) and any after
 * it are left as they are. */
static void resolve_case(char *path)
{
  struct stat st;
  char *comp, *slash;

  if (lstat(path, &st) == 0)
    return;

  comp = path;
  while (*comp == '/')
    comp++;
  while (*comp) {
    int found = 1;

    slash = strchr(comp, '/');
    if (slash)
      *slash = '\0';

    if (lstat(path, &st) != 0 && strcmp(comp, ".") && strcmp(comp, "..")) {
      char *dir = comp == path ? strdup(".")
                : comp == path + 1 ? strdup("/")
                : strndup(path, comp - path - 1);
      DIR *d = dir ? opendir(dir) : NULL;
      struct dirent *e;

      found = 0;
      while (d && (e = readdir(d)) != NULL) {
        if (strcasecmp(e->d_name, comp) == 0) {
          memcpy(comp, e->d_name, strlen(comp));
          found = 1;
          break;
        }
      }
      if (d)
        closedir(d);
      free(dir);
    }

    if (slash)
      *slash = '/';
    if (!found || !slash)
      break;
    comp = slash + 1;
  }
}

char *host_path_from_ro_path(const char *ropath)
{
  const char *end = ropath;
  size_t len = 0;
  char *out, *p;

  while (*end > 31) {
    switch (*end) {
      case '$': /* Root of disc */
        break;
      case '&': /* URD */
        break;
      case '@': /* CSD */
        len++;
        break;
      case '^': /* .. */
        len += 2;
        break;
      case '%': /* CSL */
        break;
      case '\\': /* PSD */
        break;
      default:
        len++;
        break;
    }
    end++;
  }

  out = malloc(len + 1);
  if (out == NULL)
    return NULL;

  if (ropath[0] == '.' || ropath[0] == '/') {
    /* Assume path is already in host format */
    for (end = ropath, p = out; *end > 31; end++)
      *p++ = *end;
    *p++ = '\0';
    return out;
  }

  /* Unixify */
  //fprintf(stderr, "hpfrp: '%.*s'\n", (int)(end-ropath), ropath);
  for (end = ropath, p = out; *end > 31; end++) {
    switch (*end) {
      case '$': /* Root of disc */
        break;
      case '&': /* URD */
        break;
      case '@': /* CSD */
        *p++ = '.';
        break;
      case '^': /* .. */
        *p++ = '.';
        *p++ = '.';
        break;
      case '%': /* CSL */
        break;
      case '\\': /* PSD */
        break;
      case '/':
        *p++ = '.';
        break;
      case '.':
        *p++ = '/';
        break;
      default:
        *p++ = *end;
        break;
    }
  }
  *p = '\0';
  resolve_case(out);
  //fprintf(stderr, "-> hpfrp: '%s'\n", out);

  return out;
}

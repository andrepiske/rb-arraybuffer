#ifndef LLC_ARRAYBUFFER_H
#define LLC_ARRAYBUFFER_H

#include <ruby.h>

struct LLC_ArrayBuffer {
  unsigned char *ptr;
  unsigned int size;
  VALUE backing_str;
};

#endif

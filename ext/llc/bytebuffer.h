#ifndef LLC_BYTEBUFFER_H
#define LLC_BYTEBUFFER_H

#include <ruby.h>

struct LLC_ByteBuffer {
  unsigned char *ptr;
  unsigned int size;
};

#endif

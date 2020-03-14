#ifndef LLC_DATAVIEW_H
#define LLC_DATAVIEW_H

#include <ruby.h>

struct LLC_DataView {
  VALUE bb_obj;
  unsigned int offset;
  unsigned int size;
  unsigned char flags;
};

#endif

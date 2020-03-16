#include "bytebuffer.h"
#include "extconf.h"

#ifdef HAVE_STRING_H
#include <string.h>
#endif

extern VALUE cByteBuffer;
extern VALUE mLLC;

#define DECLAREBB(self) \
  struct LLC_ByteBuffer *bb = (struct LLC_ByteBuffer*)rb_data_object_get((self))

#define CHECKBOUNDS(bb, idx) \
  if (!(bb)->ptr || (idx) < 0 || (idx) >= (bb)->size) { \
    rb_raise(rb_eArgError, "Index out of bounds: %d", (idx)); \
  }

static void
t_bb_gc_mark(struct LLC_ByteBuffer *bb) {
}

static void
t_bb_free(struct LLC_ByteBuffer *bb) {
  if (bb->ptr)
    xfree(bb->ptr);

  xfree(bb);
}

static VALUE
t_bb_allocator(VALUE klass) {
  struct LLC_ByteBuffer *bb = (struct LLC_ByteBuffer*)xmalloc(sizeof(struct LLC_ByteBuffer));
  bb->ptr = NULL;
  bb->size = 0;

  return Data_Wrap_Struct(klass, t_bb_gc_mark, t_bb_free, bb);
}

static VALUE
t_bb_initialize(VALUE self, VALUE size) {
  DECLAREBB(self);
  unsigned int s = NUM2UINT(size);
  bb->size = s;
  if (bb->ptr)
    xfree(bb->ptr);
  bb->ptr = xmalloc(s);
  memset(bb->ptr, 0, (size_t)s);
  return self;
}

static VALUE
t_bb_getbyte(VALUE self, VALUE index) {
  DECLAREBB(self);
  int idx = NUM2INT(index);
  if (idx < 0)
    idx += (int)bb->size;
  CHECKBOUNDS(bb, idx);
  return UINT2NUM((unsigned int)bb->ptr[idx]);
}

static VALUE
t_bb_setbyte(VALUE self, VALUE index, VALUE value) {
  DECLAREBB(self);
  int idx = NUM2INT(index);
  unsigned int val = NUM2UINT(value);
  if (idx < 0)
    idx += (int)bb->size;
  CHECKBOUNDS(bb, idx);
  bb->ptr[idx] = (unsigned char)val;
  return self;
}

static VALUE
t_bb_size(VALUE self) {
  DECLAREBB(self);
  return UINT2NUM(bb->size);
}

static VALUE
t_bb_each(VALUE self) {
  DECLAREBB(self);

  if (rb_block_given_p()) {
    for (int i = 0; i < bb->size; i++) {
      unsigned int val = (unsigned int)bb->ptr[i];
      rb_yield(UINT2NUM(val));
    }
  } else {
    rb_raise(rb_eArgError, "no block given");
  }

  return self;
}

static VALUE
t_bb_realloc(VALUE self, VALUE _new_size) {
  DECLAREBB(self);
  unsigned int new_size = NUM2UINT(_new_size);
  if (new_size == bb->size)
    return self;

  void *old_ptr = (void*)bb->ptr;
  if (new_size > 0) {
    char *new_ptr = (void*)xmalloc(new_size);
    if (old_ptr) {
      if (new_size > bb->size) {
        size_t diff = (size_t)(new_size - bb->size);
        memcpy(new_ptr, old_ptr, (size_t)bb->size);
        xfree(old_ptr);
        old_ptr = NULL;
        memset((char*)new_ptr + (size_t)bb->size, 0, diff);
      } else {
        memcpy(new_ptr, old_ptr, (size_t)new_size);
      }
    } else {
      memset(new_ptr, 0, new_size);
    }
    bb->size = new_size;
    bb->ptr = (unsigned char*)new_ptr;
  } else {
    bb->size = 0;
    bb->ptr = NULL;
  }

  if (old_ptr)
    xfree(old_ptr);

  return self;
}

/*
 * Returns a ASCII-8BIT string with the contents of the buffer
 *
 * The returned string is a copy of the buffer. It's encoding is always
 * ASCII-8BIT.
 * If the buffer has size zero, an empty string is returned.
 *
 * @return [String]
 */
static VALUE
t_bb_bytes(VALUE self) {
  DECLAREBB(self);
  return rb_tainted_str_new(
    (const char*)bb->ptr,
    bb->size);
}

void
Init_bytebuffer() {
  cByteBuffer = rb_define_class_under(mLLC, "ByteBuffer", rb_cObject);
  rb_define_alloc_func(cByteBuffer, t_bb_allocator);
  rb_include_module(cByteBuffer, rb_mEnumerable);

  rb_define_method(cByteBuffer, "initialize", t_bb_initialize, 1);
  rb_define_method(cByteBuffer, "[]", t_bb_getbyte, 1);
  rb_define_method(cByteBuffer, "[]=", t_bb_setbyte, 2);
  rb_define_method(cByteBuffer, "size", t_bb_size, 0);
  rb_define_alias(cByteBuffer, "length", "size");
  rb_define_method(cByteBuffer, "each", t_bb_each, 0);
  rb_define_method(cByteBuffer, "realloc", t_bb_realloc, 1);
  rb_define_method(cByteBuffer, "bytes", t_bb_bytes, 0);
}

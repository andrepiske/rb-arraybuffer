#include "arraybuffer.h"
#include "extconf.h"
#include <string.h>
#include <ruby/version.h>

#ifdef HAVE_RUBY_MEMORY_VIEW_H
#include <ruby/memory_view.h>
#endif

extern VALUE cArrayBuffer;

#define DECLAREBB(self) \
  struct LLC_ArrayBuffer *bb = (struct LLC_ArrayBuffer*)rb_data_object_get((self))

#define CHECKBOUNDS(bb, idx) \
  if (!(bb)->ptr || (idx) < 0 || (idx) >= (bb)->size) { \
    rb_raise(rb_eArgError, "Index out of bounds: %d", (idx)); \
  }

#ifdef HAVE_RUBY_MEMORY_VIEW_H
static bool
r_bb_mv_get(VALUE self, rb_memory_view_t *view, int flags) {
  DECLAREBB(self);
  if (!bb->size)
    return 0;
  rb_memory_view_init_as_byte_array(view, self, bb->ptr, (const ssize_t)bb->size, 0);
  return 1;
}

// static bool
// r_bb_mv_release(VALUE self, rb_memory_view_t *view) {
//   return 1;
// }

static bool
r_bb_mv_available_p(VALUE self) {
  DECLAREBB(self);
  return bb->size > 0 ? 1 : 0;
}

static rb_memory_view_entry_t cArrayBufferMemoryView = {
  r_bb_mv_get,
  NULL, // r_bb_mv_release,
  r_bb_mv_available_p
};
#endif

static void
t_bb_gc_mark(struct LLC_ArrayBuffer *bb) {
  if (bb->backing_str) {
    rb_gc_mark(bb->backing_str);
  }
}

static void
t_bb_free(struct LLC_ArrayBuffer *bb) {
  xfree(bb);
}

static VALUE
t_bb_allocator(VALUE klass) {
  struct LLC_ArrayBuffer *bb = (struct LLC_ArrayBuffer*)xmalloc(sizeof(struct LLC_ArrayBuffer));
  bb->ptr = NULL;
  bb->size = 0;
  bb->backing_str = NULL;

  return Data_Wrap_Struct(klass, t_bb_gc_mark, t_bb_free, bb);
}

#if (RUBY_API_VERSION_CODE >= 30100)
  // Ruby 3.1 and later

  #define BB_BACKING_PTR(bb) \
    (FL_TEST((bb)->backing_str, RSTRING_NOEMBED) ? RSTRING((bb)->backing_str)->as.heap.ptr : &RSTRING(bb->backing_str)->as.embed.ary)
#else
  // Before ruby 3.1

  #define BB_BACKING_PTR(bb) \
    (FL_TEST((bb)->backing_str, RSTRING_NOEMBED) ? RSTRING((bb)->backing_str)->as.heap.ptr : &RSTRING(bb->backing_str)->as.ary)
#endif

static void
t_bb_reassign_ptr(struct LLC_ArrayBuffer *bb) {
  if (!bb->backing_str) {
    bb->ptr = NULL;
    return;
  }

  rb_str_set_len(bb->backing_str, bb->size);
  bb->ptr = BB_BACKING_PTR(bb);
  bb->ptr[bb->size] = 0;
}

static VALUE
t_bb_initialize(VALUE self, VALUE size) {
  DECLAREBB(self);
  unsigned int s = NUM2UINT(size);
  bb->size = s;
  if (bb->backing_str)
    rb_gc_mark(bb->backing_str);

  bb->backing_str = rb_str_buf_new(s);

  t_bb_reassign_ptr(bb);
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

  rb_str_resize(bb->backing_str, new_size);
  bb->size = new_size;
  t_bb_reassign_ptr(bb);

  return self;
}

/*
 * Returns a ASCII-8BIT string with the contents of the buffer
 *
 * The returned string is the backing string of the buffer.
 * It's encoding is always ASCII-8BIT.
 * If the buffer has size zero, an empty string is returned.
 *
 * @return [String]
 */
static VALUE
t_bb_bytes(VALUE self) {
  DECLAREBB(self);
  return bb->backing_str;
}

void
Init_arraybuffer() {
  cArrayBuffer = rb_define_class("ArrayBuffer", rb_cObject);
  rb_define_alloc_func(cArrayBuffer, t_bb_allocator);
  rb_include_module(cArrayBuffer, rb_mEnumerable);

  rb_define_method(cArrayBuffer, "initialize", t_bb_initialize, 1);
  rb_define_method(cArrayBuffer, "[]", t_bb_getbyte, 1);
  rb_define_method(cArrayBuffer, "[]=", t_bb_setbyte, 2);
  rb_define_method(cArrayBuffer, "size", t_bb_size, 0);
  rb_define_alias(cArrayBuffer, "length", "size");
  rb_define_method(cArrayBuffer, "each", t_bb_each, 0);
  rb_define_method(cArrayBuffer, "realloc", t_bb_realloc, 1);
  rb_define_method(cArrayBuffer, "bytes", t_bb_bytes, 0);
  rb_define_method(cArrayBuffer, "to_s", t_bb_bytes, 0);

#ifdef HAVE_RUBY_MEMORY_VIEW_H
  rb_memory_view_register(cArrayBuffer, &cArrayBufferMemoryView);
#endif
}

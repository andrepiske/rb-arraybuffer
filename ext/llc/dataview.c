#include "dataview.h"
#include "bytebuffer.h"
#include "extconf.h"

#ifdef HAVE_STRING_H
#include <string.h>
#endif

extern VALUE cByteBuffer;
extern VALUE cDataView;
extern VALUE mLLC;

static ID idEndianess = Qundef;
static ID idBig = Qundef;
static ID idLittle = Qundef;

#define FLAG_LITTLE_ENDIAN 1

#define DECLAREDV(o) \
  struct LLC_DataView *dv = (struct LLC_DataView*)rb_data_object_get((o))
#define DECLAREBB(o) \
  struct LLC_ByteBuffer *bb = (struct LLC_ByteBuffer*)rb_data_object_get((o))
#define CHECK_LITTLEENDIAN(dv) ((dv)->flags & FLAG_LITTLE_ENDIAN)

static void
t_dv_gc_mark(struct LLC_DataView *dv) {
  if (dv->bb_obj)
    rb_gc_mark(dv->bb_obj);
}

static void
t_dv_free(struct LLC_DataView *dv) {
  xfree(dv);
}

static VALUE
t_dv_allocator(VALUE klass) {
  struct LLC_DataView *dv = (struct LLC_DataView*)xmalloc(sizeof(struct LLC_DataView));
  dv->bb_obj = Qundef;
  dv->size = 0;
  dv->offset = 0;
  dv->flags = 0;
  return Data_Wrap_Struct(klass, t_dv_gc_mark, t_dv_free, dv);
}

static VALUE
t_dv_initialize(int argc, VALUE *argv, VALUE self) {
  DECLAREDV(self);
  VALUE bb_obj;
  VALUE offset;
  VALUE size;
  VALUE kwargs;
  static ID keyword_ids[] = { 0 };

  rb_scan_args(argc, argv, "12:", &bb_obj, &offset, &size, &kwargs);

  DECLAREBB(bb_obj);

  int size_val = NIL_P(size) ? (int)bb->size : NUM2INT(size);
  if (size_val < 0)
    size_val += (int)bb->size;
  if (size_val < 0)
    rb_raise(rb_eArgError, "calculated size is negative: %d", size_val);

  int offset_val = NIL_P(offset) ? 0 : NUM2INT(offset);
  if (offset_val < 0)
    offset_val += size_val;
  if (offset_val < 0)
    rb_raise(rb_eArgError, "calculated offset is negative: %d", size_val);

  dv->offset = (unsigned long)offset_val;
  dv->size = (unsigned long)size_val;
  dv->bb_obj = bb_obj;

  if (!keyword_ids[0]) {
    keyword_ids[0] = idEndianess;
  }

  if (!NIL_P(kwargs)) {
    VALUE endianess;
    rb_get_kwargs(kwargs, keyword_ids, 0, 1, &endianess);
    if (endianess != Qundef) {
      Check_Type(endianess, T_SYMBOL);
      ID id = SYM2ID(endianess);
      if (id == idLittle)
        dv->flags |= FLAG_LITTLE_ENDIAN;
      else if (id != idBig)
        rb_raise(rb_eArgError, "endianess must be either :big or :little");
    }
  }

  return self;
}

static VALUE
t_dv_size(VALUE self) {
  DECLAREDV(self);
  return UINT2NUM(dv->size);
}

static VALUE
t_dv_offset(VALUE self) {
  DECLAREDV(self);
  return UINT2NUM(dv->offset);
}

static VALUE
t_dv_endianess(VALUE self) {
  DECLAREDV(self);
  return CHECK_LITTLEENDIAN(dv) ?
    ID2SYM(idLittle) :
    ID2SYM(idBig);
}

static VALUE
t_dv_each(VALUE self) {
  DECLAREDV(self);
  DECLAREBB(dv->bb_obj);

  int size = dv->size;
  if (size + dv->offset >= bb->size)
    size = bb->size - dv->offset;

  if (rb_block_given_p()) {
    for (int i = 0; i < size; i++) {
      unsigned int val = (unsigned int)bb->ptr[i + dv->offset];
      rb_yield(UINT2NUM(val));
    }
  } else {
    rb_raise(rb_eArgError, "no block given");
  }
  return self;
}

#define DECLARENCHECKIDX(index) int idx = NUM2INT(index); \
  if (idx < 0) idx += (int)dv->size; \
  if (idx < 0 || idx >= dv->size) rb_raise(rb_eArgError, "index out of bounds: %d", idx);

#define CHECKBOUNDSBB(v) if ((v) < 0 || (v) >= (bb)->size) \
  rb_raise(rb_eArgError, "index out of underlying buffer bounds: %d", (v));

static VALUE
t_dv_getbit(VALUE self, VALUE index) {
  DECLAREDV(self); DECLAREBB(dv->bb_obj);
  int idx = NUM2INT(index);
  if (idx < 0)
    idx += (int)dv->size * 8;
  if (idx < 0 || idx >= dv->size * 8)
    rb_raise(rb_eArgError, "index out of bounds: %d", idx);

  unsigned int bit_idx = ((unsigned int)idx) & 7;
  unsigned int byte_idx = (((unsigned int)idx) >> 3) + dv->offset;
  unsigned char bit_mask = 1 << bit_idx;

  CHECKBOUNDSBB(byte_idx);
  return (bb->ptr[byte_idx] & bit_mask) ? UINT2NUM(1) : UINT2NUM(0);
}

static VALUE
t_dv_getu8(VALUE self, VALUE index) {
  DECLAREDV(self); DECLARENCHECKIDX(index); DECLAREBB(dv->bb_obj);
  unsigned int real_idx = dv->offset + (unsigned int)idx;
  CHECKBOUNDSBB(real_idx);
  return UINT2NUM(bb->ptr[real_idx]);
}

static VALUE
t_dv_getu16(VALUE self, VALUE index) {
  DECLAREDV(self); DECLARENCHECKIDX(index); DECLAREBB(dv->bb_obj);
  unsigned int idx0 = dv->offset + (unsigned int)idx;
  unsigned int idx1 = dv->offset + (unsigned int)idx + 1;
  CHECKBOUNDSBB(idx0);
  CHECKBOUNDSBB(idx1);
  unsigned short val = 0;
  if (CHECK_LITTLEENDIAN(dv)) {
    val |= bb->ptr[idx0];
    val |= bb->ptr[idx1] << 8;
  } else {
    val |= bb->ptr[idx0] << 8;
    val |= bb->ptr[idx1];
  }
  return UINT2NUM(val);
}

static VALUE
t_dv_getu24(VALUE self, VALUE index) {
  DECLAREDV(self); DECLARENCHECKIDX(index); DECLAREBB(dv->bb_obj);
  unsigned int idx0 = dv->offset + (unsigned int)idx;
  unsigned int idx1 = dv->offset + (unsigned int)idx + 1;
  unsigned int idx2 = dv->offset + (unsigned int)idx + 2;
  CHECKBOUNDSBB(idx0);
  CHECKBOUNDSBB(idx2);
  unsigned int val = 0;
  if (CHECK_LITTLEENDIAN(dv)) {
    val |= bb->ptr[idx0];
    val |= bb->ptr[idx1] << 8;
    val |= bb->ptr[idx2] << 16;
  } else {
    val |= bb->ptr[idx0] << 16;
    val |= bb->ptr[idx1] << 8;
    val |= bb->ptr[idx2];
  }
  return UINT2NUM(val);
}

static VALUE
t_dv_getu32(VALUE self, VALUE index) {
  DECLAREDV(self); DECLARENCHECKIDX(index); DECLAREBB(dv->bb_obj);
  unsigned int idx0 = dv->offset + (unsigned int)idx;
  unsigned int idx1 = dv->offset + (unsigned int)idx + 1;
  unsigned int idx2 = dv->offset + (unsigned int)idx + 2;
  unsigned int idx3 = dv->offset + (unsigned int)idx + 3;
  CHECKBOUNDSBB(idx0);
  CHECKBOUNDSBB(idx3);
  unsigned int val = 0;
  if (CHECK_LITTLEENDIAN(dv)) {
    val |= bb->ptr[idx0];
    val |= bb->ptr[idx1] << 8;
    val |= bb->ptr[idx2] << 16;
    val |= bb->ptr[idx3] << 24;
  } else {
    val |= bb->ptr[idx0] << 24;
    val |= bb->ptr[idx1] << 16;
    val |= bb->ptr[idx2] << 8;
    val |= bb->ptr[idx3];
  }
  return UINT2NUM(val);
}

#define ADJUSTBOUNDS(val, max) if (val < 0) val = 0; else if (val > max) val = max;

static VALUE
t_dv_setu8(VALUE self, VALUE index, VALUE value) {
  DECLAREDV(self); DECLARENCHECKIDX(index); DECLAREBB(dv->bb_obj);
  unsigned int idx0 = dv->offset + (unsigned int)idx;
  CHECKBOUNDSBB(idx0);
  int val = NUM2INT(value);
  ADJUSTBOUNDS(val, 0xFF);
  bb->ptr[idx0] = (unsigned char)val;
  return self;
}

static VALUE
t_dv_setu16(VALUE self, VALUE index, VALUE value) {
  DECLAREDV(self); DECLARENCHECKIDX(index); DECLAREBB(dv->bb_obj);
  unsigned int idx0 = dv->offset + (unsigned int)idx;
  unsigned int idx1 = dv->offset + (unsigned int)idx + 1;
  CHECKBOUNDSBB(idx0);
  CHECKBOUNDSBB(idx1);
  int val = NUM2INT(value);
  ADJUSTBOUNDS(val, 0xFFFF);
  unsigned int uval = (unsigned int)val;
  if (CHECK_LITTLEENDIAN(dv)) {
    bb->ptr[idx0] = (unsigned char)(uval & 0xFF);
    bb->ptr[idx1] = (unsigned char)((uval >> 8) & 0xFF);
  } else {
    bb->ptr[idx0] = (unsigned char)((uval >> 8) & 0xFF);
    bb->ptr[idx1] = (unsigned char)(uval & 0xFF);
  }
  return self;
}

static VALUE
t_dv_setu24(VALUE self, VALUE index, VALUE value) {
  DECLAREDV(self); DECLARENCHECKIDX(index); DECLAREBB(dv->bb_obj);
  unsigned int idx0 = dv->offset + (unsigned int)idx;
  unsigned int idx1 = dv->offset + (unsigned int)idx + 1;
  unsigned int idx2 = dv->offset + (unsigned int)idx + 2;
  CHECKBOUNDSBB(idx0);
  CHECKBOUNDSBB(idx2);
  int val = NUM2INT(value);
  ADJUSTBOUNDS(val, 0xFFFFFF);
  unsigned int uval = (unsigned int)val;
  if (CHECK_LITTLEENDIAN(dv)) {
    bb->ptr[idx0] = (unsigned char)(uval & 0xFF);
    bb->ptr[idx1] = (unsigned char)((uval >> 8) & 0xFF);
    bb->ptr[idx2] = (unsigned char)((uval >> 16) & 0xFF);
  } else {
    bb->ptr[idx0] = (unsigned char)((uval >> 16) & 0xFF);
    bb->ptr[idx1] = (unsigned char)((uval >> 8) & 0xFF);
    bb->ptr[idx2] = (unsigned char)(uval & 0xFF);
  }
  return self;
}

static VALUE
t_dv_setu32(VALUE self, VALUE index, VALUE value) {
  DECLAREDV(self); DECLARENCHECKIDX(index); DECLAREBB(dv->bb_obj);
  unsigned int idx0 = dv->offset + (unsigned int)idx;
  unsigned int idx1 = dv->offset + (unsigned int)idx + 1;
  unsigned int idx2 = dv->offset + (unsigned int)idx + 2;
  unsigned int idx3 = dv->offset + (unsigned int)idx + 3;
  CHECKBOUNDSBB(idx0);
  CHECKBOUNDSBB(idx3);
  long val = NUM2LONG(value);
  ADJUSTBOUNDS(val, 0xFFFFFFFF);
  unsigned long uval = (unsigned long)val;
  if (CHECK_LITTLEENDIAN(dv)) {
    bb->ptr[idx0] = (unsigned char)(uval & 0xFF);
    bb->ptr[idx1] = (unsigned char)((uval >> 8) & 0xFF);
    bb->ptr[idx2] = (unsigned char)((uval >> 16) & 0xFF);
    bb->ptr[idx3] = (unsigned char)((uval >> 24) & 0xFF);
  } else {
    bb->ptr[idx0] = (unsigned char)((uval >> 24) & 0xFF);
    bb->ptr[idx1] = (unsigned char)((uval >> 16) & 0xFF);
    bb->ptr[idx2] = (unsigned char)((uval >> 8) & 0xFF);
    bb->ptr[idx3] = (unsigned char)(uval & 0xFF);
  }
  return self;
}

void
Init_dataview() {
  mLLC = rb_define_module("LLC");

  idEndianess = rb_intern("endianess");
  idLittle = rb_intern("little");
  idBig = rb_intern("big");

  cDataView = rb_define_class_under(mLLC, "DataView", rb_cObject);
  rb_define_alloc_func(cDataView, t_dv_allocator);
  rb_include_module(cDataView, rb_mEnumerable);

  rb_define_method(cDataView, "initialize", t_dv_initialize, -1);
  rb_define_method(cDataView, "getBit", t_dv_getbit, 1);
  rb_define_method(cDataView, "getU8", t_dv_getu8, 1);
  rb_define_method(cDataView, "getU16", t_dv_getu16, 1);
  rb_define_method(cDataView, "getU24", t_dv_getu24, 1);
  rb_define_method(cDataView, "getU32", t_dv_getu32, 1);

  rb_define_method(cDataView, "setU8", t_dv_setu8, 2);
  rb_define_method(cDataView, "setU16", t_dv_setu16, 2);
  rb_define_method(cDataView, "setU24", t_dv_setu24, 2);
  rb_define_method(cDataView, "setU32", t_dv_setu32, 2);

  rb_define_method(cDataView, "endianess", t_dv_endianess, 0);
  rb_define_method(cDataView, "offset", t_dv_offset, 0);
  rb_define_method(cDataView, "size", t_dv_size, 0);
  rb_define_alias(cDataView, "length", "size");
  rb_define_method(cDataView, "each", t_dv_each, 0);

  // rb_define_method(cDataView, "reoffset", t_dv_realloc, 1);
  // rb_define_method(cDataView, "resize", t_dv_realloc, 1);
}

#include "dataview.h"
#include "arraybuffer.h"
#include "extconf.h"

#ifdef HAVE_STRING_H
#include <string.h>
#endif

extern VALUE cArrayBuffer;
extern VALUE cDataView;

static ID idEndianess = Qundef;
static ID idBig = Qundef;
static ID idLittle = Qundef;

#define FLAG_LITTLE_ENDIAN 1

#define DECLAREDV(o) \
  struct LLC_DataView *dv = (struct LLC_DataView*)rb_data_object_get((o))
#define DECLAREBB(o) \
  struct LLC_ArrayBuffer *bb = (struct LLC_ArrayBuffer*)rb_data_object_get((o))
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

/*
 * call-seq:
 *  initialize(buffer, offset, size, endianess:)
 *
 * Constructs a new DataView that provides a view of the data in a ArrayBuffer.
 *
 * Example:
 *   buffer = ArrayBuffer.new(10)
 *   buffer[1] = 20
 *   buffer[2] = 55
 *   view = DataView.new(buffer, 1, endianess: :little)
 *   view.getU8() == ( 20 & (55 << 8) ) # true
 *
 * Or:
 *
 *   view = DataView.new(buffer, 1) # default endianess is "big endian"
 *   view.getU8() == ( (20 << 8) | 55 ) # true
 *
 * @param buffer [ArrayBuffer] The array buffer where to operate
 * @param offset [Integer] Optional. The byte offset from the array buffer. The
 *   default value is zero
 * @param size [Integer] Optional. The size in bytes that this DataView can
 *   see into the array buffer. If left blank, the current size of the array
 *   buffer will be used
 * @param endianess [:big, :little] Optional. The default value is big
 *
 */
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
    offset_val += (int)bb->size;
  if (offset_val < 0)
    rb_raise(rb_eArgError, "calculated offset is negative: %d", offset_val);

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

/*
 * Defined the new offset for the DataView.
 *
 * Changing the offset does not affect the size.
 * If passed a negative value, it will be summed with the underlying buffer
 * size. The final value must not be lower than zero.
 *
 * @param offset [Integer] The new offset
 */
static VALUE
t_dv_setoffset(VALUE self, VALUE offset) {
  DECLAREDV(self);
  DECLAREBB(dv->bb_obj);
  int offset_val = NIL_P(offset) ? 0 : NUM2INT(offset);
  if (offset_val < 0)
    offset_val += (int)bb->size;
  if (offset_val < 0)
    rb_raise(rb_eArgError, "calculated offset is negative: %d", offset_val);

  dv->offset = (unsigned long)offset_val;
  return self;
}

/*
 * Defined the new offset for the DataView.
 *
 * Changing the size does not affect the offset.
 * If passed a negative value, it will be summed with the underlying buffer
 * size. The final value must not be lower than zero.
 *
 * @param size [Integer] The new size. Must be zero or greater
 */
static VALUE
t_dv_setsize(VALUE self, VALUE size) {
  DECLAREDV(self);
  DECLAREBB(dv->bb_obj);
  int size_val = NIL_P(size) ? (int)bb->size : NUM2INT(size);
  if (size_val < 0)
    size_val += (int)bb->size;
  if (size_val < 0)
    rb_raise(rb_eArgError, "calculated size is negative: %d", size_val);

  dv->size = (unsigned long)size_val;
  return self;
}

#define DECLARENCHECKIDX(index) int idx = NUM2INT(index); \
  if (idx < 0) idx += (int)dv->size; \
  if (idx < 0 || idx >= dv->size) rb_raise(rb_eArgError, "index out of bounds: %d", idx);

#define CHECKBOUNDSBB(v) if ((v) < 0 || (v) >= (bb)->size) \
  rb_raise(rb_eArgError, "index out of underlying buffer bounds: %d", (v));

/*
 * Reads a bit at index.
 *
 * This method differs from the other in that the +index+ is given in bits, not
 * in bytes.
 * Thus it's possible to read from index +0+ to +(size * 8) - 1+
 *
 * @return [Integer] Either 0 or 1
 */
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

/*
 * Reads the byte at index as an +unsigned char+
 *
 * @return [Integer] Integer between 0 and 255
 */
static VALUE
t_dv_getu8(VALUE self, VALUE index) {
  DECLAREDV(self); DECLARENCHECKIDX(index); DECLAREBB(dv->bb_obj);
  unsigned int real_idx = dv->offset + (unsigned int)idx;
  CHECKBOUNDSBB(real_idx);
  return UINT2NUM(bb->ptr[real_idx]);
}

/*
 * Reads two bytes starting at index as an +unsigned short+
 *
 * @return [Integer] Integer between 0 and 65535
 */
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

/*
 * Reads three bytes starting at index as a 3 bytes long unsigned integer
 *
 * @return [Integer] Integer between 0 and 16777215
 */
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

/*
 * Reads four bytes starting at index as a 4 bytes long unsigned integer
 *
 * @return [Integer] Integer between 0 and 4294967295
 */
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

/*
 * Interprets one byte at index and assigns it an unsigned value
 *
 * Values lower than zero will be set to 0 and values greater than 255
 * will be capped.
 */
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

/*
 * Interprets two bytes starting at index and sets them an unsigned value
 *
 * Values lower than zero will be set to 0 and values greater than 65535
 * will be capped.
 */
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

/*
 * Interprets three bytes starting at index and sets them an unsigned value
 *
 * Values lower than zero will be set to 0 and values greater than 16777215
 * will be capped.
 */
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

/*
 * Interprets four bytes starting at index and sets them an unsigned value
 *
 * Values lower than zero will be set to 0 and values greater than 4294967295
 * will be capped.
 */
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

static VALUE
t_dv_setbytes(VALUE self, VALUE index, VALUE bytes) {
  DECLAREDV(self); DECLARENCHECKIDX(index); DECLAREBB(dv->bb_obj);
  unsigned int idx0 = dv->offset + (unsigned int)idx;
  CHECKBOUNDSBB(idx0);

  if (RB_TYPE_P(bytes, T_ARRAY)) {
    const unsigned int length = (unsigned int)rb_array_len(bytes);
    const VALUE* items = rb_array_const_ptr(bytes);
    CHECKBOUNDSBB(idx0 + length);

    for (unsigned int i = 0; i < length; i++) {
      if (!RB_FIXNUM_P(items[i]))
        rb_raise(rb_eRuntimeError, "array contains non fixnum value at index %d", i);
      int num = NUM2INT(items[i]);
      ADJUSTBOUNDS(num, 0xFF);
      bb->ptr[idx0 + i] = (unsigned char)num;
    }
  } else if (RB_TYPE_P(bytes, T_STRING)) {
    const char *str_ptr = RSTRING_PTR(bytes);
    const unsigned int length = (unsigned int)RSTRING_LEN(bytes);
    CHECKBOUNDSBB(idx0 + length);

    for (unsigned int i = 0; i < length; i++) {
      bb->ptr[idx0 + i] = (unsigned char)str_ptr[i];
    }
  } else if (RB_TYPE_P(bytes, T_DATA) &&
    (CLASS_OF(bytes) == cArrayBuffer || CLASS_OF(bytes) == cDataView)) {
    unsigned int length;
    const char *src_bytes;
    if (CLASS_OF(bytes) == cArrayBuffer) {
      struct LLC_ArrayBuffer *src_bb = (struct LLC_ArrayBuffer*)rb_data_object_get(bytes);
      length = src_bb->size;
      src_bytes = (const char*)src_bb->ptr;
    } else {
      struct LLC_DataView *src_dv = (struct LLC_DataView*)rb_data_object_get(bytes);
      struct LLC_ArrayBuffer *src_bb = (struct LLC_ArrayBuffer*)rb_data_object_get(src_dv->bb_obj);
      length = src_dv->size;
      src_bytes = (const char*)(src_bb->ptr + (size_t)src_dv->offset);
      if (src_dv->offset >= src_bb->size)
        rb_raise(rb_eRuntimeError, "offset exceeds the underlying source buffer size");
      if (src_dv->offset + length >= src_bb->size)
        rb_raise(rb_eRuntimeError, "offset + size exceeds the underlying source buffer size");
    }

    CHECKBOUNDSBB(idx0 + length);
    memcpy((void*)(bb->ptr + (size_t)idx0), src_bytes, (size_t)length);
  } else {
    rb_raise(rb_eArgError, "Invalid type: %+"PRIsVALUE, CLASS_OF(bytes));
  }

  return self;
}

void
Init_dataview() {
  idEndianess = rb_intern("endianess");
  idLittle = rb_intern("little");
  idBig = rb_intern("big");

  cDataView = rb_define_class("DataView", rb_cObject);
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

  rb_define_method(cDataView, "setBytes", t_dv_setbytes, 2);

  rb_define_method(cDataView, "endianess", t_dv_endianess, 0);
  rb_define_method(cDataView, "offset=", t_dv_setoffset, 1);
  rb_define_method(cDataView, "offset", t_dv_offset, 0);
  rb_define_method(cDataView, "size=", t_dv_setsize, 1);
  rb_define_alias(cDataView, "length=", "size=");
  rb_define_method(cDataView, "size", t_dv_size, 0);
  rb_define_alias(cDataView, "length", "size");
  rb_define_method(cDataView, "each", t_dv_each, 0);
}

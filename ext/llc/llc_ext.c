#include <ruby.h>
#include "extconf.h"

VALUE cByteBuffer = Qundef;
VALUE cDataView = Qundef;
VALUE mLLC = Qundef;

void Init_dataview();
void Init_bytebuffer();

void
Init_llc_ext() {
  mLLC = rb_define_module("LLC");

  Init_bytebuffer();
  Init_dataview();
}

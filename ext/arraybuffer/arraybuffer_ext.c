#include <ruby.h>
#include "extconf.h"

VALUE cArrayBuffer = Qundef;
VALUE cDataView = Qundef;

void Init_dataview();
void Init_arraybuffer();

void
Init_arraybuffer_ext() {
  Init_arraybuffer();
  Init_dataview();
}

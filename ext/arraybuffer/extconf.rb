#!/usr/bin/env ruby
require 'mkmf'

# preparation for compilation goes here

if have_header("ruby/memory_view.h")
  have_type("rb_memory_view_t", ["ruby/memory_view.h"])
end

create_header
create_makefile 'arraybuffer_ext'

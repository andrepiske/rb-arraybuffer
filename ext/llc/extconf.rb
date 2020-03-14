#!/usr/bin/env ruby
require 'mkmf'

# preparation for compilation goes here

have_header('string.h')

# dir_config 'llc'

create_header
create_makefile 'llc_ext'

# encoding: UTF-8
require 'mkmf'
dir_config('bz2')
have_header('bzlib.h')

$CFLAGS << ' -Wall -Wextra -Wno-unused -funroll-loops '
# $CFLAGS << ' -O0 -ggdb -Wextra'

if have_library("bz2", "BZ2_bzWriteOpen")
  if enable_config("shared", true)
     $static = nil
  end

  if RUBY_VERSION.to_f >= 1.9
    $CFLAGS << ' -DRUBY_19_COMPATIBILITY'
  end
  
  create_makefile('bzip2/bzip2')
else
  puts "libbz2 not found, maybe try manually specifying --with-bz2-dir to find it?"
end

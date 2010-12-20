#ifndef _RB_BZIP2_READER_H_
#define _RB_BZIP2_READER_H_

#include <ruby.h>

/* Instance methods */
VALUE bz_reader_init(int argc, VALUE *argv, VALUE obj);
VALUE bz_reader_read(int argc, VALUE *argv, VALUE obj);
VALUE bz_reader_ungetc(VALUE obj, VALUE a);
VALUE bz_reader_ungets(VALUE obj, VALUE a);
VALUE bz_reader_getc(VALUE obj);
VALUE bz_reader_readchar(VALUE obj);
VALUE bz_reader_gets_m(int argc, VALUE *argv, VALUE obj);
VALUE bz_reader_readline(int argc, VALUE *argv, VALUE obj);
VALUE bz_reader_readlines(int argc, VALUE *argv, VALUE obj);
VALUE bz_reader_each_line(int argc, VALUE *argv, VALUE obj);
VALUE bz_reader_each_byte(VALUE obj);
VALUE bz_reader_unused(VALUE obj);
VALUE bz_reader_set_unused(VALUE obj, VALUE a);
VALUE bz_reader_eoz(VALUE obj);
VALUE bz_reader_eof(VALUE obj);
VALUE bz_reader_closed(VALUE obj);
VALUE bz_reader_close(VALUE obj);
VALUE bz_reader_close_bang(VALUE obj);
VALUE bz_reader_finish(VALUE obj);
VALUE bz_reader_lineno(VALUE obj);
VALUE bz_reader_set_lineno(VALUE obj, VALUE lineno);

/* Class methods */
VALUE bz_reader_s_alloc(VALUE obj);
VALUE bz_reader_s_open(int argc, VALUE *argv, VALUE obj);
VALUE bz_reader_s_foreach(int argc, VALUE *argv, VALUE obj);
VALUE bz_reader_s_readlines(int argc, VALUE *argv, VALUE obj);

#endif

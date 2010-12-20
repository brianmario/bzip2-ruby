#ifndef _RB_BZIP2_WRITER_H_
#define _RB_BZIP2_WRITER_H_

#include <ruby.h>
#include "common.h"

VALUE bz_writer_internal_flush(struct bz_file *bzf);

VALUE bz_writer_close(VALUE obj);
VALUE bz_writer_close_bang(VALUE obj);
VALUE bz_writer_closed(VALUE obj);
VALUE bz_writer_flush(VALUE obj);
VALUE bz_writer_init(int argc, VALUE *argv, VALUE obj);
VALUE bz_writer_write(VALUE obj, VALUE a);
VALUE bz_writer_putc(VALUE obj, VALUE a);

VALUE bz_writer_s_alloc(VALUE obj);
VALUE bz_writer_s_open(int argc, VALUE *argv, VALUE obj);

#endif

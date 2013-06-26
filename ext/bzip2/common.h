#ifndef _RB_BZIP2_COMMON_H_
#define _RB_BZIP2_COMMON_H_

#include <ruby.h>
#include <bzlib.h>

#ifndef RUBY_19_COMPATIBILITY
#  include <rubyio.h>
//#  include <version.h>
#else
#  include <ruby/io.h>
#endif

#define BZ2_RB_CLOSE    1
#define BZ2_RB_INTERNAL 2

#define BZ_RB_BLOCKSIZE 4096
#define DEFAULT_BLOCKS 9
#define ASIZE (1 << CHAR_BIT)

/* Older versions of Ruby (< 1.8.6) need these */
#ifndef RSTRING_PTR
#  define RSTRING_PTR(s) (RSTRING(s)->ptr)
#endif
#ifndef RSTRING_LEN
#  define RSTRING_LEN(s) (RSTRING(s)->len)
#endif
#ifndef RARRAY_PTR
#  define RARRAY_PTR(s) (RARRAY(s)->ptr)
#endif
#ifndef RARRAY_LEN
#  define RARRAY_LEN(s) (RARRAY(s)->len)
#endif

struct bz_file {
    bz_stream bzs;
    VALUE in, io;
    char *buf;
    unsigned int buflen;
    int blocks, work, small;
    int flags, lineno, state;
};

struct bz_str {
    VALUE str;
    int pos;
};

struct bz_iv {
    VALUE bz2, io;
    void (*finalize)();
};

#define Get_BZ2(obj, bzf)                       \
    rb_io_taint_check(obj);                     \
    Data_Get_Struct(obj, struct bz_file, bzf);  \
    if (!RTEST(bzf->io)) {                      \
        rb_raise(rb_eIOError, "closed IO");     \
    }

#ifndef ASDFasdf
extern VALUE bz_cWriter, bz_cReader, bz_cInternal;
extern VALUE bz_eError, bz_eEOZError;

extern VALUE bz_internal_ary;

extern ID id_new, id_write, id_open, id_flush, id_read;
extern ID id_closed, id_close, id_str;
#endif

void bz_file_mark(struct bz_file * bzf);
void* bz_malloc(void *opaque, int m, int n);
void bz_free(void *opaque, void *p);
VALUE bz_raise(int err);

#endif

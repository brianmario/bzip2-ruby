#include <ruby.h>
#include <bzlib.h>

#include "common.h"
#include "reader.h"
#include "writer.h"

VALUE bz_cWriter, bz_cReader, bz_cInternal;
VALUE bz_eError, bz_eEOZError;

VALUE bz_internal_ary;

ID id_new, id_write, id_open, id_flush, id_read;
ID id_closed, id_close, id_str;

void bz_internal_finalize(VALUE data) {
    VALUE elem;
    int closed, i;
    struct bz_iv *bziv;
    struct bz_file *bzf;

    for (i = 0; i < RARRAY_LEN(bz_internal_ary); i++) {
        elem = RARRAY_PTR(bz_internal_ary)[i];
        Data_Get_Struct(elem, struct bz_iv, bziv);
        if (bziv->bz2) {
            RDATA(bziv->bz2)->dfree = free;
            if (TYPE(bziv->io) == T_FILE) {
                RFILE(bziv->io)->fptr->finalize = bziv->finalize;
            } else if (TYPE(bziv->io) == T_DATA) {
                RDATA(bziv->io)->dfree = bziv->finalize;
            }
            Data_Get_Struct(bziv->bz2, struct bz_file, bzf);
            closed = bz_writer_internal_flush(bzf);
            if (bzf->flags & BZ2_RB_CLOSE) {
                bzf->flags &= ~BZ2_RB_CLOSE;
                if (!closed && rb_respond_to(bzf->io, id_close)) {
                    rb_funcall2(bzf->io, id_close, 0, 0);
                }
            }
        }
    }
}

/*
 * call-seq:
 *   compress(str)
 *
 * Shortcut for compressing just a string.
 *
 *    Bzip2.uncompress Bzip2.compress('data') # => 'data'
 *
 * @param [String] str the string to compress
 * @return [String] +str+ compressed with bz2
 */
VALUE bz_compress(VALUE self, VALUE str) {
    VALUE bz2, argv[1] = {Qnil};

    str = rb_str_to_str(str);
    bz2 = rb_funcall2(bz_cWriter, id_new, 1, argv);
    if (OBJ_TAINTED(str)) {
        struct bz_file *bzf;
        Data_Get_Struct(bz2, struct bz_file, bzf);
        OBJ_TAINT(bzf->io);
    }
    bz_writer_write(bz2, str);
    return bz_writer_close(bz2);
}

/*
 * Returns the io stream underlying this stream. If the strem was constructed
 * with a file, that is returned. Otherwise, an empty string is returned.
 *
 * @return [File, String] similar to whatever the stream was constructed with
 * @raise [IOError] if the stream has been closed
 */
VALUE bz_to_io(VALUE obj) {
    struct bz_file *bzf;

    Get_BZ2(obj, bzf);
    return bzf->io;
}

VALUE bz_str_read(int argc, VALUE *argv, VALUE obj) {
    struct bz_str *bzs;
    VALUE res, len;
    int count;

    Data_Get_Struct(obj, struct bz_str, bzs);
    rb_scan_args(argc, argv, "01", &len);
    if (NIL_P(len)) {
        count = (int) RSTRING_LEN(bzs->str);
    } else {
        count = NUM2INT(len);
        if (count < 0) {
            rb_raise(rb_eArgError, "negative length %d given", count);
        }
    }
    if (!count || bzs->pos == -1) {
        return Qnil;
    }
    if ((bzs->pos + count) >= RSTRING_LEN(bzs->str)) {
        res = rb_str_new(RSTRING_PTR(bzs->str) + bzs->pos,
            RSTRING_LEN(bzs->str) - bzs->pos);
        bzs->pos = -1;
    } else {
        res = rb_str_new(RSTRING_PTR(bzs->str) + bzs->pos, count);
        bzs->pos += count;
    }
    return res;
}

/*
 * call-seq:
 *    uncompress(data)
 * Decompress a string of bz2 compressed data.
 *
 *    Bzip2.uncompress Bzip2.compress('asdf') # => 'asdf'
 *
 * @param [String] data bz2 compressed data
 * @return [String] +data+ as uncompressed bz2 data
 * @raise [Bzip2::Error] if +data+ is not valid bz2 data
 */
VALUE bz_uncompress(VALUE self, VALUE data) {
    VALUE bz2, nilv = Qnil, argv[1];

    argv[0] = rb_str_to_str(data);
    bz2 = rb_funcall2(bz_cReader, id_new, 1, argv);
    return bz_reader_read(1, &nilv, bz2);
}

/*
 * Internally allocates data,
 *
 * @see Bzip2::Writer#initialize
 * @see Bzip2::Reader#initialize
 * @private
 */
VALUE bz_s_new(int argc, VALUE *argv, VALUE obj) {
    VALUE res = rb_funcall2(obj, rb_intern("allocate"), 0, 0);
    rb_obj_call_init(res, argc, argv);
    return res;
}

void Init_bzip2_ext() {
    VALUE bz_mBzip2;
    VALUE bz_mBzip2Singleton;

    bz_internal_ary = rb_ary_new();
    rb_global_variable(&bz_internal_ary);
    rb_set_end_proc(bz_internal_finalize, Qnil);

    id_new    = rb_intern("new");
    id_write  = rb_intern("write");
    id_open   = rb_intern("open");
    id_flush  = rb_intern("flush");
    id_read   = rb_intern("read");
    id_close  = rb_intern("close");
    id_closed = rb_intern("closed?");
    id_str    = rb_intern("to_str");

    bz_mBzip2       = rb_define_module("Bzip2");
    bz_eError       = rb_define_class_under(bz_mBzip2, "Error", rb_eIOError);
    bz_eEOZError    = rb_define_class_under(bz_mBzip2, "EOZError", bz_eError);

    bz_mBzip2Singleton = rb_singleton_class(bz_mBzip2);
    rb_define_singleton_method(bz_mBzip2, "compress",   bz_compress,    1);
    rb_define_singleton_method(bz_mBzip2, "uncompress", bz_uncompress,  1);
    rb_define_alias(bz_mBzip2Singleton, "bzip2",      "compress");
    rb_define_alias(bz_mBzip2Singleton, "decompress", "uncompress");
    rb_define_alias(bz_mBzip2Singleton, "bunzip2",    "uncompress");

    /*
      Writer
    */
    bz_cWriter = rb_define_class_under(bz_mBzip2, "Writer", rb_cData);
#if HAVE_RB_DEFINE_ALLOC_FUNC
    rb_define_alloc_func(bz_cWriter, bz_writer_s_alloc);
#else
    rb_define_singleton_method(bz_cWriter, "allocate", bz_writer_s_alloc, 0);
#endif
    rb_define_singleton_method(bz_cWriter, "new", bz_s_new, -1);
    rb_define_singleton_method(bz_cWriter, "open", bz_writer_s_open, -1);
    rb_define_method(bz_cWriter, "initialize", bz_writer_init, -1);
    rb_define_method(bz_cWriter, "write", bz_writer_write, 1);
    rb_define_method(bz_cWriter, "putc", bz_writer_putc, 1);
    rb_define_method(bz_cWriter, "puts", rb_io_puts, -1);
    rb_define_method(bz_cWriter, "print", rb_io_print, -1);
    rb_define_method(bz_cWriter, "printf", rb_io_printf, -1);
    rb_define_method(bz_cWriter, "<<", rb_io_addstr, 1);
    rb_define_method(bz_cWriter, "flush", bz_writer_flush, 0);
    rb_define_method(bz_cWriter, "close", bz_writer_close, 0);
    rb_define_method(bz_cWriter, "close!", bz_writer_close_bang, 0);
    rb_define_method(bz_cWriter, "closed?", bz_writer_closed, 0);
    rb_define_method(bz_cWriter, "to_io", bz_to_io, 0);
    rb_define_alias(bz_cWriter, "finish", "flush");
    rb_define_alias(bz_cWriter, "closed", "closed?");

    /*
      Reader
    */
    bz_cReader = rb_define_class_under(bz_mBzip2, "Reader", rb_cData);
    rb_include_module(bz_cReader, rb_mEnumerable);
#if HAVE_RB_DEFINE_ALLOC_FUNC
    rb_define_alloc_func(bz_cReader, bz_reader_s_alloc);
#else
    rb_define_singleton_method(bz_cReader, "allocate", bz_reader_s_alloc, 0);
#endif
    rb_define_singleton_method(bz_cReader, "new", bz_s_new, -1);
    rb_define_singleton_method(bz_cReader, "open", bz_reader_s_open, -1);
    rb_define_singleton_method(bz_cReader, "foreach", bz_reader_s_foreach, -1);
    rb_define_singleton_method(bz_cReader, "readlines", bz_reader_s_readlines, -1);
    rb_define_method(bz_cReader, "initialize", bz_reader_init, -1);
    rb_define_method(bz_cReader, "read", bz_reader_read, -1);
    rb_define_method(bz_cReader, "unused", bz_reader_unused, 0);
    rb_define_method(bz_cReader, "unused=", bz_reader_set_unused, 1);
    rb_define_method(bz_cReader, "ungetc", bz_reader_ungetc, 1);
    rb_define_method(bz_cReader, "ungets", bz_reader_ungets, 1);
    rb_define_method(bz_cReader, "getc", bz_reader_getc, 0);
    rb_define_method(bz_cReader, "gets", bz_reader_gets_m, -1);
    rb_define_method(bz_cReader, "readchar", bz_reader_readchar, 0);
    rb_define_method(bz_cReader, "readline", bz_reader_readline, -1);
    rb_define_method(bz_cReader, "readlines", bz_reader_readlines, -1);
    rb_define_method(bz_cReader, "each", bz_reader_each_line, -1);
    rb_define_method(bz_cReader, "each_byte", bz_reader_each_byte, 0);
    rb_define_method(bz_cReader, "close", bz_reader_close, 0);
    rb_define_method(bz_cReader, "close!", bz_reader_close_bang, 0);
    rb_define_method(bz_cReader, "finish", bz_reader_finish, 0);
    rb_define_method(bz_cReader, "closed?", bz_reader_closed, 0);
    rb_define_method(bz_cReader, "eoz?", bz_reader_eoz, 0);
    rb_define_method(bz_cReader, "eof?", bz_reader_eof, 0);
    rb_define_method(bz_cReader, "lineno", bz_reader_lineno, 0);
    rb_define_method(bz_cReader, "lineno=", bz_reader_set_lineno, 1);
    rb_define_method(bz_cReader, "to_io", bz_to_io, 0);
    rb_define_alias(bz_cReader, "each_line", "each");
    rb_define_alias(bz_cReader, "closed", "closed?");
    rb_define_alias(bz_cReader, "eoz", "eoz?");
    rb_define_alias(bz_cReader, "eof", "eof?");

    /*
      Internal
    */
    bz_cInternal = rb_define_class_under(bz_mBzip2, "InternalStr", rb_cData);
#if HAVE_RB_DEFINE_ALLOC_FUNC
    rb_undef_alloc_func(bz_cInternal);
#else
    rb_undef_method(CLASS_OF(bz_cInternal), "allocate");
#endif
    rb_undef_method(CLASS_OF(bz_cInternal), "new");
    rb_undef_method(bz_cInternal, "initialize");
    rb_define_method(bz_cInternal, "read", bz_str_read, -1);
}

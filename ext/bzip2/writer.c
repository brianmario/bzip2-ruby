#include <ruby.h>
#include <unistd.h>
#include "common.h"
#include "writer.h"

struct bz_iv * bz_find_struct(VALUE obj, void *ptr, int *posp) {
    struct bz_iv *bziv;
    int i;

    for (i = 0; i < RARRAY_LEN(bz_internal_ary); i++) {
        Data_Get_Struct(RARRAY_PTR(bz_internal_ary)[i], struct bz_iv, bziv);
        if (ptr) {
#ifndef RUBY_19_COMPATIBILITY
            if (TYPE(bziv->io) == T_FILE && RFILE(bziv->io)->fptr == (OpenFile *)ptr) {
#else
            if (TYPE(bziv->io) == T_FILE && RFILE(bziv->io)->fptr == (rb_io_t *)ptr) {
#endif
                if (posp) {
                    *posp = i;
                }
                return bziv;
            } else if (TYPE(bziv->io) == T_DATA && DATA_PTR(bziv->io) == ptr) {
                    if (posp) *posp = i;
                    return bziv;
            }
        } else if (bziv->io == obj) {
            if (posp) *posp = i;
            return bziv;
        }
    }
    if (posp) *posp = -1;
    return 0;
}

VALUE bz_str_closed(VALUE obj) {
    return Qfalse;
}

void bz_io_data_finalize(void *ptr) {
    struct bz_file *bzf;
    struct bz_iv *bziv;
    int pos;

    bziv = bz_find_struct(0, ptr, &pos);
    if (bziv) {
        rb_ary_delete_at(bz_internal_ary, pos);
        Data_Get_Struct(bziv->bz2, struct bz_file, bzf);
        rb_protect((VALUE (*)(VALUE))bz_writer_internal_flush, (VALUE)bzf, 0);
        RDATA(bziv->bz2)->dfree = free;
        if (bziv->finalize) {
            (*bziv->finalize)(ptr);
        } else if (TYPE(bzf->io) == T_FILE) {
#ifndef RUBY_19_COMPATIBILITY
            OpenFile *file = (OpenFile *)ptr;
            if (file->f) {
                fclose(file->f);
                file->f = 0;
            }
            if (file->f2) {
                fclose(file->f2);
                file->f2 = 0;
            }
#else
            rb_io_t *file = (rb_io_t *)ptr;
            if (file->fd) {
                close(file->fd);

                file->fd = 0;
            }
            if (file->stdio_file) {
                fclose(file->stdio_file);
                file->stdio_file = 0;
            }
#endif
        }
    }

}

int bz_writer_internal_flush(struct bz_file *bzf) {
    int closed = 1;

    if (rb_respond_to(bzf->io, id_closed)) {
        closed = RTEST(rb_funcall2(bzf->io, id_closed, 0, 0));
    }
    if (bzf->buf) {
        if (!closed && bzf->state == BZ_OK) {
            bzf->bzs.next_in = NULL;
            bzf->bzs.avail_in = 0;
            do {
                bzf->bzs.next_out = bzf->buf;
                bzf->bzs.avail_out = bzf->buflen;
                bzf->state = BZ2_bzCompress(&(bzf->bzs), BZ_FINISH);
                if (bzf->state != BZ_FINISH_OK && bzf->state != BZ_STREAM_END) {
                    break;
                }
                if (bzf->bzs.avail_out < bzf->buflen) {
                    rb_funcall(bzf->io, id_write, 1, rb_str_new(bzf->buf, bzf->buflen - bzf->bzs.avail_out));
                }
            } while (bzf->state != BZ_STREAM_END);
        }
        free(bzf->buf);
        bzf->buf = 0;
        BZ2_bzCompressEnd(&(bzf->bzs));
        bzf->state = BZ_OK;
        if (!closed && rb_respond_to(bzf->io, id_flush)) {
            rb_funcall2(bzf->io, id_flush, 0, 0);
        }
    }
    return closed;
}

VALUE bz_writer_internal_close(struct bz_file *bzf) {
    struct bz_iv *bziv;
    int pos, closed;
    VALUE res;

    closed = bz_writer_internal_flush(bzf);
    bziv = bz_find_struct(bzf->io, 0, &pos);
    if (bziv) {
        if (TYPE(bzf->io) == T_FILE) {
            RFILE(bzf->io)->fptr->finalize = bziv->finalize;
        } else if (TYPE(bziv->io) == T_DATA) {
            RDATA(bziv->io)->dfree = bziv->finalize;
        }
        RDATA(bziv->bz2)->dfree = free;
        bziv->bz2 = 0;
        rb_ary_delete_at(bz_internal_ary, pos);
    }
    if (bzf->flags & BZ2_RB_CLOSE) {
        bzf->flags &= ~BZ2_RB_CLOSE;
        if (!closed && rb_respond_to(bzf->io, id_close)) {
            rb_funcall2(bzf->io, id_close, 0, 0);
        }
        res = Qnil;
    } else {
        res = bzf->io;
    }
    bzf->io = Qnil;
    return res;
}

/*
 * Closes this writer for further use. The remaining data is compressed and
 * flushed.
 *
 * If the writer was constructed with an io object, that object is returned.
 * Otherwise, the actual compressed data is returned
 *
 *    writer = Bzip2::Writer.new File.open('path', 'w')
 *    writer << 'a'
 *    writer.close # => #<File:path>
 *
 *    writer = Bzip2::Writer.new
 *    writer << 'a'
 *    writer.close # => "BZh91AY&SY...
 */
VALUE bz_writer_close(VALUE obj) {
    struct bz_file *bzf;
    VALUE res;

    Get_BZ2(obj, bzf);
    res = bz_writer_internal_close(bzf);
    return res;
}

/*
 * Calls Bzip2::Writer#close and then does some more stuff...
 */
VALUE bz_writer_close_bang(VALUE obj) {
    struct bz_file *bzf;
    int closed;

    Get_BZ2(obj, bzf);
    closed = bzf->flags & (BZ2_RB_INTERNAL|BZ2_RB_CLOSE);
    bz_writer_close(obj);
    if (!closed && rb_respond_to(bzf->io, id_close)) {
        if (rb_respond_to(bzf->io, id_closed)) {
            closed = RTEST(rb_funcall2(bzf->io, id_closed, 0, 0));
        }
        if (!closed) {
            rb_funcall2(bzf->io, id_close, 0, 0);
        }
    }
    return Qnil;
}

/*
 * Tests whether this writer is closed
 *
 * @return [Boolean] +true+ if the writer is closed or +false+ otherwise
 */
VALUE bz_writer_closed(VALUE obj) {
  struct bz_file *bzf;

  Data_Get_Struct(obj, struct bz_file, bzf);
  return RTEST(bzf->io)?Qfalse:Qtrue;
}

void bz_writer_free(struct bz_file *bzf) {
    bz_writer_internal_close(bzf);
    free(bzf);
}

/*
 * Internally allocates information about a new writer
 * @private
 */
VALUE bz_writer_s_alloc(VALUE obj) {
    struct bz_file *bzf;
    VALUE res;
    res = Data_Make_Struct(obj, struct bz_file, bz_file_mark, bz_writer_free, bzf);
    bzf->bzs.bzalloc = bz_malloc;
    bzf->bzs.bzfree = bz_free;
    bzf->blocks = DEFAULT_BLOCKS;
    bzf->state = BZ_OK;
    return res;
}

/*
 * Flushes all of the data in this stream to the underlying IO.
 *
 * If this writer was constructed with no underlying io object, the compressed
 * data is returned as a string.
 *
 * @return [String, nil]
 * @raise [IOError] if the stream has been closed
 */
VALUE bz_writer_flush(VALUE obj) {
    struct bz_file *bzf;

    Get_BZ2(obj, bzf);
    if (bzf->flags & BZ2_RB_INTERNAL) {
        return bz_writer_close(obj);
    }
    bz_writer_internal_flush(bzf);
    return Qnil;
}

/*
 * call-seq:
 *   open(filename, mode='wb', &block=nil) -> Bzip2::Writer
 *
 * @param [String] filename the name of the file to write to
 * @param [String] mode a mode string passed to Kernel#open
 * @yieldparam [Bzip2::Writer] writer the Bzip2::Writer instance
 *
 * If a block is given, the created Bzip2::Writer instance is yielded to the
 * block and will be closed when the block completes. It is guaranteed via
 * +ensure+ that the writer is closed
 *
 * If a block is not given, a Bzip2::Writer instance will be returned
 *
 *    Bzip2::Writer.open('file') { |f| f << data }
 *
 *    writer = Bzip2::Writer.open('file')
 *    writer << data
 *    writer.close
 *
 * @return [Bzip2::Writer, nil]
 */
VALUE bz_writer_s_open(int argc, VALUE *argv, VALUE obj) {
    VALUE res;
    struct bz_file *bzf;

    if (argc < 1) {
        rb_raise(rb_eArgError, "invalid number of arguments");
    }
    if (argc == 1) {
        argv[0] = rb_funcall(rb_mKernel, id_open, 2, argv[0],
            rb_str_new2("wb"));
    } else {
        argv[1] = rb_funcall2(rb_mKernel, id_open, 2, argv);
        argv += 1;
        argc -= 1;
    }
    res = rb_funcall2(obj, id_new, argc, argv);
    Data_Get_Struct(res, struct bz_file, bzf);
    bzf->flags |= BZ2_RB_CLOSE;
    if (rb_block_given_p()) {
        return rb_ensure(rb_yield, res, bz_writer_close, res);
    }
    return res;
}

VALUE bz_str_write(VALUE obj, VALUE str) {
    if (TYPE(str) != T_STRING) {
        rb_raise(rb_eArgError, "expected a String");
    }
    if (RSTRING_LEN(str)) {
        rb_str_cat(obj, RSTRING_PTR(str), RSTRING_LEN(str));
    }
    return str;
}

/*
 * call-seq:
 *    initialize(io = nil)
 *
 * @param [File] io the file which to write compressed data to
 *
 * Creates a new Bzip2::Writer for compressing a stream of data. An optional
 * io object (something responding to +write+) can be supplied which data
 * will be written to.
 *
 * If nothing is given, the Bzip2::Writer#flush method can be called to retrieve
 * the compressed stream so far.
 *
 *    writer = Bzip2::Writer.new File.open('files.bz2')
 *    writer << 'a'
 *    writer << 'b'
 *    writer.close
 *
 *    writer = Bzip2::Writer.new
 *    writer << 'abcde'
 *    writer.flush # => 'abcde' compressed
 */
VALUE bz_writer_init(int argc, VALUE *argv, VALUE obj) {
    struct bz_file *bzf;
    int blocks = DEFAULT_BLOCKS;
    int work = 0;
    VALUE a, b, c;

    switch(rb_scan_args(argc, argv, "03", &a, &b, &c)) {
        case 3:
        work = NUM2INT(c);
    /* ... */
        case 2:
        blocks = NUM2INT(b);
    }
    Data_Get_Struct(obj, struct bz_file, bzf);
    if (NIL_P(a)) {
        a = rb_str_new(0, 0);
        rb_define_method(rb_singleton_class(a), "write", bz_str_write, 1);
        rb_define_method(rb_singleton_class(a), "closed?", bz_str_closed, 0);
        bzf->flags |= BZ2_RB_INTERNAL;
    } else {
        VALUE iv;
        struct bz_iv *bziv;
#ifndef RUBY_19_COMPATIBILITY
        OpenFile *fptr;
#else
        rb_io_t *fptr;
#endif

        rb_io_taint_check(a);
        if (!rb_respond_to(a, id_write)) {
            rb_raise(rb_eArgError, "first argument must respond to #write");
        }
        if (TYPE(a) == T_FILE) {
            GetOpenFile(a, fptr);
            rb_io_check_writable(fptr);
        } else if (rb_respond_to(a, id_closed)) {
            iv = rb_funcall2(a, id_closed, 0, 0);
            if (RTEST(iv)) {
                rb_raise(rb_eArgError, "closed object");
            }
        }
        bziv = bz_find_struct(a, 0, 0);
        if (bziv) {
            if (RTEST(bziv->bz2)) {
                rb_raise(rb_eArgError, "invalid data type");
            }
            bziv->bz2 = obj;
        } else {
            iv = Data_Make_Struct(rb_cData, struct bz_iv, 0, free, bziv);
            bziv->io = a;
            bziv->bz2 = obj;
            rb_ary_push(bz_internal_ary, iv);
        }
        switch (TYPE(a)) {
            case T_FILE:
                bziv->finalize = RFILE(a)->fptr->finalize;
                RFILE(a)->fptr->finalize = (void (*)(struct rb_io_t *, int))bz_io_data_finalize;
                break;
            case T_DATA:
                bziv->finalize = RDATA(a)->dfree;
                RDATA(a)->dfree = bz_io_data_finalize;
                break;
        }
    }
    bzf->io = a;
    bzf->blocks = blocks;
    bzf->work = work;
    return obj;
}

/*
 * call-seq:
 *    write(data)
 * Actually writes some data into this stream.
 *
 * @param [String] data the data to write
 * @return [Integer] the length of the data which was written (uncompressed)
 * @raise [IOError] if the stream has been closed
 */
VALUE bz_writer_write(VALUE obj, VALUE a) {
    struct bz_file *bzf;
    int n;

    a = rb_obj_as_string(a);
    Get_BZ2(obj, bzf);
    if (!bzf->buf) {
        if (bzf->state != BZ_OK) {
            bz_raise(bzf->state);
        }
        bzf->state = BZ2_bzCompressInit(&(bzf->bzs), bzf->blocks,
            0, bzf->work);
        if (bzf->state != BZ_OK) {
            bz_writer_internal_flush(bzf);
            bz_raise(bzf->state);
        }
        bzf->buf = ALLOC_N(char, BZ_RB_BLOCKSIZE + 1);
        bzf->buflen = BZ_RB_BLOCKSIZE;
        bzf->buf[0] = bzf->buf[bzf->buflen] = '\0';
    }
    bzf->bzs.next_in  = RSTRING_PTR(a);
    bzf->bzs.avail_in = (int) RSTRING_LEN(a);
    while (bzf->bzs.avail_in) {
        bzf->bzs.next_out = bzf->buf;
        bzf->bzs.avail_out = bzf->buflen;
        bzf->state = BZ2_bzCompress(&(bzf->bzs), BZ_RUN);
        if (bzf->state == BZ_SEQUENCE_ERROR || bzf->state == BZ_PARAM_ERROR) {
            bz_writer_internal_flush(bzf);
            bz_raise(bzf->state);
        }
        bzf->state = BZ_OK;
        if (bzf->bzs.avail_out < bzf->buflen) {
            n = bzf->buflen - bzf->bzs.avail_out;
            rb_funcall(bzf->io, id_write, 1, rb_str_new(bzf->buf, n));
        }
    }
    return INT2NUM(RSTRING_LEN(a));
}

/*
 * call-seq:
 *    putc(num)
 *
 * Write one byte into this stream.
 * @param [Integer] num the number value of the character to write
 * @return [Integer] always 1
 * @raise [IOError] if the stream has been closed
 */
VALUE bz_writer_putc(VALUE obj, VALUE a) {
    char c = NUM2CHR(a);
    return bz_writer_write(obj, rb_str_new(&c, 1));
}

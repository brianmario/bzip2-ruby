#include <bzlib.h>
#include <ruby.h>

#include "reader.h"
#include "common.h"

void bz_str_mark(struct bz_str *bzs) {
    rb_gc_mark(bzs->str);
}

struct bz_file * bz_get_bzf(VALUE obj) {
    struct bz_file *bzf;

    Get_BZ2(obj, bzf);
    if (!bzf->buf) {
        if (bzf->state != BZ_OK) {
            bz_raise(bzf->state);
        }
        bzf->state = BZ2_bzDecompressInit(&(bzf->bzs), 0, bzf->small);
        if (bzf->state != BZ_OK) {
            BZ2_bzDecompressEnd(&(bzf->bzs));
            bz_raise(bzf->state);
        }
        bzf->buf = ALLOC_N(char, BZ_RB_BLOCKSIZE + 1);
        bzf->buflen = BZ_RB_BLOCKSIZE;
        bzf->buf[0] = bzf->buf[bzf->buflen] = '\0';
        bzf->bzs.total_out_hi32 = bzf->bzs.total_out_lo32 = 0;
        bzf->bzs.next_out = bzf->buf;
        bzf->bzs.avail_out = 0;
    }
    if (bzf->state == BZ_STREAM_END && !bzf->bzs.avail_out) {
        return 0;
    }
    return bzf;
}

int bz_next_available(struct bz_file *bzf, int in){
    bzf->bzs.next_out = bzf->buf;
    bzf->bzs.avail_out = 0;
    if (bzf->state == BZ_STREAM_END) {
        return BZ_STREAM_END;
    }
    if (!bzf->bzs.avail_in) {
        bzf->in = rb_funcall(bzf->io, id_read, 1, INT2FIX(1024));
        if (TYPE(bzf->in) != T_STRING || RSTRING_LEN(bzf->in) == 0) {
            BZ2_bzDecompressEnd(&(bzf->bzs));
            bzf->bzs.avail_out = 0;
            bzf->state = BZ_UNEXPECTED_EOF;
            bz_raise(bzf->state);
        }
        bzf->bzs.next_in = RSTRING_PTR(bzf->in);
        bzf->bzs.avail_in = RSTRING_LEN(bzf->in);
    }
    if ((bzf->buflen - in) < (BZ_RB_BLOCKSIZE / 2)) {
        bzf->buf = REALLOC_N(bzf->buf, char, bzf->buflen+BZ_RB_BLOCKSIZE+1);
        bzf->buflen += BZ_RB_BLOCKSIZE;
        bzf->buf[bzf->buflen] = '\0';
    }
    bzf->bzs.avail_out = bzf->buflen - in;
    bzf->bzs.next_out = bzf->buf + in;
    bzf->state = BZ2_bzDecompress(&(bzf->bzs));
    if (bzf->state != BZ_OK) {
        BZ2_bzDecompressEnd(&(bzf->bzs));
        if (bzf->state != BZ_STREAM_END) {
            bzf->bzs.avail_out = 0;
            bz_raise(bzf->state);
        }
    }
    bzf->bzs.avail_out = bzf->buflen - bzf->bzs.avail_out;
    bzf->bzs.next_out = bzf->buf;
    return 0;
}

VALUE bz_read_until(struct bz_file *bzf, const char *str, int len, int *td1) {
    VALUE res;
    int total, i, nex = 0;
    char *p, *t, *tx, *end, *pend = ((char*) str) + len;

    res = rb_str_new(0, 0);
    while (1) {
        total = bzf->bzs.avail_out;
        if (len == 1) {
            tx = memchr(bzf->bzs.next_out, *str, bzf->bzs.avail_out);
            if (tx) {
                i = tx - bzf->bzs.next_out + len;
                res = rb_str_cat(res, bzf->bzs.next_out, i);
                bzf->bzs.next_out += i;
                bzf->bzs.avail_out -= i;
                return res;
            }
        } else {
            tx = bzf->bzs.next_out;
            end = bzf->bzs.next_out + bzf->bzs.avail_out;
            while (tx + len <= end) {
                for (p = (char*) str, t = tx; p != pend; ++p, ++t) {
                    if (*p != *t) break;
                }
                if (p == pend) {
                    i = tx - bzf->bzs.next_out + len;
                    res = rb_str_cat(res, bzf->bzs.next_out, i);
                    bzf->bzs.next_out += i;
                    bzf->bzs.avail_out -= i;
                    return res;
                }
                if (td1) {
                    tx += td1[(int)*(tx + len)];
                } else {
                    tx += 1;
                }
            }
        }
        nex = 0;
        if (total) {
            nex = len - 1;
            res = rb_str_cat(res, bzf->bzs.next_out, total - nex);
            if (nex) {
                MEMMOVE(bzf->buf, bzf->bzs.next_out + total - nex, char, nex);
            }
        }
        if (bz_next_available(bzf, nex) == BZ_STREAM_END) {
            if (nex) {
                res = rb_str_cat(res, bzf->buf, nex);
            }
            if (RSTRING_LEN(res)) {
                return res;
            }
            return Qnil;
        }
    }
    return Qnil;
}

int bz_read_while(struct bz_file *bzf, char c) {
    char *end;

    while (1) {
        end = bzf->bzs.next_out + bzf->bzs.avail_out;
        while (bzf->bzs.next_out < end) {
            if (c != *bzf->bzs.next_out) {
                bzf->bzs.avail_out = end -  bzf->bzs.next_out;
                return *bzf->bzs.next_out;
            }
            ++bzf->bzs.next_out;
        }
        if (bz_next_available(bzf, 0) == BZ_STREAM_END) {
            return EOF;
        }
    }
    return EOF;
}

/*
 * Internally allocates data for a new Reader
 * @private
 */
VALUE bz_reader_s_alloc(VALUE obj) {
    struct bz_file *bzf;
    VALUE res;
    res = Data_Make_Struct(obj, struct bz_file, bz_file_mark, free, bzf);
    bzf->bzs.bzalloc = bz_malloc;
    bzf->bzs.bzfree = bz_free;
    bzf->blocks = DEFAULT_BLOCKS;
    bzf->state = BZ_OK;
    return res;
}

VALUE bz_reader_close __((VALUE));

/*
 * call-seq:
 *   open(filename, &block=nil) -> Bzip2::Reader
 *
 * @param [String] filename the name of the file to read from
 * @yieldparam [Bzip2::Reader] reader the Bzip2::Reader instance
 *
 * If a block is given, the created Bzip2::Reader instance is yielded to the
 * block and will be closed when the block completes. It is guaranteed via
 * +ensure+ that the reader is closed
 *
 * If a block is not given, a Bzip2::Reader instance will be returned
 *
 *    Bzip2::Reader.open('file') { |f| puts f.gets }
 *
 *    reader = Bzip2::Reader.open('file')
 *    puts reader.gets
 *    reader.close
 *
 * @return [Bzip2::Reader, nil]
 */
VALUE bz_reader_s_open(int argc, VALUE *argv, VALUE obj) {
    VALUE res;
    struct bz_file *bzf;

    if (argc < 1) {
        rb_raise(rb_eArgError, "invalid number of arguments");
    }
    argv[0] = rb_funcall2(rb_mKernel, id_open, 1, argv);
    if (NIL_P(argv[0])) {
        return Qnil;
    }
    res = rb_funcall2(obj, id_new, argc, argv);
    Data_Get_Struct(res, struct bz_file, bzf);
    bzf->flags |= BZ2_RB_CLOSE;
    if (rb_block_given_p()) {
        return rb_ensure(rb_yield, res, bz_reader_close, res);
    }
    return res;
}

/*
 * call-seq:
 *    initialize(io)
 *
 * Creates a new stream for reading a bzip file or string
 *
 * @param [File, string, #read] io the source for input data. If the source is
 *    a file or something responding to #read, then data will be read via #read,
 *    otherwise if the input is a string it will be taken as the literal data
 *    to decompress
 */
VALUE bz_reader_init(int argc, VALUE *argv, VALUE obj) {
    struct bz_file *bzf;
    int small = 0;
    VALUE a, b;
    int internal = 0;

    if (rb_scan_args(argc, argv, "11", &a, &b) == 2) {
        small = RTEST(b);
    }
    rb_io_taint_check(a);
    if (OBJ_TAINTED(a)) {
        OBJ_TAINT(obj);
    }
    if (rb_respond_to(a, id_read)) {
        if (TYPE(a) == T_FILE) {
#ifndef RUBY_19_COMPATIBILITY
            OpenFile *fptr;
#else
            rb_io_t *fptr;
#endif

            GetOpenFile(a, fptr);
            rb_io_check_readable(fptr);
        } else if (rb_respond_to(a, id_closed)) {
            VALUE iv = rb_funcall2(a, id_closed, 0, 0);
            if (RTEST(iv)) {
                rb_raise(rb_eArgError, "closed object");
            }
        }
    } else {
        struct bz_str *bzs;
        VALUE res;

        if (!rb_respond_to(a, id_str)) {
            rb_raise(rb_eArgError, "first argument must respond to #read");
        }
        a = rb_funcall2(a, id_str, 0, 0);
        if (TYPE(a) != T_STRING) {
            rb_raise(rb_eArgError, "#to_str must return a String");
        }
        res = Data_Make_Struct(bz_cInternal, struct bz_str,
            bz_str_mark, free, bzs);
        bzs->str = a;
        a = res;
        internal = BZ2_RB_INTERNAL;
    }
    Data_Get_Struct(obj, struct bz_file, bzf);
    bzf->io = a;
    bzf->small = small;
    bzf->flags |= internal;
    return obj;
}

/*
 * call-seq:
 *    read(len = nil)
 *
 * Read decompressed data from the stream.
 *
 *    Bzip2::Reader.new(Bzip2.compress('ab')).read    # => "ab"
 *    Bzip2::Reader.new(Bzip2.compress('ab')).read(1) # => "a"
 *
 * @return [String, nil] the decompressed data read or +nil+ if eoz has been
 *    reached
 * @param [Integer] len the number of decompressed bytes which should be read.
 *    If nothing is specified, the entire stream is read
 */
VALUE bz_reader_read(int argc, VALUE *argv, VALUE obj) {
    struct bz_file *bzf;
    VALUE res, length;
    int total;
    int n;

    rb_scan_args(argc, argv, "01", &length);
    if (NIL_P(length)) {
        n = -1;
    } else {
        n = NUM2INT(length);
        if (n < 0) {
            rb_raise(rb_eArgError, "negative length %d given", n);
        }
    }
    bzf = bz_get_bzf(obj);
    if (!bzf) {
        return Qnil;
    }
    res = rb_str_new(0, 0);
    if (OBJ_TAINTED(obj)) {
        OBJ_TAINT(res);
    }
    if (n == 0) {
        return res;
    }
    while (1) {
        total = bzf->bzs.avail_out;
        if (n != -1 && (RSTRING_LEN(res) + total) >= n) {
            n -= RSTRING_LEN(res);
            res = rb_str_cat(res, bzf->bzs.next_out, n);
            bzf->bzs.next_out += n;
            bzf->bzs.avail_out -= n;
            return res;
        }
        if (total) {
            res = rb_str_cat(res, bzf->bzs.next_out, total);
        }
        if (bz_next_available(bzf, 0) == BZ_STREAM_END) {
            return res;
        }
    }
    return Qnil;
}

int bz_getc(VALUE obj) {
    VALUE length = INT2FIX(1);
    VALUE res = bz_reader_read(1, &length, obj);
    if (NIL_P(res) || RSTRING_LEN(res) == 0) {
        return EOF;
    }
    return RSTRING_PTR(res)[0];
}

/*
 * call-seq:
 *    ungetc(byte)
 *
 * "Ungets" a character/byte. This rewinds the stream by 1 character and inserts
 * the given character into that position. The next read will return the given
 * character as the first one read
 *
 *    reader = Bzip2::Reader.new Bzip2.compress('abc')
 *    reader.getc         # => 97
 *    reader.ungetc 97    # => nil
 *    reader.getc         # => 97
 *    reader.ungetc 42    # => nil
 *    reader.getc         # => 42
 *    reader.getc         # => 98
 *    reader.getc         # => 99
 *    reader.ungetc 100   # => nil
 *    reader.getc         # => 100
 *
 * @param [Integer] byte the byte to 'unget'
 * @return [nil] always
 */
VALUE bz_reader_ungetc(VALUE obj, VALUE a) {
    struct bz_file *bzf;
    int c = NUM2INT(a);

    Get_BZ2(obj, bzf);
    if (!bzf->buf) {
        bz_raise(BZ_SEQUENCE_ERROR);
    }
    if (bzf->bzs.avail_out < bzf->buflen) {
        bzf->bzs.next_out -= 1;
        bzf->bzs.next_out[0] = c;
        bzf->bzs.avail_out += 1;
    } else {
        bzf->buf = REALLOC_N(bzf->buf, char, bzf->buflen + 2);
        bzf->buf[bzf->buflen++] = c;
        bzf->buf[bzf->buflen] = '\0';
        bzf->bzs.next_out = bzf->buf;
        bzf->bzs.avail_out = bzf->buflen;
    }
    return Qnil;
}

/*
 * call-seq:
 *    ungets(str)
 *
 * Equivalently "unget" a string. When called on a string that was just read
 * from the stream, this inserts the string back into the stream to br read
 * again.
 *
 * When called with a string which hasn't been read from the stream, it does
 * the same thing, and the next read line/data will start from the beginning
 * of the given data and the continue on with the rest of the stream.
 *
 *    reader = Bzip2::Reader.new Bzip2.compress("a\nb")
 *    reader.gets           # => "a\n"
 *    reader.ungets "a\n"   # => nil
 *    reader.gets           # => "a\n"
 *    reader.ungets "foo"   # => nil
 *    reader.gets           # => "foob"
 *
 * @param [String] str the string to insert back into the stream
 * @return [nil] always
 */
VALUE bz_reader_ungets(VALUE obj, VALUE a) {
    struct bz_file *bzf;

    Check_Type(a, T_STRING);
    Get_BZ2(obj, bzf);
    if (!bzf->buf) {
        bz_raise(BZ_SEQUENCE_ERROR);
    }
    if ((bzf->bzs.avail_out + RSTRING_LEN(a)) < bzf->buflen) {
        bzf->bzs.next_out -= RSTRING_LEN(a);
        MEMCPY(bzf->bzs.next_out, RSTRING_PTR(a), char, RSTRING_LEN(a));
        bzf->bzs.avail_out += RSTRING_LEN(a);
    } else {
        bzf->buf = REALLOC_N(bzf->buf, char, bzf->buflen + RSTRING_LEN(a) + 1);
        MEMCPY(bzf->buf + bzf->buflen, RSTRING_PTR(a), char,RSTRING_LEN(a));
        bzf->buflen += RSTRING_LEN(a);
        bzf->buf[bzf->buflen] = '\0';
        bzf->bzs.next_out = bzf->buf;
        bzf->bzs.avail_out = bzf->buflen;
    }
    return Qnil;
}

VALUE bz_reader_gets(VALUE obj) {
    struct bz_file *bzf;
    VALUE str = Qnil;

    bzf = bz_get_bzf(obj);
    if (bzf) {
        str = bz_read_until(bzf, "\n", 1, 0);
        if (!NIL_P(str)) {
            bzf->lineno++;
            OBJ_TAINT(str);
        }
    }
    return str;
}

VALUE bz_reader_gets_internal(int argc, VALUE *argv, VALUE obj, int *td, int init) {
    struct bz_file *bzf;
    VALUE rs, res;
    const char *rsptr;
    int rslen, rspara, *td1;

    rs = rb_rs;
    if (argc) {
        rb_scan_args(argc, argv, "1", &rs);
        if (!NIL_P(rs)) {
            Check_Type(rs, T_STRING);
        }
    }
    if (NIL_P(rs)) {
        return bz_reader_read(1, &rs, obj);
    }
    rslen = RSTRING_LEN(rs);
    if (rs == rb_default_rs || (rslen == 1 && RSTRING_PTR(rs)[0] == '\n')) {
        return bz_reader_gets(obj);
    }

    if (rslen == 0) {
        rsptr = "\n\n";
        rslen = 2;
        rspara = 1;
    } else {
        rsptr = RSTRING_PTR(rs);
        rspara = 0;
    }

    bzf = bz_get_bzf(obj);
    if (!bzf) {
        return Qnil;
    }
    if (rspara) {
        bz_read_while(bzf, '\n');
    }
    td1 = 0;
    if (rslen != 1) {
        if (init) {
            int i;

            for (i = 0; i < ASIZE; i++) {
                td[i] = rslen + 1;
            }
            for (i = 0; i < rslen; i++) {
                td[(int)*(rsptr + i)] = rslen - i;
            }
        }
        td1 = td;
    }

    res = bz_read_until(bzf, rsptr, rslen, td1);
    if (rspara) {
        bz_read_while(bzf, '\n');
    }

    if (!NIL_P(res)) {
        bzf->lineno++;
        OBJ_TAINT(res);
    }
    return res;
}

/*
 * Specs were missing for this method originally and playing around with it
 * gave some very odd results, so unless you know what you're doing, I wouldn't
 * mess around with this...
 */
VALUE bz_reader_set_unused(VALUE obj, VALUE a) {
    struct bz_file *bzf;

    Check_Type(a, T_STRING);
    Get_BZ2(obj, bzf);
    if (!bzf->in) {
        bzf->in = rb_str_new(RSTRING_PTR(a), RSTRING_LEN(a));
    } else {
        bzf->in = rb_str_cat(bzf->in, RSTRING_PTR(a), RSTRING_LEN(a));
    }
    bzf->bzs.next_in = RSTRING_PTR(bzf->in);
    bzf->bzs.avail_in = RSTRING_LEN(bzf->in);
    return Qnil;
}

/*
 * Reads one character from the stream, returning the byte read.
 *
 *    reader = Bzip2::Reader.new Bzip2.compress('ab')
 *    reader.getc # => 97
 *    reader.getc # => 98
 *    reader.getc # => nil
 *
 * @return [Integer, nil] the byte value of the character read or +nil+ if eoz
 *    has been reached
 */
VALUE bz_reader_getc(VALUE obj) {
    VALUE str;
    VALUE len = INT2FIX(1);

    str = bz_reader_read(1, &len, obj);
    if (NIL_P(str) || RSTRING_LEN(str) == 0) {
        return Qnil;
    }
    return INT2FIX(RSTRING_PTR(str)[0] & 0xff);
}

void bz_eoz_error() {
    rb_raise(bz_eEOZError, "End of Zip component reached");
}

/*
 * Performs the same as Bzip2::Reader#getc except Bzip2::EOZError is raised if
 * eoz has been readhed
 *
 * @raise [Bzip2::EOZError] if eoz has been reached
 */
VALUE bz_reader_readchar(VALUE obj) {
    VALUE res = bz_reader_getc(obj);

    if (NIL_P(res)) {
        bz_eoz_error();
    }
    return res;
}

/*
 * call-seq:
 *    gets(sep = "\n")
 *
 * Reads a line from the stream until the separator is reached. This does not
 * throw an exception, but rather returns nil if an eoz/eof error occurs
 *
 *    reader = Bzip2::Reader.new Bzip2.compress("a\nb")
 *    reader.gets # => "a\n"
 *    reader.gets # => "b"
 *    reader.gets # => nil
 *
 * @return [String, nil] the read data or nil if eoz has been reached
 * @see Bzip2::Reader#readline
 */
VALUE bz_reader_gets_m(int argc, VALUE *argv, VALUE obj) {
    int td[ASIZE];
    VALUE str = bz_reader_gets_internal(argc, argv, obj, td, Qtrue);

    if (!NIL_P(str)) {
        rb_lastline_set(str);
    }
    return str;
}

/*
 * call-seq:
 *    readline(sep = "\n")
 *
 * Reads one line from the stream and returns it (including the separator)
 *
 *    reader = Bzip2::Reader.new Bzip2.compress("a\nb")
 *    reader.readline # => "a\n"
 *    reader.readline # => "b"
 *    reader.readline # => raises Bzip2::EOZError
 *
 *
 * @param [String] sep the newline separator character
 * @return [String] the read line
 * @see Bzip2::Reader.readlines
 * @raise [Bzip2::EOZError] if the stream has reached its end
 */
VALUE bz_reader_readline(int argc, VALUE *argv, VALUE obj) {
    VALUE res = bz_reader_gets_m(argc, argv, obj);

    if (NIL_P(res)) {
        bz_eoz_error();
    }
    return res;
}

/*
 * call-seq:
 *    readlines(sep = "\n")
 *
 * Reads the lines of the files and returns the result as an array.
 *
 * If the stream has reached eoz, then an empty array is returned
 *
 * @param [String] sep the newline separator character
 * @return [Array] an array of lines read
 * @see Bzip2::Reader.readlines
 */
VALUE bz_reader_readlines(int argc, VALUE *argv, VALUE obj) {
    VALUE line, ary;
    int td[ASIZE], in;

    in = Qtrue;
    ary = rb_ary_new();
    while (!NIL_P(line = bz_reader_gets_internal(argc, argv, obj, td, in))) {
        in = Qfalse;
        rb_ary_push(ary, line);
    }
    return ary;
}

/*
 * call-seq:
 *    each(sep = "\n", &block)
 *
 * Iterates over the lines of the stream.
 *
 * @param [String] sep the byte which separates lines
 * @yieldparam [String] line the next line of the file (including the separator
 *    character)
 * @see Bzip2::Reader.foreach
 */
VALUE bz_reader_each_line(int argc, VALUE *argv, VALUE obj) {
    VALUE line;
    int td[ASIZE], in;

    in = Qtrue;
    while (!NIL_P(line = bz_reader_gets_internal(argc, argv, obj, td, in))) {
        in = Qfalse;
        rb_yield(line);
    }
    return obj;
}

/*
 * call-seq:
 *    each_byte(&block)
 *
 * Iterates over the decompressed bytes of the file.
 *
 *    Bzip2::Writer.open('file'){ |f| f << 'asdf' }
 *    reader = Bzip2::Reader.new File.open('file')
 *    reader.each_byte{ |b| puts "#{b} #{b.chr}" }
 *
 *    # Output:
 *    # 97 a
 *    # 115 s
 *    # 100 d
 *    # 102 f
 *
 * @yieldparam [Integer] byte the decompressed bytes of the file
 */
VALUE bz_reader_each_byte(VALUE obj) {
    int c;

    while ((c = bz_getc(obj)) != EOF) {
        rb_yield(INT2FIX(c & 0xff));
    }
    return obj;
}

/*
 * Specs were missing for this method originally and playing around with it
 * gave some very odd results, so unless you know what you're doing, I wouldn't
 * mess around with this...
 */
VALUE bz_reader_unused(VALUE obj) {
    struct bz_file *bzf;
    VALUE res;

    Get_BZ2(obj, bzf);
    if (!bzf->in || bzf->state != BZ_STREAM_END) {
        return Qnil;
    }
    if (bzf->bzs.avail_in) {
        res = rb_tainted_str_new(bzf->bzs.next_in, bzf->bzs.avail_in);
        bzf->bzs.avail_in = 0;
    } else {
        res = rb_tainted_str_new(0, 0);
    }
    return res;
}

/*
 * Test whether the end of the bzip stream has been reached
 *
 * @return [Boolean] +true+ if the reader is at the end of the bz stream or
 *                   +false+ otherwise
 */
VALUE bz_reader_eoz(VALUE obj) {
    struct bz_file *bzf;

    Get_BZ2(obj, bzf);
    if (!bzf->in || !bzf->buf) {
        return Qnil;
    }
    if (bzf->state == BZ_STREAM_END && !bzf->bzs.avail_out) {
        return Qtrue;
    }
    return Qfalse;
}

/*
 * Test whether the bzip stream has reached its end (see Bzip2::Reader#eoz?)
 * and then tests that the undlerying IO has also reached an eof
 *
 * @return [Boolean] +true+ if the stream has reached or +false+ otherwise.
 */
VALUE bz_reader_eof(VALUE obj) {
    struct bz_file *bzf;
    VALUE res;

    res = bz_reader_eoz(obj);
    if (RTEST(res)) {
        Get_BZ2(obj, bzf);
        if (bzf->bzs.avail_in) {
            res = Qfalse;
        } else {
            res = bz_reader_getc(obj);
            if (NIL_P(res)) {
                res = Qtrue;
            } else {
                bz_reader_ungetc(obj, res);
                res = Qfalse;
            }
        }
    }
    return res;
}

/*
 * Tests whether this reader has be closed.
 *
 * @return [Boolean] +true+ if it is or +false+ otherwise.
 */
VALUE bz_reader_closed(VALUE obj) {
    struct bz_file *bzf;

    Data_Get_Struct(obj, struct bz_file, bzf);
    return RTEST(bzf->io)?Qfalse:Qtrue;
}

/*
 * Closes this reader to disallow further reads.
 *
 *    reader = Bzip2::Reader.new File.open('file')
 *    reader.close
 *
 *    reader.closed? # => true
 *
 * @return [File] the io with which the reader was created.
 * @raise [IOError] if the stream has already been closed
 */
VALUE bz_reader_close(VALUE obj) {
    struct bz_file *bzf;
    VALUE res;

    Get_BZ2(obj, bzf);
    if (bzf->buf) {
        free(bzf->buf);
        bzf->buf = 0;
    }
    if (bzf->state == BZ_OK) {
        BZ2_bzDecompressEnd(&(bzf->bzs));
    }
    if (bzf->flags & BZ2_RB_CLOSE) {
        int closed = 0;
        if (rb_respond_to(bzf->io, id_closed)) {
            VALUE iv = rb_funcall2(bzf->io, id_closed, 0, 0);
            closed = RTEST(iv);
        }
        if (!closed && rb_respond_to(bzf->io, id_close)) {
            rb_funcall2(bzf->io, id_close, 0, 0);
        }
    }
    if (bzf->flags & (BZ2_RB_CLOSE|BZ2_RB_INTERNAL)) {
        res = Qnil;
    } else {
        res = bzf->io;
    }
    bzf->io = 0;
    return res;
}

/*
 * Originally undocument and had no sepcs. Appears to call Bzip2::Reader#read
 * and then mark the stream as finished, but this didn't work for me...
 */
VALUE bz_reader_finish(VALUE obj) {
    struct bz_file *bzf;

    Get_BZ2(obj, bzf);
    if (bzf->buf) {
        rb_funcall2(obj, id_read, 0, 0);
        free(bzf->buf);
    }
    bzf->buf = 0;
    bzf->state = BZ_OK;
    return Qnil;
}

/*
 * Originally undocument and had no sepcs. Appears to work nearly the same
 * as Bzip2::Reader#close...
 */
VALUE bz_reader_close_bang(VALUE obj) {
    struct bz_file *bzf;
    int closed;

    Get_BZ2(obj, bzf);
    closed = bzf->flags & (BZ2_RB_CLOSE|BZ2_RB_INTERNAL);
    bz_reader_close(obj);
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

struct foreach_arg {
    int argc;
    VALUE sep;
    VALUE obj;
};

VALUE bz_reader_foreach_line(struct foreach_arg *arg) {
    VALUE str;
    int td[ASIZE], in;

    in = Qtrue;
    while (!NIL_P(str = bz_reader_gets_internal(arg->argc, &arg->sep, arg->obj, td, in))) {
        in = Qfalse;
        rb_yield(str);
    }
    return Qnil;
}

/*
 * call-seq:
 *    foreach(filename, &block)
 *
 * Reads a bz2 compressed file and yields each line to the block
 *
 *    Bzip2::Writer.open('file'){ |f| f << "a\n" << "b\n" << "c\n\nd" }
 *    Bzip2::Reader.foreach('file'){ |l| p l }
 *
 *    # Output:
 *    # "a\n"
 *    # "b\n"
 *    # "c\n"
 *    # "\n"
 *    # "d"
 *
 * @param [String] filename the path to the file to open
 * @yieldparam [String] each line of the file
 */
VALUE bz_reader_s_foreach(int argc, VALUE *argv, VALUE obj) {
    VALUE fname, sep;
    struct foreach_arg arg;
    struct bz_file *bzf;

    if (!rb_block_given_p()) {
        rb_raise(rb_eArgError, "call out of a block");
    }
    rb_scan_args(argc, argv, "11", &fname, &sep);
#ifdef SafeStringValue
    SafeStringValue(fname);
#else
    Check_SafeStr(fname);
#endif
    arg.argc = argc - 1;
    arg.sep = sep;
    arg.obj = rb_funcall2(rb_mKernel, id_open, 1, &fname);
    if (NIL_P(arg.obj)) {
        return Qnil;
    }
    arg.obj = rb_funcall2(obj, id_new, 1, &arg.obj);
    Data_Get_Struct(arg.obj, struct bz_file, bzf);
    bzf->flags |= BZ2_RB_CLOSE;
    return rb_ensure(bz_reader_foreach_line, (VALUE)&arg, bz_reader_close, arg.obj);
}

VALUE bz_reader_i_readlines(struct foreach_arg *arg) {
    VALUE str, res;
    int td[ASIZE], in;

    in = Qtrue;
    res = rb_ary_new();
    while (!NIL_P(str = bz_reader_gets_internal(arg->argc, &arg->sep, arg->obj, td, in))) {
        in = Qfalse;
        rb_ary_push(res, str);
    }
    return res;
}

/*
 * call-seq:
 *    readlines(filename, separator="\n")
 *
 * Opens the given bz2 compressed file for reading and decompresses the file,
 * returning an array of the lines of the file. A line is denoted by the
 * separator argument.
 *
 *    Bzip2::Writer.open('file'){ |f| f << "a\n" << "b\n" << "c\n\nd" }
 *
 *    Bzip2::Reader.readlines('file')      # => ["a\n", "b\n", "c\n", "\n", "d"]
 *    Bzip2::Reader.readlines('file', 'c') # => ["a\nb\nc", "\n\nd"]
 *
 * @param [String] filename the path to the file to read
 * @param [String] separator the character to denote a newline in the file
 * @see Bzip2::Reader#readlines
 * @return [Array] an array of lines for the file
 * @raise [Bzip2::Error] if the file is not a valid bz2 compressed file
 */
VALUE bz_reader_s_readlines(int argc, VALUE *argv, VALUE obj) {
    VALUE fname, sep;
    struct foreach_arg arg;
    struct bz_file *bzf;

    rb_scan_args(argc, argv, "11", &fname, &sep);
#ifdef SafeStringValue
    SafeStringValue(fname);
#else
    Check_SafeStr(fname);
#endif
    arg.argc = argc - 1;
    arg.sep = sep;
    arg.obj = rb_funcall2(rb_mKernel, id_open, 1, &fname);
    if (NIL_P(arg.obj)) {
        return Qnil;
    }
    arg.obj = rb_funcall2(obj, id_new, 1, &arg.obj);
    Data_Get_Struct(arg.obj, struct bz_file, bzf);
    bzf->flags |= BZ2_RB_CLOSE;
    return rb_ensure(bz_reader_i_readlines, (VALUE)&arg, bz_reader_close, arg.obj);
}

/*
 * Returns the current line number that the stream is at. This number is based
 * on the newline separator being "\n"
 *
 *    reader = Bzip2::Reader.new Bzip2.compress("a\nb")
 *    reader.lineno     # => 0
 *    reader.readline   # => "a\n"
 *    reader.lineno     # => 1
 *    reader.readline   # => "b"
 *    reader.lineno     # => 2

 * @return [Integer] the current line number
 */
VALUE bz_reader_lineno(VALUE obj) {
    struct bz_file *bzf;

    Get_BZ2(obj, bzf);
    return INT2NUM(bzf->lineno);
}

/*
 * call-seq:
 *    lineno=(num)
 *
 * Sets the internal line number count that this stream should be set at
 *
 *    reader = Bzip2::Reader.new Bzip2.compress("a\nb")
 *    reader.lineno     # => 0
 *    reader.readline   # => "a\n"
 *    reader.lineno     # => 1
 *    reader.lineno = 0
 *    reader.readline   # => "b"
 *    reader.lineno     # => 1
 *
 * @note This does not actually rewind or move the stream forward
 * @param [Integer] lineno the line number which the stream should consider
 *    being set at
 * @return [Integer] the line number provided
 */
VALUE bz_reader_set_lineno(VALUE obj, VALUE lineno) {
    struct bz_file *bzf;

    Get_BZ2(obj, bzf);
    bzf->lineno = NUM2INT(lineno);
    return lineno;
}

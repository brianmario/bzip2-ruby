#include <ruby.h>
#include <rubyio.h>
#include <bzlib.h>

static VALUE bz_cWriter, bz_cReader, bz_cInternal;
static VALUE bz_eError, bz_eConfigError;
static VALUE bz_eEOZError, bz_eSequenceError, bz_eParamError;
static VALUE bz_eMemError, bz_eDataError, bz_eDataMagicError;
static VALUE bz_eIOError, bz_eFullError;

#define BZ2_RB_CLOSE    1
#define BZ2_RB_INTERNAL 2

struct bz_file {
    bz_stream bzs;
    VALUE buf, in;
    VALUE io;
    int blocks, work, small;
    int flags, state;
};

struct bz_str {
    VALUE str;
    int pos;
};

struct OpenFileBZ2 {
    struct OpenFile orig;
    void (*finalize)();
    VALUE bz2;
};

#define Get_BZ2(obj, bzf)			\
    Data_Get_Struct(obj, struct bz_file, bzf);	\
    if (!RTEST(bzf->io)) {			\
	rb_raise(bz_eIOError, "closed BZ2");	\
    }

static VALUE
bz_raise(error)
    int error;
{
    VALUE exc;
    char *msg;

    switch (error) {
    case BZ_SEQUENCE_ERROR:
	exc = bz_eSequenceError;
	msg = "uncorrect sequence"; 
	break;
    case BZ_PARAM_ERROR: 
	exc = bz_eParamError;
	msg = "out of range";
	break;
    case BZ_MEM_ERROR: 
	exc = bz_eMemError;
	msg = "not enough memory is available"; 
	break;
    case BZ_DATA_ERROR:
	exc =  bz_eDataError;
	msg = "data integrity error is detected";
	break;
    case BZ_DATA_ERROR_MAGIC:
	exc = bz_eDataMagicError; 
	msg = "compressed stream does not start with the correct magic bytes";
	break;
    case BZ_IO_ERROR: 
	exc = bz_eIOError; 
	msg = "error reading or writing"; 
	break;
    case BZ_UNEXPECTED_EOF: 
	exc = bz_eEOZError;
	msg = "compressed file finishes before the logical end of stream is detected";
	break;
    case BZ_OUTBUFF_FULL:
	exc = bz_eFullError;
	msg = "output buffer full";
	break;
    case BZ_CONFIG_ERROR:
	exc = bz_eConfigError;
	msg = "library has been improperly compiled on your platform";
	break;
    default:
	msg = "unknown error";
	exc = bz_eError;
    }
    rb_raise(exc, msg);
}
    
static VALUE
bz_str_mark(bzs)
    struct bz_str *bzs;
{
    rb_gc_mark(bzs->str);
}

static void
bz_file_mark(bzf)
    struct bz_file *bzf;
{
    rb_gc_mark(bzf->io);
    rb_gc_mark(bzf->buf);
    rb_gc_mark(bzf->in);
}

static void
bz_internal_flush(bzf)
    struct bz_file *bzf;
{
    int n, state;

    if (bzf->buf) {
	bzf->bzs.next_in = NULL;
	bzf->bzs.avail_in = 0;
	do {
	    bzf->bzs.next_out = RSTRING(bzf->buf)->ptr;
	    bzf->bzs.avail_out = RSTRING(bzf->buf)->len;
	    state = BZ2_bzCompress(&(bzf->bzs), BZ_FINISH);
	    if (bzf->bzs.avail_out < RSTRING(bzf->buf)->len) {
		n = RSTRING(bzf->buf)->len - bzf->bzs.avail_out;
		rb_funcall(bzf->io, rb_intern("write"), 1, 
			   rb_str_new(RSTRING(bzf->buf)->ptr, n));
	    }
	} while (state != BZ_STREAM_END);
	if (rb_respond_to(bzf->io, rb_intern("flush"))) {
	    rb_funcall2(bzf->io, rb_intern("flush"), 0, 0);
	}
	BZ2_bzCompressEnd(&(bzf->bzs));
    }
    bzf->buf = 0;
}

static void
bz_writer_internal_close(bzf)
    struct bz_file *bzf;
{
    bz_internal_flush(bzf);
    if (bzf->flags & BZ2_RB_CLOSE) {
	if (rb_respond_to(bzf->io, rb_intern("close"))) {
	    rb_funcall2(bzf->io, rb_intern("close"), 0, 0);
	}
	bzf->flags &= ~BZ2_RB_CLOSE;
    }
    else if (TYPE(bzf->io) == T_FILE) {
	struct OpenFileBZ2 *bz2 = (struct OpenFileBZ2 *)RFILE(bzf->io)->fptr;
	bz2->orig.finalize = bz2->finalize;
    }
    bzf->io = Qnil;
}

static VALUE
bz_writer_close(obj)
    VALUE obj;
{
    struct bz_file *bzf;
    int internal;
    VALUE res;

    Get_BZ2(obj, bzf);
    internal = bzf->flags & BZ2_RB_INTERNAL;
    if (internal) {
	res = bzf->io;
    }
    bz_writer_internal_close(bzf);
    if (internal) {
	return res;
    }
    return Qnil;
}

static void
bz_writer_free(bzf)
    struct bz_file *bzf;
{
    bz_writer_internal_close(bzf);
    ruby_xfree(bzf);
}

static void
bz_file_finalize(bz2)
    struct OpenFileBZ2 *bz2;
{
    struct bz_file *bzf;

    Get_BZ2(bz2->bz2, bzf);
    bz_internal_flush(bzf);
    ruby_xfree(bzf);
    RDATA(bz2->bz2)->dfree = 0;
    if (bz2->finalize) {
	(*bz2->finalize)(bz2->orig);
    }
    else {
	if (bz2->orig.f) {
	    fclose(bz2->orig.f);
	}
	if (bz2->orig.f2) {
	    fclose(bz2->orig.f2);
	}
    }
    bz2->orig.finalize = bz2->finalize = 0;
}

static void *
bz_malloc(opaque, m, n)
    void *opaque;
    int m, n;
{
    return ruby_xmalloc(m * n);
}

static void
bz_free(opaque, p)
    void *opaque, *p;
{
    ruby_xfree(p);
}

#define DEFAULT_BLOCKS 9

static VALUE
bz_writer_s_alloc(obj)
    VALUE obj;
{
    struct bz_file *bzf;
    VALUE res;
    res = Data_Make_Struct(obj, struct bz_file, bz_file_mark, 
			   bz_writer_free, bzf);
    bzf->bzs.bzalloc = bz_malloc;
    bzf->bzs.bzfree = bz_free;
    bzf->blocks = DEFAULT_BLOCKS;
    return res;
}

static VALUE
bz_writer_flush(obj)
    VALUE obj;
{
    struct bz_file *bzf;

    Get_BZ2(obj, bzf);
    if (bzf->flags & BZ2_RB_INTERNAL) {
	return bz_writer_close(obj);
    }
    bz_internal_flush(bzf);
    return Qnil;
}

static VALUE
bz_writer_s_new(argc, argv, obj)
    int argc;
    VALUE obj, *argv;
{
    VALUE res = rb_funcall2(obj, rb_intern("allocate"), 0, 0);
    rb_obj_call_init(res, argc, argv);
    if (rb_block_given_p()) {
	rb_ensure(rb_yield, res, bz_writer_close, res);
	return Qnil;
    }
    return res;
}

static VALUE
bz_writer_s_open(argc, argv, obj)
    int argc;
    VALUE obj, *argv;
{
    VALUE res;
    struct bz_file *bzf;

    if (argc < 1) {
	rb_raise(rb_eArgError, "invalid number of arguments");
    }
    if (argc == 1) {
	argv[0] = rb_funcall(rb_mKernel, rb_intern("open"), 2, argv[0], 
			     rb_str_new2("wb"));
    }
    else {
	argv[1] = rb_funcall2(rb_mKernel, rb_intern("open"), 2, argv);
	argv += 1;
	argc -= 1;
    }
    res = rb_funcall2(obj, rb_intern("allocate"), 0, 0);
    Data_Get_Struct(res, struct bz_file, bzf);
    bzf->flags |= BZ2_RB_CLOSE;
    rb_obj_call_init(res, argc, argv);
    if (rb_block_given_p()) {
	rb_ensure(rb_yield, res, bz_writer_close, res);
	return Qnil;
    }
    return res;
}

static VALUE
bz_str_write(obj, str)
    VALUE obj, str;
{
    if (TYPE(str) != T_STRING) {
	rb_raise(rb_eArgError, "expected a String");
    }
    if (RSTRING(str)->len) {
	rb_str_cat(obj, RSTRING(str)->ptr, RSTRING(str)->len);
    }
    return str;
}

static VALUE
bz_writer_init(argc, argv, obj)
    int argc;
    VALUE obj, *argv;
{
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
	bzf->flags |= BZ2_RB_INTERNAL;
    }
    if (!rb_respond_to(a, rb_intern("write"))) {
	rb_raise(rb_eArgError, "first argument must respond to #write");
    }
    if (!(bzf->flags & BZ2_RB_CLOSE) && TYPE(a) == T_FILE) {
	OpenFile *fp;
	struct OpenFileBZ2 *bz2;

	GetOpenFile(a, fp);
	bz2 = ALLOC(struct OpenFileBZ2);
	MEMCPY(&(bz2->orig), RFILE(a)->fptr, OpenFile, 1);
	bz2->finalize = RFILE(a)->fptr->finalize;
	bz2->bz2 = obj;
	bz2->orig.finalize = bz_file_finalize;
	ruby_xfree(RFILE(a)->fptr);
	RFILE(a)->fptr = (OpenFile *)bz2;
    }
    bzf->io = a;
    bzf->blocks = blocks;
    bzf->work = work;
    return obj;
}

static VALUE
bz_writer_write(obj, a)
    VALUE obj, a;
{
    struct bz_file *bzf;
    int n, status;

    a = rb_obj_as_string(a);
    Get_BZ2(obj, bzf);
    if (!bzf->buf) {
	status = BZ2_bzCompressInit(&(bzf->bzs), bzf->blocks, 0, bzf->work);
	if (status != BZ_OK) {
	    bz_raise(status);
	}
	bzf->buf = rb_str_new(0, 4096);
    }
    bzf->bzs.next_in = RSTRING(a)->ptr;
    bzf->bzs.avail_in = RSTRING(a)->len;
    while (bzf->bzs.avail_in) {
	bzf->bzs.next_out = RSTRING(bzf->buf)->ptr;
	bzf->bzs.avail_out = RSTRING(bzf->buf)->len;
	status = BZ2_bzCompress(&(bzf->bzs), BZ_RUN);
	if (status == BZ_SEQUENCE_ERROR || status == BZ_PARAM_ERROR) {
	    bz_raise(status);
	}
	if (bzf->bzs.avail_out < RSTRING(bzf->buf)->len) {
	    n = RSTRING(bzf->buf)->len - bzf->bzs.avail_out;
	    rb_funcall(bzf->io, rb_intern("write"), 1, 
		       rb_str_new2(RSTRING(bzf->buf)->ptr), n);
	}
    }
    return INT2NUM(RSTRING(a)->len);
}

static VALUE
bz_writer_putc(obj, a)
    VALUE obj, a;
{
    char c = NUM2CHR(a);
    return bz_writer_write(obj, rb_str_new(&c, 1));
}

static VALUE
bz_compress(argc, argv, obj)
    int argc;
    VALUE obj, *argv;
{
    VALUE bz2, str;

    if (!argc) {
	rb_raise(rb_eArgError, "need a String to compress");
    }
    Check_Type(argv[0], T_STRING);
    str = argv[0];
    argv[0] = Qnil;
    bz2 = rb_funcall2(bz_cWriter, rb_intern("new"), argc, argv);
    bz_writer_write(bz2, str);
    return bz_writer_close(bz2);
}

static VALUE
bz_reader_s_alloc(obj)
    VALUE obj;
{
    return bz_writer_s_alloc(obj);
}

static VALUE
bz_reader_s_new(argc, argv, obj)
    int argc;
    VALUE obj, *argv;
{
    VALUE res = rb_funcall2(obj, rb_intern("allocate"), 0, 0);
    rb_obj_call_init(res, argc, argv);
    return res;
}

static VALUE bz_reader_close __((VALUE));

static VALUE
bz_reader_s_open(argc, argv, obj)
    int argc;
    VALUE obj, *argv;
{
    VALUE res;
    struct bz_file *bzf;

    if (argc < 1) {
	rb_raise(rb_eArgError, "invalid number of arguments");
    }
    argv[0] = rb_funcall(rb_mKernel, rb_intern("open"), 1, argv[0]);
    if (NIL_P(argv[0])) return Qnil;
    res = rb_funcall2(obj, rb_intern("new"), argc, argv);
    Data_Get_Struct(res, struct bz_file, bzf);
    bzf->flags |= BZ2_RB_CLOSE;
    if (rb_block_given_p()) {
	rb_ensure(rb_yield, res, bz_reader_close, res);
	return Qnil;
    }
    return res;
}

static VALUE
bz_reader_init(argc, argv, obj)
    int argc;
    VALUE obj, *argv;
{
    struct bz_file *bzf;
    int small = 0;
    VALUE a, b;
    int internal = 0;

    if (rb_scan_args(argc, argv, "11", &a, &b) == 2) {
	small = RTEST(b);
    }
    if (TYPE(a) == T_STRING) {
	struct bz_str *bzs;

	VALUE res = Data_Make_Struct(bz_cInternal, struct bz_str, 
				     bz_str_mark, ruby_xfree, bzs);
	bzs->str = a;
	a = res;
	internal = BZ2_RB_INTERNAL;
    }
    if (!rb_respond_to(a, rb_intern("read"))) {
	rb_raise(rb_eArgError, "first argument must respond to #read");
    }
    Data_Get_Struct(obj, struct bz_file, bzf);
    bzf->io = a;
    bzf->small = small;
    bzf->flags |= internal;
    return obj;
}

static VALUE
bz_reader_read(argc, argv, obj)
    int argc;
    VALUE obj, *argv;
{
    struct bz_file *bzf;
    VALUE res, length;
    int total;
    int n;

    rb_scan_args(argc, argv, "01", &length);
    if (NIL_P(length)) {
	n = -1;
    }
    else {
	n = NUM2INT(length);
	if (n < 0) {
	    rb_raise(rb_eArgError, "negative length %d given", n);
	}
    }
    Get_BZ2(obj, bzf);
    if (!bzf->buf) {
	int status = BZ2_bzDecompressInit(&(bzf->bzs), 0, bzf->small);
	if (status != BZ_OK) {
	    bz_raise(status);
	}
	bzf->state = 0;
	bzf->buf = rb_str_new(0, 4096);
	bzf->bzs.total_out_hi32 = bzf->bzs.total_out_lo32 = 0;
	bzf->bzs.next_out = RSTRING(bzf->buf)->ptr;
	bzf->bzs.avail_out = RSTRING(bzf->buf)->len;
    }
    if (bzf->state == BZ_STREAM_END && 
	bzf->bzs.avail_out == RSTRING(bzf->buf)->len) {
	return Qnil;
    }
    res = rb_str_new(0, 0);
    if (n == 0) {
	return res;
    }
    while (1) {
	total = RSTRING(bzf->buf)->len - bzf->bzs.avail_out;
	if (n != -1 && (RSTRING(res)->len + total) >= n) {
	    n -= RSTRING(res)->len;
	    res = rb_str_cat(res, bzf->bzs.next_out - total, n);
	    MEMMOVE(bzf->bzs.next_out - total,
		   bzf->bzs.next_out - total + n, char,
		   total - n);
	    bzf->bzs.next_out -= n;
	    bzf->bzs.avail_out += n;
	    return res;
	}
	if (total) {
	    res = rb_str_cat(res, bzf->bzs.next_out - total, total);
	}
	bzf->bzs.total_out_hi32 = bzf->bzs.total_out_lo32 = 0;
	bzf->bzs.next_out = RSTRING(bzf->buf)->ptr;
	bzf->bzs.avail_out = RSTRING(bzf->buf)->len;
	if (bzf->state == BZ_STREAM_END) {
	    return res;
	}
	if (!bzf->bzs.avail_in) {
	    bzf->in = rb_funcall(bzf->io, rb_intern("read"), 1, INT2FIX(1024));
	    if (TYPE(bzf->in) != T_STRING || RSTRING(bzf->in) == 0) {
		bz_raise(BZ_UNEXPECTED_EOF);
	    }
	    bzf->bzs.next_in = RSTRING(bzf->in)->ptr;
	    bzf->bzs.avail_in = RSTRING(bzf->in)->len;
	}
	bzf->state = BZ2_bzDecompress(&(bzf->bzs));
	if (bzf->state != BZ_OK && bzf->state != BZ_STREAM_END) {
	    bz_raise(bzf->state);
	}    
    }
    return Qnil;
}

static int
bz_getc(obj)
    VALUE obj;
{
    VALUE length = INT2FIX(1);
    VALUE res = bz_reader_read(1, &length, obj);
    if (NIL_P(res) || RSTRING(res)->len == 0) {
	return EOF;
    }
    return RSTRING(res)->ptr[0];
}

static VALUE
bz_reader_ungetc(obj, a)
    VALUE obj, a;
{
    struct bz_file *bzf;
    int c = NUM2INT(a);

    Get_BZ2(obj, bzf);
    if (!bzf->buf) {
	bz_raise(BZ_SEQUENCE_ERROR);
    }
    if (bzf->bzs.avail_out) {
	MEMMOVE(RSTRING(bzf->buf)->ptr + 1, RSTRING(bzf->buf)->ptr, char,
		RSTRING(bzf->buf)->len - bzf->bzs.avail_out);
	RSTRING(bzf->buf)->ptr[0] = c;
	bzf->bzs.next_out += 1;
	bzf->bzs.avail_out -= 1;
    }
    else {
	VALUE tmp = rb_str_new((unsigned char *)&c, 1);
	bzf->buf = rb_str_cat(tmp, RSTRING(bzf->buf)->ptr, 
			      RSTRING(bzf->buf)->len);
    }
    return Qnil;
}

VALUE
bz_reader_gets(obj)
    VALUE obj;
{
    VALUE str = Qnil;
    int c;
    char buf[8192];
    char *bp, *bpe = buf + sizeof buf - 3;
    int cnt;
    int append = 0;

  again:
    bp = buf;
    for (;;) {
	c = bz_getc(obj);
	if (c == EOF) {
	    break;
	}
	if ((*bp++ = c) == '\n') break;
	if (bp == bpe) break;
    }
    cnt = bp - buf;

    if (c == EOF && !append && cnt == 0) {
	str = Qnil;
	goto return_gets;
    }

    if (append)
	rb_str_cat(str, buf, cnt);
    else
	str = rb_str_new(buf, cnt);

    if (c != EOF && RSTRING(str)->ptr[RSTRING(str)->len-1] != '\n') {
	append = 1;
	goto again;
    }

  return_gets:
    if (!NIL_P(str)) {
	OBJ_TAINT(str);
    }

    return str;
}

static VALUE
bz_reader_gets_internal(argc, argv, obj)
    int argc;
    VALUE obj, *argv;
{
    VALUE str = Qnil;
    int c, newline;
    char *rsptr;
    int rslen, rspara = 0;
    VALUE rs, res;

    if (argc == 0) {
	rs = rb_rs;
    }
    else {
	rb_scan_args(argc, argv, "1", &rs);
	if (!NIL_P(rs)) Check_Type(rs, T_STRING);
    }

    if (NIL_P(rs)) {
	rsptr = 0;
	rslen = 0;
    }
    else if (rs == rb_default_rs) {
	return bz_reader_gets(obj);
    }
    else {
	rslen = RSTRING(rs)->len;
	if (rslen == 0) {
	    rsptr = "\n\n";
	    rslen = 2;
	    rspara = 1;
	}
	else if (rslen == 1 && RSTRING(rs)->ptr[0] == '\n') {
	    return bz_reader_gets(obj);
	}
	else {
	    rsptr = RSTRING(rs)->ptr;
	}
    }

    if (rspara) {
	do {
	    c = bz_getc(obj);
	    if (c != '\n') {
		bz_reader_ungetc(obj, INT2NUM(c));
		break;
	    }
	} while (c != EOF);
    }

    newline = rslen ? rsptr[rslen - 1] : 0777;
    {
	char buf[8192];
	VALUE length = INT2FIX(8192);
	char *bp, *bpe = buf + sizeof buf - 3;
	int cnt;
	int append = 0;

      again:
	bp = buf;

	if (rslen) {
	    for (;;) {
		c = bz_getc(obj);
		if (c == EOF) {
		    break;
		}
		if ((*bp++ = c) == newline) break;
		if (bp == bpe) break;
	    }
	    cnt = bp - buf;
	}
	else {
	    res = bz_reader_read(1, &length, obj);
	    c = cnt = 0;
	    if (!NIL_P(res)) {
		cnt = RSTRING(res)->len;
	    }
	    if (cnt == 0) {
		c = EOF;
	    }
	    else {
		strncpy(buf, RSTRING(res)->ptr, cnt);
	    }
	}

	if (c == EOF && !append && cnt == 0) {
	    str = Qnil;
	    goto return_gets;
	}

	if (append)
	    rb_str_cat(str, buf, cnt);
	else
	    str = rb_str_new(buf, cnt);

	if (c != EOF &&
	    (!rslen ||
	     RSTRING(str)->len < rslen ||
	     memcmp(RSTRING(str)->ptr+RSTRING(str)->len-rslen,rsptr,rslen))) {
	    append = 1;
	    goto again;
	}
    }

  return_gets:
    if (rspara) {
	while (c != EOF) {
	    c = bz_getc(obj);
	    if (c != '\n') {
		bz_reader_ungetc(obj, INT2NUM(c));
		break;
	    }
	}
    }

    if (!NIL_P(str)) {
	OBJ_TAINT(str);
    }

    return str;
}

static VALUE
bz_reader_unused_set(obj, a)
    VALUE obj, a;
{
    struct bz_file *bzf;

    Check_Type(a, T_STRING);
    Get_BZ2(obj, bzf);
    if (!bzf->in) {
	bzf->in = rb_str_new(RSTRING(a)->ptr, RSTRING(a)->len);
    }
    else {
	bzf->in = rb_str_cat(bzf->in, RSTRING(a)->ptr, RSTRING(a)->len);
    }
    bzf->bzs.next_in = RSTRING(bzf->in)->ptr;
    bzf->bzs.avail_in = RSTRING(bzf->in)->len;
    return Qnil;
}

static VALUE
bz_reader_getc(obj)
    VALUE obj;
{
    VALUE str;
    VALUE len = INT2FIX(1);

    str = bz_reader_read(1, &len, obj);
    if (NIL_P(str) || RSTRING(str)->len == 0) {
	return Qnil;
    }
    return INT2FIX(RSTRING(str)->ptr[0] & 0xff);
}

static void
bz_eoz_error()
{
    rb_raise(bz_eEOZError, "End of Zipfile reached");
}

static VALUE
bz_reader_readchar(obj)
    VALUE obj;
{
    VALUE res = bz_reader_getc(obj);

    if (NIL_P(res)) {
	bz_eoz_error();
    }
    return res;
}

static VALUE
bz_reader_gets_m(argc, argv, obj)
    int argc;
    VALUE obj, *argv;
{
    VALUE str = bz_reader_gets_internal(argc, argv, obj);

    if (!NIL_P(str)) {
	rb_lastline_set(str);
    }
    return str;
}

static VALUE
bz_reader_readline(argc, argv, obj)
    int argc;
    VALUE obj, *argv;
{
    VALUE res = bz_reader_gets_m(argc, argv, obj);

    if (NIL_P(res)) {
	bz_eoz_error();
    }
    return res;
}

static VALUE
bz_reader_readlines(argc, argv, obj)
    int argc;
    VALUE obj, *argv;
{
    VALUE line, ary;

    ary = rb_ary_new();
    while (!NIL_P(line = bz_reader_gets_internal(argc, argv, obj))) {
	rb_ary_push(ary, line);
    }
    return ary;
}

static VALUE
bz_reader_each_line(argc, argv, obj)
    int argc;
    VALUE obj, *argv;
{
    VALUE line;

    while (!NIL_P(line = bz_reader_gets_internal(argc, argv, obj))) {
	rb_yield(line);
    }
    return obj;
}

static VALUE
bz_reader_each_byte(obj)
    VALUE obj;
{
    int c;

    while ((c = bz_getc(obj)) != EOF) {
	rb_yield(INT2FIX(c & 0xff));
    }
    return obj;
}

static VALUE
bz_reader_unused(obj)
    VALUE obj;
{
    struct bz_file *bzf;
    VALUE res;

    Get_BZ2(obj, bzf);
    if (!bzf->in || bzf->state != BZ_STREAM_END) {
	return Qnil;
    }
    if (bzf->bzs.avail_in) {
	res = rb_tainted_str_new(bzf->bzs.next_in, 
				 RSTRING(bzf->in)->len - bzf->bzs.avail_in);
	bzf->bzs.avail_in = 0;
    }
    else {
	res = rb_tainted_str_new(0, 0);
    }
    return res;
}

static VALUE
bz_reader_eoz(obj)
    VALUE obj;
{
    struct bz_file *bzf;

    Get_BZ2(obj, bzf);
    if (!bzf->in || !bzf->buf) {
	return Qnil;
    }
    if (bzf->state == BZ_STREAM_END && 
	bzf->bzs.avail_out == RSTRING(bzf->buf)->len) {
	return Qtrue;
    }
    return Qfalse;
}

static VALUE
bz_reader_eof(obj)
    VALUE obj;
{
    struct bz_file *bzf;
    VALUE res;

    res = bz_reader_eoz(obj);
    if (RTEST(res)) {
	Get_BZ2(obj, bzf);
	if (bzf->bzs.avail_in) {
	    res = Qfalse;
	}
	else {
	    res = bz_reader_getc(obj);
	    if (NIL_P(res)) {
		res = Qtrue;
	    }
	    else {
		bz_reader_ungetc(res);
		res = Qfalse;
	    }
	}
    }
    return res;
}

static VALUE
bz_reader_closed(obj)
    VALUE obj;
{
    struct bz_file *bzf;

    Data_Get_Struct(obj, struct bz_file, bzf);
    return bzf->io?Qfalse:Qtrue;
}

static VALUE
bz_reader_close(obj)
    VALUE obj;
{
    struct bz_file *bzf;

    Get_BZ2(obj, bzf);
    if (rb_respond_to(bzf->io, rb_intern("close"))) {
	rb_funcall2(bzf->io, rb_intern("close"), 0, 0);
    }
    bzf->io = bzf->buf = 0;
    return Qnil;
}

static VALUE
bz_reader_close_m(argc, argv, obj)
    int argc;
    VALUE obj, *argv;
{
    struct bz_file *bzf;
    VALUE end;

    if (rb_scan_args(argc, argv, "01", &end) && !RTEST(end)) {
	Get_BZ2(obj, bzf);
	if (bzf->buf) {
	    rb_funcall2(obj, rb_intern("read"), 0, 0);
	}
	bzf->buf = 0;
	return Qnil;
    }
    return bz_reader_close(obj);
}

struct foreach_arg {
    int argc;
    VALUE sep;
    VALUE obj;
};

static VALUE
bz_reader_foreach_line(arg)
    struct foreach_arg *arg;
{
    VALUE str;

    while (!NIL_P(str = bz_reader_gets_internal(arg->argc, &arg->sep, arg->obj))) {
	rb_yield(str);
    }
    return Qnil;
}

static VALUE
bz_reader_s_foreach(argc, argv, obj)
    int argc;
    VALUE obj, *argv;
{
    VALUE fname, sep;
    struct foreach_arg arg;

    if (!rb_block_given_p()) {
	rb_raise(rb_eArgError, "call out of a block");
    }
    rb_scan_args(argc, argv, "11", &fname, &sep);
    Check_SafeStr(fname);
    arg.argc = argc - 1;
    arg.sep = sep;
    arg.obj = rb_funcall2(rb_mKernel, rb_intern("open"), 1, &fname);
    if (NIL_P(arg.obj)) return Qnil;
    arg.obj = rb_funcall2(obj, rb_intern("new"), 1, &arg.obj);
    return rb_ensure(bz_reader_foreach_line, (VALUE)&arg, bz_reader_close, arg.obj);
}

static VALUE
bz_reader_i_readlines(arg)
    struct foreach_arg *arg;
{
    VALUE str, res;

    res = rb_ary_new();
    while (!NIL_P(str = bz_reader_gets_internal(arg->argc, &arg->sep, arg->obj))) {
	rb_ary_push(res, str);
    }
    return res;
}

static VALUE
bz_reader_s_readlines(argc, argv, obj)
    int argc;
    VALUE obj, *argv;
{
    VALUE fname, sep;
    struct foreach_arg arg;

    rb_scan_args(argc, argv, "11", &fname, &sep);
    Check_SafeStr(fname);
    arg.argc = argc - 1;
    arg.sep = sep;
    arg.obj = rb_funcall2(rb_mKernel, rb_intern("open"), 1, &fname);
    if (NIL_P(arg.obj)) return Qnil;
    arg.obj = rb_funcall2(obj, rb_intern("new"), 1, &arg.obj);
    return rb_ensure(bz_reader_i_readlines, (VALUE)&arg, bz_reader_close, arg.obj);
}

static VALUE
bz_str_read(obj, len)
    VALUE obj, len;
{
    struct bz_str *bzs;
    VALUE res;
    int count;
    
    count = NUM2INT(len);
    Data_Get_Struct(obj, struct bz_str, bzs);
    if (!count || bzs->pos == -1) {
	return Qnil;
    }
    if ((bzs->pos + count) >= RSTRING(bzs->str)->len) {
	res = rb_str_new(RSTRING(bzs->str)->ptr + bzs->pos, 
			 RSTRING(bzs->str)->len - bzs->pos);
	bzs->pos = -1;
    }
    else {
	res = rb_str_new(RSTRING(bzs->str)->ptr + bzs->pos, count);
	bzs->pos += count;
    }
    return res;
}

static VALUE
bz_uncompress(argc, argv, obj)
    int argc;
    VALUE obj, *argv;
{
    VALUE bz2, nilv = Qnil;

    if (!argc) {
	rb_raise(rb_eArgError, "need a String to Uncompress");
    }
    Check_Type(argv[0], T_STRING);
    bz2 = rb_funcall2(bz_cReader, rb_intern("new"), argc, argv);
    return bz_reader_read(1, &nilv, bz2);
}

void Init_bz2()
{
    VALUE bz_mBZ2;

    bz_mBZ2 = rb_define_module("BZ2");
    bz_eError = rb_define_class_under(bz_mBZ2, "Error", rb_eIOError);
    bz_eConfigError = rb_define_class_under(bz_mBZ2, "ConfigError", rb_eFatal);
    bz_eEOZError = rb_define_class_under(bz_mBZ2, "EOZError", bz_eError);
    bz_eSequenceError = rb_define_class_under(bz_mBZ2, "SequenceError", bz_eError);
    bz_eParamError = rb_define_class_under(bz_mBZ2, "ParamError", bz_eError);
    bz_eMemError = rb_define_class_under(bz_mBZ2, "MemError", bz_eError);
    bz_eDataError = rb_define_class_under(bz_mBZ2, "DataError", bz_eError);
    bz_eDataMagicError = rb_define_class_under(bz_mBZ2, "DataMagicError", bz_eError);
    bz_eIOError = rb_define_class_under(bz_mBZ2, "IOError", bz_eError);
    bz_eFullError = rb_define_class_under(bz_mBZ2, "OutBuffFullError", bz_eError);
    rb_define_module_function(bz_mBZ2, "compress", bz_compress, -1);
    rb_define_module_function(bz_mBZ2, "uncompress", bz_uncompress, -1);
    /*
      Writer
    */
    bz_cWriter = rb_define_class_under(bz_mBZ2, "Writer", rb_cData);
    rb_define_singleton_method(bz_cWriter, "allocate", bz_writer_s_alloc, 0);
    rb_define_singleton_method(bz_cWriter, "new", bz_writer_s_new, -1);
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
    /*
      Reader
    */
    bz_cReader = rb_define_class_under(bz_mBZ2, "Reader", rb_cData);
    rb_include_module(bz_cReader, rb_mEnumerable);
    rb_define_singleton_method(bz_cReader, "allocate", bz_reader_s_alloc, 0);
    rb_define_singleton_method(bz_cReader, "new", bz_reader_s_new, -1);
    rb_define_singleton_method(bz_cReader, "open", bz_reader_s_open, -1);
    rb_define_singleton_method(bz_cReader, "foreach", bz_reader_s_foreach, -1);
    rb_define_singleton_method(bz_cReader, "readlines", bz_reader_s_readlines, -1);
    rb_define_method(bz_cReader, "initialize", bz_reader_init, -1);
    rb_define_method(bz_cReader, "read", bz_reader_read, -1);
    rb_define_method(bz_cReader, "unused", bz_reader_unused, 0);
    rb_define_method(bz_cReader, "unused=", bz_reader_unused_set, 1);
    rb_define_method(bz_cReader, "ungetc", bz_reader_ungetc, 1);
    rb_define_method(bz_cReader, "getc", bz_reader_getc, 0);
    rb_define_method(bz_cReader, "gets", bz_reader_gets_m, -1);
    rb_define_method(bz_cReader, "readchar", bz_reader_readchar, 0);
    rb_define_method(bz_cReader, "readline", bz_reader_readline, -1);
    rb_define_method(bz_cReader, "readlines", bz_reader_readlines, -1);
    rb_define_method(bz_cReader, "each", bz_reader_each_line, -1);
    rb_define_method(bz_cReader, "each_line", bz_reader_each_line, -1);
    rb_define_method(bz_cReader, "each_byte", bz_reader_each_byte, 0);
    rb_define_method(bz_cReader, "close", bz_reader_close_m, -1);
    rb_define_method(bz_cReader, "closed", bz_reader_closed, 0);
    rb_define_method(bz_cReader, "closed?", bz_reader_closed, 0);
    rb_define_method(bz_cReader, "eoz?", bz_reader_eoz, 0);
    rb_define_method(bz_cReader, "eoz", bz_reader_eoz, 0);
    rb_define_method(bz_cReader, "eof?", bz_reader_eof, 0);
    rb_define_method(bz_cReader, "eof", bz_reader_eof, 0);
    /*
      Internal
    */
    bz_cInternal = rb_define_class_under(bz_mBZ2, "InternalStr", rb_cData);
    rb_undef_method(CLASS_OF(bz_cInternal), "new");
    rb_undef_method(CLASS_OF(bz_cInternal), "allocate");
    rb_undef_method(bz_cInternal, "initialize");
    rb_define_method(bz_cInternal, "read", bz_str_read, 1);
}

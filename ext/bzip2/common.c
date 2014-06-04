#include <ruby.h>
#include <bzlib.h>

#include "common.h"

void bz_file_mark(struct bz_file * bzf) {
    rb_gc_mark(bzf->io);
    rb_gc_mark(bzf->in);
}

void * bz_malloc(void *opaque, int m, int n) {
    return malloc(m * n);
}

void bz_free(void *opaque, void *p) {
    free(p);
}

void bz_raise(int error) {
    VALUE exc;
    const char *msg;

    exc = bz_eError;
    switch (error) {
        case BZ_SEQUENCE_ERROR:
            msg = "incorrect sequence";
            break;
        case BZ_PARAM_ERROR:
            msg = "parameter out of range";
            break;
        case BZ_MEM_ERROR:
            msg = "not enough memory is available";
            break;
        case BZ_DATA_ERROR:
            msg = "data integrity error is detected";
            break;
        case BZ_DATA_ERROR_MAGIC:
            msg = "compressed stream does not start with the correct magic bytes";
            break;
        case BZ_IO_ERROR:
            msg = "error reading or writing";
            break;
        case BZ_UNEXPECTED_EOF:
            exc = bz_eEOZError;
            msg = "compressed file finishes before the logical end of stream is detected";
            break;
        case BZ_OUTBUFF_FULL:
            msg = "output buffer full";
            break;
        default:
            msg = "unknown error";
            exc = bz_eError;
    }
    rb_raise(exc, "%s", msg);
}

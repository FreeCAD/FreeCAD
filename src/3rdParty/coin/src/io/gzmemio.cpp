/*
   gzmemio.c, heavily based on gzio.c from zlib-1.1.4. Some cleanup
   and changes done by the Coin team to be able to read gzipped files
   from memory instead of from a FILE *. The zlib copyright notice is
   included below.

   Copyright (C) 1995-2002 Jean-loup Gailly and Mark Adler

   This software is provided 'as-is', without any express or implied
   warranty.  In no event will the authors be held liable for any damages
   arising from the use of this software.

   Permission is granted to anyone to use this software for any purpose,
   including commercial applications, and to alter it and redistribute it
   freely, subject to the following restrictions:

   1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required.
   2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.
   3. This notice may not be removed or altered from any source distribution.

   Jean-loup Gailly        Mark Adler
   jloup@gzip.org          madler@alumni.caltech.edu
*/

#include "io/gzmemio.h"
#include "coindefs.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cassert>

#include "glue/zlib.h"

/* stuff copied from the zlib.h header file (we want to avoid
   including it here so that zlib is not required to compile Coin) */
struct internal_state;
typedef void * (*alloc_func)(void * opaque, unsigned int items, unsigned int size);
typedef void   (*free_func)(void * opaque, void * address);

#define Z_DEFAULT_COMPRESSION  (-1)
#define Z_DEFAULT_STRATEGY 0
#define Z_OK 0
#define Z_DATA_ERROR   (-3)
#define Z_STREAM_ERROR (-2)
#define Z_ERRNO        (-1)
#define Z_STREAM_END    1
#define Z_NO_FLUSH      0
#define Z_DEFLATED   8
#define MAX_WBITS   15 /* 32K LZ77 window */

/* This zlib struct should never change */
typedef struct {
  unsigned char * next_in;  /* next input byte */
  unsigned int avail_in;  /* number of bytes available at next_in */
  unsigned long total_in;  /* total nb of input bytes read so far */
  
  unsigned char * next_out; /* next output byte should be put there */
  unsigned int avail_out; /* remaining free space at next_out */
  unsigned long total_out; /* total nb of bytes output so far */
  
  char * msg;      /* last error message, NULL if no error */
  struct internal_state * state; /* not visible by applications */
  
  alloc_func zalloc;  /* used to allocate the internal state */
  free_func  zfree;   /* used to free the internal state */
  void * opaque;  /* private data object passed to zalloc and zfree */
  
  int data_type;  /* best guess about the data type: ascii or binary */
  unsigned long adler;      /* adler32 value of the uncompressed data */
  unsigned long reserved;   /* reserved for future use */
} z_stream;

/*
  Needed for inflateInit2().
*/
int 
cc_gzm_sizeof_z_stream(void)
{
  return sizeof(z_stream);
}

#define Z_BUFSIZE 16384
#define Z_NO_DEFLATE 1

#define Z_ALLOC(size) malloc(size)
#define Z_TRYFREE(p) {if (p) free(p);}

static int gz_magic[2] = {0x1f, 0x8b}; /* gzip magic header */

/* gzip flag byte */
#define Z_ASCII_FLAG   0x01 /* bit 0 set: file probably ascii text */
#define Z_HEAD_CRC     0x02 /* bit 1 set: header CRC present */
#define Z_EXTRA_FIELD  0x04 /* bit 2 set: extra field present */
#define Z_ORIG_NAME    0x08 /* bit 3 set: original file name present */
#define Z_COMMENT      0x10 /* bit 4 set: file comment present */
#define Z_RESERVED     0xE0 /* bits 5..7: reserved */

typedef struct {
  unsigned char * buf;
  unsigned int buflen;
  unsigned int currpos;
} cc_gzm_file;

typedef struct {
  z_stream stream;
  int z_err;   /* error code for last stream operation */
  int z_eof;   /* set if end of input file */
  uint8_t * inbuf;  /* input buffer */
  uint8_t * outbuf; /* output buffer */
  uint32_t crc;     /* crc32 of uncompressed data */
  char * msg;    /* error message */
  char * path;   /* path name for debugging only */
  int transparent; /* 1 if input file is not a .gz file */
  char mode;    /* 'w' or 'r' */
  int32_t startpos; /* start of compressed data in file (header skipped) */

  /* memory file */
  cc_gzm_file * memfile;
} cc_gzm_stream;

static int get_byte(cc_gzm_stream * s);
static void check_header(cc_gzm_stream * s);
static int destroy(cc_gzm_stream * s);
static uint32_t getInt32(cc_gzm_stream * s);

static int cc_gzm_fseek(cc_gzm_file * file, long offset, int whence);
static int cc_gzm_ftell(cc_gzm_file * file);
static size_t cc_gzm_fread(void * ptr, size_t size, size_t nmemb, cc_gzm_file * file);
static int cc_gzm_ferror(cc_gzm_file * file);

/* ===========================================================================
     Opens a gzip (.gz) mem buffer for reading or writing.
*/

void * cc_gzm_open(const uint8_t * buffer, uint32_t len)
{
  int err;
  //int level = Z_DEFAULT_COMPRESSION; /* compression level */
  //int strategy = Z_DEFAULT_STRATEGY; /* compression strategy */
  cc_gzm_stream * s;


  s = (cc_gzm_stream *) Z_ALLOC(sizeof(cc_gzm_stream));
  if (!s) return NULL;

  s->stream.zalloc = (alloc_func)0;
  s->stream.zfree = (free_func)0;
  s->stream.opaque = (void *)0;
  s->stream.next_in = s->inbuf = NULL;
  s->stream.next_out = s->outbuf = NULL;
  s->stream.avail_in = s->stream.avail_out = 0;
  s->z_err = Z_OK;
  s->z_eof = 0;
  s->crc = cc_zlibglue_crc32(0L, NULL, 0);
  s->msg = NULL;
  s->transparent = 0;
  s->path = NULL;
  s->memfile = NULL;

  s->mode = 'r'; /* read */
  if (s->mode == 'w') { /* not supported yet */
#ifdef Z_NO_DEFLATE
    err = Z_STREAM_ERROR;
#else
    err = cc_zlibglue_deflateInit2(&(s->stream), level,
                                   Z_DEFLATED, -MAX_WBITS, DEF_MEM_LEVEL, strategy);
    /* windowBits is passed < 0 to suppress zlib header */

    s->stream.next_out = s->outbuf = (uint8_t*)Z_ALLOC(Z_BUFSIZE);
#endif
    if (err != Z_OK || s->outbuf == NULL) {
      destroy(s);
      return NULL;
    }
  }
  else {
    s->memfile = (cc_gzm_file*) Z_ALLOC(sizeof(cc_gzm_file));
    s->memfile->buf = (uint8_t*) buffer;
    s->memfile->buflen = len;
    s->memfile->currpos = 0;

    s->stream.next_in  = s->inbuf = (uint8_t*)Z_ALLOC(Z_BUFSIZE);
    
    err = cc_zlibglue_inflateInit2(&(s->stream), -MAX_WBITS);
    /* windowBits is passed < 0 to tell that there is no zlib header.
     * Note that in this case inflate *requires* an extra "dummy" byte
     * after the compressed stream in order to complete decompression and
     * return Z_STREAM_END. Here the gzip CRC32 ensures that 4 bytes are
     * present after the compressed stream.
     */
    if (err != Z_OK || s->inbuf == NULL) {
      destroy(s);
      return NULL;
    }
  }
  s->stream.avail_out = Z_BUFSIZE;

  if (s->mode == 'w') {
#ifndef Z_NO_DEFLATE
    /* Write a very simple .gz header:
     */
    fprintf(s->file, "%c%c%c%c%c%c%c%c%c%c", gz_magic[0], gz_magic[1],
            Z_DEFLATED, 0 /*flags*/, 0,0,0,0 /*time*/, 0 /*xflags*/, OS_CODE);
    s->startpos = 10L;
    /* We use 10L instead of ftell(s->file) to because ftell causes an
     * fflush on some systems. This version of the library doesn't use
     * startpos anyway in write mode, so this initialization is not
     * necessary.
     */
#endif /* Z_NO_DEFLATE */
  }
  else {
    check_header(s); /* skip the .gz header */
    s->startpos = (cc_gzm_ftell(s->memfile) - s->stream.avail_in);
  }
  return (void*) s;
}

/* ===========================================================================
     Read a byte from a cc_gzm_stream; update next_in and avail_in. Return EOF
   for end of file.
   IN assertion: the stream s has been successfully opened for reading.
*/
static int
get_byte(cc_gzm_stream * s)
{
  if (s->z_eof) return EOF;
  if (s->stream.avail_in == 0) {
    /* errno = 0; */
    s->stream.avail_in = cc_gzm_fread(s->inbuf, 1, Z_BUFSIZE, s->memfile);
    if (s->stream.avail_in == 0) {
      s->z_eof = 1;
      if (cc_gzm_ferror(s->memfile)) s->z_err = Z_ERRNO;
      return EOF;
    }
    s->stream.next_in = s->inbuf;
  }
  s->stream.avail_in--;
  return *(s->stream.next_in)++;
}

/* ===========================================================================
      Check the gzip header of a cc_gzm_stream opened for reading. Set the stream
    mode to transparent if the gzip magic header is not present; set s->err
    to Z_DATA_ERROR if the magic header is present but the rest of the header
    is incorrect.
    IN assertion: the stream s has already been created successfully;
       s->stream.avail_in is zero for the first time, but may be non-zero
       for concatenated .gz files.
*/
static void
check_header(cc_gzm_stream * s)
{
  int method; /* method byte */
  int flags;  /* flags byte */
  uint32_t len;
  int c;

  /* Check the gzip magic header */
  for (len = 0; len < 2; len++) {
    c = get_byte(s);
    if (c != gz_magic[len]) {
      if (len != 0) s->stream.avail_in++, s->stream.next_in--;
      if (c != EOF) {
        s->stream.avail_in++, s->stream.next_in--;
        s->transparent = 1;
      }
      s->z_err = s->stream.avail_in != 0 ? Z_OK : Z_STREAM_END;
      return;
    }
  }
  method = get_byte(s);
  flags = get_byte(s);
  if (method != Z_DEFLATED || (flags & Z_RESERVED) != 0) {
    s->z_err = Z_DATA_ERROR;
    return;
  }

  /* Discard time, xflags and OS code: */
  for (len = 0; len < 6; len++) (void)get_byte(s);

  if ((flags & Z_EXTRA_FIELD) != 0) { /* skip the extra field */
    len  =  (uint32_t)get_byte(s);
    len += ((uint32_t)get_byte(s))<<8;
    /* len is garbage if EOF but the loop below will quit anyway */
    while (len-- != 0 && get_byte(s) != EOF) ;
  }
  if ((flags & Z_ORIG_NAME) != 0) { /* skip the original file name */
    while ((c = get_byte(s)) != 0 && c != EOF) ;
  }
  if ((flags & Z_COMMENT) != 0) {   /* skip the .gz file comment */
    while ((c = get_byte(s)) != 0 && c != EOF) ;
  }
  if ((flags & Z_HEAD_CRC) != 0) {  /* skip the header crc */
    for (len = 0; len < 2; len++) (void)get_byte(s);
  }
  s->z_err = s->z_eof ? Z_DATA_ERROR : Z_OK;
}

 /* ===========================================================================
 * Cleanup then free the given cc_gzm_stream. Return a zlib error code.
   Try freeing in the reverse order of allocations.
 */
static int destroy (cc_gzm_stream * s)
{
  int err = Z_OK;

  if (!s) return Z_STREAM_ERROR;

  Z_TRYFREE(s->msg);

  if (s->stream.state != NULL) {
    if (s->mode == 'w') {
#ifdef Z_NO_DEFLATE
      err = Z_STREAM_ERROR;
#else
      err = deflateEnd(&(s->stream));
#endif
    }
    else if (s->mode == 'r') {
      err = cc_zlibglue_inflateEnd(&(s->stream));
    }
  }

  if (s->z_err < 0) err = s->z_err;

  Z_TRYFREE(s->memfile);
  Z_TRYFREE(s->inbuf);
  Z_TRYFREE(s->outbuf);
  Z_TRYFREE(s->path);
  Z_TRYFREE(s);
  return err;
}

/* ===========================================================================
     Reads the given number of uncompressed bytes from the compressed file.
   gzread returns the number of bytes actually read (0 for end of file).
*/
int
cc_gzm_read (void * file, void * buf, uint32_t len)
{
  cc_gzm_stream *s = (cc_gzm_stream*)file;
  uint8_t *start = (uint8_t*)buf; /* starting point for crc computation */
  uint8_t * next_out; /* == stream.next_out but not forced far (for MSDOS) */

  if (s == NULL || s->mode != 'r') return Z_STREAM_ERROR;

  if (s->z_err == Z_DATA_ERROR || s->z_err == Z_ERRNO) return -1;
  if (s->z_err == Z_STREAM_END) return 0;  /* EOF */

  next_out = (uint8_t*)buf;
  s->stream.next_out = (uint8_t*)buf;
  s->stream.avail_out = len;

  while (s->stream.avail_out != 0) {
    if (s->transparent) {
      /* Copy first the lookahead bytes: */
      uint32_t n = s->stream.avail_in;
      if (n > s->stream.avail_out) n = s->stream.avail_out;
      if (n > 0) {
        memcpy(s->stream.next_out, s->stream.next_in, n);
        next_out += n;
        s->stream.next_out = next_out;
        s->stream.next_in   += n;
        s->stream.avail_out -= n;
        s->stream.avail_in  -= n;
      }
      if (s->stream.avail_out > 0) {
        s->stream.avail_out -= cc_gzm_fread(next_out, 1, s->stream.avail_out,
                                         s->memfile);
      }
      len -= s->stream.avail_out;
      s->stream.total_in  += (uint32_t)len;
      s->stream.total_out += (uint32_t)len;
      if (len == 0) s->z_eof = 1;
      return (int)len;
    }
    if (s->stream.avail_in == 0 && !s->z_eof) {

      /* errno = 0; */
      s->stream.avail_in = cc_gzm_fread(s->inbuf, 1, Z_BUFSIZE, s->memfile);
      if (s->stream.avail_in == 0) {
        s->z_eof = 1;
        if (cc_gzm_ferror(s->memfile)) {
          s->z_err = Z_ERRNO;
          break;
        }
      }
      s->stream.next_in = s->inbuf;
    }
    s->z_err = cc_zlibglue_inflate(&(s->stream), Z_NO_FLUSH);

    if (s->z_err == Z_STREAM_END) {
      /* Check CRC and original size */
      s->crc = cc_zlibglue_crc32(s->crc, (const char*) start, (uint32_t)(s->stream.next_out - start));
      start = s->stream.next_out;

      if (getInt32(s) != s->crc) {
        s->z_err = Z_DATA_ERROR;
      } else {
        (void)getInt32(s);
        /* The uncompressed length returned by above getint32_t() may
         * be different from s->stream.total_out) in case of
         * concatenated .gz files. Check for such files:
         */
        check_header(s);
        if (s->z_err == Z_OK) {
          uint32_t total_in = s->stream.total_in;
          uint32_t total_out = s->stream.total_out;

          cc_zlibglue_inflateReset(&(s->stream));
          s->stream.total_in = total_in;
          s->stream.total_out = total_out;
          s->crc = cc_zlibglue_crc32(0L, NULL, 0);
        }
      }
    }
    if (s->z_err != Z_OK || s->z_eof) break;
  }
  s->crc = cc_zlibglue_crc32(s->crc, (const char*) start, (uint32_t)(s->stream.next_out - start));

  return (int)(len - s->stream.avail_out);
}

/* ===========================================================================
      Reads one byte from the compressed file. gzgetc returns this byte
   or -1 in case of end of file or error.
*/
int
cc_gzm_getc(void * file)
{
  unsigned char c;

  return cc_gzm_read(file, &c, 1) == 1 ? c : -1;
}

/* ===========================================================================
   Reads bytes from the compressed file until len-1 characters are
   read, or a newline character is read and transferred to buf, or an
   end-of-file condition is encountered.  The string is then terminated
   with a null character.
   gzgets returns buf, or NULL in case of error.

   The current implementation is not optimized at all.
*/
char *
cc_gzm_gets(void * file, char * buf, int len)
{
  char * b = buf;
  if (buf == NULL || len <= 0) return NULL;

  while (--len > 0 && cc_gzm_read(file, buf, 1) == 1 && *buf++ != '\n') ;
  *buf = '\0';
  return b == buf && len > 0 ? NULL : b;
}

#ifndef Z_NO_DEFLATE


/* ===========================================================================
 * Update the compression level and strategy
 */
int
cc_gzm_setparams(void * file, int level, int strategy)
{
  cc_gzm_stream *s = (cc_gzm_stream*)file;

  if (s == NULL || s->mode != 'w') return Z_STREAM_ERROR;

  /* Make room to allow flushing */
  if (s->stream.avail_out == 0) {
    s->stream.next_out = s->outbuf;
    if (fwrite(s->outbuf, 1, Z_BUFSIZE, s->file) != Z_BUFSIZE) {
      s->z_err = Z_ERRNO;
    }
    s->stream.avail_out = Z_BUFSIZE;
  }
  return deflateParams (&(s->stream), level, strategy);
}

/* ===========================================================================
     Writes the given number of uncompressed bytes into the compressed file.
   gzwrite returns the number of bytes actually written (0 in case of error).
*/
int
cc_gzm_write(void * file, void * buf, unsigned int len)
{
  cc_gzm_stream *s = (cc_gzm_stream*)file;

  if (s == NULL || s->mode != 'w') return Z_STREAM_ERROR;

  s->stream.next_in = (uint8_t*)buf;
  s->stream.avail_in = len;

  while (s->stream.avail_in != 0) {

    if (s->stream.avail_out == 0) {

      s->stream.next_out = s->outbuf;
      if (fwrite(s->outbuf, 1, Z_BUFSIZE, s->file) != Z_BUFSIZE) {
        s->z_err = Z_ERRNO;
        break;
      }
      s->stream.avail_out = Z_BUFSIZE;
    }
    s->z_err = cc_zlibglue_deflate(&(s->stream), Z_NO_FLUSH);
    if (s->z_err != Z_OK) break;
  }
  s->crc = cc_zlibglue_crc32(s->crc, (const uint8_t *)buf, len);

  return (int)(len - s->stream.avail_in);
}

/* ===========================================================================
      Writes c, converted to an unsigned char, into the compressed file.
   gzputc returns the value that was written, or -1 in case of error.
*/
int
cc_gzm_putc(void * file, int c)
{
  unsigned char cc = (unsigned char) c; /* required for big endian systems */
  return cc_gzm_write(file, &cc, 1) == 1 ? (int)cc : -1;
}

/* ===========================================================================
      Writes the given null-terminated string to the compressed file, excluding
   the terminating null character.
      gzputs returns the number of characters written, or -1 in case of error.
*/
int
cc_gzm_puts(void * file, const char * s)
{
  return gzwrite(file, (char*)s, (unsigned)strlen(s));
}


/* ===========================================================================
     Flushes all pending output into the compressed file. The parameter
     flush is as in the deflate() function.
*/
static int do_flush (void * file, int flush)
{
  uint32_t len;
  int done = 0;
  cc_gzm_stream *s = (cc_gzm_stream*)file;

  if (s == NULL || s->mode != 'w') return Z_STREAM_ERROR;

  s->stream.avail_in = 0; /* should be zero already anyway */

  for (;;) {
    len = Z_BUFSIZE - s->stream.avail_out;

    if (len != 0) {
      if ((uint32_t)fwrite(s->outbuf, 1, len, s->file) != len) {
        s->z_err = Z_ERRNO;
        return Z_ERRNO;
      }
      s->stream.next_out = s->outbuf;
      s->stream.avail_out = Z_BUFSIZE;
    }
    if (done) break;
    s->z_err = cc_zlibglue_deflate(&(s->stream), flush);

    /* Ignore the second of two consecutive flushes: */
    if (len == 0 && s->z_err == Z_BUF_ERROR) s->z_err = Z_OK;

    /* deflate has finished flushing only when it hasn't used up
     * all the available space in the output buffer:
     */
    done = (s->stream.avail_out != 0 || s->z_err == Z_STREAM_END);

    if (s->z_err != Z_OK && s->z_err != Z_STREAM_END) break;
  }
  return  s->z_err == Z_STREAM_END ? Z_OK : s->z_err;
}

int cc_gzm_flush (void * file, int flush)
{
  cc_gzm_stream *s = (cc_gzm_stream*)file;
  int err = do_flush (file, flush);

  if (err) return err;
  fflush(s->file);
  return  s->z_err == Z_STREAM_END ? Z_OK : s->z_err;
}
#endif /* Z_NO_DEFLATE */

/* ===========================================================================
   Sets the starting position for the next gzread or gzwrite on the
   given compressed file. The offset represents a number of bytes in
   the gzseek returns the resulting offset location as measured in
   bytes from the beginning of the uncompressed stream, or -1 in case
   of error.  SEEK_END is not implemented, returns error.  In this
   version of the library, gzseek can be extremely slow.
*/
off_t
cc_gzm_seek(void * file, off_t offset, int whence)
{
  cc_gzm_stream *s = (cc_gzm_stream*)file;

  if (s == NULL || whence == SEEK_END ||
      s->z_err == Z_ERRNO || s->z_err == Z_DATA_ERROR) {
    return -1L;
  }

  if (s->mode == 'w') {
#ifdef Z_NO_DEFLATE
    return -1L;
#else
    if (whence == SEEK_SET) {
      offset -= s->stream.total_in;
    }
    if (offset < 0) return -1L;

    /* At this point, offset is the number of zero bytes to write. */
    if (s->inbuf == NULL) {
      s->inbuf = (uint8_t*)Z_ALLOC(Z_BUFSIZE); /* for seeking */
      zmemzero(s->inbuf, Z_BUFSIZE);
    }
    while (offset > 0)  {
      uint32_t size = Z_BUFSIZE;
      if (offset < Z_BUFSIZE) size = (uint32_t)offset;

      size = gzwrite(file, s->inbuf, size);
      if (size == 0) return -1L;

      offset -= size;
    }
    return (off_t)s->stream.total_in;
#endif
  }
  /* Rest of function is for reading only */

  /* compute absolute position */
  if (whence == SEEK_CUR) {
    offset += s->stream.total_out;
  }
  if (offset < 0) return -1L;

  if (s->transparent) {
    /* map to fseek */
    s->stream.avail_in = 0;
    s->stream.next_in = s->inbuf;
    if (cc_gzm_fseek(s->memfile, offset, SEEK_SET) < 0) return -1L;

    s->stream.total_in = s->stream.total_out = (uint32_t)offset;
    return offset;
  }

  /* For a negative seek, rewind and use positive seek */
  if ((uint32_t)offset >= s->stream.total_out) {
    offset -= s->stream.total_out;
  } else if (cc_zlibglue_gzrewind(file) < 0) {
    return -1L;
  }
  /* offset is now the number of bytes to skip. */

  if (offset != 0 && s->outbuf == NULL) {
    s->outbuf = (uint8_t*)Z_ALLOC(Z_BUFSIZE);
  }
  while (offset > 0)  {
    int size = Z_BUFSIZE;
    if (offset < Z_BUFSIZE) size = (int)offset;

    size = cc_zlibglue_gzread(file, s->outbuf, (uint32_t)size);
    if (size <= 0) return -1L;
    offset -= size;
  }
  return (off_t) s->stream.total_out;
}

/* ===========================================================================
     Rewinds input file.
*/
int cc_gzm_rewind(void * file)
{
  cc_gzm_stream *s = (cc_gzm_stream*)file;

  if (s == NULL || s->mode != 'r') return -1;

  s->z_err = Z_OK;
  s->z_eof = 0;
  s->stream.avail_in = 0;
  s->stream.next_in = s->inbuf;
  s->crc = cc_zlibglue_crc32(0L, NULL, 0);

  if (s->startpos == 0) { /* not a compressed file */
    s->memfile->currpos = 0;
    return 0;
  }

  (void) cc_zlibglue_inflateReset(&s->stream);
  return cc_gzm_fseek(s->memfile, s->startpos, SEEK_SET);
}

/* ===========================================================================
   Returns the starting position for the next gzread or gzwrite on the
   given compressed file. This position represents a number of bytes in the
   uncompressed data stream.
*/
off_t cc_gzm_tell(void * file)
{
  return cc_gzm_seek(file, 0L, SEEK_CUR);
}

/* ===========================================================================
   Returns 1 when EOF has previously been detected reading the given
   input stream, otherwise zero.
*/
int cc_gzm_eof(void * file)
{
  cc_gzm_stream *s = (cc_gzm_stream*)file;

  return (s == NULL || s->mode != 'r') ? 0 : s->z_eof;
}

#ifndef Z_NO_DEFLATE
/* ===========================================================================
   Outputs a int32_t in LSB order to the given file
*/
static void
putInt32(cc_gzm_file * file, uint32_t x)
{
  int n;
  for (n = 0; n < 4; n++) {
    cc_gzm_fputc((int)(x & 0xff), file);
    x >>= 8;
  }
}
#endif /* Z_NO_DEFLATE */

/* ===========================================================================
   Reads a int32_t in LSB order from the given cc_gzm_stream. Sets z_err in case
   of error.
*/
static uint32_t
getInt32(cc_gzm_stream * s)
{
  uint32_t x = (uint32_t)get_byte(s);
  int c;

  x += ((uint32_t)get_byte(s))<<8;
  x += ((uint32_t)get_byte(s))<<16;
  c = get_byte(s);
  if (c == EOF) s->z_err = Z_DATA_ERROR;
  x += ((uint32_t)c)<<24;
  return x;
}

/* ===========================================================================
   Flushes all pending output if necessary, closes the compressed file
   and deallocates all the (de)compression state.
*/
int cc_gzm_close(void * file)
{
  int err;
  cc_gzm_stream *s = (cc_gzm_stream*)file;

  if (s == NULL) return Z_STREAM_ERROR;

  if (s->mode == 'w') {
#ifdef Z_NO_DEFLATE
    err = Z_STREAM_ERROR;
    return err;
#else
    err = do_flush (file, Z_FINISH);
    if (err != Z_OK) return destroy((cc_gzm_stream*)file);

    putInt32(s->file, s->crc);
    putInt32(s->file, s->stream.total_in);
#endif
  }
  return destroy((cc_gzm_stream*)file);
}

/* stdio layer */

static int
cc_gzm_fseek(cc_gzm_file * file, long offset, int whence)
{
  switch (whence) {
  case SEEK_SET:
    if (offset > 0 && offset <= (long) file->buflen) {
      file->currpos = offset;
      return 0;
    }
    assert(0 && "illegal seek");
    return -1;
    break;
  case SEEK_CUR:
    if ((file->currpos + offset > 0) &&
        (file->currpos + offset <= file->buflen)) {
      file->currpos += offset;
      return 0;
    }
    assert(0 && "illegal seek");
    return -1;
    break;
  case SEEK_END:
    if ((file->buflen + offset > 0) &&
        (file->buflen + offset <= file->buflen)) {
      file->currpos = file->buflen + offset;
      return 0;
    }
    assert(0 && "illegal seek");
    return -1;
    break;
  default:
    assert(0 && "illegal whence");
    return -1;
  }
  return 0;
}

static int
cc_gzm_ftell(cc_gzm_file * file)
{
  return file->currpos;
}

static size_t
cc_gzm_fread(void * ptr, size_t size, size_t nmemb, cc_gzm_file * file)
{
  uint32_t remain;

  assert(size == 1); /* to simplify implementation */

  remain = file->buflen - file->currpos;
  if (remain > nmemb) remain = nmemb;
  if (remain == 0) return 0;

  (void) memcpy(ptr, file->buf + file->currpos, remain);
  file->currpos += remain;
  return remain;
}

static int
cc_gzm_ferror(cc_gzm_file * COIN_UNUSED_ARG(file))
{
  return 0;
}

#undef Z_BUFSIZE
#undef Z_NO_DEFLATE
#undef Z_ALLOC
#undef Z_TRYFREE
#undef Z_ASCII_FLAG
#undef Z_HEAD_CRC
#undef Z_EXTRA_FIELD
#undef Z_ORIG_NAME
#undef Z_COMMENT
#undef Z_RESERVED

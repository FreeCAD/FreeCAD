/**************************************************************************\
 * Copyright (c) Kongsberg Oil & Gas Technologies AS
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 
 * Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * 
 * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 * 
 * Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
\**************************************************************************/

#include "io/SoInput_Reader.h"

#include <cstring>
#include <cassert>
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H

#ifdef HAVE_UNISTD_H
#include <unistd.h> // dup()
#endif // HAVE_UNISTD_H

#ifdef HAVE_IO_H
#include <io.h> // Win32 dup()
#endif // HAVE_IO_H

#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#include <Inventor/errors/SoDebugError.h>

#include "io/gzmemio.h"
#include "glue/zlib.h"
#include "glue/bzip2.h"

// We don't want to include bzlib.h, so we just define the constants
// we use here
#ifndef BZ_OK
#define BZ_OK 0
#endif // BZ_OK

#ifndef BZ_STREAM_END
#define BZ_STREAM_END 4
#endif // BZ_STREAM_END

//
// abstract class
//

SoInput_Reader::SoInput_Reader(void)
  : dummyname("")
{
}

SoInput_Reader::~SoInput_Reader()
{
}

const SbString &
SoInput_Reader::getFilename(void)
{
  return this->dummyname;
}

FILE *
SoInput_Reader::getFilePointer(void)
{
  return NULL;
}

// creates the correct reader based on the file type in fp (will
// examine the file header). If fullname is empty, it's assumed that
// file FILE pointer is passed from the user, and that we cannot
// necessarily find the file handle.
SoInput_Reader *
SoInput_Reader::createReader(FILE * fp, const SbString & fullname)
{
  SoInput_Reader * reader = NULL;
  SbBool trycompression = FALSE;

#ifdef HAVE_FSTAT
  // need to make sure stream is seekable to enable compression
  // support, because we need to fseek() the stream
  int fn = fileno(fp);
  struct stat sb;
  if ( fstat(fn, &sb) == 0 ) {
    if ( sb.st_mode & S_IFREG ) { // regular file
      trycompression = TRUE;
    }
  }
#endif // HAVE_FSTAT

  if ( trycompression ) {
    static const size_t HEADER_SIZE = 4;
    unsigned char header[HEADER_SIZE];
    long offset = ftell(fp);
    SbBool valid_header = TRUE;
    if (fread(header, 1, HEADER_SIZE, fp)<HEADER_SIZE) {
      valid_header = FALSE;
    }
    (void) fseek(fp, offset, SEEK_SET);
    fflush(fp); // needed since we fetch the file descriptor later

    if (valid_header && header[0] == 'B' && header[1] == 'Z') {
      if (!cc_bzglue_available()) {
        SoDebugError::postWarning("SoInput_Reader::createReader",
                                  "File seems to be in bzip2 format, but "
                                  "libbz2 support is not available.");
      }
      else {
        int bzerror = BZ_OK;
        void * bzfp = cc_bzglue_BZ2_bzReadOpen(&bzerror,  fp, 0, 0, NULL, 0);
        if ((bzerror == BZ_OK) && (bzfp != NULL)) {
          reader = new SoInput_BZ2FileReader(fullname.getString(), bzfp);
        }
        else {
          SoDebugError::postWarning("SoInput_Reader::createReader",
                                    "Unable to open bzip2 file.");
        }
      }
    }
    if ((reader == NULL) && valid_header &&
        (header[0] == 0x1f) &&
        (header[1] == 0x8b)) {
      if (!cc_zlibglue_available()) {
        SoDebugError::postWarning("SoInput_Reader::createReader",
                                  "File seems to be in gzip format, but "
                                  "zlib support is not available.");
      }
      else {
        int fd = fileno(fp);
        // need to use dup() if we didn't open the file since gzdclose
        // will close it
        if (fd >= 0 && fullname.getLength() && fullname != "<stdin>")
          fd = dup(fd);
        if (fd >= 0) {
          void * gzfp = 0;
#ifdef HAVE_GZDOPEN
          gzfp = cc_zlibglue_gzdopen(fd, "rb");
#else // gzdopen() after reading from the compressed file does not work on Mac OS X
          if (fullname.getLength()) {
            gzfp = cc_zlibglue_gzopen(fullname.getString(), "rb");
          } else {
            SoDebugError::postWarning("SoInput_Reader::createReader",
                                      "Passing FILE* for gzipped files on "
                                      "Mac OS X not allowed, unable to open.");
          }
#endif
          if (gzfp) {
            reader = new SoInput_GZFileReader(fullname.getString(), gzfp);
          }
        }
        else {
          SoDebugError::postWarning("SoInput_Reader::createReader",
                                    "Unable to create file descriptor from stream.");
        }
      }
    }
  }

  if (reader == NULL) {
    reader = new SoInput_FileReader(fullname.getString(), fp);
  }
  return reader;
}



//
// standard FILE * class
//

SoInput_FileReader::SoInput_FileReader(const char * const filenamearg, FILE * filepointer)
{
  this->fp = filepointer;
  this->filename = filenamearg;
}

SoInput_FileReader::~SoInput_FileReader()
{
  // Close files which are not a memory buffer nor the stdin and
  // which we do have a filename for (if we don't have a filename,
  // the FILE ptr was just passed in through setFilePointer() and
  // is the library programmer's responsibility).
  if (this->fp &&
      (this->filename != "<stdin>") &&
      (this->filename.getLength())) {
    fclose(this->fp);
  }
}

SoInput_Reader::ReaderType
SoInput_FileReader::getType(void) const
{
  return REGULAR_FILE;
}

size_t
SoInput_FileReader::readBuffer(char * buf, const size_t readlen)
{
  return fread(buf, 1, readlen, this->fp);
}

const SbString &
SoInput_FileReader::getFilename(void)
{
  return this->filename;
}

FILE *
SoInput_FileReader::getFilePointer(void)
{
  return this->fp;
}

//
// standard membuffer class
//

SoInput_MemBufferReader::SoInput_MemBufferReader(const void * bufPointer, size_t bufSize)
{
  this->buf = (char*) bufPointer;
  this->buflen = bufSize;
  this->bufpos = 0;
}

SoInput_MemBufferReader::~SoInput_MemBufferReader()
{
}

SoInput_Reader::ReaderType
SoInput_MemBufferReader::getType(void) const
{
  return MEMBUFFER;
}

size_t
SoInput_MemBufferReader::readBuffer(char * buffer, const size_t readlen)
{
  size_t len = this->buflen - this->bufpos;
  if (len > readlen) len = readlen;

  memcpy(buffer, this->buf + this->bufpos, len);
  this->bufpos += len;

  return len;
}

//
// gzip readers
//
//
// gzipped membuffer class
//

SoInput_GZMemBufferReader::SoInput_GZMemBufferReader(const void * bufPointer, size_t bufSize)
{
  // FIXME: the bufSize cast (as size_t can be 64 bits wide) is there
  // to humour the interface of gzmemio -- should really be fixed
  // in the interface instead. 20050525 mortene.
  this->gzmfile = cc_gzm_open((uint8_t *)bufPointer, (uint32_t)bufSize);
  this->buf = bufPointer;
}

SoInput_GZMemBufferReader::~SoInput_GZMemBufferReader()
{
  cc_gzm_close(this->gzmfile);
}

SoInput_Reader::ReaderType
SoInput_GZMemBufferReader::getType(void) const
{
  return GZMEMBUFFER;
}

size_t
SoInput_GZMemBufferReader::readBuffer(char * buffer, const size_t readlen)
{
  // FIXME: about the cast; see note about the call to cc_gzm_open()
  // above. 20050525 mortene.
  return cc_gzm_read(this->gzmfile, buffer, (uint32_t)readlen);
}

//
// gzFile class
//

SoInput_GZFileReader::SoInput_GZFileReader(const char * const filenamearg, void * fp)
{
  this->gzfp = fp;
  this->filename = filenamearg;
}

SoInput_GZFileReader::~SoInput_GZFileReader()
{
  assert(this->gzfp);
  cc_zlibglue_gzclose(this->gzfp);
}

SoInput_Reader::ReaderType
SoInput_GZFileReader::getType(void) const
{
  return GZFILE;
}

size_t
SoInput_GZFileReader::readBuffer(char * buf, const size_t readlen)
{
  // FIXME: about the cast; see note about the call to cc_gzm_open()
  // above. 20050525 mortene.
  int result = cc_zlibglue_gzread(this->gzfp, buf, (uint32_t)readlen);

  // the signature of this this function was changed to return size_t
  // without checking that gzread() actually returns a signed
  // integer. We need to check for this and not just cast to size_t on
  // return
  if (result < 0) result = 0; // EOF

  return (size_t) result;
}

const SbString &
SoInput_GZFileReader::getFilename(void)
{
  return this->filename;
}

//
// bzFile class
//

SoInput_BZ2FileReader::SoInput_BZ2FileReader(const char * const filenamearg, void * fp)
{
  this->bzfp = fp;
  this->filename = filenamearg;
}

SoInput_BZ2FileReader::~SoInput_BZ2FileReader()
{
  if (this->bzfp) {
    int bzerror = BZ_OK;
    cc_bzglue_BZ2_bzReadClose(&bzerror, this->bzfp);
  }
}

SoInput_Reader::ReaderType
SoInput_BZ2FileReader::getType(void) const
{
  return BZ2FILE;
}

size_t
SoInput_BZ2FileReader::readBuffer(char * buf, const size_t readlen)
{
  if (this->bzfp == NULL) { return 0; }

  int bzerror = BZ_OK;
  // FIXME: about the cast; see note about the call to cc_gzm_open()
  // above. 20050525 mortene.
  int ret = cc_bzglue_BZ2_bzRead(&bzerror, this->bzfp,
                                 buf, (uint32_t)readlen);
  if ((bzerror != BZ_OK) && (bzerror != BZ_STREAM_END)) {
    ret = 0;
    cc_bzglue_BZ2_bzReadClose(&bzerror, this->bzfp);
    this->bzfp = NULL;
  }
  // the signature of this this function was changed to return size_t
  // without checking that bzRead() actually returns a signed
  // integer. We need to check for this and not just cast to size_t on
  // return.
  if (ret < 0) ret = 0; // might not be necessary, but will catch other errors
  return (size_t) ret;
}

const SbString &
SoInput_BZ2FileReader::getFilename(void)
{
  return this->filename;
}

#undef BZ_OK
#undef BZ_STREAM_END

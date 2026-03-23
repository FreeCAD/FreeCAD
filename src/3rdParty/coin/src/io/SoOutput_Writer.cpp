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

#include "io/SoOutput_Writer.h"
#include "coindefs.h"

#include <cstring>
#include <cassert>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#ifdef HAVE_UNISTD_H
#include <unistd.h> // dup()
#endif // HAVE_UNISTD_H

#ifdef HAVE_IO_H
#include <io.h> // Win32 dup()
#endif // HAVE_IO_H

#include <Inventor/errors/SoDebugError.h>
#include <Inventor/SbName.h>

#include "glue/zlib.h"
#include "glue/bzip2.h"

// We don't want to include bzlib.h, so we just define the constants
// we use here

#ifndef BZ_OK
#define BZ_OK 0
#endif // BZ_OK

#ifndef BZ_IO_ERROR
#define BZ_IO_ERROR (-6)
#endif // BZ_IO_ERROR

//
// abstract interface class
//

SoOutput_Writer::SoOutput_Writer(void)
{
}

SoOutput_Writer::~SoOutput_Writer()
{
}

FILE * 
SoOutput_Writer::getFilePointer(void)
{
  return NULL;
}


SoOutput_Writer * 
SoOutput_Writer::createWriter(FILE * fp, 
                              const SbBool shouldclose,
                              const SbName & compmethod,
                              const float level)
{
  if (compmethod == "GZIP") {
    if (cc_zlibglue_available()) {
      return new SoOutput_GZFileWriter(fp, shouldclose, level);
    }
    SoDebugError::postWarning("SoOutput_Writer::createWriter",
                              "Requested zlib compression, but zlib is not available.");
  }
  if (compmethod == "BZIP2") {
    if (cc_bzglue_available()) {
      return new SoOutput_BZ2FileWriter(fp, shouldclose, level);
    }
    SoDebugError::postWarning("SoOutput_Writer::createWriter",
                              "Requested bzip2 compression, but libz2 is not available.");
  }
  else if (compmethod != "NONE") {
    SoDebugError::postWarning("SoOutput_Writer::createWriter",
                              "Requested zlib compression, but zlib is not available.");

  }
  return new SoOutput_FileWriter(fp, shouldclose);
}


//
// standard stdio FILE writer
//

SoOutput_FileWriter::SoOutput_FileWriter(FILE * fptr, const SbBool shouldclosearg)
{
  this->fp = fptr;
  this->shouldclose = shouldclosearg;
}

SoOutput_FileWriter::~SoOutput_FileWriter()
{
  if (this->shouldclose) {
    assert(this->fp);
    fclose(this->fp);
  }
}


SoOutput_Writer::WriterType
SoOutput_FileWriter::getType(void) const
{
  return REGULAR_FILE;
}

size_t
SoOutput_FileWriter::write(const char * buf, size_t numbytes, const SbBool COIN_UNUSED_ARG(binary))
{
  assert(this->fp);
  return fwrite(buf, 1, numbytes, this->fp);
}

FILE * 
SoOutput_FileWriter::getFilePointer(void)
{
  return this->fp;
}

size_t 
SoOutput_FileWriter::bytesInBuf(void)
{
  return ftell(this->fp);
}


//
// membuffer writer
//

SoOutput_MemBufferWriter::SoOutput_MemBufferWriter(void * buffer,
                                                   const size_t len,
                                                   SoOutputReallocCB * reallocFunc,
                                                   size_t offsetarg)
{
  this->buf = (char*) buffer;
  this->bufsize = len;
  this->reallocfunc = reallocFunc;
  this->startoffset = this->offset = offsetarg;
}

SoOutput_MemBufferWriter::~SoOutput_MemBufferWriter()
{
}

SoOutput_Writer::WriterType
SoOutput_MemBufferWriter::getType(void) const
{
  return MEMBUFFER;
}

size_t
SoOutput_MemBufferWriter::write(const char * constc, size_t length, const SbBool binary)
{
  // Needs a \0 at the end if we're writing in ASCII.
  const size_t writelen = binary ? length : length + 1;

  if (this->makeRoomInBuf(writelen)) {
    char * writeptr = this->buf + this->offset;
    (void)memcpy(writeptr, constc, length);
    writeptr += length;
    this->offset += length;
    if (!binary) *writeptr = '\0'; // Terminate.
    return length;
  }
  return 0;
}

size_t 
SoOutput_MemBufferWriter::bytesInBuf(void)
{
  return this->offset;
}

SbBool
SoOutput_MemBufferWriter::makeRoomInBuf(size_t bytes)
{
  if ((this->offset + bytes) > this->bufsize) {
    if (this->reallocfunc) {
      this->bufsize = SbMax(this->offset + bytes, 2 * this->bufsize);
      this->buf = (char*) this->reallocfunc(this->buf, this->bufsize);
      if (this->buf) return TRUE;
    }
    return FALSE;
  }
  return TRUE;
}

//
// zlib writer
//

SoOutput_GZFileWriter::SoOutput_GZFileWriter(FILE * fp, const SbBool shouldclose, const float level)
{
  this->gzfp = NULL;

  int fd = fileno(fp);
  if (fd >= 0 && !shouldclose) fd = dup(fd);

  if (fd >= 0) {
    SbString mode = "wb";
    // convert level from [0.0, 1.0] to [1, 9]
    mode.addIntString((int) SbClamp((level * 8.0f) + 1.0f, 1.0f, 9.0f));

    this->gzfp = cc_zlibglue_gzdopen(fd, mode.getString());
    if (!this->gzfp) {
      SoDebugError::postWarning("SoOutput_GZFileWriter::SoOutput_GZFileWriter", 
                                "Unable to open file for writing.");    
    }
  }
  else {
    SoDebugError::postWarning("SoOutput_GZFileWriter::SoOutput_GZFileWriter", 
                              "Unable to create file descriptor from stream.");
    
  }
}

SoOutput_GZFileWriter::~SoOutput_GZFileWriter()
{
  if (this->gzfp) {
    cc_zlibglue_gzclose(this->gzfp);
  }
}


SoOutput_Writer::WriterType
SoOutput_GZFileWriter::getType(void) const
{
  return GZFILE;
}

size_t
SoOutput_GZFileWriter::write(const char * buf, size_t numbytes, const SbBool COIN_UNUSED_ARG(binary))
{
  if (this->gzfp) {
    // FIXME: the numbytes cast (as size_t can be 64 bits wide) is
    // there to humour the interface of *gzwrite() -- should really be
    // fixed in the interface instead. 20050526 mortene.
    return cc_zlibglue_gzwrite(this->gzfp, buf, (int)numbytes);
  }
  return 0;
}

size_t 
SoOutput_GZFileWriter::bytesInBuf(void)
{
  if (this->gzfp) {
    return cc_zlibglue_gztell(this->gzfp);
  }
  return 0;
}

//
// bzip2 writer
//

SoOutput_BZ2FileWriter::SoOutput_BZ2FileWriter(FILE * fparg, const SbBool shouldclose, const float level)
{
  this->fp = shouldclose ? fparg : NULL;
  this->writecounter = 0;

  int bzerror = BZ_OK;
  int numblocks =  (int) SbClamp((level * 8.0f) + 1.0f, 1.0f, 9.0f);
  
  this->bzfp = cc_bzglue_BZ2_bzWriteOpen(&bzerror, fparg, numblocks, 0, 0);
  if (this->bzfp && (bzerror != BZ_OK)) {
    SoDebugError::postWarning("SoOutput_BZ2FileWriter::SoOutput_BZF2ileWriter", 
                              "Unable to open file for writing.");    
    cc_bzglue_BZ2_bzWriteClose(&bzerror, this->bzfp, 0, NULL, NULL);
    this->bzfp = NULL;
  }
}

SoOutput_BZ2FileWriter::~SoOutput_BZ2FileWriter()
{
  if (this->bzfp) {
    int bzerror = BZ_OK;
    cc_bzglue_BZ2_bzWriteClose(&bzerror, this->bzfp, 0, NULL, NULL);
    if (bzerror != BZ_OK) {
      SoDebugError::postWarning("SoOutput_BZ2FileWriter::~SoOutput_BZ2FileWriter", 
                                "Error when closing bzip2 file.");    
      
    }
  }
  if (this->fp) fclose(fp);
}


SoOutput_Writer::WriterType
SoOutput_BZ2FileWriter::getType(void) const
{
  return BZ2FILE;
}

size_t
SoOutput_BZ2FileWriter::write(const char * buf, size_t numbytes, const SbBool COIN_UNUSED_ARG(binary))
{
  if (this->bzfp) {
    int bzerror = BZ_OK;
    // FIXME: about the cast; see note about the call to *gzmwrite()
    // above. 20050526 mortene.
    cc_bzglue_BZ2_bzWrite(&bzerror, this->bzfp, (void*) buf, (int)numbytes);
    
    if (bzerror != BZ_OK) {
      assert(bzerror == BZ_IO_ERROR);
      SoDebugError::postWarning("SoOutput_BZ2FileWriter::write", 
                                "I/O error while writing.");    
      cc_bzglue_BZ2_bzWriteClose(&bzerror, this->bzfp, 0, NULL, NULL);
      this->bzfp = NULL;
      return 0;
    }
    this->writecounter += numbytes;
    return numbytes;
  }
  return 0;
}

size_t 
SoOutput_BZ2FileWriter::bytesInBuf(void)
{
  return this->writecounter;
}

#undef BZ_OK
#undef BZ_IO_ERROR

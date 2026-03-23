#ifndef COIN_SOOUTPUT_WRITER_H
#define COIN_SOOUTPUT_WRITER_H

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

#ifndef COIN_INTERNAL
#error this is a private header file
#endif /* ! COIN_INTERNAL */

// *************************************************************************

#include <Inventor/SoOutput.h>
#include <stdio.h>

// *************************************************************************

class SoOutput_Writer {
public:
  SoOutput_Writer(void);
  virtual ~SoOutput_Writer();

  // add more enums as more writers are added
  enum WriterType {
    REGULAR_FILE,
    MEMBUFFER,
    GZFILE,
    BZ2FILE
  };

  // default method returns NULL. Should return the FILE pointer if
  // the Writer uses stdio to write.
  virtual FILE * getFilePointer(void);

  // must be overloaded to return the number of bites written so far
  virtual size_t bytesInBuf(void) = 0;

  // must be overloaded to return the writer type
  virtual WriterType getType(void) const = 0 ;

  // must be overloaded to write numbytes bytes to buf. Should
  // return the number of bytes actually written.
  virtual size_t write(const char * buf, size_t numbytes, const SbBool binary) = 0;

  static SoOutput_Writer * createWriter(FILE * fp,
                                        const SbBool shouldclose,
                                        const SbName & compmethod,
                                        const float level);

};

// class for stdio writing
class SoOutput_FileWriter : public SoOutput_Writer {
public:
  SoOutput_FileWriter(FILE * fp, const SbBool shouldclose);
  virtual ~SoOutput_FileWriter();

  virtual size_t bytesInBuf(void);
  virtual WriterType getType(void) const;
  virtual size_t write(const char * buf, size_t numbytes, const SbBool binary);
  virtual FILE * getFilePointer(void);

public:
  FILE * fp;
  SbBool shouldclose;
};

// class for membuffer writing
class SoOutput_MemBufferWriter : public SoOutput_Writer {
public:
  SoOutput_MemBufferWriter(void * buffer,
                           const size_t len,
                           SoOutputReallocCB * reallocFunc,
                           size_t offset);
  virtual ~SoOutput_MemBufferWriter();

  virtual size_t bytesInBuf(void);
  virtual WriterType getType(void) const;
  virtual size_t write(const char * buf, size_t numbytes, const SbBool binary);

public:

  SbBool makeRoomInBuf(size_t bytes);

  char * buf;
  size_t bufsize;
  SoOutputReallocCB * reallocfunc;
  size_t offset;
  size_t startoffset;
};

// class for zlib writing
class SoOutput_GZFileWriter : public SoOutput_Writer {
public:
  SoOutput_GZFileWriter(FILE * fp, const SbBool shouldclose, const float level);
  virtual ~SoOutput_GZFileWriter();

  virtual size_t bytesInBuf(void);
  virtual WriterType getType(void) const;
  virtual size_t write(const char * buf, size_t numbytes, const SbBool binary);

public:
  void * gzfp;
};

class SoOutput_BZ2FileWriter : public SoOutput_Writer {
public:
  SoOutput_BZ2FileWriter(FILE * fp, const SbBool shouldclose, const float level);
  virtual ~SoOutput_BZ2FileWriter();

  virtual size_t bytesInBuf(void);
  virtual WriterType getType(void) const;
  virtual size_t write(const char * buf, size_t numbytes, const SbBool binary);

public:
  void * bzfp;
  FILE * fp;
  size_t writecounter;
};

#endif // COIN_SOOUTPUT_WRITER_H

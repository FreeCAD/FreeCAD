#ifndef COIN_SOINPUT_READER_H
#define COIN_SOINPUT_READER_H

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

#include <Inventor/SbString.h>
#include <stdio.h>

// *************************************************************************

class SoInput_Reader {
public:
  SoInput_Reader(void);
  virtual ~SoInput_Reader();

  // add a new enum for each reader that is implemented
  enum ReaderType {
    REGULAR_FILE,
    MEMBUFFER,
    GZFILE,
    BZ2FILE,
    GZMEMBUFFER
  };

  // must be overloaded to return type
  virtual ReaderType getType(void) const = 0;

  // must be overloaded to read data. Should return number of bytes
  // read or 0 if eof
  virtual size_t readBuffer(char * buf, const size_t readlen) = 0;

  // should be overloaded to return filename. Default method returns
  // an empty string.
  virtual const SbString & getFilename(void);

  // default method returns NULL. Should only be overloaded if the
  // reader uses FILE * to read data.
  virtual FILE * getFilePointer(void);

  static SoInput_Reader * createReader(FILE * fp, const SbString & fullname);

public:
  SbString dummyname;
};

class SoInput_FileReader : public SoInput_Reader {
public:
  SoInput_FileReader(const char * const filename, FILE * filepointer);
  virtual ~SoInput_FileReader();

  virtual ReaderType getType(void) const;
  virtual size_t readBuffer(char * buf, const size_t readlen);

  virtual const SbString & getFilename(void);
  virtual FILE * getFilePointer(void);

public:
  SbString filename;
  FILE * fp;

};

class SoInput_MemBufferReader : public SoInput_Reader {
public:
  SoInput_MemBufferReader(const void * bufPointer, size_t bufSize);
  virtual ~SoInput_MemBufferReader();

  virtual ReaderType getType(void) const;
  virtual size_t readBuffer(char * buf, const size_t readlen);

public:
  char * buf;
  size_t buflen;
  size_t bufpos;
};

class SoInput_GZMemBufferReader : public SoInput_Reader {
public:
  SoInput_GZMemBufferReader(const void * bufPointer, size_t bufSize);
  virtual ~SoInput_GZMemBufferReader();

  virtual ReaderType getType(void) const;
  virtual size_t readBuffer(char * buf, const size_t readlen);

public:
  void * gzmfile;
  const void * buf;
};


class SoInput_GZFileReader : public SoInput_Reader {
public:
  SoInput_GZFileReader(const char * const filename, void * fp);
  virtual ~SoInput_GZFileReader();

  virtual ReaderType getType(void) const;
  virtual size_t readBuffer(char * buf, const size_t readlen);

  virtual const SbString & getFilename(void);

public:
  void * gzfp;
  SbString filename;
};

class SoInput_BZ2FileReader : public SoInput_Reader {
public:
  SoInput_BZ2FileReader(const char * const filename, void * fp);
  virtual ~SoInput_BZ2FileReader();

  virtual ReaderType getType(void) const;
  virtual size_t readBuffer(char * buf, const size_t readlen);

  virtual const SbString & getFilename(void);

public:
  void * bzfp;
  SbString filename;
};

#endif // COIN_SOINPUT_READER_H

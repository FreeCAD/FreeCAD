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

//  Part of the 3DS File Loader for Open Inventor
//
//  developed originally by PC John (peciva@fit.vutbr.cz)

#include "SoStream.h"

#include "coindefs.h"

#include <Inventor/SbString.h>
#include <Inventor/errors/SoDebugError.h>
#include <Inventor/SoInput.h>
#include <Inventor/C/tidbits.h>

#include <cstdlib>
#include <cstring>
#include <cassert>

#define STUB assert(FALSE && "Unimplemented SoStream behaviour.")



#define MAX_NUM_LENGTH 32


SoStream::StreamEndianOrdering SoStream::getHostEndianOrdering()
{
  switch (coin_host_get_endianness()) {
  case COIN_HOST_IS_LITTLEENDIAN: return SoStream::LITTLE_ENDIAN_STREAM;
  case COIN_HOST_IS_BIGENDIAN: return SoStream::BIG_ENDIAN_STREAM;
  default: assert(FALSE && "unknown byteordering"); return SoStream::HOST_DEP;
  }
}

void SoStream::updateNeedEndianConversion()
{ needEndianConversion = endianOrdering != getHostEndianOrdering()
                         && endianOrdering != HOST_DEP; }

void SoStream::setEndianOrdering(const StreamEndianOrdering value)
{ endianOrdering = value; updateNeedEndianConversion(); }

SoStream::StreamEndianOrdering SoStream::getEndianOrdering() const
{ return endianOrdering; }

#define HTON(_type_) \
static void hton_##_type_(_type_ value, char *buf, const SbBool needConv) \
{ \
  union { \
    _type_ v; \
    char b[sizeof(_type_)]; \
  }; \
  v = value; \
  if (needConv) { \
    unsigned int j = sizeof(_type_)-1; \
    for (unsigned int i=0; i<sizeof(_type_); i++) \
      buf[i] = b[j--]; \
  } else \
    for (unsigned int i=0; i<sizeof(_type_); i++) \
      buf[i] = b[i]; \
}
#define NTOH(_type_) \
static _type_ ntoh_##_type_(char *buf, const SbBool needConv) \
{ \
  if (needConv) { \
    union { \
      _type_ v; \
      char b[sizeof(_type_)]; \
    }; \
    unsigned int j = sizeof(_type_)-1; \
    for (unsigned int i=0; i<sizeof(_type_); i++) \
      b[i] = buf[j--]; \
    return v; \
  } else \
    return *(reinterpret_cast<_type_*>(buf)); \
}

inline static void hton_uint8_t(uint8_t value, char *buf, const SbBool COIN_UNUSED_ARG(needConv))  { buf[0] = value; }
inline static uint8_t ntoh_uint8_t(char *buf, const SbBool COIN_UNUSED_ARG(needConv))  { return buf[0]; }
inline static void hton_int8_t(int8_t value, char *buf, const SbBool COIN_UNUSED_ARG(needConv))  { buf[0] = value; }
inline static int8_t ntoh_int8_t(char *buf, const SbBool COIN_UNUSED_ARG(needConv))  { return buf[0]; }
HTON(uint16_t);
NTOH(uint16_t);
inline static void hton_int16_t(int16_t value, char *buf, const SbBool needConv)  { hton_uint16_t(value, buf, needConv); }
inline static int16_t ntoh_int16_t(char *buf, const SbBool needConv)  { return static_cast<int16_t>(ntoh_uint16_t(buf, needConv)); }
HTON(uint32_t);
NTOH(uint32_t);
inline static void hton_int32_t(int32_t value, char *buf, const SbBool needConv)  { hton_uint32_t(value, buf, needConv); }
inline static int32_t ntoh_int32_t(char *buf, const SbBool needConv)  { return static_cast<int32_t>(ntoh_uint32_t(buf, needConv)); }
HTON(float);
NTOH(float);
HTON(double);
NTOH(double);


#define SOSTREAM_RW_OP(_suffix_, _type_, _printString_, _readCode_) \
SbBool SoStream::read##_suffix_(_type_ &value) \
{ \
  if (isBinary()) { \
    char temp[sizeof(_type_)]; \
    readBinaryArray(&temp, sizeof(_type_)); \
    value = ntoh_##_type_(temp, needEndianConversion); \
  } else { \
    _readCode_; \
  } \
  return !isBad(); \
} \
SbBool SoStream::write##_suffix_(const _type_ value) \
{ \
  if (isBinary()) { \
    char temp[sizeof(_type_)]; \
    hton_##_type_(value, temp, needEndianConversion); \
    writeBinaryArray(const_cast<void *>(reinterpret_cast<const void*>(&value)), sizeof(_type_)); \
  } else { \
    SbString s; \
    s.sprintf(_printString_, value); \
    writeBinaryArray(const_cast<void *>(static_cast<const void*>(s.getString())), static_cast<size_t>(s.getLength())); \
  } \
  return !isBad(); \
}


#define SOSTREAM_INT_READ(_convertFunc_, _convertType_, _retType_, _check_) \
do { \
  if (isBad()) \
    return FALSE; \
 \
  char buf[MAX_NUM_LENGTH]; \
  char *s = buf; \
  const char *e = &buf[MAX_NUM_LENGTH-1]; \
  SbBool gotNum; \
 \
  getChar(*s); \
  if (*s == '-' || *s == '+') \
    s++; \
  else \
    ungetChar(*s); \
 \
  getChar(*s); \
  if (*s == '0') { \
    s++; \
    getChar(*s); \
    if (*s == 'x') { \
      s++; \
      gotNum = readHexInt(s, e); \
    } else { \
      ungetChar(*s); \
      gotNum = readDigInt(s, e); \
    } \
  } else { \
    ungetChar(*s); \
    gotNum = readDigInt(s, e); \
  } \
 \
  if (!gotNum) { setBadBit(); return FALSE; } \
 \
  char *ce; \
  *s = '\0'; \
  _convertType_ tempVal = _convertFunc_(buf, &ce, 0); \
 \
  if (ce != s) \
    assert(FALSE && "Wrong integer number buffer parsing."); \
 \
  _check_ \
  value = static_cast<_retType_>(tempVal); \
} while(0)


#define DEFAULT_CHECK0

#define DEFAULT_CHECK1(_maxVal_) \
  if (tempVal > _maxVal_) \
    setBadBit(); \
  else

#define DEFAULT_CHECK2(_minVal_, _maxVal_) \
  if (tempVal < _minVal_ || tempVal > _maxVal_) \
    setBadBit(); \
  else


#define SOSTREAM_F_READ(_convertFunc_, _retType_) \
do { \
  if (isBad()) \
    return FALSE; \
 \
  char buf[MAX_NUM_LENGTH]; \
  char *s = buf; \
  const char *e = &buf[MAX_NUM_LENGTH-1]; \
  SbBool gotNum; \
 \
  getChar(*s); \
  if (*s == '-' || *s == '+') \
    s++; \
  else \
    ungetChar(*s); \
 \
  gotNum = readDigInt(s, e); \
  if (!gotNum) { \
    getChar(*s); \
    if (*s == 'I') { \
      s++; getChar(*s); \
      if (*s == 'N') { \
        s++; getChar(*s); \
          if (*s == 'F') { \
          s++; \
          goto gotAll; \
        } else setBadBit(); \
      } else setBadBit(); \
    } else { \
      if (*s == 'N') { \
        s++; getChar(*s); \
        if (*s == 'A') { \
          s++; getChar(*s); \
          if (*s == 'N') { \
            s++; \
            goto gotAll; \
          } else setBadBit(); \
        } else setBadBit(); \
      } else \
        ungetChar(*s); \
    } \
    if (isBad()) \
      return FALSE; \
  } \
 \
  getChar(*s); \
  if (*s == '.') { \
    s++; \
    gotNum = readDigInt(s, e) || gotNum; \
  } else ungetChar(*s); \
 \
  if (!gotNum) { \
    setBadBit(); \
    return FALSE; \
  } \
 \
  getChar(*s); \
  if (*s == 'e' || *s == 'E') { \
    s++; \
    getChar(*s); \
    if (*s == '-' || *s == '+') s++; \
    else ungetChar(*s); \
 \
    if (!readDigInt(s, e)) { \
      setBadBit(); \
      return FALSE; \
    } \
  } else \
    ungetChar(*s); \
 \
gotAll: \
  \
  char *ce; \
  *s = '\0'; \
  double tempVal = _convertFunc_(buf, &ce); \
 \
  if (ce != s) \
    assert(FALSE && "Wrong float number buffer parsing."); \
 \
  value = static_cast<_retType_>(tempVal); \
} while(0)



SbBool SoStream::getChar(char &c)
{
  assert(isBinary() && "getChar can be used for text files only.");
  if (!isReadable()) setBadBit();
  if (isBad()) return FALSE;

  switch(streamType) {
  case MEMORY:
    if (bufPos < bufferSize) {
      c = buffer[bufPos++];
      return TRUE;
    }
    setBadBit();
    return FALSE;
  case FILE_STREAM:
    STUB;
    return FALSE;
  case SO_INPUT_WRAP:
    return soinput->read(c);
  default:
    assert(FALSE);
    return FALSE;
  }
}


void SoStream::ungetChar(const char c)
{
  assert(isBinary() && "ungetChar can be used for text files only.");
  if (!isReadable()) setBadBit();
  if (isBad()) return;

  switch (streamType) {
  case MEMORY:
    if (bufPos > 0 && bufPos <= bufferSize) {
      --bufPos;
      return;
    }
    assert(FALSE && "Strange playing with file pointer.");
    setBadBit();
	break;
  case FILE_STREAM:
    STUB;
    return;
  case SO_INPUT_WRAP:
    soinput->putBack(c);
    return;
  default:
    assert(FALSE && "not handled");
    break;
  }
}


void SoStream::putChar(const char c)
{
  assert(isBinary() && "putChar can be used for text files only.");
  if (!isWriteable()) setBadBit();
  if (isBad()) return;

  switch (streamType) {
  case FILE_STREAM: STUB; break;
  case MEMORY: {
    assert(bufPos <= bufferSize && "Wrong file buffer position.");
    if (bufPos == bufferSize)
      if (!reallocBuffer(bufferSize+1)) {
        setBadBit();
        return;
      }
    buffer[bufPos++] = c;
  }
  break;
  default:
    STUB;
  }
}


size_t SoStream::readBinaryArray(void *buf, size_t size)
{
  if (!isReadable()) setBadBit();
  if (isBad()) return 0;

  switch (streamType) {
  case MEMORY:
    assert(bufPos <= bufferSize && "Wrong read buffer position.");
    size_t amount;
    if (bufPos + size > bufferSize)
      amount = bufferSize - bufPos;
    else
      amount = size;
    memcpy(buf, &buffer[bufPos], amount);
    bufPos += amount;
    return amount;
  case FILE_STREAM:
    return fread(buf, 1, size, filep);
  case SO_INPUT_WRAP: {
    size_t pos = soinput->getNumBytesRead();
    if (soinput->readBinaryArray(static_cast<unsigned char*>(buf), (int)size))  return size;
    else  return soinput->getNumBytesRead() - pos;
  }
  default:
    assert(FALSE);
    return 0;
  }
}


size_t SoStream::writeBinaryArray(void *buf, size_t size)
{
  if (!isWriteable()) setBadBit();
  if (isBad()) return 0;

  switch (streamType) {
  case FILE_STREAM: return fwrite(buf, 1, size, filep);
  case MEMORY: {
    assert(bufPos <= bufferSize && "Wrong write buffer position.");
    if (bufPos+size >= bufferSize)
      // if not enough space => realloc
      if (!reallocBuffer(bufPos+size)) {
        // if can not realloc => setBadBit and copy only what is possible
        setBadBit();
        size = bufferSize - bufPos;
      }
    memcpy(&buffer[bufPos], buf, size);
    bufPos += size;
    return size;
  }
  case SO_INPUT_WRAP: assert(FALSE && "Unsupported operation."); return 0;
  default: assert(FALSE); return 0;
  }
}



SOSTREAM_RW_OP(  Int8,   int8_t,   "%d", SOSTREAM_INT_READ(strtol,   int32_t,   int8_t, DEFAULT_CHECK2(-128, 127)));
SOSTREAM_RW_OP( UInt8,  uint8_t, "0x%x", SOSTREAM_INT_READ(strtoul, uint32_t,  uint8_t, DEFAULT_CHECK1(0xff)));
SOSTREAM_RW_OP( Int16,  int16_t,   "%d", SOSTREAM_INT_READ(strtol,   int32_t,  int16_t, DEFAULT_CHECK2(-32768, 32767)));
SOSTREAM_RW_OP(UInt16, uint16_t, "0x%x", SOSTREAM_INT_READ(strtoul, uint32_t, uint16_t, DEFAULT_CHECK1(0xffff)));
SOSTREAM_RW_OP( Int32,  int32_t,   "%d", SOSTREAM_INT_READ(strtol,   int32_t,  int32_t, DEFAULT_CHECK0));
SOSTREAM_RW_OP(UInt32, uint32_t, "0x%x", SOSTREAM_INT_READ(strtoul, uint32_t, uint32_t, DEFAULT_CHECK0));



SbBool SoStream::readDigInt(char COIN_UNUSED_ARG(*s), const char COIN_UNUSED_ARG(*e))
{
  STUB;
  return FALSE;
}


SbBool SoStream::readHexInt(char COIN_UNUSED_ARG(*s), const char COIN_UNUSED_ARG(*e))
{
  STUB;
  return FALSE;
}



SOSTREAM_RW_OP(Float, float, "%e", SOSTREAM_F_READ(strtod, float));
SOSTREAM_RW_OP(Double, double, "%e", SOSTREAM_F_READ(strtod, double));



size_t SoStream::readBuffer(void *buf, size_t bufSize)
{
  return readBinaryArray(buf, bufSize);
}
size_t SoStream::writeBuffer(void *buf, size_t bufSize)
{
  return writeBinaryArray(buf, bufSize);
}



SbBool SoStream::readFromStream(SoStream &stream)
{
  size_t amount = stream.getSize() - stream.getPos();
  readFromStream(stream, amount);
  return !isBad();
}
SbBool SoStream::writeToStream(SoStream &stream)
{
  return stream.readFromStream(*this);
}
size_t SoStream::readFromStream(SoStream &stream, size_t bytes)
{
  size_t al;
  if (bytes > 1048576) al = 1048576;
  else al = bytes;
  void *temp = malloc(al);
  size_t trans = 0;
  do {
    if (trans+al > bytes)  al = (bytes - trans);
    size_t amount = stream.readBuffer(temp, al);
    size_t written = this->writeBuffer(temp, amount);
    trans += written;
    if (isBad()) break;
  } while (trans < bytes);
  free(temp);

  return trans;
}
size_t SoStream::writeToStream(SoStream &stream, size_t bytes)
{
  return stream.readFromStream(*this, bytes);
}



SbBool SoStream::readZString(char *buf, int bufSize)
{
  uint8_t c;
  do {
    if (bufSize-- == 0) { setBadBit(); return FALSE; }
    operator >> (c);
    *(buf++) = c;
  } while (c != '\0');
  return !isBad();
}
SbBool SoStream::writeZString(const char *buf)
{
  do {
    operator << (buf);
  } while (*(buf++) != '\0');
  return !isBad();
}



SbBool SoStream::readStream(SoStream COIN_UNUSED_ARG(&stream))
{
  STUB;
  return FALSE;
}
SbBool SoStream::writeStream(const SoStream COIN_UNUSED_ARG(&stream))
{
  STUB;
  return FALSE;
}



void SoStream::setPos(size_t pos)
{
  switch (streamType) {
  case FILE_STREAM:  fseek(filep, (long)pos, SEEK_SET); break;
  case MEMORY:  if (pos > bufferSize) setBadBit();
                else bufPos = pos;
                break;
  case SO_INPUT_WRAP: {
    size_t cpos = getPos();
    if (pos >= cpos) {
      size_t num = pos - cpos;
      char c[16];
      size_t d;
      while (num > 0) {
        d = num<=16 ? num : 16;
        if (readBinaryArray(&c, d) != d) {
          setBadBit();
          break;
        }
        num -= d;
      }
    } else {
      assert(FALSE && "SoStream::setPos() to move back in the stream is not supported.");
      setBadBit();
    }
    break;
  }
  default:  assert(FALSE && "Unsupported operation."); break;
  }
}

size_t SoStream::getPos() const
{
  switch (streamType) {
  case FILE_STREAM:  return ftell(filep);
  case MEMORY:  return bufPos;
  case SO_INPUT_WRAP:  return soinput->getNumBytesRead();
  default:  assert(FALSE); return 0;
  }
}

size_t SoStream::getSize() const
{
  switch (streamType) {
  case FILE_STREAM : {
    long cpos = ftell(filep);
    fseek(filep, 0, SEEK_END);
    size_t size = ftell(filep);
    fseek(filep, cpos, SEEK_SET);
    return size;
  }
  case MEMORY:  return bufferSize;
  case SO_INPUT_WRAP: assert(FALSE && "Unsupported operation."); return 0;
  default: assert(FALSE); return 0;
  }
}


void SoStream::setBadBit()  { badBit = TRUE; }
void SoStream::clearBadBit()  { badBit = FALSE; }
SbBool SoStream::isBad() const  { return badBit; }

void SoStream::setAccessRights(SbBool readEnabled, SbBool writeEnabled)
{ readable = readEnabled; writeable = writeEnabled; }
SbBool SoStream::isReadable() const  { return readable; }
SbBool SoStream::isWriteable() const  { return writeable; }


SbBool SoStream::reallocBuffer(size_t newSize)
{
  assert(streamType == MEMORY);
  assert(newSize > 0);

  if (newSize <= bufferAllocSize) {
    bufferSize = newSize;
    return TRUE;
  }

  size_t newAllocSize = size_t(bufferAllocSize * 1.5);
  if (newAllocSize < newSize)
    newAllocSize = newSize;

  char *newBuffer = static_cast<char*>(realloc(buffer, newAllocSize));
  if (newBuffer != NULL) {
    buffer = newBuffer;
    bufferSize = newSize;
    bufferAllocSize = newAllocSize;
    return TRUE;
  } else
    return FALSE;
}


void SoStream::setBinary(const SbBool flag)  { binaryStream = flag; }
SbBool SoStream::isBinary() const  { return binaryStream; }


SbBool SoStream::getBuffer(void *&buf, size_t &size) const
{
  if (streamType != MEMORY)
    return FALSE;
  buf = buffer;
  size = bufferSize;
  return TRUE;
}
size_t SoStream::getBufferSize() const  { return bufferSize; }

void SoStream::setBuffer(void COIN_UNUSED_ARG(*buf), size_t COIN_UNUSED_ARG(size))
{
  emptyBuffer();
  STUB;
}
void SoStream::resetBuffer()
{
  assert(streamType == MEMORY && "You can perform resetBuffer() only on memory streams.");
  bufPos = 0;
}
void SoStream::emptyBuffer(size_t streamSize)
{
  closeStream();
  streamType = MEMORY;
  if (streamSize == 0) {
    buffer = NULL;
  } else {
    buffer = static_cast<char*>(malloc(streamSize));
  }
  bufferSize = 0;
  bufferAllocSize = streamSize;
  bufPos = 0;
}



void SoStream::loadBufferFromFile(FILE *fp)
{
  this->emptyBuffer();
  SoStream temp(fp);
  this->readFromStream(temp);
  setPos(0);
}
void SoStream::loadBufferFromFile(const char *const fileName)
{
  this->emptyBuffer();
  SoStream temp(fileName);
  this->readFromStream(temp);
  setPos(0);
}
void SoStream::storeBufferToFile(FILE *fp)
{
  size_t cpos = getPos();
  setPos(0);
  SoStream temp(fp);
  temp.readFromStream(*this);
  setPos(cpos);
}
void SoStream::storeBufferToFile(const char *const fileName)
{
  size_t cpos = getPos();
  setPos(0);
  SoStream temp(fileName);
  temp.readFromStream(*this);
  setPos(cpos);
}



void SoStream::commonInit()
{
  streamType = CLOSED;
  binaryStream = FALSE;
  badBit = FALSE;
  readable = writeable = TRUE;
  endianOrdering = BIG_ENDIAN_STREAM;
  updateNeedEndianConversion();
}

SoStream::SoStream()
{
  commonInit();
}

SoStream::SoStream(size_t streamSize)
{
  commonInit();
  emptyBuffer(streamSize);
}

SoStream::SoStream(FILE *fp)
{
  commonInit();
  setFilePointer(fp);
}

SoStream::SoStream(const char *const fileName)
{
  commonInit();
  openFile(fileName);
}

SoStream::~SoStream()
{
  closeStream();
}


SoStream::StreamType SoStream::getStreamType() const  { return streamType; }

void SoStream::closeStream()
{
  switch (streamType) {
  case FILE_STREAM:
    fclose(filep);
    break;
  case MEMORY:
    free(buffer);
    break;
  default:
    break;
  }
  streamType = CLOSED;
  clearBadBit();
}



void SoStream::setFilePointer(FILE *newFP)
{
  closeStream();
  streamType = FILE_STREAM;
  filep = newFP;
}
FILE* SoStream::getFilePointer()  const
{
  assert(streamType == FILE_STREAM);
  return static_cast<FILE*>(filep);
}

SbBool SoStream::openFile(const char *const fileName)
{
  FILE *f = fopen(fileName, "r+b");
  setFilePointer(f);
  return TRUE;
}
void SoStream::closeFile()
{
  STUB;
}



void SoStream::wrapSoInput(SoInput *input)
{
  SoStream::closeStream();
  streamType = SO_INPUT_WRAP;
  soinput = input;
}

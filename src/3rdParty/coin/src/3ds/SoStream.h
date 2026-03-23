#ifndef COIN_SOSTREAM_H
#define COIN_SOSTREAM_H

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

#include <Inventor/system/inttypes.h>
#include <Inventor/C/basic.h>
#include <cstdio>

typedef void* SoStreamReallocCB(void *ptr, size_t newSize);

class SoInput;


class SoStream {
public:

  // Warning: Do not use chars together with << and >> operators, probably due to how
  // some old compilers (like gcc 2.96) treat internal chars as int32_t instead of
  // (u)int8_t (sizeof still returns 1). This causes char to be written to the stream
  // as four bytes instead of one. Also do not try to append overloaded << and >> char
  // operators, because it makes it impossible to compile the code on some compilers.

  inline SoStream& operator >> (int8_t &value)       { readInt8(value); return *this; }
  inline SoStream& operator << (const int8_t value)  { writeInt8(value); return *this; }
  inline SoStream& operator >> (uint8_t &value)      { readUInt8(value); return *this; }
  inline SoStream& operator << (const uint8_t value) { writeUInt8(value); return *this; }

  inline SoStream& operator >> (int16_t &value)       { readInt16(value); return *this; }
  inline SoStream& operator << (const int16_t value)  { writeInt16(value); return *this; }
  inline SoStream& operator >> (uint16_t &value)      { readUInt16(value); return *this; }
  inline SoStream& operator << (const uint16_t value) { writeUInt16(value); return *this; }

  inline SoStream& operator >> (int32_t &value)       { readInt32(value); return *this; }
  inline SoStream& operator << (const int32_t value)  { writeInt32(value); return *this; }
  inline SoStream& operator >> (uint32_t &value)      { readUInt32(value); return *this; }
  inline SoStream& operator << (const uint32_t value) { writeUInt32(value); return *this; }

  inline SoStream& operator >> (float &value)      { readFloat(value); return *this; }
  inline SoStream& operator << (const float value) { writeFloat(value); return *this; }

  inline SoStream& operator << (const double value) { writeDouble(value); return *this; }
  inline SoStream& operator >> (double &value)      { readDouble(value); return *this; }

  inline SoStream& operator >> (SoStream &stream)             { readStream(stream); return *this; }
  inline SoStream& operator << (const SoStream &stream) { writeStream(stream); return *this; }

// sbstring
// sbname
// arrays...

  inline SbBool readChar(char &value)  { return readUInt8(*reinterpret_cast<uint8_t*>(&value)); }
  inline SbBool writeChar(const char value)  { return writeUInt8(value); }
  inline SbBool readSChar(signed char &value)  { return readInt8(*reinterpret_cast<int8_t*>(&value)); }
  inline SbBool writeSChar(const signed char value)  { return writeInt8(value); }
  inline SbBool readUChar(unsigned char &value)  { return readUInt8(*reinterpret_cast<uint8_t*>(&value)); }
  inline SbBool writeUChar(const unsigned char value)  { return writeUInt8(value); }

  virtual SbBool readInt8(int8_t &value);
  virtual SbBool writeInt8(const int8_t value);
  virtual SbBool readUInt8(uint8_t &value);
  virtual SbBool writeUInt8(const uint8_t value);

  virtual SbBool readInt16(int16_t &value);
  virtual SbBool writeInt16(const int16_t value);
  virtual SbBool readUInt16(uint16_t &value);
  virtual SbBool writeUInt16(const uint16_t value);

  virtual SbBool readInt32(int32_t &value);
  virtual SbBool writeInt32(const int32_t value);
  virtual SbBool readUInt32(uint32_t &value);
  virtual SbBool writeUInt32(const uint32_t value);

  virtual SbBool readFloat(float &value);
  virtual SbBool writeFloat(const float value);

  virtual SbBool readDouble(double &value);
  virtual SbBool writeDouble(const double value);

  virtual SbBool readZString(char *buf, int bufSize);
  virtual SbBool writeZString(const char *const buf);

  virtual size_t readBuffer(void *buf, size_t bufSize);
  virtual size_t writeBuffer(void *buf, size_t bufSize);

  virtual SbBool readFromStream(SoStream &stream);
  virtual SbBool writeToStream(SoStream &stream);
  virtual size_t readFromStream(SoStream &stream, size_t bytes);
  virtual size_t writeToStream(SoStream &stream, size_t bytes);

  virtual SbBool readStream(SoStream &stream);
  virtual SbBool writeStream(const SoStream &stream);


  virtual void setPos(size_t pos);
  virtual size_t getPos() const;
  virtual size_t getSize() const;


  enum StreamType { CLOSED, MEMORY, FILE_STREAM, SO_INPUT_WRAP };
  enum StreamEndianOrdering { BIG_ENDIAN_STREAM, LITTLE_ENDIAN_STREAM, HOST_DEP };

  SoStream();
  SoStream(size_t streamSize);
  SoStream(FILE *fp);
  SoStream(const char *const fileName);
  virtual ~SoStream();

  virtual StreamType getStreamType() const;
  virtual void closeStream();
  virtual void wrapSoInput(SoInput *input);

  virtual void setFilePointer(FILE *newFP);
  virtual FILE* getFilePointer() const;
  virtual SbBool openFile(const char *const fileName);
  virtual void closeFile();

  virtual void setBuffer(void *buf, size_t size);
  virtual SbBool getBuffer(void *&buf, size_t &size) const;
  virtual size_t getBufferSize() const;
  virtual void resetBuffer();
  virtual void emptyBuffer(size_t streamSize = 0);

  virtual void loadBufferFromFile(FILE *fp);
  virtual void loadBufferFromFile(const char *const fileName);
  virtual void storeBufferToFile(FILE *fp);
  virtual void storeBufferToFile(const char *const fileName);

  virtual void setBinary(const SbBool flag);
  virtual SbBool isBinary() const;
  virtual void setAccessRights(SbBool readEnabled, SbBool writeEnabled);
  virtual SbBool isReadable() const;
  virtual SbBool isWriteable() const;
  virtual void setEndianOrdering(const StreamEndianOrdering value);
  virtual StreamEndianOrdering getEndianOrdering() const;
  static StreamEndianOrdering getHostEndianOrdering();

  virtual void setBadBit();
  virtual void clearBadBit();
  virtual SbBool isBad() const;

protected:
  virtual size_t readBinaryArray(void *buf, size_t bufSize);
  virtual size_t writeBinaryArray(void *buf, size_t bufSize);
  virtual SbBool getChar(char &c);
  virtual void ungetChar(const char c);
  virtual void putChar(const char c);

  virtual SbBool reallocBuffer(size_t newSize);

  SbBool readDigInt(char *s, const char *e);
  SbBool readHexInt(char *s, const char *e);
private:
  StreamType streamType;
  SbBool binaryStream;
  SbBool badBit;
  union {
    FILE *filep;
    char *buffer;
    SoInput *soinput;
  };
  size_t bufferSize;
  size_t bufferAllocSize;
  size_t bufPos;
  SbBool readable, writeable;
  StreamEndianOrdering endianOrdering;
  SbBool needEndianConversion;
  SoStreamReallocCB *reallocCallback;

  void commonInit();
  void updateNeedEndianConversion();
};


#endif /* !COIN_SOSTREAM_H */

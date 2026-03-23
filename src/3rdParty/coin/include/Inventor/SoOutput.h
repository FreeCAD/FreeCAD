#ifndef COIN_SOOUTPUT_H
#define COIN_SOOUTPUT_H

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
#include <Inventor/SbBasic.h>
#include <Inventor/SbString.h>
#include <cstdio>

class SbDict;
class SbName;
class SoBase;
class SoOutputP;
class SoProto;
class SoField;
class SoFieldContainer;

typedef void * SoOutputReallocCB(void * ptr, size_t newSize);

class COIN_DLL_API SoOutput {
public:
  enum Stage { COUNT_REFS, WRITE };
  // Bitwise flags for the annotations.
  enum Annotations { ADDRESSES = 0x01, REF_COUNTS = 0x02 };

  SoOutput(void);
  SoOutput(SoOutput * dictOut);
  virtual ~SoOutput();

  virtual void setFilePointer(FILE * newFP);
  virtual FILE * getFilePointer(void) const;
  virtual SbBool openFile(const char * const fileName);
  virtual void closeFile(void);

  SbBool setCompression(const SbName & compmethod,
                        const float level = 0.5f);
  static const SbName * getAvailableCompressionMethods(unsigned int & num);

  virtual void setBuffer(void * bufPointer, size_t initSize,
                         SoOutputReallocCB * reallocFunc, int32_t offset = 0);
  virtual SbBool getBuffer(void * & bufPointer, size_t & nBytes) const;
  virtual size_t getBufferSize(void) const;
  virtual void resetBuffer(void);
  virtual void setBinary(const SbBool flag);
  virtual SbBool isBinary(void) const;
  virtual void setHeaderString(const SbString & str);
  virtual void resetHeaderString(void);
  virtual void setFloatPrecision(const int precision);

  void setStage(Stage stage);
  Stage getStage(void) const;

  void incrementIndent(const int levels = 1);
  void decrementIndent(const int levels = 1);

  virtual void write(const char c);
  virtual void write(const char * s);
  virtual void write(const SbString & s);
  virtual void write(const SbName & n);
  virtual void write(const int i);
  virtual void write(const unsigned int i);
  virtual void write(const short s);
  virtual void write(const unsigned short s);
  virtual void write(const float f);
  virtual void write(const double d);
#ifdef __CYGWIN__
  //These function are not virtual as they are meant to be only wrappers to the real function calls, due to limitations in Cygwin g++ type demangling.
  void write(long int i);
  void write(long unsigned int i);
#endif //__CYGWIN__
  virtual void writeBinaryArray(const unsigned char * c, const int length);
  virtual void writeBinaryArray(const int32_t * const l, const int length);
  virtual void writeBinaryArray(const float * const f, const int length);
  virtual void writeBinaryArray(const double * const d, const int length);

  virtual void indent(void);
  virtual void reset(void);
  void setCompact(SbBool flag);
  SbBool isCompact(void) const;
  void setAnnotation(uint32_t bits);
  uint32_t getAnnotation(void);

  static SbString getDefaultASCIIHeader(void);
  static SbString getDefaultBinaryHeader(void);

  int addReference(const SoBase * base);
  int findReference(const SoBase * base) const;
  void setReference(const SoBase * base, int refid);

  void addDEFNode(SbName name);
  SbBool lookupDEFNode(SbName name);
  void removeDEFNode(SbName name);

  void pushProto(SoProto * proto);
  SoProto * getCurrentProto(void) const;
  void popProto(void);
  
  void addRoute(SoFieldContainer * from, const SbName & fromfield,
                SoFieldContainer * to, const SbName & tofield);
  void resolveRoutes(void);
  
protected:
  SbBool isToBuffer(void) const;
  size_t bytesInBuf(void) const;
  SbBool makeRoomInBuf(size_t nBytes);
  void convertShort(short s, char * to);
  void convertInt32(int32_t l, char * to);
  void convertFloat(float f, char * to);
  void convertDouble(double d, char * to);
  void convertShortArray(short * from, char * to, int len);
  void convertInt32Array(int32_t * from, char * to, int len);
  void convertFloatArray(float * from, char * to, int len);
  void convertDoubleArray(double * from, char * to, int len);

  static SbString padHeader(const SbString & inString);

  SbBool wroteHeader;

private:
  SoOutputP * pimpl;

  void constructorCommon(void);

  void checkHeader(void);
  void writeBytesWithPadding(const char * const p, const size_t nr);
  
  friend class SoBase; // Need to be able to remove items from dict.
  friend class SoWriterefCounter; // ditto
  void removeSoBase2IdRef(const SoBase * base);
};

#endif // !COIN_SOOUTPUT_H

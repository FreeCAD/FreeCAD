#ifndef COIN_SOINPUT_H
#define COIN_SOINPUT_H

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
#include <Inventor/lists/SbList.h>
#include <cstdio> // FILE
#ifndef COIN_INTERNAL
 // For Open Inventor compatibility.
 #include <Inventor/SoDB.h>
#endif // COIN_INTERNAL

// *************************************************************************

class SbDict;
class SoBase;
class SbString;
class SbTime;
class SbName;
class SbStringList;
class SoInput_FileInfo;
class SoProto;
class SoField;
class SoFieldContainer;
class SoInputP;

// *************************************************************************

class COIN_DLL_API SoInput {
public:
  SoInput(void);
  SoInput(SoInput * dictIn);

  SoProto * findProto(const SbName & name);
  void addProto(SoProto * proto);
  void pushProto(SoProto * proto);
  SoProto * getCurrentProto(void) const;
  void popProto(void);

  void addRoute(const SbName & fromnode, const SbName & fromfield,
                const SbName & tonode, const SbName & tofield);
  SbBool checkISReference(SoFieldContainer * container, const SbName & fieldname,
                          SbBool & readok);

  virtual ~SoInput(void);

  virtual void setFilePointer(FILE * newFP);
  virtual SbBool openFile(const char * fileName, SbBool okIfNotFound = FALSE);
  virtual SbBool pushFile(const char * fileName);
  virtual void closeFile(void);
  virtual SbBool isValidFile(void);
  virtual SbBool isValidBuffer(void);
  virtual FILE * getCurFile(void) const;
  virtual const char * getCurFileName(void) const;
  virtual void setBuffer(const void * bufpointer, size_t bufsize);
          void setStringArray(const char * strings[]);
  virtual size_t getNumBytesRead(void) const;
  virtual SbString getHeader(void);
  virtual float getIVVersion(void);
  virtual SbBool isBinary(void);

  virtual SbBool get(char & c);
  virtual SbBool getASCIIBuffer(char & c);
  virtual SbBool getASCIIFile(char & c);
  virtual SbBool readHex(uint32_t & l);
  virtual SbBool read(char & c);
  virtual SbBool read(char & c, SbBool skip);
  virtual SbBool read(SbString & s);
  virtual SbBool read(SbName & n, SbBool validIdent = FALSE);
  virtual SbBool read(int & i);
  virtual SbBool read(unsigned int & i);
  virtual SbBool read(short & s);
  virtual SbBool read(unsigned short & s);
  virtual SbBool read(float & f);
  virtual SbBool read(double & d);
  virtual SbBool readByte(int8_t & b);
  virtual SbBool readByte(uint8_t & b);
#ifdef __CYGWIN__
  //These function are not virtual as they are meant to be only wrappers to the real function calls, due to limitations in Cygwin g++ type demangling.
  SbBool read(long int & i);
  SbBool read(long unsigned int & i);
#endif //__CYGWIN__
  virtual SbBool readBinaryArray(unsigned char * c, int length);
  virtual SbBool readBinaryArray(int32_t * l, int length);
  virtual SbBool readBinaryArray(float * f, int length);
  virtual SbBool readBinaryArray(double * d, int length);
  virtual SbBool eof(void) const;

  SbBool isFileVRML1(void);
  SbBool isFileVRML2(void);
  virtual void resetFilePointer(FILE * fptr);

  virtual void getLocationString(SbString & string) const;
  virtual void putBack(const char c);
  virtual void putBack(const char * str);
  virtual void addReference(const SbName & name, SoBase * base,
                            SbBool addToGlobalDict = TRUE);
  virtual void removeReference(const SbName & name);
  virtual SoBase * findReference(const SbName & name) const;

  static void addDirectoryFirst(const char * dirName);
  static void addDirectoryLast(const char * dirName);
  static void addEnvDirectoriesFirst(const char * envVarName,
                                     const char * separator = ":\t ");
  static void addEnvDirectoriesLast(const char * envVarName,
                                    const char * separator = ":\t ");
  static void removeDirectory(const char * dirName);
  static void clearDirectories(void);
  static const SbStringList & getDirectories(void);

  static void init(void);

  static SbString getPathname(const char * const filename);
  static SbString getPathname(const SbString & s);
  static SbString getBasename(const char * const filename);
  static SbString getBasename(const SbString & s);

  static SbString searchForFile(const SbString & basename,
                                const SbStringList & directories,
                                const SbStringList & subdirectories);


protected:
  virtual SbBool popFile(void);
  void setIVVersion(float version);
  FILE * findFile(const char * fileName, SbString & fullName);
  void initFile(FILE * newFP, const char * fileName, SbString * fullName,
                SbBool openedHere, SbDict * refDict = NULL);
  SbBool checkHeader(SbBool bValidateBufferHeader = FALSE);
  SbBool fromBuffer(void) const;
  SbBool skipWhiteSpace(void);
  size_t freeBytesInBuf(void) const;
  SbBool readInteger(int32_t & l);
  SbBool readUnsignedInteger(uint32_t & l);
  SbBool readReal(double & d);
  SbBool readUnsignedIntegerString(char * str);
  int readDigits(char * str);
  int readHexDigits(char * str);
  int readChar(char * str, char charToRead);

  SbBool makeRoomInBuf(size_t nBytes);
  void convertShort(char * from, short * s);
  void convertInt32(char * from, int32_t * l);
  void convertFloat(char * from, float * f);
  void convertDouble(char * from, double * d);
  void convertShortArray(char * from, short * to, int len);
  void convertInt32Array(char * from, int32_t * to, int len);
  void convertFloatArray(char * from, float * to, int len);
  void convertDoubleArray(char * from, double * to, int len);
  SbBool isFileURL(const char * url);
  char * URLToFile(char * out_buf, const char * in_buf);
  SbBool IsURL(const char * c_strng);

  static void setDirectories(SbStringList * dirs);

private:
  friend class SoDB;
  friend class SoInputP;

  static void clean(void);
  void constructorsCommon(void);

  static void addDirectoryIdx(const int idx, const char * dirName);
  static void addEnvDirectoriesIdx(int startidx, const char * envVarName,
                                   const char * separator);
  static SbStringList * dirsearchlist;

  SbList<SoInput_FileInfo *> filestack;
  SoInput_FileInfo * getTopOfStack(void) const {
    return this->filestack[0];
  }

  SoInputP * pimpl;
};

#endif // !COIN_SOINPUT_H

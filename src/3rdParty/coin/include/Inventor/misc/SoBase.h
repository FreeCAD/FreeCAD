#ifndef COIN_SOBASE_H
#define COIN_SOBASE_H

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

#include <Inventor/SoType.h>
#include <Inventor/lists/SoAuditorList.h>
#include <Inventor/C/base/rbptree.h>

class SbString;
class SoBaseList;
class SoInput;
class SoOutput;

class COIN_DLL_API SoBase {
public:
  static void initClass(void);

  void ref(void) const;
  void unref(void) const;
  void unrefNoDelete(void) const;
  int32_t getRefCount(void) const;

  void touch(void);

  virtual SoType getTypeId(void) const = 0;
  SbBool isOfType(SoType type) const;
  static SoType getClassTypeId(void);

  virtual SbName getName(void) const;
  virtual void setName(const SbName & newname);

  static void addName(SoBase * const base, const char * const name);
  static void removeName(SoBase * const base, const char * const name);

  virtual void startNotify(void);
  virtual void notify(SoNotList * l);

  void addAuditor(void * const auditor, const SoNotRec::Type type);
  void removeAuditor(void * const auditor, const SoNotRec::Type type);
  const SoAuditorList & getAuditors(void) const;

  virtual void addWriteReference(SoOutput * out, SbBool isfromfield = FALSE);
  SbBool shouldWrite(void);

  static void incrementCurrentWriteCounter(void);
  static void decrementCurrentWriteCounter(void);

  static SoBase * getNamedBase(const SbName & name, SoType type);
  static int getNamedBases(const SbName & name, SoBaseList & baselist,
                           SoType type);

  static SbBool read(SoInput * input, SoBase *& base, SoType expectedtype);
  static void setInstancePrefix(const SbString & c);

  static void setTraceRefs(SbBool trace);
  static SbBool getTraceRefs(void);

  static SbBool connectRoute(SoInput * input,
                             const SbName & fromnodename, const SbName & fromfieldname,
                             const SbName & tonodename, const SbName & tofieldname);

  void assertAlive(void) const;
  static SbBool readRoute(SoInput * input);

protected:
  // Note: these are bitflags.
  enum BaseFlags { IS_ENGINE = 0x01, IS_GROUP = 0x02 };

  SoBase(void);
  virtual ~SoBase();

  virtual void destroy(void);

  SbBool hasMultipleWriteRefs(void) const;
  SbBool writeHeader(SoOutput * out, SbBool isgroup, SbBool isengine) const;
  void writeFooter(SoOutput * out) const;
  virtual const char * getFileFormatName(void) const;

  virtual SbBool readInstance(SoInput * input, unsigned short flags) = 0;

  static uint32_t getCurrentWriteCounter(void);
  static void staticDataLock(void);
  static void staticDataUnlock(void);

  virtual SoNotRec createNotRec(void);

private:
  static void cleanClass(void);

  static SoType classTypeId;

  struct {
    mutable signed int referencecount : 28;
    mutable unsigned int alive : 4;
  } objdata;

  void doNotify(SoNotList * l, const void * auditor, const SoNotRec::Type type);
  cc_rbptree auditortree;

  class PImpl;
  friend class PImpl; // MSVC6
};

#endif // !COIN_SOBASE_H

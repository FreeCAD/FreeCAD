#ifndef COIN_SOFIELDCONTAINER_H
#define COIN_SOFIELDCONTAINER_H

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

#include <Inventor/misc/SoBase.h>

class SbString;
class SoFieldData;
class SoFieldList;
class SoOutput;

class COIN_DLL_API SoFieldContainer : public SoBase {
  typedef SoBase inherited;

public:
  static void initClass(void);
  static SoType getClassTypeId(void);
  static void cleanupClass(void);

  void setToDefaults(void);
  SbBool hasDefaultValues(void) const;

  SbBool fieldsAreEqual(const SoFieldContainer * container) const;
  void copyFieldValues(const SoFieldContainer * container,
                       SbBool copyconnections = FALSE);

  SbBool set(const char * const fielddata);
  void get(SbString & fielddata);

  virtual int getFields(SoFieldList & l) const;
  virtual int getAllFields(SoFieldList & l) const;
  virtual SoField * getField(const SbName & name) const;
  virtual SoField * getEventIn(const SbName & name) const;
  virtual SoField * getEventOut(const SbName & name) const;
  SbBool getFieldName(const SoField * const field, SbName & name) const;

  SbBool enableNotify(const SbBool flag);
  SbBool isNotifyEnabled(void) const;

  SbBool set(const char * fielddata, SoInput * input);
  void get(SbString & fielddata, SoOutput * out);

  virtual void notify(SoNotList * l);

  virtual SbBool validateNewFieldValue(SoField * field, void * newval);

  virtual void addWriteReference(SoOutput * out, SbBool isfromfield = FALSE);
  virtual void writeInstance(SoOutput * out);

  SbBool getIsBuiltIn(void) const;
  virtual const SoFieldData * getFieldData(void) const;

  virtual void copyContents(const SoFieldContainer * from,
                            SbBool copyconnections);
  virtual SoFieldContainer * copyThroughConnection(void) const;

  static void initCopyDict(void);
  static void addCopy(const SoFieldContainer * orig,
                      const SoFieldContainer * copy);
  static SoFieldContainer * checkCopy(const SoFieldContainer * orig);
  static SoFieldContainer * findCopy(const SoFieldContainer * orig,
                                     const SbBool copyconnections);
  static void copyDone(void);

  virtual void getFieldsMemorySize(size_t & managed, size_t & unmanaged) const;

  void setUserData(void * userdata) const;
  void * getUserData(void) const;

protected:
  SoFieldContainer(void);
  virtual ~SoFieldContainer();

  virtual SbBool readInstance(SoInput * in, unsigned short flags);
  SbBool isBuiltIn;

private:
  static SoType classTypeId;
  SbBool donotify;

}; // SoFieldContainer

#endif // !COIN_SOFIELDCONTAINER_H

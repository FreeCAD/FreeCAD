#ifndef COIN_SOGLOBALFIELD_H
#define COIN_SOGLOBALFIELD_H

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
#endif /* !COIN_INTERNAL */

#include <Inventor/fields/SoFieldContainer.h>

class SoGlobalField : public SoFieldContainer {
  typedef SoFieldContainer inherited;

public:
  SoGlobalField(const SbName & name, SoField * field);

  static void initClass(void);
  static SoType getClassTypeId(void);
  virtual SoType getTypeId(void) const;

  SoField * getGlobalField(void) const;
  virtual const SoFieldData * getFieldData(void) const;

  virtual void setName(const SbName & newname);

  virtual void addWriteReference(SoOutput * out, SbBool isfromfield = FALSE);
  virtual void writeInstance(SoOutput * out);

  static SoGlobalField * getGlobalFieldContainer(const SbName & name);
  static void addGlobalFieldContainer(SoGlobalField * fieldcontainer);
  static void removeGlobalFieldContainer(SoGlobalField * fieldcontainer);
  static SoBaseList * getGlobalFieldContainers(void);

protected:
  ~SoGlobalField();

  virtual SbBool readInstance(SoInput * in, unsigned short flags);

private:
  static void * createInstance(void);
  static void clean(void);
  static int getGlobalFieldIndex(const SbName & name);

  static SoType classTypeId;
  SoFieldData * classfielddata;
  static SoBaseList * allcontainers;
};

#endif // !COIN_SOGLOBALFIELD_H

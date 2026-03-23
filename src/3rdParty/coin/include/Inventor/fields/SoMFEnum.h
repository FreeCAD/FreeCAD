#ifndef COIN_SOMFENUM_H
#define COIN_SOMFENUM_H

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

#include <Inventor/fields/SoMField.h>
#include <Inventor/fields/SoSubField.h>

class COIN_DLL_API SoMFEnum : public SoMField {
  typedef SoMField inherited;

  SO_MFIELD_HEADER(SoMFEnum, int, int);

public:
  static void initClass(void);

  void setValue(const SbName name);
  void set1Value(const int idx, const SbName name);
  void setEnums(const int num, const int * const values,
                const SbName * const names);

  int getNumEnums(void) const;
  int getEnum(const int idx, SbName & name) const;

protected:
  virtual SbBool findEnumValue(const SbName & name, int & value);
  virtual SbBool findEnumName(int value, const SbName * & name) const;

  SbBool legalValuesSet;
  int numEnums;
  int * enumValues;
  SbName * enumNames;
};

// Macro definitions. See comment in SoSFEnum.h.

#include <Inventor/fields/SoSFEnum.h> // PRIVATE_SOENUM_SET_TYPE()

#define SO_ENGINE_SET_MF_ENUM_TYPE(_fieldname_, _enumtype_) \
  PRIVATE_SOENUM_SET_TYPE(_fieldname_, _enumtype_, inputdata)

#define SO_NODE_SET_MF_ENUM_TYPE(_fieldname_, _enumtype_) \
  PRIVATE_SOENUM_SET_TYPE(_fieldname_, _enumtype_, fieldData)

#endif // !COIN_SOMFENUM_H

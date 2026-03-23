#ifndef COIN_SOSFENUM_H
#define COIN_SOSFENUM_H

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

#include <Inventor/fields/SoSField.h>
#include <Inventor/fields/SoSubField.h>
#include <Inventor/SbString.h>

class COIN_DLL_API SoSFEnum : public SoSField {
  typedef SoSField inherited;

  SO_SFIELD_HEADER(SoSFEnum, int, int);

public:
  static void initClass(void);

  void setValue(const SbName name);
  void setEnums(const int num, const int * vals, const SbName * names);

  int getNumEnums(void) const;
  int getEnum(const int idx, SbName & name) const;

protected:
  virtual SbBool findEnumValue(const SbName & name, int & val);
  virtual SbBool findEnumName(int value, const SbName * & name) const;

  int numEnums;
  int * enumValues;
  SbName * enumNames;
  SbBool legalValuesSet;
};

// These macros are called from both node and engine classes which
// uses SoSFEnum instances, so we put them here instead of in
// SoSub[Node|Engine].h.

#define PRIVATE_SOENUM_SET_TYPE(_fieldname_, _enumtype_, _fieldstorage_) \
  do { \
    /* Don't need to worry about name clashes, as we're inside our own scope. */ \
    int num; \
    const int * values; \
    const SbName * names; \
    _fieldstorage_->getEnumData(SO__QUOTE(_enumtype_), num, values, names); \
    this->_fieldname_.setEnums(num, values, names); \
  } WHILE_0


#define SO_ENGINE_SET_SF_ENUM_TYPE(_fieldname_, _enumtype_) \
  PRIVATE_SOENUM_SET_TYPE(_fieldname_, _enumtype_, inputdata)

#define SO_NODE_SET_SF_ENUM_TYPE(_fieldname_, _enumtype_) \
  PRIVATE_SOENUM_SET_TYPE(_fieldname_, _enumtype_, fieldData)

#endif // !COIN_SOSFENUM_H

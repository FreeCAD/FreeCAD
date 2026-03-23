#ifndef COIN_SONOTREC_H
#define COIN_SONOTREC_H

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

#include <Inventor/SbBasic.h>
#include <cstdio>

class SoBase;


class COIN_DLL_API SoNotRec {
public:
  enum Type {
    CONTAINER,
    PARENT,
    SENSOR,
    FIELD,
    ENGINE
  };

  enum OperationType {
    UNSPECIFIED,
    FIELD_UPDATE,
    GROUP_ADDCHILD,
    GROUP_INSERTCHILD,
    GROUP_REPLACECHILD,
    GROUP_REMOVECHILD,
    GROUP_REMOVEALLCHILDREN
  };

  SoNotRec(SoBase * const notifbase);
  void setType(const SoNotRec::Type type);
  SoBase * getBase(void) const;
  SoNotRec::Type getType(void) const;
  const SoNotRec * getPrevious(void) const;
  void setPrevious(const SoNotRec * const prev);
  void print(FILE * const file) const;
  SoNotRec::OperationType getOperationType(void) const;
  int getIndex(void) const;
  int getFieldNumIndices(void) const;
  const SoBase * getGroupChild(void) const;
  const SoBase * getGroupPrevChild(void) const;
  void setOperationType(const SoNotRec::OperationType opType);
  void setIndex(const int idx);
  void setFieldNumIndices(const int fldnumind);
  void setGroupChild(const SoBase * const gc);
  void setGroupPrevChild(const SoBase * const pc);

private:
  Type type;
  SoBase * base;
  const SoNotRec * prev;
  int index, fieldNumIndices;
  OperationType operationType;
  const SoBase * groupChild;
  const SoBase * groupPrevChild;
};

#endif // !COIN_SONOTREC_H

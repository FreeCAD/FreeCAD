#ifndef COIN_SOVERTEXATTRIBUTE_H
#define COIN_SOVERTEXATTRIBUTE_H

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

#include <Inventor/nodes/SoNode.h>
#include <Inventor/nodes/SoSubNode.h>
#include <Inventor/elements/SoVertexAttributeElement.h>
#include <Inventor/fields/SoSFName.h>
#include <Inventor/fields/SoMField.h>
#include <Inventor/fields/SoSFEnum.h>
#include <Inventor/tools/SbPimplPtr.h>

class SoMField;
class SoVertexAttributeP;

class COIN_DLL_API SoVertexAttribute : public SoNode {
  typedef SoNode inherited;

public:
  SoVertexAttribute(void);
  static void initClass(void);

  static SoType getClassTypeId(void);
  virtual SoType getTypeId(void) const;

  SoSFName name;
  SoSFName typeName;

  SoMField * getValuesField(void) const;

  virtual void doAction(SoAction * action);
  virtual void GLRender(SoGLRenderAction * action);
  virtual void write(SoWriteAction * action);
  virtual void copyContents(const SoFieldContainer * from, 
                            SbBool copyconnections); 
  virtual void notify(SoNotList * l);

protected:
  virtual ~SoVertexAttribute(void);

  virtual SbBool readInstance(SoInput * in, unsigned short flags);

private:
  static SoType classTypeId;
  static void * createInstance(void);
  virtual const SoFieldData * getFieldData(void) const;

  void initFieldData(void);

  SoVertexAttribute(const SoVertexAttribute & rhs); // N/A
  SoVertexAttribute & operator = (const SoVertexAttribute & rhs); // N/A

  SbPimplPtr<SoVertexAttributeP> pimpl;

}; // SoVertexAttribute

// *************************************************************************

//template <int Type>
//class SoAnyVertexAttribute : public SoVertexAttribute {
//public:
//
//private:
//  SoAnyVertexAttribute(void) { }
//
//}; // SoAnyVertexAttribute

// *************************************************************************

#endif // !COIN_SOVERTEXATTRIBUTE_H

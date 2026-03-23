#ifndef COIN_SOGLVERTEXATTRIBUTEELEMENT_H
#define COIN_SOGLVERTEXATTRIBUTEELEMENT_H

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

#include <Inventor/elements/SoVertexAttributeElement.h>
#include <Inventor/SbBasic.h>

class SoGLRenderAction;
class SoVertexAttributeData;

class COIN_DLL_API SoGLVertexAttributeElement : public SoVertexAttributeElement {
  typedef SoVertexAttributeElement inherited;
  SO_ELEMENT_HEADER(SoGLVertexAttributeElement);

public:
  static void initClass(void);
  virtual void init(SoState * state);

  void enableVBO(SoGLRenderAction * action) const;
  void disableVBO(SoGLRenderAction * action) const;
  void send(const int index) const;
  static const SoGLVertexAttributeElement * getInstance(SoState * const state);

protected:
  virtual ~SoGLVertexAttributeElement();
  virtual void addElt(SoVertexAttributeData * attribdata);
};

#endif // COIN_SOGLVERTEXATTRIBUTEELEMENT_H

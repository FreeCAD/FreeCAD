#ifndef COIN_SOGLMULTITEXTURECOORDINATEELEMENT_H
#define COIN_SOGLMULTITEXTURECOORDINATEELEMENT_H

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

#include <Inventor/elements/SoMultiTextureCoordinateElement.h>

class SoGLMultiTextureCoordinateElementP;
typedef void SoTexCoordTexgenCB(void * data);

class COIN_DLL_API SoGLMultiTextureCoordinateElement : public SoMultiTextureCoordinateElement {
  typedef SoMultiTextureCoordinateElement inherited;

  SO_ELEMENT_HEADER(SoGLMultiTextureCoordinateElement);
public:
  static void initClass(void);
protected:
  virtual ~SoGLMultiTextureCoordinateElement();

public:
  virtual void init(SoState * state);
  virtual void push(SoState * state);
  virtual void pop(SoState * state,
                   const SoElement * prevTopElement);

  static  void setTexGen(SoState * const state, SoNode * const node,
                         const int unit,
                         SoTexCoordTexgenCB * const texgenFunc,
                         void * const texgenData = NULL,
                         SoTextureCoordinateFunctionCB * const func = NULL,
                         void * const funcData = NULL);

  virtual CoordType getType(const int unit = 0) const;

  static const SoGLMultiTextureCoordinateElement * getInstance(SoState * const state);

  void send(const int unit, const int index) const;
  void send(const int unit, const int index, const SbVec3f &c, const SbVec3f &n) const;

  class GLUnitData {
  public:
    GLUnitData() : texgenCB(NULL), texgenData(NULL) {}
    GLUnitData(const GLUnitData & org) : texgenCB(org.texgenCB), texgenData(org.texgenData) {}
    SoTexCoordTexgenCB * texgenCB;
    void * texgenData;
  };

  void initRender(const SbBool * enabled, const int maxenabled) const;

  // Coin-3 support
  void send(const int index) const {
    for (int i = 0; i <= this->multimax; i++) {
      if (this->multienabled[i]) {
        this->send(i, index);
      }
    }
  }
  void send(const int index, const SbVec3f &c, const SbVec3f &n) const {
    for (int i = 0; i <= this->multimax; i++) {
      if (this->multienabled[i]) {
        this->send(i, index, c, n);
      }
    }
  }
  void initMulti(SoState * state) const;
  static  void setTexGen(SoState * const state, SoNode * const node,
                         SoTexCoordTexgenCB * const texgenFunc,
                         void * const texgenData = NULL,
                         SoTextureCoordinateFunctionCB * const func = NULL,
                         void * const funcData = NULL) {
    setTexGen(state, node, 0, texgenFunc, texgenData, func, funcData);
  }

protected:
  virtual void setElt(const int unit,
                      SoTexCoordTexgenCB *func,
                      void *data = NULL);

private:
  void doCallback(const int unit) const;
  SoGLMultiTextureCoordinateElementP * pimpl;
  mutable const SbBool * multienabled;
  mutable int multimax;
};

#endif // !COIN_SOGLMULTITEXTURECOORDINATEELEMENT_H

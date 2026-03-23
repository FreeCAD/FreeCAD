#ifndef COIN_SOMULTITEXTUREMATRIXELEMENT_H
#define COIN_SOMULTITEXTUREMATRIXELEMENT_H

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

#include <Inventor/elements/SoAccumulatedElement.h>
#include <Inventor/SbMatrix.h>

class SoMultiTextureMatrixElementP;

class COIN_DLL_API SoMultiTextureMatrixElement : public SoAccumulatedElement {
  typedef SoAccumulatedElement inherited;

  SO_ELEMENT_HEADER(SoMultiTextureMatrixElement);
public:
  static void initClass(void);
protected:
  virtual ~SoMultiTextureMatrixElement();

public:
  virtual void init(SoState * state);
  virtual void push(SoState * state);
  static void set(SoState * const state, SoNode * const node,
                  const int unit,
                  const SbMatrix & matrix);
  static void mult(SoState * const state, SoNode * const node,
                   const int unit,
                   const SbMatrix & matrix);
  static const SbMatrix & get(SoState * const state, const int unit = 0);

  class UnitData {
  public:
    UnitData() : textureMatrix(SbMatrix::identity()) {}
    UnitData(const UnitData & org) : textureMatrix(org.textureMatrix) {}
    SbMatrix textureMatrix;
  };

protected:
  int getNumUnits() const;
  UnitData & getUnitData(const int unit);
  const UnitData & getUnitData(const int unit) const;

  virtual void multElt(const int unit, const SbMatrix & matrix);
  virtual void setElt(const int unit, const SbMatrix & matrix);
  virtual const SbMatrix & getElt(const int unit) const;

private:
  SoMultiTextureMatrixElementP * pimpl;

 public: // Coin-3 support
  static void makeIdentity(SoState * const state, SoNode * const node, const int unit = 0) {
    set(state, node, unit, SbMatrix::identity());
  }
  static void set(SoState * const state, SoNode * const node,
                  const SbMatrix & matrix) {
    set(state, node, 0, matrix);
  }
  static void mult(SoState * const state, SoNode * const node,
                   const SbMatrix & matrix) {
    mult(state, node, 0, matrix);
  }
  static void translateBy(SoState * const state, SoNode * const node,
                          const SbVec3f & translation) {
    SbMatrix m;
    m.setTranslate(translation);
    mult(state, node, 0, m);
  }
  static void rotateBy(SoState * const state, SoNode * const node,
                       const SbRotation & rotation) {
    SbMatrix m;
    m.setRotate(rotation);
    mult(state, node, 0, m);
  }
  static void scaleBy(SoState * const state, SoNode * const node,
                      const SbVec3f & scaleFactor) {
    SbMatrix m;
    m.setScale(scaleFactor);
    mult(state, node, 0, m);
  }
};

#endif // !COIN_SOTEXTUREMATRIXELEMENT_H

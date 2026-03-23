#ifndef COIN_SOLOCALBBOXMATRIXELEMENT_H
#define COIN_SOLOCALBBOXMATRIXELEMENT_H

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

#include <Inventor/elements/SoModelMatrixElement.h>

class COIN_DLL_API SoLocalBBoxMatrixElement : public SoElement {
  typedef SoElement inherited;

  SO_ELEMENT_HEADER(SoLocalBBoxMatrixElement);
public:
  static void initClass(void);
protected:
  virtual ~SoLocalBBoxMatrixElement();

public:
  virtual void init(SoState * state);

  virtual void push(SoState * state);

  static void makeIdentity(SoState * const state);

  static void set(SoState * const state, const SbMatrix & matrix);

  static void mult(SoState * const state, const SbMatrix & matrix);
  static void translateBy(SoState * const state,
                           const SbVec3f & translation);
  static void rotateBy(SoState * const state, const SbRotation & rotation);
  static void scaleBy(SoState * const state, const SbVec3f & scaleFactor);

  static SbMatrix pushMatrix(SoState * const state);
  static void popMatrix(SoState * const state, const SbMatrix & matrix);

  static void resetAll(SoState * const state);

  static const SbMatrix & get(SoState * const state);

  virtual SbBool matches(const SoElement * element) const;
  virtual SoElement * copyMatchInfo(void) const;

protected:
  SbMatrix localMatrix;

private:
  SbMatrix modelInverseMatrix;

};

#endif // !COIN_SOLOCALBBOXMATRIXELEMENT_H

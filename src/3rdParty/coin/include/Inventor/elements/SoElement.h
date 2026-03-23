#ifndef COIN_SOELEMENT_H
#define COIN_SOELEMENT_H

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
#include <Inventor/SoType.h>
#include <Inventor/misc/SoState.h>
#include <cstdio>

class SoNode;

class COIN_DLL_API SoElement {
public:
  static void initClass(void);

  static SoType getClassTypeId(void);
  static int getClassStackIndex(void);
  const SoType getTypeId(void) const;
  int getStackIndex(void) const;

  virtual void init(SoState * state);

  virtual void push(SoState * state);
  virtual void pop(SoState * state, const SoElement * prevTopElement);

  virtual SbBool matches(const SoElement * element) const = 0;
  virtual SoElement * copyMatchInfo(void) const = 0;

  static void initElements(void); // only for base class (SoElement)

  static int getNumStackIndices(void);
  static SoType getIdFromStackIndex(const int stackIndex);

  void setDepth(const int depth);

  int getDepth(void) const;

  virtual void print(FILE * file = stdout) const;
  virtual ~SoElement();

protected:
  SoElement(void);
  static int classStackIndex;

  static SoElement * getElement(SoState * const state, const int stackIndex);
  static const SoElement * getConstElement(SoState * const state, const int stackIndex);
  
  void capture(SoState * const state) const;

  virtual void captureThis(SoState * state) const;

  void setTypeId(const SoType typeId);
  void setStackIndex(const int index);

  SoType typeId;
  int stackIndex;

  static int createStackIndex(const SoType id);
  static SoTypeList * stackToType;

  int depth;

  SoElement * getNextInStack(void) const;
  SoElement * getNextFree(void) const;

private:

  static SoType classTypeId;

  friend class SoState; // FIXME: bad design. 19990629 mortene.
  static void cleanup(void);
  SoElement * nextup;
  SoElement * nextdown;
};

// inlined methods

inline SoElement *
SoElement::getElement(SoState * const state,
                      const int stackIndex)
{
  return state->getElement(stackIndex);
}

inline void 
SoElement::capture(SoState * const state) const
{
  if (state->isCacheOpen()) this->captureThis(state);
}

inline const SoElement *
SoElement::getConstElement(SoState * const state,
                           const int stackIndex)
{
  const SoElement * element = state->getConstElement(stackIndex);
  element->capture(state);
  return element;
}

#endif // !COIN_SOELEMENT_H

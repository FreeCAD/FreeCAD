#ifndef COIN_SODEPTHBUFFERELEMENT_H
#define COIN_SODEPTHBUFFERELEMENT_H

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

#include <Inventor/elements/SoElement.h>
#include <Inventor/elements/SoSubElement.h>
#include <Inventor/SbVec2f.h>

class COIN_DLL_API SoDepthBufferElement : public SoElement {
  typedef SoElement inherited;
  SO_ELEMENT_HEADER(SoDepthBufferElement);

public:
  static void initClass(void);

  enum DepthWriteFunction {
    NEVER,
    ALWAYS,
    LESS,
    LEQUAL,
    EQUAL,
    GEQUAL,
    GREATER,
    NOTEQUAL
  };

  static void set(SoState * state, SbBool test, SbBool write,
                  DepthWriteFunction function, SbVec2f range);

  static void get(SoState * state, SbBool & test_out, SbBool & write_out,
                  DepthWriteFunction & function_out, SbVec2f & range_out);

  static SbBool getTestEnable(SoState * state);
  static SbBool getWriteEnable(SoState * state);
  static DepthWriteFunction getFunction(SoState * state);
  static SbVec2f getRange(SoState * state);

  virtual void init(SoState * state);
  virtual void push(SoState * state);
  virtual void pop(SoState * state,
                   const SoElement * prevTopElement);

  virtual SbBool matches(const SoElement * element) const;
  virtual SoElement * copyMatchInfo(void) const;

protected:
  virtual ~SoDepthBufferElement();

  virtual void setElt(SbBool test, SbBool write,
                      DepthWriteFunction function, SbVec2f range);

  SbBool test;
  SbBool write;
  DepthWriteFunction function;
  SbVec2f range;

}; // SoDepthBufferElement

#endif // !COIN_SODEPTHBUFFERELEMENT_H

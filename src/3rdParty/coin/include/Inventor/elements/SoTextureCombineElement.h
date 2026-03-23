#ifndef COIN_SOTEXTURECOMBINEELEMENT_H
#define COIN_SOTEXTURECOMBINEELEMENT_H

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

#include <Inventor/elements/SoReplacedElement.h>
#include <Inventor/SbVec2s.h>
#include <Inventor/SbVec3s.h>
#include <Inventor/SbColor4f.h>

class SoState;
class SoTextureCombineElementP;

class COIN_DLL_API SoTextureCombineElement : public SoElement {
  typedef SoElement inherited;

  SO_ELEMENT_HEADER(SoTextureCombineElement);
public:
  static void initClass(void);
protected:
  virtual ~SoTextureCombineElement();

public:

  enum Source {
    PRIMARY_COLOR = 0x8577,
    TEXTURE = 0x1702,
    CONSTANT = 0x8576,
    PREVIOUS =  0x8578
  };
  enum Operand {
    SRC_COLOR = 0x0300,
    ONE_MINUS_SRC_COLOR = 0x0301,
    SRC_ALPHA = 0x0302,
    ONE_MINUS_SRC_ALPHA = 0x0303
  };
  enum Operation {
    REPLACE = 0x1E01,
    MODULATE = 0x2100,
    ADD = 0x0104,
    ADD_SIGNED = 0x8574,
    SUBTRACT = 0x84E7,
    INTERPOLATE = 0x8575,
    DOT3_RGB = 0x86AE,
    DOT3_RGBA = 0x86AF
  };

  virtual void init(SoState * state);

  static void set(SoState * const state, SoNode * const node,
                  const int unit,
                  const Operation rgboperation,
                  const Operation alphaoperation,
                  const Source * rgbsource,
                  const Source * alphasource,
                  const Operand * rgboperand,
                  const Operand * alphaoperand,
                  const SbColor4f & constantcolor,
                  const float rgbscale,
                  const float alphascale);

  static SbBool isDefault(SoState * const state,
                          const int unit);

  static void get(SoState * const state,
                  const int unit,
                  Operation & rgboperation,
                  Operation & alphaoperation,
                  Source * rgbsource,
                  Source * alphasource,
                  Operand * rgboperand,
                  Operand * alphaoperand,
                  SbColor4f & constantcolor,
                  float & rgbscale,
                  float & alphascale);

  virtual void push(SoState * state);
  virtual SbBool matches(const SoElement * elem) const;
  SoElement * copyMatchInfo(void) const;

  virtual void setElt(const int unit,
                      const SbUniqueId nodeid,
                      const Operation rgboperation,
                      const Operation alphaoperation,
                      const Source * rgbsource,
                      const Source * alphasource,
                      const Operand * rgboperand,
                      const Operand * alphaoperand,
                      const SbColor4f & constantcolor,
                      const float rgbscale,
                      const float alphascale);

  static void apply(SoState * state, const int unit);

  class UnitData {
  public:
    UnitData();
    UnitData(const UnitData & org);

    SbUniqueId nodeid;
    Source rgbsource[3];
    Source alphasource[3];
    Operand rgboperand[3];
    Operand alphaoperand[3];
    Operation rgboperation;
    Operation alphaoperation;
    SbColor4f constantcolor;
    float rgbscale;
    float alphascale;
  };

protected:
  const UnitData & getUnitData(const int unit) const;

private:
  SoTextureCombineElementP * pimpl;
};

#endif // !COIN_SOTEXTURECOMBINEELEMENT_H

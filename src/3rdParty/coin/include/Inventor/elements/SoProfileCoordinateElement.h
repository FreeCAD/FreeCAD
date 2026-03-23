#ifndef COIN_SOPROFILECOORDINATEELEMENT_H
#define COIN_SOPROFILECOORDINATEELEMENT_H

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
#include <Inventor/SbVec2f.h>
#include <Inventor/SbVec3f.h>
#include <cassert>

/*
 * TODO
 * - conversion between coord2 and coord3 (done in Inventor)
 */

class COIN_DLL_API SoProfileCoordinateElement : public SoReplacedElement {
  typedef SoReplacedElement inherited;

  SO_ELEMENT_HEADER(SoProfileCoordinateElement);
public:
  static void initClass(void);
protected:
  virtual ~SoProfileCoordinateElement();

public:
  virtual void init(SoState * state);
  static void set2(SoState * const state, SoNode * const node,
                   const int32_t numCoords, const SbVec2f * const coords);
  static void set3(SoState * const state, SoNode * const node,
                   const int32_t numCoords, const SbVec3f * const coords);
  static const SoProfileCoordinateElement * getInstance(SoState * const state);
  int32_t getNum(void) const;
  const SbVec2f & get2(const int index) const;
  const SbVec3f & get3(const int index) const;

  SbBool is2D(void) const;

  static SbVec2f getDefault2(void);
  static SbVec3f getDefault3(void);

  const SbVec2f * getArrayPtr2(void) const;
  const SbVec3f * getArrayPtr3(void) const;

protected:
  int32_t numCoords;
  const SbVec2f * coords2;
  const SbVec3f * coords3;
  SbBool coordsAre2D;

private:
  static void clean(void);
  static SbVec2f * initdefaultcoords;
};

#endif // !COIN_SOPROFILECOORDINATEELEMENT_H

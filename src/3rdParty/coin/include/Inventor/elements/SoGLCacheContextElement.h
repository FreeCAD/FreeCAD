#ifndef COIN_SOGLCACHECONTEXTELEMENT_H
#define COIN_SOGLCACHECONTEXTELEMENT_H

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

#include <Inventor/elements/SoSubElement.h>

// This shouldn't strictly be necessary, but the OSF1/cxx compiler
// complains if this is left out, while using the "friend class
// SoGLDisplayList" statement in the class definition.
class SoGLDisplayList;

typedef void SoScheduleDeleteCB(void * closure, uint32_t contextid);

// *************************************************************************

class COIN_DLL_API SoGLCacheContextElement : public SoElement {
  typedef SoElement inherited;

  SO_ELEMENT_HEADER(SoGLCacheContextElement);

public:
  static void initClass(void);
protected:
  virtual ~SoGLCacheContextElement();

public:
  virtual void init(SoState * state);

  virtual SbBool matches(const SoElement * elt) const;
  virtual SoElement * copyMatchInfo(void) const;
  static void set(SoState * state, int context,
                  SbBool twopasstransparency,
                  SbBool remoterendering);
  static int get(SoState * state);
  static int getExtID(const char * str);
  static void getOpenGLVersion(SoState * state, int & major, int & minor);
  static SbBool extSupported(SoState * state, int extid);
  static SbBool areMipMapsFast(SoState * state);
  enum {
    DO_AUTO_CACHE = 1,
    DONT_AUTO_CACHE
  };
  static void shouldAutoCache(SoState * state, int bits);
  static void setAutoCacheBits(SoState * state, int bits);
  static int resetAutoCacheBits(SoState * state);
  static SbBool getIsRemoteRendering(SoState * state);

  static uint32_t getUniqueCacheContext(void);

  static void scheduleDeleteCallback(const uint32_t contextid,
                                     SoScheduleDeleteCB * cb,
                                     void * closure);

  static void incNumShapes(SoState * state);
  static int getNumShapes(SoState * state);
  static void incNumSeparators(SoState * state);
  static int getNumSeparators(SoState * state);

private:
  friend class SoGLDisplayList;
  static void scheduleDelete(SoState * state, SoGLDisplayList * dl);
  static void cleanupContext(uint32_t contextid, void * closure);

private:
  int context;
  SbBool twopass;
  int autocachebits;
  int numshapes;
  int numseparators;

  enum { RENDERING_UNSET, RENDERING_SET_DIRECT, RENDERING_SET_INDIRECT };
  int rendering;
  SbBool isDirectRendering(SoState * state) const;
};

// *************************************************************************

// For compatibility with client code originally written with SGI/TGS
// Inventor:
#ifndef COIN_INTERNAL
#include <Inventor/elements/SoGLDisplayList.h>
#endif // ! COIN_INTERNAL

#endif // !COIN_SOGLCACHECONTEXTELEMENT_H

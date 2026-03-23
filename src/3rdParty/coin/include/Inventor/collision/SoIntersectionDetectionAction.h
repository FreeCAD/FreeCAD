#ifndef COIN_SOINTERSECTIONDETECTIONACTION_H
#define COIN_SOINTERSECTIONDETECTIONACTION_H

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

#include <Inventor/tools/SbPimplPtr.h>
#include <Inventor/actions/SoSubAction.h>
#include <Inventor/actions/SoAction.h>
#include <Inventor/actions/SoCallbackAction.h>

struct SoIntersectingPrimitive {
  SoPath * path;
  enum PrimitiveType {
    SEGMENT = 2,
    LINE_SEGMENT = 2,
    TRIANGLE = 3
  } type;
  SbVec3f vertex[3];
  SbVec3f xf_vertex[3];
};

class COIN_DLL_API SoIntersectionDetectionAction : public SoAction {
  typedef SoAction hinherited;
  SO_ACTION_HEADER(SoIntersectionDetectionAction);
public:
  static void initClass(void);
  SoIntersectionDetectionAction(void);
  virtual ~SoIntersectionDetectionAction(void);

  enum Resp {
    NEXT_PRIMITIVE,
    NEXT_SHAPE,
    ABORT
  };

  typedef SoCallbackAction::Response SoIntersectionVisitationCB(void * closure, const SoPath * where);
  typedef SbBool SoIntersectionFilterCB(void * closure, const SoPath * p1, const SoPath * p2);
  typedef Resp SoIntersectionCB(void * closure, const SoIntersectingPrimitive * p1, const SoIntersectingPrimitive * p2);

  void setIntersectionDetectionEpsilon(float epsilon);
  float getIntersectionDetectionEpsilon(void) const;

  static void setIntersectionEpsilon(float epsilon);
  static float getIntersectionEpsilon(void);

  void setTypeEnabled(SoType type, SbBool enable);
  SbBool isTypeEnabled(SoType type, SbBool checkgroups = FALSE) const;

  void setManipsEnabled(SbBool enable);
  SbBool isManipsEnabled(void) const;

  void setDraggersEnabled(SbBool enable);
  SbBool isDraggersEnabled(void) const;

  void setShapeInternalsEnabled(SbBool enable);
  SbBool isShapeInternalsEnabled(void) const;

  void addVisitationCallback(SoType type, SoIntersectionVisitationCB * cb, void * closure);
  void removeVisitationCallback(SoType type, SoIntersectionVisitationCB * cb, void * closure);

  virtual void apply(SoNode * node);
  virtual void apply(SoPath * path);
  virtual void apply(const SoPathList & paths, SbBool obeysRules = FALSE);

  virtual void setFilterCallback(SoIntersectionFilterCB * cb, void * closure = NULL);
  virtual void addIntersectionCallback(SoIntersectionCB * cb, void * closure  = NULL);
  virtual void removeIntersectionCallback(SoIntersectionCB * cb, void * closure  = NULL);

private:
  class PImpl;
  SbPimplPtr<PImpl> pimpl;

  SoIntersectionDetectionAction(const SoIntersectionDetectionAction & rhs); // N/A
  SoIntersectionDetectionAction & operator = (const SoIntersectionDetectionAction & rhs); // N/A

};

#endif // !COIN_SOINTERSECTIONDETECTIONACTION_H

#ifndef COIN_SOEXTSELECTION_H
#define COIN_SOEXTSELECTION_H

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

#include <Inventor/nodes/SoSubNode.h>
#include <Inventor/nodes/SoSelection.h>
#include <Inventor/fields/SoSFEnum.h>
#include <cstddef> // NULL

#ifndef COIN_INTERNAL
#include <Inventor/actions/SoCallbackAction.h>
#endif // !COIN_INTERNAL

class SbColor;
class SbVec3f;
class SbVec2f;
class SbVec2s;
class SbViewportRegion;
class SoPrimitiveVertex;

// This shouldn't strictly be necessary, but the OSF1/cxx compiler
// complains if this is left out, while using the "friend class
// SoExtSelectionP" statement in the class definition.
class SoExtSelectionP;


typedef SbBool SoExtSelectionTriangleCB(void * userdata,
                                        SoCallbackAction * action,
                                        const SoPrimitiveVertex * v1,
                                        const SoPrimitiveVertex * v2,
                                        const SoPrimitiveVertex * v3);

typedef SbBool SoExtSelectionLineSegmentCB(void * userdata,
                                           SoCallbackAction * action,
                                           const SoPrimitiveVertex * v1,
                                           const SoPrimitiveVertex * v2);

typedef SbBool SoExtSelectionPointCB(void * userdata,
                                     SoCallbackAction * action,
                                     const SoPrimitiveVertex * v1);

typedef SoPath * SoLassoSelectionFilterCB(void * userdata, const SoPath * path);


class COIN_DLL_API SoExtSelection : public SoSelection {
  typedef SoSelection inherited;

  SO_NODE_HEADER(SoExtSelection);

public:
  static void initClass(void);
  SoExtSelection(void);

  enum LassoType {
    NOLASSO, LASSO, RECTANGLE
  };

  enum LassoPolicy {
    FULL_BBOX, PART_BBOX, FULL, PART
  };

  enum LassoMode {
    ALL_SHAPES,
    VISIBLE_SHAPES
  };

  SoSFEnum lassoType;
  SoSFEnum lassoPolicy;
  SoSFEnum lassoMode;

  void useOverlay(SbBool overlay = TRUE);
  SbBool isUsingOverlay(void);
  SoSeparator * getOverlaySceneGraph(void);
  void setOverlayLassoColorIndex(const int index);
  int getOverlayLassoColorIndex(void);
  void setLassoColor(const SbColor & color);
  const SbColor & getLassoColor(void);
  void setLassoWidth(const float width);
  float getLassoWidth(void);
  void setOverlayLassoPattern(const unsigned short pattern);
  unsigned short getOverlayLassoPattern(void);
  void animateOverlayLasso(const SbBool animate = TRUE);
  SbBool isOverlayLassoAnimated(void);

  virtual void handleEvent(SoHandleEventAction * action);
  virtual void GLRenderBelowPath(SoGLRenderAction * action);

  void select(SoNode * root, int numcoords, SbVec2f * lasso, 
              const SbViewportRegion & vp, SbBool shiftpolicy);
  void select(SoNode * root, int numcoords, SbVec3f * lasso,
              const SbViewportRegion & vp, SbBool shiftkeypolicy);
  const SbVec2s * getLassoCoordsDC(int & numCoords);
  const SbVec3f * getLassoCoordsWC(int & numCoords);
  const SoPathList & getSelectionPathList() const;

  void setLassoFilterCallback(SoLassoSelectionFilterCB * f, void * userdata = NULL,
                              const SbBool callonlyifselectable = TRUE);

  void setTriangleFilterCallback(SoExtSelectionTriangleCB * func,
                                 void * userdata = NULL);
  void setLineSegmentFilterCallback(SoExtSelectionLineSegmentCB * func,
                                    void * userdata = NULL);
  void setPointFilterCallback(SoExtSelectionPointCB * func,
                              void * userdata = NULL);
  SbBool wasShiftDown(void) const;

protected:
  virtual ~SoExtSelection();

private:
  void draw(SoGLRenderAction * action);

  friend class SoExtSelectionP;
  class SoExtSelectionP * pimpl;
};

#endif // !COIN_SOEXTSELECTION_H

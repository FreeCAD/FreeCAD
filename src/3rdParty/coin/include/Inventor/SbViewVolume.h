#ifndef COIN_SBVIEWVOLUME_H
#define COIN_SBVIEWVOLUME_H

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

#include <cstdio>

#include <Inventor/SbBasic.h>
#include <Inventor/SbVec3f.h>
#include <Inventor/SbDPViewVolume.h>

class SbBox3f;
class SbLine;
class SbMatrix;
class SbPlane;
class SbRotation;
class SbVec2f;
class SbVec3f;

class COIN_DLL_API SbViewVolume {
public:
  enum ProjectionType { ORTHOGRAPHIC = 0, PERSPECTIVE = 1 };

public:
  SbViewVolume(void);
  ~SbViewVolume(void);
  void getMatrices(SbMatrix& affine, SbMatrix& proj) const;
  SbMatrix getMatrix(void) const;
  SbMatrix getCameraSpaceMatrix(void) const;
  void projectPointToLine(const SbVec2f& pt, SbLine& line) const;
  void projectPointToLine(const SbVec2f& pt,
                          SbVec3f& line0, SbVec3f& line1) const;
  void projectToScreen(const SbVec3f& src, SbVec3f& dst) const;
  SbPlane getPlane(const float distFromEye) const;
  SbVec3f getSightPoint(const float distFromEye) const;
  SbVec3f getPlanePoint(const float distFromEye,
                        const SbVec2f& normPoint) const;
  SbRotation getAlignRotation(SbBool rightAngleOnly = FALSE) const;
  float getWorldToScreenScale(const SbVec3f& worldCenter,
                              float normRadius) const;
  SbVec2f projectBox(const SbBox3f& box) const;
  SbViewVolume narrow(float left, float bottom,
                      float right, float top) const;
  SbViewVolume narrow(const SbBox3f& box) const;
  void ortho(float left, float right,
             float bottom, float top,
             float nearval, float farval);
  void perspective(float fovy, float aspect,
                   float nearval, float farval);
  void frustum(float left, float right,
               float bottom, float top,
               float nearval, float farval);
  void rotateCamera(const SbRotation& q);
  void translateCamera(const SbVec3f& v);
  SbVec3f zVector(void) const;
  SbViewVolume zNarrow(float nearval, float farval) const;
  void scale(float factor);
  void scaleWidth(float ratio);
  void scaleHeight(float ratio);
  ProjectionType getProjectionType(void) const;
  const SbVec3f& getProjectionPoint(void) const;
  const SbVec3f& getProjectionDirection(void) const;
  float getNearDist(void) const;
  float getWidth(void) const;
  float getHeight(void) const;
  float getDepth(void) const;

  void print(FILE * fp) const;
  void getViewVolumePlanes(SbPlane planes[6]) const;
  void transform(const SbMatrix &matrix);
  SbVec3f getViewUp(void) const;

  SbBool intersect(const SbVec3f & p) const;
  SbBool intersect(const SbVec3f & p0, const SbVec3f & p1,
                   SbVec3f & closestpoint) const;
  SbBool intersect(const SbBox3f & box) const;
  SbBox3f intersectionBox(const SbBox3f & box) const;

  SbBool outsideTest(const SbPlane & p,
                     const SbVec3f & bmin, const SbVec3f & bmax) const;
  const SbDPViewVolume & getDPViewVolume(void) const;

public:
  // Warning! It's extremely bad design to keep these data members
  // public, but we have no choice since this is how it's done in
  // the original SGI Open Inventor. We've seen example code that
  // use these variables directly so we'll have to be compatible
  // here. Please don't use these variables directly unless you're
  // very sure about what you're doing.
  ProjectionType type;
  SbVec3f projPoint;
  SbVec3f projDir;
  float nearDist;
  float nearToFar;
  SbVec3f llf;
  SbVec3f lrf;
  SbVec3f ulf;

private:
  void getPlaneRectangle(const float depth, SbVec3f & lowerleft,
                         SbVec3f & lowerright, SbVec3f & upperleft,
                         SbVec3f & upperright) const;

  SbDPViewVolume dpvv;
};

#endif // !COIN_SBVIEWVOLUME_H

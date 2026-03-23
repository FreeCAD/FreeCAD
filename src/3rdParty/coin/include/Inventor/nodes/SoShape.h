#ifndef COIN_SOSHAPE_H
#define COIN_SOSHAPE_H

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
#include <Inventor/elements/SoMultiTextureCoordinateElement.h>
#include <Inventor/SbMatrix.h>
#include <Inventor/SbVec2s.h>
#include <Inventor/SbBox3f.h>

class SoPrimitiveVertex;
class SoDetail;
class SoPickedPoint;
class SoFaceDetail;
class SoState;
class SoCoordinateElement;
class SbVec2f;
class SoMaterialBundle;
class SoBoundingBoxCache;

class COIN_DLL_API SoShape : public SoNode {
  typedef SoNode inherited;

  SO_NODE_ABSTRACT_HEADER(SoShape);

public:
  static void initClass(void);

  enum TriangleShape {
    TRIANGLE_STRIP, TRIANGLE_FAN, TRIANGLES, POLYGON,
    // The rest of the enums are not part of the original Inventor API.
    QUADS, QUAD_STRIP, POINTS, LINES, LINE_STRIP
  };

  virtual SbBool affectsState(void) const;
  virtual void notify(SoNotList * nl);

  virtual void getBoundingBox(SoGetBoundingBoxAction * action);
  virtual void GLRender(SoGLRenderAction * action);
  virtual void rayPick(SoRayPickAction * action);
  virtual void callback(SoCallbackAction * action);
  virtual void computeBBox(SoAction * action, SbBox3f & box,
                           SbVec3f & center) =  0;
  virtual void getPrimitiveCount(SoGetPrimitiveCountAction * action);

  static void getScreenSize(SoState * const state, const SbBox3f & boundingbox,
                            SbVec2s & rectsize);
  static float getDecimatedComplexity(SoState * state, float complexity);
  const SoBoundingBoxCache * getBoundingBoxCache(void) const;
  
protected:
  SoShape(void);
  virtual ~SoShape();

  float getComplexityValue(SoAction * action);
  virtual void generatePrimitives(SoAction * action) =  0;
  virtual SbBool shouldGLRender(SoGLRenderAction * action);
  void beginSolidShape(SoGLRenderAction * action);
  void endSolidShape(SoGLRenderAction * action);
  void GLRenderBoundingBox(SoGLRenderAction * action);
  SbBool shouldPrimitiveCount(SoGetPrimitiveCountAction * action);

  SbBool shouldRayPick(SoRayPickAction * const action);
  void computeObjectSpaceRay(SoRayPickAction * const action);
  void computeObjectSpaceRay(SoRayPickAction * const action,
                             const SbMatrix & matrix);
  virtual SoDetail * createTriangleDetail(SoRayPickAction * action,
                                          const SoPrimitiveVertex * v1,
                                          const SoPrimitiveVertex * v2,
                                          const SoPrimitiveVertex * v3,
                                          SoPickedPoint * pp);
  virtual SoDetail * createLineSegmentDetail(SoRayPickAction * action,
                                             const SoPrimitiveVertex * v1,
                                             const SoPrimitiveVertex * v2,
                                             SoPickedPoint * pp);
  virtual SoDetail * createPointDetail(SoRayPickAction * action,
                                       const SoPrimitiveVertex * v,
                                       SoPickedPoint * pp);

  void invokeTriangleCallbacks(SoAction * const action,
                               const SoPrimitiveVertex * const v1,
                               const SoPrimitiveVertex * const v2,
                               const SoPrimitiveVertex * const v3);
  void invokeLineSegmentCallbacks(SoAction * const action,
                                  const SoPrimitiveVertex * const v1,
                                  const SoPrimitiveVertex * const v2);
  void invokePointCallbacks(SoAction * const action,
                            const SoPrimitiveVertex * const v);
  void beginShape(SoAction * const action, const TriangleShape shapetype,
                  SoDetail * const detail = NULL);
  void shapeVertex(const SoPrimitiveVertex * const v);
  void endShape(void);

  void generateVertex(SoPrimitiveVertex * const pv,
                      const SbVec3f & point,
                      const SbBool useTexFunc,
                      const SoMultiTextureCoordinateElement * const tce,
                      const float s,
                      const float t,
                      const SbVec3f & normal);
  void generateVertex(SoPrimitiveVertex * const pv,
                      const SbVec3f & point,
                      const SbBool useTexFunc,
                      const SoMultiTextureCoordinateElement * const tce,
                      const float s,
                      const float t,
                      const float r,
                      const SbVec3f & normal);

  SbBool startVertexArray(SoGLRenderAction * action,
                          const SoCoordinateElement * coords,
                          const SbVec3f * pervertexnormals,
                          const SbBool texpervertex,
                          const SbBool colorpervertex);
  
  void finishVertexArray(SoGLRenderAction * action,
                         const SbBool vbo,
                         const SbBool normpervertex,
                         const SbBool texpervertex,
                         const SbBool colorpervertex);
private:
  class SoShapeP * pimpl;
  void validatePVCache(SoGLRenderAction * action);
  void getBBox(SoAction * action, SbBox3f & box, SbVec3f & center);
  void rayPickBoundingBox(SoRayPickAction * action);
  friend class soshape_primdata;           // internal class
  friend class so_generate_prim_private;   // a very private class
};

#endif // !COIN_SOSHAPE_H

#ifndef COIN_SOSHAPEHINTSELEMENT_H
#define COIN_SOSHAPEHINTSELEMENT_H

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

class COIN_DLL_API SoShapeHintsElement : public SoElement {
  typedef SoElement inherited;

  SO_ELEMENT_HEADER(SoShapeHintsElement);
public:
  static void initClass(void);
protected:
  virtual ~SoShapeHintsElement();

public:
  enum VertexOrdering {
    UNKNOWN_ORDERING,
    CLOCKWISE,
    COUNTERCLOCKWISE,
    ORDERING_AS_IS
  };

  enum ShapeType {
    UNKNOWN_SHAPE_TYPE,
    SOLID,
    SHAPE_TYPE_AS_IS
  };

  enum FaceType {
    UNKNOWN_FACE_TYPE,
    CONVEX,
    FACE_TYPE_AS_IS
  };

  virtual void init(SoState * state);
  virtual void push(SoState * state);
  virtual void pop(SoState * state, const SoElement * prevtopelement);

  virtual SbBool matches(const SoElement * element) const;
  virtual SoElement *copyMatchInfo(void) const;

  static void set(SoState * const state, SoNode * const node,
                  const VertexOrdering vertexOrdering,
                  const ShapeType shapeType, const FaceType faceType);
  static void set(SoState * const state,
                  const VertexOrdering vertexOrdering,
                  const ShapeType shapeType, const FaceType faceType);
  static void get(SoState * const state, VertexOrdering & vertexOrdering,
                  ShapeType & shapeType, FaceType & faceType);

  static VertexOrdering getVertexOrdering(SoState * const state);
  static ShapeType getShapeType(SoState * const state);
  static FaceType getFaceType(SoState * const state);

  static VertexOrdering getDefaultVertexOrdering();
  static ShapeType getDefaultShapeType();
  static FaceType getDefaultFaceType();

  virtual void print(FILE * file) const;

protected:
  void updateLazyElement(SoState * state);
  virtual void setElt(VertexOrdering vertexOrdering,
                      ShapeType shapeType, FaceType faceType);

  VertexOrdering vertexOrdering;
  ShapeType shapeType;
  FaceType faceType;

};

#endif // !COIN_SOSHAPEHINTSELEMENT_H

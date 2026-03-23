#ifndef COIN_SOSHAPEHINTS_H
#define COIN_SOSHAPEHINTS_H

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

#include <Inventor/elements/SoShapeHintsElement.h>
#include <Inventor/fields/SoSFEnum.h>
#include <Inventor/fields/SoSFFloat.h>
#include <Inventor/fields/SoSFBool.h>
#include <Inventor/nodes/SoSubNode.h>

class COIN_DLL_API SoShapeHints : public SoNode {
  typedef SoNode inherited;

  SO_NODE_HEADER(SoShapeHints);

public:
  static void initClass(void);
  SoShapeHints(void);

  enum VertexOrdering {
    UNKNOWN_ORDERING = SoShapeHintsElement::UNKNOWN_ORDERING,
    CLOCKWISE = SoShapeHintsElement::CLOCKWISE,
    COUNTERCLOCKWISE = SoShapeHintsElement::COUNTERCLOCKWISE
  };

  enum ShapeType {
    UNKNOWN_SHAPE_TYPE = SoShapeHintsElement::UNKNOWN_SHAPE_TYPE,
    SOLID = SoShapeHintsElement::SOLID
  };

  enum FaceType {
    UNKNOWN_FACE_TYPE = SoShapeHintsElement::UNKNOWN_FACE_TYPE,
    CONVEX = SoShapeHintsElement::CONVEX
  };

  enum WindingType {
    NO_WINDING_TYPE = 0
  };

  SoSFEnum vertexOrdering;
  SoSFEnum shapeType;
  SoSFEnum faceType;
  SoSFEnum windingType;
  SoSFBool useVBO;
  SoSFFloat creaseAngle;

  virtual void doAction(SoAction * action);
  virtual void GLRender(SoGLRenderAction * action);
  virtual void callback(SoCallbackAction * action);
  virtual void getBoundingBox(SoGetBoundingBoxAction * action);
  virtual void pick(SoPickAction * action);

protected:
  virtual ~SoShapeHints();
};

#endif // !COIN_SOSHAPEHINTS_H

#ifndef COIN_SOSHAPE_PRIMDATA_H
#define COIN_SOSHAPE_PRIMDATA_H

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

#ifndef COIN_INTERNAL
#error this is a private header file
#endif /* !COIN_INTERNAL */

#include <Inventor/nodes/SoShape.h>

// Private class used only by SoShape to aid in generating primitives,
// handling details ++

class SoDetail;
class SoAction;
class SoPrimitiveVertex;
class SoPointDetail;
class SoFaceDetail;
class SoLineDetail;

class soshape_primdata {
public:
  soshape_primdata(void);

  ~soshape_primdata();

  void beginShape(SoShape * shape, SoAction * action,
                  SoShape::TriangleShape shapetype,
                  SoDetail * detail);
  void endShape(void);

  void shapeVertex(const SoPrimitiveVertex * const v);

  int getPointDetailIndex(const SoPrimitiveVertex * v) const;

private:
  void copyVertex(const int src, const int dest);
  void setVertex(const int idx, const SoPrimitiveVertex * const v);
  void handleFaceDetail(const int numv);
  void handleLineDetail(void);
  SoDetail * createPickDetail(void);
  static void tess_callback(void * v0, void * v1, void * v2, void * data);

  void copyMaterialIndex(const int lastvertex);
  void copyNormalIndex(const int lastvertex);

private:
  friend class SoShape;

  SoShape::TriangleShape shapetype;
  SoAction * action;
  SoShape * shape;
  SoPrimitiveVertex * vertsArray;
  SoPointDetail * pointDetails;
  SoFaceDetail * faceDetail;
  SoLineDetail * lineDetail;
  int arraySize;
  int counter;
  class SbTesselator * tess;
  class SbGLUTessellator * glutess;
  int faceCounter;

  SbBool matPerFace;
  SbBool normPerFace;
};


#endif // !COIN_SOSHAPE_PRIMDATA_H

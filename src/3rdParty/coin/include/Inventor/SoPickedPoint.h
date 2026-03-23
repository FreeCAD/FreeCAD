#ifndef COIN_SOPICKEDPOINT_H
#define COIN_SOPICKEDPOINT_H

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

#include <Inventor/SbBasic.h>
#include <Inventor/SbVec3f.h>
#include <Inventor/SbVec4f.h>
#include <Inventor/lists/SoDetailList.h>
#include <Inventor/SbViewportRegion.h>

class SoPath;
class SoDetail;
class SoNode;
class SoState;
class SbMatrix;

class COIN_DLL_API SoPickedPoint {
public:
  SoPickedPoint(const SoPickedPoint & pp);
  SoPickedPoint(const SoPath * const path, SoState * const state,
                const SbVec3f & objSpacePoint);
  ~SoPickedPoint();
  SoPickedPoint *copy() const;
  const SbVec3f &getPoint() const;
  const SbVec3f &getNormal() const;
  const SbVec4f &getTextureCoords() const;
  int getMaterialIndex() const;
  SoPath *getPath() const;
  SbBool isOnGeometry() const;
  const SoDetail *getDetail(const SoNode * const node = NULL) const;
  const SbMatrix &getObjectToWorld(const SoNode * const node = NULL) const;
  const SbMatrix &getWorldToObject(const SoNode * const node = NULL) const;
  const SbMatrix &getObjectToImage(const SoNode * const node = NULL) const;
  const SbMatrix &getImageToObject(const SoNode * const node = NULL) const;
  SbVec3f getObjectPoint(const SoNode * const node = NULL) const;
  SbVec3f getObjectNormal(const SoNode * const node = NULL) const;
  SbVec4f getObjectTextureCoords(const SoNode * const node = NULL) const;

  void setObjectNormal(const SbVec3f &normal);
  void setObjectTextureCoords(const SbVec4f &texCoords);
  void setMaterialIndex(const int index);
  void setDetail(SoDetail * detail, SoNode * node);

private:

  SbVec3f point, objPoint;
  SbVec3f normal, objNormal;
  SbVec4f texCoords, objTexCoords;
  int materialIndex;
  SoPath *path;
  SbBool onGeometry;
  SoDetailList detailList;
  SoState *state;
  SbViewportRegion viewport;

  class SoGetMatrixAction *getMatrixAction() const;
  void applyMatrixAction(const SoNode * const node) const;
};

#endif // !COIN_SOPICKEDPOINT_H

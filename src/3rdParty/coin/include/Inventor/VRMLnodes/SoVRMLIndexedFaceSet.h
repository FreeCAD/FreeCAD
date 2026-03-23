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

#ifndef COIN_SOVRMLINDEXEDFACESET_H
#define COIN_SOVRMLINDEXEDFACESET_H

#include <Inventor/nodes/SoSubNode.h>
#include <Inventor/VRMLnodes/SoVRMLIndexedShape.h>
#include <Inventor/fields/SoSFBool.h>
#include <Inventor/fields/SoSFFloat.h>

#ifndef SO_END_FACE_INDEX // also defined in SoIndexedFaceSet.h
#define SO_END_FACE_INDEX (-1)
#endif // !SO_END_FACE_INDEX

class SoVRMLIndexedFaceSetP;

class COIN_DLL_API SoVRMLIndexedFaceSet : public SoVRMLIndexedShape
{
  typedef SoVRMLIndexedShape inherited;
  SO_NODE_HEADER(SoVRMLIndexedFaceSet);

public:
  static void initClass(void);
  SoVRMLIndexedFaceSet(void);

  SoSFBool ccw;
  SoSFBool solid;
  SoSFBool convex;
  SoSFFloat creaseAngle;

  virtual void GLRender( SoGLRenderAction * action );
  virtual void getPrimitiveCount( SoGetPrimitiveCountAction * action );

  virtual SbBool generateDefaultNormals(SoState * s, SoNormalBundle * nb );
  virtual SbBool generateDefaultNormals(SoState * state, SoNormalCache * nc);

protected:
  virtual ~SoVRMLIndexedFaceSet();

  virtual void generatePrimitives( SoAction * action );


private:

  enum Binding {
    OVERALL,
    NONE = OVERALL,
    PER_FACE,
    PER_FACE_INDEXED,
    PER_VERTEX,
    PER_VERTEX_INDEXED
  };

  Binding findMaterialBinding(SoState * state) const;
  Binding findNormalBinding(SoState * state) const;

  virtual void notify(SoNotList * list);
  
  SbBool useConvexCache(SoAction * action, 
                        const SbVec3f * normals, 
                        const int32_t * nindices, 
                        const SbBool normalsfromcache);
  
  SoVRMLIndexedFaceSetP * pimpl;
};

#endif // ! COIN_SOVRMLINDEXEDFACESET_H

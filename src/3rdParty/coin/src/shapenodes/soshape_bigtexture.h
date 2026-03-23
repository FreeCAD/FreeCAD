#ifndef COIN_SOSHAPE_BIGTEXTURE_H
#define COIN_SOSHAPE_BIGTEXTURE_H

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

#include <Inventor/lists/SbList.h>
#include <Inventor/elements/SoMultiTextureImageElement.h>
#include <Inventor/SbVec2f.h>
#include <Inventor/SbPlane.h>

class SoGLBigImage;
class SoState;
class SoPrimitiveVertex;
class SoMaterialBundle;
class SoShape;
class SbClip;
class SbVec3f;

// FIXME: this class will also be part of the global namespace of
// the compiled Coin library (at least when compiled to a UNIX-style
// library). That should be avoided. 20020220 mortene.

class soshape_bigtexture {
public:
  soshape_bigtexture(void);
  ~soshape_bigtexture();

  void beginShape(SoGLBigImage * image,
                  const float quality);
  void triangle(SoState * state,
                const SoPrimitiveVertex * v1,
                const SoPrimitiveVertex * v2,
                const SoPrimitiveVertex * v3);
  SbBool endShape(SoState * state, SoShape * shape,
                  SoMaterialBundle & mb);


private:
  void clip_triangles(SoState * state);
  void handle_triangle(SoState * state,
                       SoPrimitiveVertex * v1,
                       SoPrimitiveVertex * v2,
                       SoPrimitiveVertex * v3,
                       const SoMultiTextureImageElement::Wrap wrap[2],
                       const int transs, 
                       const int transt);
  
  SbList <SoPrimitiveVertex*> vertexlist;
  
  class bt_region {
  public:
    SbVec2f start, end, tcmul;
    SbPlane planes[4];
    SbList <SoPrimitiveVertex*> pvlist;
    SbList <int> facelist;
  };

  SbList <SoPrimitiveVertex*> * pvlist;
  int pvlistcnt;
  SbClip * clipper;
  SoGLBigImage * image;
  float quality;
  bt_region * regions;
  int numallocregions;
  int numregions;

  SoPrimitiveVertex * get_new_pv(void);

  static void * clipcb(const SbVec3f & v0, void * vdata0,
                       const SbVec3f & v1, void * vdata1,
                       const SbVec3f & newvertex,
                       void * userdata);
};

#endif // !COIN_SOSHAPE_BIGTEXTURE_H

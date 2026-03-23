#ifndef COIN_SOGL_H
#define COIN_SOGL_H

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

// This file contains GL code which is shared internally. It contains
// mostly rendering code shared between several node types.

#include <Inventor/SbString.h>
#include <Inventor/C/glue/gl.h>

class SoShape;
class SoState;
class SoAction;
class SoMaterialBundle;
class SoGLCoordinateElement;
class SoTextureCoordinateBundle;
class SoVertexAttributeBundle;
class SbVec3f;
class SbVec2f;

// flags for cone, cylinder and cube

#define SOGL_RENDER_SIDE         0x01
#define SOGL_RENDER_TOP          0x02
#define SOGL_RENDER_BOTTOM       0x04
#define SOGL_MATERIAL_PER_PART   0x08
#define SOGL_NEED_NORMALS        0x10
#define SOGL_NEED_TEXCOORDS      0x20
#define SOGL_NEED_3DTEXCOORDS    0x40
#define SOGL_NEED_MULTITEXCOORDS 0x80 // internal

// Convenience function for access to OpenGL wrapper from an SoState
// pointer.
const cc_glglue * sogl_glue_instance(const SoState * state);


// render
void sogl_render_cone(const float bottomRadius,
                      const float height,
                      const int numslices,
                      SoMaterialBundle * const material,
                      const unsigned int flags,
                      SoState * state);


void sogl_render_cylinder(const float radius,
                          const float height,
                          const int numslices,
                          SoMaterialBundle * const material,
                          const unsigned int flags,
                          SoState * state);

void sogl_render_sphere(const float radius,
                        const int numstacks,
                        const int numslices,
                        SoMaterialBundle * const material,
                        const unsigned int flags,
                        SoState * state);

void sogl_render_cube(const float width,
                      const float height,
                      const float depth,
                      SoMaterialBundle * const material,
                      const unsigned int flags,
                      SoState * state);

// FIXME: must be kept around due to ABI & API compatibility reasons
// for now, but should consider taking it out for the next major Coin
// release. 20030519 mortene.
void sogl_offscreencontext_callback(void (*cb)(void *, SoAction*),
                                    void * closure);

//
// optimized faceset rendering functions.
// the functions are automagically generated based on a template function.
// in addition to these 50 different functions, the template also generates
// different versions based on the OpenGL version, and if the vertex array
// extension is available for OpenGL 1.0 implementations.
// OpenGL 1.2 features is not currently used :(
//
//
//

void sogl_render_faceset(const SoGLCoordinateElement * const coords,
                         const int32_t *vertexindices,
                         int num_vertexindices,
                         const SbVec3f *normals,
                         const int32_t *normindices,
                         SoMaterialBundle * const materials,
                         const int32_t *matindices,
                         SoTextureCoordinateBundle * const texcoords,
                         const int32_t *texindices,
                         SoVertexAttributeBundle * const attribs,
                         const int nbind,
                         const int mbind,
                         const int attribbind,
                         const int dotexture,
                         const int doattribs);

void
sogl_render_tristrip(const SoGLCoordinateElement * const coords,
                     const int32_t *vertexindices,
                     int num_vertexindices,
                     const SbVec3f *normals,
                     const int32_t *normindices,
                     SoMaterialBundle *const materials,
                     const int32_t *matindices,
                     const SoTextureCoordinateBundle * const texcoords,
                     const int32_t *texindices,
                     const int nbind,
                     const int mbind,
                     const int texture);

void
sogl_render_lineset(const SoGLCoordinateElement * const coords,
                    const int32_t *vertexindices,
                    int num_vertexindices,
                    const SbVec3f *normals,
                    const int32_t *normindices,
                    SoMaterialBundle *const materials,
                    const int32_t *matindices,
                    const SoTextureCoordinateBundle * const texcoords,
                    const int32_t *texindices,
                    int nbind,
                    int mbind,
                    const int texture,
                    const int drawAsPoints);

void
sogl_render_pointset(const SoGLCoordinateElement * coords,
                     const SbVec3f * normals,
                     SoMaterialBundle * mb,
                     const SoTextureCoordinateBundle * tb,
                     int32_t numpts,
                     int32_t idx);

SbBool sogl_glerror_debugging(void);

void sogl_autocache_update(SoState * state, const int numprimitives, 
                           SbBool didusevbo);

#endif // !COIN_SOGL_H

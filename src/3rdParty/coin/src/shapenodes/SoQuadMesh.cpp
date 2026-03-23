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

/*!
  \class SoQuadMesh SoQuadMesh.h Inventor/nodes/SoQuadMesh.h
  \brief The SoQuadMesh class is used to render and optimize a quadrilateral mesh.

  \ingroup coin_nodes

  This node uses the coordinates in order, either from the state or
  from the SoQuadMesh::vertexProperty node, to construct a
  quadrilateral mesh.

  The quads are generated in row major order, using the two fields
  SoQuadMesh::verticesPerColumn and SoQuadMesh::verticesPerRow to
  specify the mesh. E.g. if SoQuadMesh::verticesPerColumn is 3 and
  SoQuadMesh::verticesPerRow is 2, two quads will be generated with
  the first one using (in order) coordinates 0, 1, 3 and 2, the second
  one using coordinates 2, 3, 5 and 4 (you get three rows of vertices;
  the first row uses vertices 0 and 1, the second row 2 and 3, and the
  third row 4 and 5).

  Here's a quick and simple usage example code snippet:

  \code
  // Vertices for the Quad mesh.
  static float vertices[25][3] = {
    // Row 1
    {-11, 0, 1}, {0, 11, 1}, {11, 0, 1}, {0, -11, 1}, {-11, 0, 1},
    // Row 2
    {-9, 0, 1}, {0, 9, 1}, {9, 0, 1}, {0, -9, 1}, {-9, 0, 1},
    // Row 3
    {-9, 0, -1}, {0, 9, -1}, {9, 0, -1}, {0, -9, -1}, {-9, 0, -1},
    // Row 4
    {-11, 0, -1}, {0, 11, -1}, {11, 0, -1}, {0, -11, -1}, {-11, 0, -1},
    // Row 5
    {-11, 0, 1}, {0, 11, 1}, {11, 0, 1}, {0, -11, 1}, {-11, 0, 1}
  };

  // This function generates an object by using the SoQuadMesh node
  // Return:
  //  SoSeparator *
  static SoSeparator *
  quadMesh(void)
  {
    SoSeparator * qm = new SoSeparator;

    // Define coordinates
    SoCoordinate3 * coords = new SoCoordinate3;
    coords->point.setValues(0, 30, vertices);
    qm->addChild(coords);

    // QuadMesh
    SoQuadMesh * mesh = new SoQuadMesh;
    mesh->verticesPerRow = 5;
    mesh->verticesPerColumn = 5;
    qm->addChild(mesh);

    return qm;
  }
  \endcode

  The quadmesh geometry resulting from this code looks like this:<br>

  <center>
  \image html quadmesh.png "Rendering of Example Scenegraph"
  </center>

  Here is another example, this time making a 2x2 grid, with a
  texture:

  \verbatim
  #Inventor V2.1 ascii

  Separator {
    Complexity { textureQuality 0.01 }
    Texture2 {
      image 2 2 4 0xff0000ff 0x00ff00ff 0xffff00ff 0xff00ffff
    }
    Coordinate3 {
      point [
        0 2 0,
        1 2 0,
        2 2 0,
        0 1 0,
        1 1 0,
        2 1 0,
        0 0 0,
        1 0 0,
        2 0 0
      ]
    }
    QuadMesh {
      verticesPerRow 3
      verticesPerColumn 3
    }
  }
  \endverbatim


  For SoQuadMesh, normals and materials can be bound PER_PART (per
  row), PER_FACE, PER_VERTEX and OVERALL. The default material binding
  is OVERALL. The default normal binding is PER_VERTEX.


  A note about SoQuadMesh shading: the quads in the mesh are just
  passed on to OpenGL's GL_QUAD primitive rendering. Under certain
  circumstances, this can lead to artifacts in how your meshes are
  shaded. This is an inherent problem with drawing quads in meshes.

  There is a work around solution for the above mentioned problem that
  can be applied with Coin: by setting the global environment variable
  \c COIN_QUADMESH_PRECISE_LIGHTING to "1", the quads will be broken
  up into triangles before rendered, and shading will likely look much
  better. Be aware that this technique causes rendering of the
  SoQuadMesh to slow down by an approximate factor of 6.

  The "precise lighting" technique is currently limited to work only
  when SoQuadMesh rendering is parameterized with 3D coordinates, a
  material binding that is \e not per vertex, and if texture mapping is
  done is must be without using any of the SoTextureCoordinateFunction
  subclass nodes.

  <b>FILE FORMAT/DEFAULTS:</b>
  \code
    QuadMesh {
        vertexProperty NULL
        startIndex 0
        verticesPerColumn 1
        verticesPerRow 1
    }
  \endcode

  \sa SoTriangleStripSet SoIndexedTriangleStripSet
*/

#include <Inventor/nodes/SoQuadMesh.h>

#include <cmath> // ilogb
#include <cfloat> // _logb

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H

#include <Inventor/C/tidbits.h>
#include <Inventor/SbPlane.h>
#include <Inventor/SoPrimitiveVertex.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/actions/SoGetPrimitiveCountAction.h>
#include <Inventor/bundles/SoMaterialBundle.h>
#include <Inventor/bundles/SoTextureCoordinateBundle.h>
#include <Inventor/caches/SoNormalCache.h>
#include <Inventor/details/SoFaceDetail.h>
#include <Inventor/elements/SoGLCoordinateElement.h>
#include <Inventor/elements/SoGLLazyElement.h>
#include <Inventor/elements/SoMaterialBindingElement.h>
#include <Inventor/elements/SoNormalBindingElement.h>
#include <Inventor/elements/SoShapeHintsElement.h>
#include <Inventor/errors/SoDebugError.h>
#include <Inventor/misc/SoState.h>
#include <Inventor/system/gl.h>

#include "rendering/SoGL.h"
#include "nodes/SoSubNodeP.h"

/*!
  \var SoSFInt32 SoQuadMesh::verticesPerColumn
  Specifies to number of vertices in each column.
*/
/*!
  \var SoSFInt32 SoQuadMesh::verticesPerRow
  Specifies the number of vertices in each row.
*/

SO_NODE_SOURCE(SoQuadMesh);

/*!
  Constructor.
*/
SoQuadMesh::SoQuadMesh()
{
  SO_NODE_INTERNAL_CONSTRUCTOR(SoQuadMesh);

  SO_NODE_ADD_FIELD(verticesPerColumn, (1));
  SO_NODE_ADD_FIELD(verticesPerRow, (1));
}

/*!
  Destructor.
*/
SoQuadMesh::~SoQuadMesh()
{
}

// Documented in superclass.
void
SoQuadMesh::computeBBox(SoAction * action, SbBox3f & box, SbVec3f & center)
{
  inherited::computeCoordBBox(action,
                              this->verticesPerRow.getValue() *
                              this->verticesPerColumn.getValue(),
                              box, center);
}

//
// translates current material binding to the internal Binding enum.
//
SoQuadMesh::Binding
SoQuadMesh::findMaterialBinding(SoState * const state) const
{
  SoMaterialBindingElement::Binding matbind =
    SoMaterialBindingElement::get(state);

  Binding binding;
  switch (matbind) {
  case SoMaterialBindingElement::OVERALL:
    binding = OVERALL;
    break;
  case SoMaterialBindingElement::PER_VERTEX:
  case SoMaterialBindingElement::PER_VERTEX_INDEXED:
    binding = PER_VERTEX;
    break;
  case SoMaterialBindingElement::PER_PART:
  case SoMaterialBindingElement::PER_PART_INDEXED:
    binding = PER_ROW;
    break;
  case SoMaterialBindingElement::PER_FACE:
  case SoMaterialBindingElement::PER_FACE_INDEXED:
    binding = PER_FACE;
    break;
  default:
    binding = OVERALL;
#if COIN_DEBUG
    SoDebugError::postWarning("SoQuadMesh::findMaterialBinding",
                              "unknown material binding setting");
#endif // COIN_DEBUG
    break;
  }
  return binding;
}


//
// translates current normal binding to the internal Binding enum.
//
SoQuadMesh::Binding
SoQuadMesh::findNormalBinding(SoState * const state) const
{
  SoNormalBindingElement::Binding normbind =
    SoNormalBindingElement::get(state);

  Binding binding;
  switch (normbind) {
  case SoNormalBindingElement::OVERALL:
    binding = OVERALL;
    break;
  case SoNormalBindingElement::PER_VERTEX:
  case SoNormalBindingElement::PER_VERTEX_INDEXED:
    binding = PER_VERTEX;
    break;
  case SoNormalBindingElement::PER_PART:
  case SoNormalBindingElement::PER_PART_INDEXED:
    binding = PER_ROW;
    break;
  case SoNormalBindingElement::PER_FACE:
  case SoNormalBindingElement::PER_FACE_INDEXED:
    binding = PER_FACE;
    break;
  default:
    binding = PER_VERTEX;
#if COIN_DEBUG
    SoDebugError::postWarning("SoQuadMesh::findNormalBinding",
                              "unknown normal binding setting");
#endif // COIN_DEBUG
    break;
  }
  return binding;
}


#define QUADMESH_WEIGHTS_NR 32
static float precompWeights[QUADMESH_WEIGHTS_NR];
static float precalculateWeight(int i)
{
  int exponent = i - (QUADMESH_WEIGHTS_NR / 2);
  double p2 = ldexp(0.75, exponent);
  double p = sqrt(p2);
  return float(p / (1.0 + p));
}
static float qmeshGetWeight(float value)
{
#if defined(HAVE_ILOGB)
  int exponent = ilogb(value) + (QUADMESH_WEIGHTS_NR / 2);
#elif defined(HAVE__LOGB)
  int exponent = ((int) _logb(value)) + (QUADMESH_WEIGHTS_NR / 2);
#else // HAVE__LOGB
  // FIXME: implement coin_ilogb(double). pederb, 2002-12-19
  int exponent = QUADMESH_WEIGHTS_NR / 2;
#endif // !HAVE_ILOGB && ! HAVE__LOGB
  if (exponent < 0) return 0.f;
  if (exponent >= QUADMESH_WEIGHTS_NR) return 1.f;
  return precompWeights[exponent];
}

// qmeshNormalize
// v.length() must be in 0..sqrt(toLength2)/4 range
static SbBool qmeshNormalize(SbVec3f & v, float toLength2)
{
  // FIXME: this function should be optimized by look-up table
  // (sqrt and divisions are expensive operations)
  float vl2 = v.sqrLength();
  if (vl2 > 0.f) {
    v *= (float) sqrt(toLength2 / (vl2 * 4.0));
    return TRUE;
  }
  return FALSE;
}

namespace { namespace SoGL { namespace QuadMesh {

#define IDX(r,c) ((r)*rowsize+(c))

  enum AttributeBinding {
    OVERALL = 0,
    PER_ROW = 1,
    PER_FACE = 2,
    PER_VERTEX = 3
  };

  template < int NormalBinding,
             int MaterialBinding,
             int TexturingEnabled >
  static void GLRender(const SoGLCoordinateElement * coords,
                       const SbVec3f *normals,
                       SoMaterialBundle * mb,
                       const SoTextureCoordinateBundle * tb,
                       SbBool needNormals,
                       int rowsize,
                       int colsize,
                       int start,
                       SbBool preciseLighting)
  {
    assert(rowsize >= 0 && colsize >= 0 && start >= 0);
    assert(coords->getNum() - start >= rowsize * colsize);

    const SbBool is3d = coords->is3D();
    const SbVec3f * coords3d = is3d ? coords->getArrayPtr3() : NULL;
    const SbVec4f * coords4d = is3d ? NULL : coords->getArrayPtr4();

    if (preciseLighting == FALSE) {

      // This is the same code as in SoGLCoordinateElement::send().
      // It is inlined here for speed (~15% speed increase).
#define SEND_VERTEX(_idx_) \
      if (is3d) glVertex3fv((const GLfloat*) (coords3d + (_idx_)));        \
      else glVertex4fv((const GLfloat*) (coords4d + (_idx_)));

      int midx = 0;

      SbVec3f dummynormal(0.0f, 0.0f, 1.0f);
      const SbVec3f * currnormal = &dummynormal;
      if (normals) currnormal = normals;

      if ((AttributeBinding)NormalBinding == OVERALL) {
        if (needNormals) {
          glNormal3fv((const GLfloat *)currnormal);
        }
      }

      int curridx; // for optimization only

      for (int i = 0; i < colsize-1; i++) {
        int j = 0;
        glBegin(GL_QUAD_STRIP);
        if ((AttributeBinding)NormalBinding == PER_ROW) {
          currnormal = normals++;
          glNormal3fv((const GLfloat *)currnormal);
        }
        if ((AttributeBinding)MaterialBinding == PER_ROW) {
          mb->send(midx++,TRUE);
        }

        for (j = 0; j < rowsize; j++) {
          curridx = IDX(i,j);
          if ((AttributeBinding)NormalBinding == PER_VERTEX) {
            currnormal = &normals[curridx];
            glNormal3fv((const GLfloat *)currnormal);
          }
          if ((AttributeBinding)NormalBinding == PER_FACE) {
            // j != 1, since we send four vertices for the first quad, then two
            // vertices for all other quads
            if (j != 1) {
              currnormal = normals++;
              glNormal3fv((const GLfloat *)currnormal);
            }
          }
          if ((AttributeBinding)MaterialBinding == PER_VERTEX) {
            mb->send(curridx, TRUE);
          }
          if ((AttributeBinding)MaterialBinding == PER_FACE) {
            // j != 1, since we send four vertices for the first quad, then two
            // vertices for all other quads
            if (j != 1) mb->send(midx++, TRUE);
          }
          if (TexturingEnabled == TRUE) {
            tb->send(curridx, coords->get3(start + curridx),
                     *currnormal);
          }
          SEND_VERTEX(start + curridx);
          curridx = IDX(i+1,j);
          if ((AttributeBinding)NormalBinding == PER_VERTEX) {
            currnormal = &normals[curridx];
            glNormal3fv((const GLfloat *)currnormal);
          }
          if ((AttributeBinding)MaterialBinding == PER_VERTEX) {
            mb->send(curridx, TRUE);
          }
          if (TexturingEnabled == TRUE) {
            tb->send(curridx, coords->get3(start + curridx), *currnormal);
          }
          SEND_VERTEX(start + curridx);
        }
        glEnd(); // end of strip/row
      }
#undef SEND_VERTEX

    } else { // PreciseLighting == TRUE

      // Alternative code for rendering with more precise lighting by
      // placing an extra vertice in the middle of the quad, and
      // thereby splitting it to 2 triangles

      // Developed and contributed by PC John (Jan Peciva).

      int midx = 0;
      SbVec3f dummynormal(0.0f, 0.0f, 1.0f);
      const SbVec3f * currnormal = &dummynormal;
      if (normals) currnormal = normals;

      if ((AttributeBinding)NormalBinding == OVERALL) {
        if (needNormals) {
          glNormal3fv((const GLfloat *)currnormal);
        }
      }

      const SbVec3f *c1d3 = NULL,*c2d3 = NULL,*c3d3 = NULL,*c4d3 = NULL;
      SbVec3f ccd3;
      const SbVec4f *c1d4 = NULL,*c2d4 = NULL,*c3d4 = NULL,*c4d4 = NULL;
      SbVec4f ccd4;
      SbVec4f sum234d4,sum134d4,sum124d4,sum123d4;
      SbVec4f vec1d4,vec2d4,vec3d4,vec4d4;
      float s1,s2,s3,s4;
      float w1,w2,w3,w4;
      const SbVec3f *n1,*n2,*n3,*n4;
      SbVec3f nc;
      const SbVec4f *t1,*t2, * t3 = NULL, * t4 = NULL;
      SbVec4f tc;

      int curridx1 = 0;
      int curridx2 = rowsize;
      for (int i = 0; i < colsize-1; i++) {
        int j = 0;
        if ((AttributeBinding)NormalBinding == PER_ROW) {
          currnormal = normals++;
          glNormal3fv((const GLfloat *)currnormal);
        }
        if ((AttributeBinding)MaterialBinding == PER_ROW) {
          mb->send(midx++, TRUE);
        }

        if (is3d) {
          c3d3 = &coords3d[start+curridx1];
          c4d3 = &coords3d[start+curridx2];
        } else {
          c3d4 = &coords4d[start+curridx1];
          c4d4 = &coords4d[start+curridx2];
        }
        if ((AttributeBinding)NormalBinding == PER_VERTEX) {
          n3 = &normals[curridx1];
          n4 = &normals[curridx2];
        }
        curridx1++;
        curridx2++;

        for (j = 1; j < rowsize; j++) {
          if (is3d) {
            c1d3 = c3d3;
            c2d3 = c4d3;
            c3d3 = &coords3d[start+curridx1];
            c4d3 = &coords3d[start+curridx2];
            ccd3 = ((*c1d3)+(*c2d3)+(*c3d3)+(*c4d3)) * 0.25f;
          } else {
            c1d4 = c3d4;
            c2d4 = c4d4;
            c3d4 = &coords4d[start+curridx1];
            c4d4 = &coords4d[start+curridx2];
            assert(!"4d coordinates handling unimplemented yet");
          }

          if ((AttributeBinding)NormalBinding == PER_VERTEX ||
              TexturingEnabled == TRUE) {
            if (is3d) {
              s1 = ((*c1d3) - ccd3).sqrLength();
              s2 = ((*c2d3) - ccd3).sqrLength();
              s3 = ((*c3d3) - ccd3).sqrLength();
              s4 = ((*c4d3) - ccd3).sqrLength();
            } else {
              // FIXME: 4D coordinates are not currently implemented
              // for HQ rendering - following code is never used
              //sum234d4 = qmeshAddVec4f(c2, sum34d4);
              //vec1d4 = qmeshAddSpec4f(c1, sum234d4);
              //sum134d4 = qmeshAddVec4f(c1, sum34d4);
              //vec2d4 = qmeshAddSpec4f(c2, sum134d4);
              //sum124d4 = qmeshAddVec4f(c4, sum12d4);
              //vec3d4 = qmeshAddSpec4f(c3, sum124d4);
              //sum123d4 = qmeshAddVec4f(c3, sum12d4);
              //vec4d4 = qmeshAddSpec4f(c4, sum123d4);
              //s1 = qmeshSqrLen(vec1d4);
              //s2 = qmeshSqrLen(vec2d4);
              //s3 = qmeshSqrLen(vec3d4);
              //s4 = qmeshSqrLen(vec4d4);
            }

            if ((AttributeBinding)NormalBinding == PER_VERTEX ||
                TexturingEnabled == TRUE) {
              w1 = qmeshGetWeight(s1/s4) * 0.5f;
              w2 = qmeshGetWeight(s2/s3) * 0.5f;
              w3 = 0.5f - w2;
              w4 = 0.5f - w1;
            }
          }

          if ((AttributeBinding)NormalBinding == PER_VERTEX) {
            n1 = n3;
            n2 = n4;
            n3 = &normals[curridx1];
            n4 = &normals[curridx2];
            nc = ((*n1)*w1 + (*n2)*w2 + (*n3)*w3 + (*n4)*w4);
            if (!qmeshNormalize(nc, n1->sqrLength() + n2->sqrLength() +
                                n3->sqrLength() + n4->sqrLength())) {
              if (is3d) {
                SbPlane p1(*c1d3,*c2d3,*c4d3);
                SbPlane p2(*c1d3,*c4d3,*c3d3);
                SbVec3f n = p1.getNormal() + p2.getNormal();
                SbBool quadok = qmeshNormalize(n, n1->sqrLength() + n2->sqrLength() +
                                               n3->sqrLength() + n4->sqrLength());
#if COIN_DEBUG
                if ( !quadok )
                  SoDebugError::postWarning("SoQuadMesh::GLRender",
                                            "Can not compute normal because of "
                                            "wrong quad coordinates.");
#endif // COIN_DEBUG
              } else {
                // FIXME
              }
            }
          }

          if ((AttributeBinding)MaterialBinding == PER_VERTEX) {
            assert(FALSE && "yet unimplemented");
          }

          if (TexturingEnabled == TRUE) {
            t1 = t3;
            t2 = t4;
            if (!tb->isFunction()) {
              t3 = &((SoTextureCoordinateBundle*)tb)->get(curridx1);
              t4 = &((SoTextureCoordinateBundle*)tb)->get(curridx2);
            } else {
              assert(FALSE && "unimplemented");
            }
            tc = ((*t1)*w1 + (*t2)*w2 + (*t3)*w3 + (*t4)*w4);
          }

          glBegin(GL_TRIANGLE_FAN);

          if ((AttributeBinding)NormalBinding == PER_FACE) {
            currnormal = normals++;
            glNormal3fv((const GLfloat *)currnormal);
          }
          if ((AttributeBinding)MaterialBinding == PER_FACE) {
            mb->send(curridx1, TRUE);
          }

          // CENTER vertex
          if ((AttributeBinding)NormalBinding == PER_VERTEX) {
            glNormal3fv(nc.getValue());
          }
          if ((AttributeBinding)MaterialBinding == PER_VERTEX) {
            assert(FALSE && "unimplemented");
          }
          if (TexturingEnabled == TRUE) {
            // tb->send(?curridx?, cc, nc) was replaced by
            // glTexCoord for center vertex
            glTexCoord4fv((const GLfloat*)&tc);
          }
          if (is3d) {
            glVertex3fv((const GLfloat*)&ccd3);
          } else {
            glVertex4fv((const GLfloat*)&ccd4);
          }

          // FIRST vertex
          if ((AttributeBinding)NormalBinding == PER_VERTEX) {
            glNormal3fv(n1->getValue());
          }
          if ((AttributeBinding)MaterialBinding == PER_VERTEX) {
            assert(FALSE && "unimplemented");
          }
          if (TexturingEnabled == TRUE) {
            if (is3d) {
              if ((AttributeBinding)NormalBinding == PER_VERTEX) {
                tb->send(curridx1-1, *c1d3, *n1);
              } else {
                tb->send(curridx1-1, *c1d3, *currnormal);
              }
            } else {
              if ((AttributeBinding)NormalBinding == PER_VERTEX) {
                //tb->send(curridx1-1, *c1d4, *n1);
              } else {
                //tb->send(curridx1-1, *c1d4, *currnormal);
              }
            }
          }
          if (is3d) {
            glVertex3fv((const GLfloat*)c1d3);
          } else {
            glVertex4fv((const GLfloat*)c1d4);
          }

          // SECOND vertex
          if ((AttributeBinding)NormalBinding == PER_VERTEX) {
            glNormal3fv(n2->getValue());
          }
          if ((AttributeBinding)MaterialBinding == PER_VERTEX) {
            assert(!"unimplemented");
          }
          if (TexturingEnabled == TRUE) {
            if (is3d) {
              if ((AttributeBinding)NormalBinding == PER_VERTEX) {
                tb->send(curridx2-1, *c2d3, *n2);
              } else {
                tb->send(curridx2-1, *c2d3, *currnormal);
              }
            } else {
              if ((AttributeBinding)NormalBinding == PER_VERTEX) {
                //tb->send(curridx2-1, *c2d4, *n2);
              } else {
                //tb->send(curridx2-1, *c2d4, *currnormal);
              }
            }
          }
          if (is3d) {
            glVertex3fv((const GLfloat*)c2d3);
          } else {
            glVertex4fv((const GLfloat*)c2d4);
          }

          // FOURTH vertex
          if ((AttributeBinding)NormalBinding == PER_VERTEX) {
            glNormal3fv(n4->getValue());
          }
          if ((AttributeBinding)MaterialBinding == PER_VERTEX) {
            assert(FALSE && "unimplemented");
          }
          if (TexturingEnabled == TRUE) {
            if (is3d) {
              if ((AttributeBinding)NormalBinding == PER_VERTEX) {
                tb->send(curridx2, *c4d3, *n4);
              } else {
                tb->send(curridx2, *c4d3, *currnormal);
              }
            } else {
              if ((AttributeBinding)NormalBinding == PER_VERTEX) {
                //tb->send(curridx2, *c4d4, *n4);
              } else {
                //tb->send(curridx2, *c4d4, *currnormal);
              }
            }
          }
          if (is3d) {
            glVertex3fv((const GLfloat*)c4d3);
          } else {
            glVertex4fv((const GLfloat*)c4d4);
          }

          // THIRD vertex
          if ((AttributeBinding)NormalBinding == PER_VERTEX) {
            glNormal3fv(n3->getValue());
          }
          if ((AttributeBinding)MaterialBinding == PER_VERTEX) {
            assert(!"unimplemented");
          }
          if (TexturingEnabled == TRUE) {
            if (is3d) {
              if ((AttributeBinding)NormalBinding == PER_VERTEX) {
                tb->send(curridx1, *c3d3, *n3);
              } else {
                tb->send(curridx1, *c3d3, *currnormal);
              }
            } else {
              if ((AttributeBinding)NormalBinding == PER_VERTEX) {
                //tb->send(curridx1, *c3d4, *n3);
              } else {
                //tb->send(curridx1, *c3d4, *currnormal);
              }
            }
          }
          if (is3d) {
            glVertex3fv((const GLfloat*)c3d3);
          } else {
            glVertex4fv((const GLfloat*)c3d4);
          }

          // again FIRST vertex
          if ((AttributeBinding)NormalBinding == PER_VERTEX) {
            glNormal3fv(n1->getValue());
          }
          if ((AttributeBinding)MaterialBinding == PER_VERTEX) {
            assert(!"unimplemented");
          }
          if (TexturingEnabled == TRUE) {
            if (is3d) {
              if ((AttributeBinding)NormalBinding == PER_VERTEX) {
                tb->send(curridx1-1, *c1d3, *n1);
              } else {
                tb->send(curridx1-1, *c1d3, *currnormal);
              }
            } else {
              if ((AttributeBinding)NormalBinding == PER_VERTEX) {
                //tb->send(curridx1-1, *c1d4, *n1);
              } else {
                //tb->send(curridx1-1, *c1d4, *currnormal);
              }
            }
          }
          if (is3d) {
            glVertex3fv((const GLfloat*)c1d3);
          } else {
            glVertex4fv((const GLfloat*)c1d4);
          }

          glEnd();

          curridx1++;
          curridx2++;
        }
      }
    }
  }

#undef IDX

} } } // namespace

/*!
  \copydetails SoNode::initClass(void)
*/
void
SoQuadMesh::initClass(void)
{
  SO_NODE_INTERNAL_INIT_CLASS(SoQuadMesh, SO_FROM_INVENTOR_1);

  for (int pc = 0; pc < QUADMESH_WEIGHTS_NR; pc++)
    precompWeights[pc] = precalculateWeight(pc);
}

// -----

#define SOGL_QUADMESH_GLRENDER_CALL_FUNC(normalbinding, materialbinding, texturing, args) \
  SoGL::QuadMesh::GLRender<normalbinding, materialbinding, texturing> args

#define SOGL_QUADMESH_GLRENDER_RESOLVE_ARG3(normalbinding, materialbinding, texturing, args) \
  if (texturing) {                                                      \
    SOGL_QUADMESH_GLRENDER_CALL_FUNC(normalbinding, materialbinding, TRUE, args); \
  } else {                                                              \
    SOGL_QUADMESH_GLRENDER_CALL_FUNC(normalbinding, materialbinding, FALSE, args); \
  }

#define SOGL_QUADMESH_GLRENDER_RESOLVE_ARG2(normalbinding, materialbinding, texturing, args) \
  switch (materialbinding) {                                            \
  case SoGL::QuadMesh::OVERALL:                                         \
    SOGL_QUADMESH_GLRENDER_RESOLVE_ARG3(normalbinding, SoGL::QuadMesh::OVERALL, texturing, args); \
    break;                                                              \
  case SoGL::QuadMesh::PER_ROW:                                                \
    SOGL_QUADMESH_GLRENDER_RESOLVE_ARG3(normalbinding, SoGL::QuadMesh::PER_ROW, texturing, args); \
    break;                                                              \
  case SoGL::QuadMesh::PER_FACE:                                        \
    SOGL_QUADMESH_GLRENDER_RESOLVE_ARG3(normalbinding, SoGL::QuadMesh::PER_FACE, texturing, args); \
    break;                                                              \
  case SoGL::QuadMesh::PER_VERTEX:                                      \
    SOGL_QUADMESH_GLRENDER_RESOLVE_ARG3(normalbinding, SoGL::QuadMesh::PER_VERTEX, texturing, args); \
    break;                                                              \
  default:                                                              \
    assert(!"invalid materialbinding argument");                        \
    break;                                                              \
  }

#define SOGL_QUADMESH_GLRENDER_RESOLVE_ARG1(normalbinding, materialbinding, texturing, args) \
  switch (normalbinding) {                                              \
  case SoGL::QuadMesh::OVERALL:                                         \
    SOGL_QUADMESH_GLRENDER_RESOLVE_ARG2(SoGL::QuadMesh::OVERALL, materialbinding, texturing, args); \
    break;                                                              \
  case SoGL::QuadMesh::PER_ROW:                                         \
    SOGL_QUADMESH_GLRENDER_RESOLVE_ARG2(SoGL::QuadMesh::PER_ROW, materialbinding, texturing, args); \
    break;                                                              \
  case SoGL::QuadMesh::PER_FACE:                                        \
    SOGL_QUADMESH_GLRENDER_RESOLVE_ARG2(SoGL::QuadMesh::PER_FACE, materialbinding, texturing, args); \
    break;                                                              \
  case SoGL::QuadMesh::PER_VERTEX:                                      \
    SOGL_QUADMESH_GLRENDER_RESOLVE_ARG2(SoGL::QuadMesh::PER_VERTEX, materialbinding, texturing, args); \
    break;                                                              \
  default:                                                              \
    assert(!"invalid normalbinding argument");                          \
    break;                                                              \
  }

#define SOGL_QUADMESH_GLRENDER(normalbinding, materialbinding, texturing, args) \
  SOGL_QUADMESH_GLRENDER_RESOLVE_ARG1(normalbinding, materialbinding, texturing, args)

// Documented in superclass.
void
SoQuadMesh::GLRender(SoGLRenderAction * action)
{
  SoState * state = action->getState();
  SbBool didpush = FALSE;

  if (this->vertexProperty.getValue()) {
    state->push();
    didpush = TRUE;
    this->vertexProperty.getValue()->GLRender(action);
  }

  if (!this->shouldGLRender(action)) {
    if (didpush) state->pop();
    return;
  }

  const int rowsize = this->verticesPerRow.getValue();
  const int colsize = this->verticesPerColumn.getValue();

  // send approx number of triangles for autocache handling
  sogl_autocache_update(state, (rowsize-1)*(colsize-1)*2, FALSE);

  const SoCoordinateElement * tmp;
  const SbVec3f * normals;
  SbBool doTextures;

  SoMaterialBundle mb(action);
  SoTextureCoordinateBundle tb(action, TRUE, FALSE);
  doTextures = tb.needCoordinates();
  SbBool needNormals = !mb.isColorOnly() || tb.isFunction();

  SoVertexShape::getVertexData(action->getState(), tmp, normals,
                               needNormals);

  const SoGLCoordinateElement * coords = (SoGLCoordinateElement *)tmp;

  const int start = this->startIndex.getValue();

  Binding mbind = findMaterialBinding(action->getState());
  Binding nbind = findNormalBinding(action->getState());
  if (!needNormals) nbind = OVERALL;

  SoNormalCache * nc = NULL;

  if (needNormals && normals == NULL) {
    nc = this->generateAndReadLockNormalCache(state);
    normals = nc->getNormals();
  }

  mb.sendFirst(); // make sure we have the correct material

  // this is needed to get correct per-face material/normal
  // rendering. It must be after the SoMaterialBundle::sendFirst()
  // call.
  if ((nbind == PER_FACE) || (mbind == PER_FACE)) {
    SoGLLazyElement::sendFlatshading(state, TRUE);
  }

  // Check if precise lighting rendering is requested.
  static int preciselighting = -1;
  if (preciselighting == -1) {
    const char * env = coin_getenv("COIN_QUADMESH_PRECISE_LIGHTING");
    preciselighting = env && (atoi(env) > 0);
  }

  // Even if precise lighting rendering is requested, we need to check
  // that the rendering path is supported.
  SbBool pl = preciselighting &&
    coords->is3D() && (mbind != PER_VERTEX) && !tb.isFunction();

  // Robustness test to make sure the start index is positive
  if (start < 0) {
    static uint32_t current_errors = 0;
    if (current_errors < 1) {
      SoDebugError::postWarning("SoQuadMesh::GLRender", "Erroneous "
                                "startIndex: %d. Should be >= 0. This message will only "
                                "be shown once, but there might be more errors", start);
    }
    current_errors++;
    goto glrender_done;
  }

  // Robustness test to make sure rowsize and colsize are valid
  if (rowsize < 2 || colsize  < 2) {
    static uint32_t current_errors = 0;
    if (current_errors < 1) {
      SoDebugError::postWarning("SoQuadMesh::GLRender", "Erroneous quadmesh "
                                "dimension [%d %d] with %d coordinates available. "
                                "Must specify >= 2 rows and columns. "
                                "Aborting rendering. This message will only be shown "
                                "once, but there might be more errors.",
                                rowsize, colsize, coords->getNum() - start);
    }
    current_errors++;
    goto glrender_done;
  }

  // Robustness test to make sure rowsize and colsize specify
  // coordinates that really exist.
  if (coords->getNum() - start < rowsize * colsize) {
    static uint32_t current_errors = 0;
    if (current_errors < 1) {
      SoDebugError::postWarning("SoQuadMesh::GLRender", "Erroneous quadmesh "
                                "dimension [%d %d] with %d coordinates available. "
                                "Ignoring. This message will only be shown once, but "
                                "there might be more errors.",
                                rowsize, colsize, coords->getNum() - start);
    }
    current_errors++;
    goto glrender_done;
  }

  SOGL_QUADMESH_GLRENDER(nbind, mbind, doTextures, (coords,
                                                    normals,
                                                    &mb,
                                                    &tb,
                                                    needNormals,
                                                    rowsize,
                                                    colsize,
                                                    start,
                                                    pl));

 glrender_done:

  if (nc) {
    this->readUnlockNormalCache();
  }

  if (didpush) state->pop();
}

#undef SOGL_QUADMESH_GLRENDER_CALL_FUNC
#undef SOGL_QUADMESH_GLRENDER_RESOLVE_ARG3
#undef SOGL_QUADMESH_GLRENDER_RESOLVE_ARG2
#undef SOGL_QUADMESH_GLRENDER_RESOLVE_ARG1
#undef SOGL_QUADMESH_GLRENDER

// Documented in superclass.
SbBool
SoQuadMesh::generateDefaultNormals(SoState * state, SoNormalCache * nc)
{
  // FIXME: consider creaseAngle? pederb, 20000809

  if (verticesPerRow.getValue() < 2 || verticesPerColumn.getValue() < 2)
    return TRUE; // nothing to generate

  SbBool ccw = TRUE;
  if (SoShapeHintsElement::getVertexOrdering(state) ==
      SoShapeHintsElement::CLOCKWISE) ccw = FALSE;

  const SbVec3f * coords = SoCoordinateElement::getInstance(state)->getArrayPtr3();
  assert(coords);

  int numcoords = SoCoordinateElement::getInstance(state)->getNum() - startIndex.getValue();

  Binding binding = findNormalBinding(state);

  switch (binding) {
  case PER_VERTEX:
    nc->generatePerVertexQuad(coords + startIndex.getValue(),
                              numcoords,
                              verticesPerRow.getValue(),
                              verticesPerColumn.getValue(),
                              ccw);
    break;
  case PER_FACE:
    nc->generatePerFaceQuad(coords + startIndex.getValue(),
                            numcoords,
                            verticesPerRow.getValue(),
                            verticesPerColumn.getValue(),
                            ccw);
    break;
  case PER_ROW:
    nc->generatePerRowQuad(coords + startIndex.getValue(),
                           numcoords,
                           verticesPerRow.getValue(),
                           verticesPerColumn.getValue(),
                           ccw);
    break;
  case OVERALL:
    break;
  default:
    assert(0);
    return FALSE;
    break;
  }
  return TRUE;
}

// Documented in superclass.
void
SoQuadMesh::getPrimitiveCount(SoGetPrimitiveCountAction *action)
{
  if (!this->shouldPrimitiveCount(action)) return;

  action->addNumTriangles(2 * this->verticesPerRow.getValue() *
                          this->verticesPerColumn.getValue());
}

// Documented in superclass. Overridden to return FALSE. Normals are
// generated in normal cache.
SbBool
SoQuadMesh::generateDefaultNormals(SoState * /* state */, SoNormalBundle * /* nb */)
{
  return FALSE;
}

// Documented in superclass.
void
SoQuadMesh::generatePrimitives(SoAction *action)
{
  SoState * state = action->getState();

  if (this->vertexProperty.getValue()) {
    state->push();
    this->vertexProperty.getValue()->doAction(action);
  }

  const int rowsize = this->verticesPerRow.getValue();
  const int colsize = this->verticesPerColumn.getValue();

  if (rowsize < 2 || colsize < 2) {
    if (vertexProperty.getValue()) state->pop();
    return;
  }
  const SoCoordinateElement *coords;
  const SbVec3f * normals;
  SbBool doTextures;
  SbBool needNormals = TRUE;

  SoVertexShape::getVertexData(action->getState(), coords, normals,
                               needNormals);

  SoTextureCoordinateBundle tb(action, FALSE, FALSE);
  doTextures = tb.needCoordinates();

  int start = this->startIndex.getValue();

  Binding mbind = findMaterialBinding(action->getState());
  Binding nbind = findNormalBinding(action->getState());

  SoNormalCache * nc = NULL;

  if (needNormals && normals == NULL) {
    nc = this->generateAndReadLockNormalCache(state);
    normals = nc->getNormals();
  }

  SbVec3f dummynormal(0.0f, 0.0f, 1.0f);
  const SbVec3f * currnormal = &dummynormal;
  if (normals) currnormal = normals;

  SoPrimitiveVertex vertex;
  SoFaceDetail faceDetail;
  SoPointDetail pointDetail;
  vertex.setDetail(&pointDetail);

  if (nbind == OVERALL && needNormals) {
    vertex.setNormal(*currnormal);
  }

  int curridx; // for optimization only

#define IDX(r,c) ((r)*rowsize+(c))

  int normnr = 0;
  int midx = 0;
  for (int i = 0; i < colsize-1; i++) {
    int j = 0;
    this->beginShape(action, QUAD_STRIP, &faceDetail);
    if (nbind == PER_ROW) {
      pointDetail.setNormalIndex(normnr);
      currnormal = &normals[normnr++];
      vertex.setNormal(*currnormal);
    }
    if (mbind == PER_ROW) {
      pointDetail.setMaterialIndex(midx);
      vertex.setMaterialIndex(midx++);
    }
    SbBool first = TRUE;
    faceDetail.setFaceIndex(0);
    for (j = 0; j < rowsize; j++) {
      curridx = IDX(i,j);
      if (nbind == PER_VERTEX) {
        pointDetail.setNormalIndex(curridx);
        currnormal = &normals[curridx];
        vertex.setNormal(*currnormal);
      }
      else if (nbind == PER_FACE) {
        pointDetail.setNormalIndex(normnr);
        currnormal = &normals[normnr++];
        vertex.setNormal(*currnormal);
      }
      if (mbind == PER_VERTEX) {
        pointDetail.setMaterialIndex(curridx);
        vertex.setMaterialIndex(curridx);
      }
      else if (mbind == PER_FACE) {
        pointDetail.setMaterialIndex(midx);
        vertex.setMaterialIndex(midx++);
      }
      if (doTextures) {
        if (tb.isFunction()) {
          vertex.setTextureCoords(tb.get(coords->get3(start+curridx), *currnormal));
          if (tb.needIndices()) pointDetail.setTextureCoordIndex(curridx);
        }
        else {
          pointDetail.setTextureCoordIndex(curridx);
          vertex.setTextureCoords(tb.get(curridx));
        }
      }
      pointDetail.setCoordinateIndex(start + curridx);
      vertex.setPoint(coords->get3(start + curridx));
      this->shapeVertex(&vertex);

      curridx = IDX(i+1,j);
      if (nbind == PER_VERTEX) {
        pointDetail.setNormalIndex(curridx);
        currnormal = &normals[curridx];
        vertex.setNormal(*currnormal);
      }
      if (mbind == PER_VERTEX) {
        pointDetail.setMaterialIndex(curridx);
        vertex.setMaterialIndex(curridx);
      }
      if (doTextures) {
        if (tb.isFunction()) {
          vertex.setTextureCoords(tb.get(coords->get3(start+curridx), *currnormal));
          if (tb.needIndices()) pointDetail.setTextureCoordIndex(curridx);
        }
        else {
          pointDetail.setTextureCoordIndex(curridx);
          vertex.setTextureCoords(tb.get(curridx));
        }
      }
      pointDetail.setCoordinateIndex(start + curridx);
      vertex.setPoint(coords->get3(start + curridx));
      this->shapeVertex(&vertex);
      if (first) first = FALSE;
      else faceDetail.incFaceIndex();
    }
    this->endShape();
    faceDetail.incPartIndex();
  }
#undef IDX

  if (nc) {
    this->readUnlockNormalCache();
  }

  if (this->vertexProperty.getValue())
    state->pop();

}

#undef QUADMESH_WEIGHTS_NR

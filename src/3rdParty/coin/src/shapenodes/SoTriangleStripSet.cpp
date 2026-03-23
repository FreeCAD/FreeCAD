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
  \class SoTriangleStripSet SoTriangleStripSet.h Inventor/nodes/SoTriangleStripSet.h
  \brief The SoTriangleStripSet class is used to render and control non-indexed triangle strips.

  \ingroup coin_nodes

  Triangle strips are specified using the numVertices field.

  Coordinates, normals, materials and texture coordinates are all
  fetched in order from the current state or from the vertexProperty
  node if set. (To render triangle strips from vertex indices, use the
  SoIndexedTriangleStripSet node.)

  The numVertices field may be used for multiple strips, coordinates
  will be fetched with a monotonically increasing index from the
  coordinates on the traversal state stack (i.e. typically from the
  last SoCoordinate3 node encountered during traversal).

  For example, if numVertices is set to [3, 4, 5, 3], this node would
  specify a triangle from coordinates 0, 1 and 2, a triangle strip
  from coordinates 3, 4, 5 and 6, a triangle strip from coordinates 7,
  8, 9, 10 and 11 and finally a triangle from coordinates 12, 13, 14.

  Or to put it another way: in a triangle strip there will always be two
  vertices more than there are triangles.  Realize that you are
  handling data on the vertex level (not polygon level), and that the
  triangles are laid out like this, given 5 vertices:

  \verbatim

  1-----3-----5
   \   / \   /
    \ /   \ /
     2-----4

  \endverbatim

  The above figure in scene graph file format:

  \verbatim
  #Inventor V2.1 ascii

  Separator {
    Coordinate3 {
      point [ -2 1 0, -1 -1 0, 0 1 0, 1 -1 0, 2 1 0 ]
    }

    TriangleStripSet {
      numVertices [ 5 ]
    }
  }
  \endverbatim

  The scene graph above in a viewer:

  <center>
  \image html trianglestripset.png "Rendering of Example File"
  </center>


  Strips are converted into triangles the way OpenGL does it, of
  course, so for the dirty details, check out the documentation of
  the OpenGL \c GL_TRIANGLE_STRIP primitive rendering type.

  Binding PER_PART (per strip), PER_VERTEX, PER_FACE or OVERALL can be
  set for material, and normals. The default material binding is
  OVERALL. The default normal binding is PER_VERTEX.

  <b>FILE FORMAT/DEFAULTS:</b>
  \code
    TriangleStripSet {
        vertexProperty NULL
        startIndex 0
        numVertices -1
    }
  \endcode

  \sa SoIndexedTriangleStripSet
*/

#include <Inventor/nodes/SoTriangleStripSet.h>
#include "coindefs.h"

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H

#include <Inventor/misc/SoState.h>
#include <Inventor/SoPrimitiveVertex.h>
#include <Inventor/bundles/SoTextureCoordinateBundle.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/system/gl.h>
#include <Inventor/actions/SoGetPrimitiveCountAction.h>
#include <Inventor/elements/SoGLLazyElement.h>
#include <Inventor/elements/SoGLCoordinateElement.h>
#include <Inventor/elements/SoNormalBindingElement.h>
#include <Inventor/elements/SoMaterialBindingElement.h>
#include <Inventor/errors/SoDebugError.h>
#include <Inventor/bundles/SoMaterialBundle.h>
#include <Inventor/elements/SoShapeHintsElement.h>
#include <Inventor/elements/SoCreaseAngleElement.h>
#include <Inventor/caches/SoNormalCache.h>
#include <Inventor/misc/SoNormalGenerator.h>
#include <Inventor/details/SoFaceDetail.h>
#include <Inventor/details/SoPointDetail.h>

#include "nodes/SoSubNodeP.h"
#include "rendering/SoGL.h"

/*!
  \var SoMFInt32 SoTriangleStripSet::numVertices

  Specifies the number of vertices in each triangle strip. The
  vertices are fetched from the current coordinate node, or from the
  vertexProperty field if present.
*/

SO_NODE_SOURCE(SoTriangleStripSet);

/*!
  Constructor.
*/
SoTriangleStripSet::SoTriangleStripSet()
{
  SO_NODE_INTERNAL_CONSTRUCTOR(SoTriangleStripSet);

  SO_NODE_ADD_FIELD(numVertices, (-1));
}

/*!
  Destructor.
*/
SoTriangleStripSet::~SoTriangleStripSet()
{
}

// doc from parent
void
SoTriangleStripSet::computeBBox(SoAction * action,
                                SbBox3f & box, SbVec3f & center)
{
  int32_t numvertices = 0;
  for (int i=0; i < this->numVertices.getNum(); i++)
    numvertices += this->numVertices[i];

  inherited::computeCoordBBox(action, numvertices, box, center);
}

/*!
  \COININTERNAL
*/
SoTriangleStripSet::Binding
SoTriangleStripSet::findMaterialBinding(SoState * const state) const
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
    binding = PER_STRIP;
    break;
  case SoMaterialBindingElement::PER_FACE:
  case SoMaterialBindingElement::PER_FACE_INDEXED:
    binding = PER_FACE;
    break;
  default:
    binding = OVERALL;
#if COIN_DEBUG
    SoDebugError::postWarning("SoTriangleStripSet::findMaterialBinding",
                              "unknown material binding setting");
#endif // COIN_DEBUG
    break;
  }
  return binding;
}

/*!
  \COININTERNAL
*/
SoTriangleStripSet::Binding
SoTriangleStripSet::findNormalBinding(SoState * const state) const
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
    binding = PER_STRIP;
    break;
  case SoNormalBindingElement::PER_FACE:
  case SoNormalBindingElement::PER_FACE_INDEXED:
    binding = PER_FACE;
    break;
  default:
    binding = PER_VERTEX;
#if COIN_DEBUG
    SoDebugError::postWarning("SoTriangleStripSet::findNormalBinding",
                              "unknown normal binding setting");
#endif // COIN_DEBUG
    break;
  }
  return binding;
}

namespace { namespace SoGL { namespace TriStripSet {

  enum AttributeBinding {
    OVERALL = 0,
    PER_STRIP = 1,
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
                       int COIN_UNUSED_ARG(nbind),
                       int COIN_UNUSED_ARG(mbind),
                       int COIN_UNUSED_ARG(doTextures),
                       int32_t idx,
                       const int32_t *ptr,
                       const int32_t *end,
                       SbBool needNormals)
  {
    const SbVec3f * coords3d = NULL;
    const SbVec4f * coords4d = NULL;
    const SbBool is3d = coords->is3D();
    if (is3d) {
      coords3d = coords->getArrayPtr3();
    }
    else {
      coords4d = coords->getArrayPtr4();
    }

    // This is the same code as in SoGLCoordinateElement::send().
    // It is inlined here for speed (~15% speed increase).
#define SEND_VERTEX(_idx_)                                      \
    if (is3d) glVertex3fv((const GLfloat*) (coords3d + _idx_)); \
    else glVertex4fv((const GLfloat*) (coords4d + _idx_));

    int matnr = 0;
    int texnr = 0;
    int n;

    SbVec3f dummynormal(0.0f, 0.0f, 1.0f);
    const SbVec3f * currnormal = &dummynormal;
    if (normals) currnormal = normals;
    if ((AttributeBinding)NormalBinding == OVERALL) {
      if (needNormals) glNormal3fv((const GLfloat *)currnormal);
    }

    while (ptr < end) {
      n = *ptr++ - 2;
      assert(n > 0);

      glBegin(GL_TRIANGLE_STRIP);

      if ((AttributeBinding)NormalBinding == PER_VERTEX ||
          (AttributeBinding)NormalBinding == PER_FACE ||
          (AttributeBinding)NormalBinding == PER_STRIP) {
        currnormal = normals++;
        glNormal3fv((const GLfloat *)currnormal);
      }
      if ((AttributeBinding)MaterialBinding == PER_STRIP ||
          (AttributeBinding)MaterialBinding == PER_FACE ||
          (AttributeBinding)MaterialBinding == PER_VERTEX) {
        mb->send(matnr++, TRUE);
      }
      
      if (TexturingEnabled == TRUE) {
        tb->send(texnr++, coords->get3(idx), *currnormal);
      }
      SEND_VERTEX(idx); // vertex 1
      idx++;

      if ((AttributeBinding)NormalBinding == PER_VERTEX) {
        currnormal = normals++;
        glNormal3fv((const GLfloat *)currnormal);
      }
      if ((AttributeBinding)MaterialBinding == PER_VERTEX) {
        mb->send(matnr++, TRUE);
      }

      // workaround for nvidia color-per-face-bug
      if ((AttributeBinding)MaterialBinding == PER_FACE ||
          (AttributeBinding)MaterialBinding == PER_STRIP) {
        mb->send(matnr-1, TRUE);
      }
      // end of nvidia workaround

      if (TexturingEnabled == TRUE) {
        tb->send(texnr++, coords->get3(idx), *currnormal);
      }
      SEND_VERTEX(idx); // vertex 2
      idx++;

      while (n--) {
        if ((AttributeBinding)NormalBinding == PER_FACE ||
            (AttributeBinding)NormalBinding == PER_VERTEX) {
          currnormal = normals++;
          glNormal3fv((const GLfloat *)currnormal);
        }
        if ((AttributeBinding)MaterialBinding == PER_FACE ||
            (AttributeBinding)MaterialBinding == PER_VERTEX) {
          mb->send(matnr++, TRUE);
        }

        // workaround for nvidia color-per-face-bug
        if ((AttributeBinding)MaterialBinding == PER_STRIP) {
          mb->send(matnr-1, TRUE);
        }
        // end of nvidia workaround

        if (TexturingEnabled == TRUE) {
          tb->send(texnr++, coords->get3(idx), *currnormal);
        }
        SEND_VERTEX(idx); // vertex 3-n
        idx++;
      }
      glEnd();
    }
#undef SEND_VERTEX
  }

} } } // namespace

/*!
  \copydetails SoEngine::initClass(void)
*/
void
SoTriangleStripSet::initClass(void)
{
  SO_NODE_INTERNAL_INIT_CLASS(SoTriangleStripSet, SO_FROM_INVENTOR_1);
}

#define SOGL_TRISTRIPSET_GLRENDER_CALL_FUNC(normalbinding, materialbinding, texturing, args) \
  SoGL::TriStripSet::GLRender<normalbinding, materialbinding, texturing> args

#define SOGL_TRISTRIPSET_GLRENDER_RESOLVE_ARG3(normalbinding, materialbinding, texturing, args) \
  if (texturing) {                                                        \
    SOGL_TRISTRIPSET_GLRENDER_CALL_FUNC(normalbinding, materialbinding, TRUE, args); \
  } else {                                                                \
    SOGL_TRISTRIPSET_GLRENDER_CALL_FUNC(normalbinding, materialbinding, FALSE, args); \
  }

#define SOGL_TRISTRIPSET_GLRENDER_RESOLVE_ARG2(normalbinding, materialbinding, texturing, args) \
  switch (materialbinding) {                                              \
  case SoGL::TriStripSet::OVERALL:                                        \
    SOGL_TRISTRIPSET_GLRENDER_RESOLVE_ARG3(normalbinding, OVERALL, texturing, args); \
    break;                                                                \
  case SoGL::TriStripSet::PER_STRIP:                                      \
    SOGL_TRISTRIPSET_GLRENDER_RESOLVE_ARG3(normalbinding, PER_STRIP, texturing, args); \
    break;                                                                \
  case SoGL::TriStripSet::PER_FACE:                                       \
    SOGL_TRISTRIPSET_GLRENDER_RESOLVE_ARG3(normalbinding, PER_FACE, texturing, args); \
    break;                                                                \
  case SoGL::TriStripSet::PER_VERTEX:                                     \
    SOGL_TRISTRIPSET_GLRENDER_RESOLVE_ARG3(normalbinding, PER_VERTEX, texturing, args); \
    break;                                                                \
  default:                                                                \
    assert(!"invalid tristripset normal binding");                        \
    break;                                                                \
  }

#define SOGL_TRISTRIPSET_GLRENDER_RESOLVE_ARG1(normalbinding, materialbinding, texturing, args) \
  switch (normalbinding) {                                                \
  case SoGL::TriStripSet::OVERALL:                                        \
    SOGL_TRISTRIPSET_GLRENDER_RESOLVE_ARG2(OVERALL, materialbinding, texturing, args); \
    break;                                                                \
  case SoGL::TriStripSet::PER_STRIP:                                      \
    SOGL_TRISTRIPSET_GLRENDER_RESOLVE_ARG2(PER_STRIP, materialbinding, texturing, args); \
    break;                                                                \
  case SoGL::TriStripSet::PER_FACE:                                       \
    SOGL_TRISTRIPSET_GLRENDER_RESOLVE_ARG2(PER_FACE, materialbinding, texturing, args); \
    break;                                                                \
  case SoGL::TriStripSet::PER_VERTEX:                                     \
    SOGL_TRISTRIPSET_GLRENDER_RESOLVE_ARG2(PER_VERTEX, materialbinding, texturing, args); \
    break;                                                                \
  default:                                                                \
    assert(!"invalid tristripset normal binding");                        \
    break;                                                                \
  }

#define SOGL_TRISTRIPSET_GLRENDER(normalbinding, materialbinding, texturing, args) \
  SOGL_TRISTRIPSET_GLRENDER_RESOLVE_ARG1(normalbinding, materialbinding, texturing, args)

// doc from parent
void
SoTriangleStripSet::GLRender(SoGLRenderAction * action)
{
  int32_t idx = this->startIndex.getValue();
  int32_t dummyarray[1];
  const int32_t * ptr = this->numVertices.getValues(0);
  const int32_t * end = ptr + this->numVertices.getNum();
  if ((end-ptr == 1) && ptr[0] == 0) return; // nothing to render

  SoState * state = action->getState();
  this->fixNumVerticesPointers(state, ptr, end, dummyarray);

  SbBool didpush = FALSE;

  if (this->vertexProperty.getValue()) {
    state->push();
    didpush = TRUE;
    this->vertexProperty.getValue()->GLRender(action);
  }

  Binding mbind = this->findMaterialBinding(action->getState());
  Binding nbind = this->findNormalBinding(action->getState());

  if ((nbind == PER_FACE && mbind != PER_VERTEX) ||
      (mbind == PER_FACE && nbind != PER_VERTEX)) {
    if (!didpush) {
      didpush = TRUE;
      state->push();
    }
    SoLazyElement::setShadeModel(state, TRUE);
  }

  if (!this->shouldGLRender(action)) {
    if (didpush)
      state->pop();
    return;
  }

  const SoCoordinateElement * tmp;
  const SbVec3f * normals;
  SbBool doTextures;

  SoTextureCoordinateBundle tb(action, TRUE, FALSE);
  doTextures = tb.needCoordinates();

  SoMaterialBundle mb(action);
  SbBool needNormals = !mb.isColorOnly() || tb.isFunction();

  SoVertexShape::getVertexData(action->getState(), tmp, normals,
                               needNormals);

  const SoGLCoordinateElement * coords = (SoGLCoordinateElement *)tmp;

  if (!needNormals) nbind = OVERALL;


  SoNormalCache * nc = NULL;
  if (needNormals && normals == NULL) {
    nc = this->generateAndReadLockNormalCache(state);
    normals = nc->getNormals();
  }

  mb.sendFirst(); // make sure we have the correct material

  SOGL_TRISTRIPSET_GLRENDER(nbind, mbind, doTextures, (coords,
                                                       normals,
                                                       &mb,
                                                       &tb,
                                                       nbind,
                                                       mbind,
                                                       doTextures,
                                                       idx,
                                                       ptr,
                                                       end,
                                                       needNormals));

  if (nc) {
    this->readUnlockNormalCache();
  }

  if (didpush)
    state->pop();

  int numv = this->numVertices.getNum();
  // send approx number of triangles for autocache handling
  sogl_autocache_update(state, numv ?
                        (this->numVertices[0]-2)*numv : 0, FALSE);
}

#undef SOGL_TRISTRIPSET_GLRENDER_CALL_FUNC
#undef SOGL_TRISTRIPSET_GLRENDER_RESOLVE_ARG1
#undef SOGL_TRISTRIPSET_GLRENDER_RESOLVE_ARG2
#undef SOGL_TRISTRIPSET_GLRENDER_RESOLVE_ARG3
#undef SOGL_TRISTRIPSET_GLRENDER

// doc from parent
SbBool
SoTriangleStripSet::generateDefaultNormals(SoState * state, SoNormalCache * nc)
{
  SbBool ccw = TRUE;
  if (SoShapeHintsElement::getVertexOrdering(state) ==
      SoShapeHintsElement::CLOCKWISE) ccw = FALSE;

  const SoCoordinateElement * coords =
    SoCoordinateElement::getInstance(state);
  assert(coords);

  SoNormalGenerator * gen =
    new SoNormalGenerator(ccw, this->numVertices.getNum()*3);

  int32_t idx = startIndex.getValue();
  int32_t dummyarray[1];
  const int32_t * ptr = numVertices.getValues(0);
  const int32_t * end = ptr + numVertices.getNum();
  this->fixNumVerticesPointers(state, ptr, end, dummyarray);
  const int32_t * start = ptr;

  while (ptr < end) {
    int num = *ptr++ - 3;
    assert(num >= 0);
    SbVec3f striptri[3];
    striptri[0] = coords->get3(idx++);
    striptri[1] = coords->get3(idx++);
    striptri[2] = coords->get3(idx++);
    gen->triangle(striptri[0], striptri[1], striptri[2]);
    SbBool flag = FALSE;
    while (num--) {
      if (flag) striptri[1] = striptri[2];
      else striptri[0] = striptri[2];
      flag = !flag;
      striptri[2] = coords->get3(idx++);
      gen->triangle(striptri[0], striptri[1], striptri[2]);
    }
  }

  const ptrdiff_t range = end - start;
  switch (this->findNormalBinding(state)) {
  case OVERALL:
    gen->generateOverall();
    break;
  case PER_STRIP:
    gen->generatePerStrip(start, (int)range);
    break;
  case PER_FACE:
    gen->generatePerFace();
    break;
  case PER_VERTEX:
    gen->generate(SoCreaseAngleElement::get(state), start, (int)range);
    break;
  }
  nc->set(gen);
  return TRUE;
}

// doc from parent
void
SoTriangleStripSet::getPrimitiveCount(SoGetPrimitiveCountAction *action)
{
  if (!this->shouldPrimitiveCount(action)) return;

  int32_t dummyarray[1];
  const int32_t * ptr = numVertices.getValues(0);
  const int32_t * end = ptr + numVertices.getNum();
  const ptrdiff_t range = end - ptr;
  if ((range == 1) && ptr[0] == 0) return;

  this->fixNumVerticesPointers(action->getState(), ptr, end, dummyarray);

  if (action->canApproximateCount()) {
    // this is a wild guess, disable? pederb, 20000131
    action->addNumTriangles((int)(range * 8));
  }
  else {
    int cnt = 0;
    while (ptr < end) {
      cnt += *ptr++ - 2;
    }
    action->addNumTriangles(cnt);
  }
}

// Documented in superclass.
SbBool
SoTriangleStripSet::generateDefaultNormals(SoState * /* state */,
                                           SoNormalBundle * /* nb */)
{
  // Normals are generated in normal cache.
  return FALSE;
}

// Documented in superclass.
void
SoTriangleStripSet::generatePrimitives(SoAction *action)
{
  if (this->numVertices.getNum() == 1 &&
      this->numVertices[0] == 0) return;

  SoState * state = action->getState();

  if (this->vertexProperty.getValue()) {
    state->push();
    this->vertexProperty.getValue()->doAction(action);
  }

  const SoCoordinateElement * coords;
  const SbVec3f * normals;
  SbBool doTextures;
  SbBool needNormals = TRUE;

  SoVertexShape::getVertexData(action->getState(), coords, normals,
                               needNormals);

  SoTextureCoordinateBundle tb(action, FALSE, FALSE);
  doTextures = tb.needCoordinates();

  Binding mbind = this->findMaterialBinding(action->getState());
  Binding nbind = this->findNormalBinding(action->getState());

  SoNormalCache * nc = NULL;

  if (needNormals && normals == NULL) {
    nc = this->generateAndReadLockNormalCache(state);
    normals = nc->getNormals();
  }

  int32_t idx = startIndex.getValue();
  int32_t dummyarray[1];
  const int32_t * ptr = numVertices.getValues(0);
  const int32_t * end = ptr + numVertices.getNum();
  this->fixNumVerticesPointers(action->getState(), ptr, end, dummyarray);

  int matnr = 0;
  int texnr = 0;
  int normnr = 0;
  int n;

  SbVec3f dummynormal(0.0f, 0.0f, 1.0f);
  const SbVec3f * currnormal = &dummynormal;
  if (normals) currnormal = normals;

  SoPrimitiveVertex vertex;
  SoFaceDetail faceDetail;
  SoPointDetail pointDetail;

  vertex.setNormal(*currnormal);
  vertex.setDetail(&pointDetail);

  while (ptr < end) {
    n = *ptr++ - 3;
    if (n < 0) continue; // triangle with < 3 vertices, try next one

    faceDetail.setFaceIndex(0);
    this->beginShape(action, TRIANGLE_STRIP, &faceDetail);

    // first vertex
    if (nbind != OVERALL) {
      pointDetail.setNormalIndex(normnr);
      currnormal = &normals[normnr++];
      vertex.setNormal(*currnormal);
    }
    if (mbind != OVERALL) {
      pointDetail.setMaterialIndex(matnr);
      vertex.setMaterialIndex(matnr++);
    }
    if (doTextures) {
      if (tb.isFunction()) {
        vertex.setTextureCoords(tb.get(coords->get3(idx), *currnormal));
        if (tb.needIndices()) pointDetail.setTextureCoordIndex(texnr++);
      }
      else {
        pointDetail.setTextureCoordIndex(texnr);
        vertex.setTextureCoords(tb.get(texnr++));
      }
    }
    pointDetail.setCoordinateIndex(idx);
    vertex.setPoint(coords->get3(idx++));
    this->shapeVertex(&vertex);

    // second vertex
    if (nbind == PER_VERTEX) {
      pointDetail.setNormalIndex(normnr);
      currnormal = &normals[normnr++];
      vertex.setNormal(*currnormal);
    }
    if (mbind == PER_VERTEX) {
      pointDetail.setMaterialIndex(matnr);
      vertex.setMaterialIndex(matnr++);
    }
    if (doTextures) {
      if (tb.isFunction()) {
        vertex.setTextureCoords(tb.get(coords->get3(idx), *currnormal));
        if (tb.needIndices()) pointDetail.setTextureCoordIndex(texnr++);
      }
      else {
        pointDetail.setTextureCoordIndex(texnr);
        vertex.setTextureCoords(tb.get(texnr++));
      }
    }
    pointDetail.setCoordinateIndex(idx);
    vertex.setPoint(coords->get3(idx++));
    this->shapeVertex(&vertex);

    // third vertex
    if (nbind == PER_VERTEX) {
      pointDetail.setNormalIndex(normnr);
      currnormal = &normals[normnr++];
      vertex.setNormal(*currnormal);
    }
    if (mbind == PER_VERTEX) {
      pointDetail.setMaterialIndex(matnr);
      vertex.setMaterialIndex(matnr++);
    }
    if (doTextures) {
      if (tb.isFunction()) {
        vertex.setTextureCoords(tb.get(coords->get3(idx), *currnormal));
        if (tb.needIndices()) pointDetail.setTextureCoordIndex(texnr++);
      }
      else {
        pointDetail.setTextureCoordIndex(texnr);
        vertex.setTextureCoords(tb.get(texnr++));
      }
    }
    pointDetail.setCoordinateIndex(idx);
    vertex.setPoint(coords->get3(idx++));
    this->shapeVertex(&vertex);

    // loop for vertices 4-n
    while (n--) {
      if (nbind >= PER_FACE) {
        pointDetail.setNormalIndex(normnr);
        currnormal = &normals[normnr++];
        vertex.setNormal(*currnormal);
      }
      if (mbind >= PER_FACE) {
        pointDetail.setMaterialIndex(matnr);
        vertex.setMaterialIndex(matnr++);
      }
      if (doTextures) {
        if (tb.isFunction()) {
          vertex.setTextureCoords(tb.get(coords->get3(idx), *currnormal));
          if (tb.needIndices()) pointDetail.setTextureCoordIndex(texnr++);
        }
        else {
          pointDetail.setCoordinateIndex(texnr);
          vertex.setTextureCoords(tb.get(texnr++));
        }
      }
      pointDetail.setCoordinateIndex(idx);
      vertex.setPoint(coords->get3(idx++));
      this->shapeVertex(&vertex);
      faceDetail.incFaceIndex();
    }
    this->endShape();
    faceDetail.incPartIndex();
  }

  if (nc) {
    this->readUnlockNormalCache();
  }

  if (this->vertexProperty.getValue())
    state->pop();
}

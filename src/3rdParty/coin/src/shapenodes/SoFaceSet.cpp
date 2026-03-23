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
  \class SoFaceSet SoFaceSet.h Inventor/nodes/SoFaceSet.h
  \brief The SoFaceSet class is used to render and organize non-indexed polygonal face data.

  \ingroup coin_nodes

  Facesets are specified using the numVertices field. Coordinates,
  normals, materials and texture coordinates are fetched in order from
  the current state or from the vertexProperty node if set. For
  example, if numVertices is set to [3, 4, 5, 3], this node would
  specify a triangle from coordinates 0, 1 and 2, a quad from
  coordinates 3, 4, 5 and 6, a polygon from coordinates 7, 8, 9, 10
  and 11 and finally a triangle from coordinates 12, 13, 14.

  Binding PER_VERTEX, PER_FACE or OVERALL can be set for material,
  and normals. The default material binding is OVERALL. The default
  normal binding is PER_VERTEX.

  For more elaborate documentation about face sets, see the class
  documentation of the SoIndexedFaceSet node, which contains examples
  and more detailed information about various issues also relevant for
  this node.

  <b>FILE FORMAT/DEFAULTS:</b>
  \code
    FaceSet {
        vertexProperty NULL
        startIndex 0
        numVertices -1
    }
  \endcode

  \sa SoIndexedFaceSet, SoIndexedTriangleStripSet
*/

#include <Inventor/nodes/SoFaceSet.h>
#include "coindefs.h"

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H

#include <Inventor/misc/SoState.h>
#include <Inventor/SoPrimitiveVertex.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/system/gl.h>
#include <Inventor/elements/SoGLCoordinateElement.h>
#include <Inventor/elements/SoNormalBindingElement.h>
#include <Inventor/elements/SoMaterialBindingElement.h>
#include <Inventor/elements/SoVertexAttributeBindingElement.h>
#include <Inventor/errors/SoDebugError.h>
#include <Inventor/bundles/SoMaterialBundle.h>
#include <Inventor/bundles/SoVertexAttributeBundle.h>
#include <Inventor/elements/SoShapeHintsElement.h>
#include <Inventor/elements/SoCreaseAngleElement.h>
#include <Inventor/caches/SoNormalCache.h>
#include <Inventor/misc/SoNormalGenerator.h>
#include <Inventor/misc/SoGLDriverDatabase.h>
#include <Inventor/bundles/SoTextureCoordinateBundle.h>
#include <Inventor/details/SoFaceDetail.h>
#include <Inventor/actions/SoGetPrimitiveCountAction.h>
#include <Inventor/caches/SoConvexDataCache.h>
#include <Inventor/elements/SoCacheElement.h>
#include <Inventor/elements/SoModelMatrixElement.h>
#include <Inventor/elements/SoGLLazyElement.h>
#include <Inventor/elements/SoGLVBOElement.h>

#ifdef COIN_THREADSAFE
#include <Inventor/threads/SbRWMutex.h>
#endif // COIN_THREADSAFE

#include "nodes/SoSubNodeP.h"
#include "rendering/SoVBO.h"
#include "rendering/SoGL.h"

// *************************************************************************

/*!
  \var SoMFInt32 SoFaceSet::numVertices
  Used to specify faces. Each entry specifies the number of coordinates
  in a face. The coordinates are taken in order from the state or from
  the vertexProperty node.
*/

// *************************************************************************

// for concavestatus
#define STATUS_UNKNOWN 0
#define STATUS_CONVEX  1
#define STATUS_CONCAVE 2

#define UNKNOWN_TYPE -1
#define MIXED_TYPE -2

// *************************************************************************

#ifndef DOXYGEN_SKIP_THIS
class SoFaceSetP {
public:
  SoFaceSetP(void)
#ifdef COIN_THREADSAFE
    : convexmutex(SbRWMutex::READ_PRECEDENCE)
#endif // COIN_THREADSAFE
  { }
  SoConvexDataCache * convexCache;
  int concavestatus;
  int primitivetype;

#ifdef COIN_THREADSAFE
  // FIXME: a mutex for every instance seems a bit excessive,
  // especially since Microsoft Windows might have rather strict limits on the
  // total amount of mutex resources a process (or even a user) can
  // allocate. so consider making this a class-wide instance instead.
  // -mortene.
  SbRWMutex convexmutex;
#endif // COIN_THREADSAFE

  void readLockConvexCache(void) {
#ifdef COIN_THREADSAFE
    this->convexmutex.readLock();
#endif // COIN_THREADSAFE
  }
  void readUnlockConvexCache(void) {
#ifdef COIN_THREADSAFE
    this->convexmutex.readUnlock();
#endif // COIN_THREADSAFE
  }
  void writeLockConvexCache(void) {
#ifdef COIN_THREADSAFE
    this->convexmutex.writeLock();
#endif // COIN_THREADSAFE
  }
  void writeUnlockConvexCache(void) {
#ifdef COIN_THREADSAFE
    this->convexmutex.writeUnlock();
#endif // COIN_THREADSAFE
  }
};
#endif // DOXYGEN_SKIP_THIS

#define PRIVATE(obj) ((obj)->pimpl)

// *************************************************************************

SO_NODE_SOURCE(SoFaceSet);

/*!
  Constructor.
*/
SoFaceSet::SoFaceSet()
{
  PRIVATE(this) = new SoFaceSetP;
  PRIVATE(this)->convexCache = NULL;
  PRIVATE(this)->concavestatus = STATUS_UNKNOWN;
  PRIVATE(this)->primitivetype = UNKNOWN_TYPE;

  SO_NODE_INTERNAL_CONSTRUCTOR(SoFaceSet);

  SO_NODE_ADD_FIELD(numVertices, (-1));
}

/*!
  Destructor.
*/
SoFaceSet::~SoFaceSet()
{
  if (PRIVATE(this)->convexCache) PRIVATE(this)->convexCache->unref();
  delete PRIVATE(this);
}

// doc from parent
void
SoFaceSet::computeBBox(SoAction * action, SbBox3f & box, SbVec3f & center)
{
  int32_t numvertices = 0;
  for (int i=0; i < this->numVertices.getNum(); i++)
    numvertices += this->numVertices[i];

  inherited::computeCoordBBox(action, numvertices, box, center);
}

//
// translates current material binding to the internal enum
//
SoFaceSet::Binding
SoFaceSet::findMaterialBinding(SoState * const state) const
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
  case SoMaterialBindingElement::PER_FACE:
  case SoMaterialBindingElement::PER_FACE_INDEXED:
    binding = PER_FACE;
    break;
  default:
    binding = OVERALL;
#if COIN_DEBUG
    SoDebugError::postWarning("SoFaceSet::findMaterialBinding",
                              "unknown material binding setting");
#endif // COIN_DEBUG
    break;
  }
  return binding;
}


//
// translates current normal binding to the internal enum
//
SoFaceSet::Binding
SoFaceSet::findNormalBinding(SoState * const state) const
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
  case SoNormalBindingElement::PER_FACE:
  case SoNormalBindingElement::PER_FACE_INDEXED:
    binding = PER_FACE;
    break;
  default:
    binding = PER_VERTEX;
#if COIN_DEBUG
    SoDebugError::postWarning("SoFaceSet::findNormalBinding",
                              "unknown normal binding setting");
#endif // COIN_DEBUG
    break;
  }
  return binding;
}

namespace { namespace SoGL { namespace FaceSet {

  enum AttributeBinding {
    OVERALL = 0,
    PER_FACE = 1,
    PER_VERTEX = 2
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
    // Make sure specified coordinate startindex is valid
    assert(idx >= 0);

    const SbVec3f * coords3d = NULL;
    const SbVec4f * coords4d = NULL;
    const SbBool is3d = coords->is3D();
    if (is3d) {
      coords3d = coords->getArrayPtr3();
    }
    else {
      coords4d = coords->getArrayPtr4();
    }
    int numcoords = coords->getNum();

    // This is the same code as in SoGLCoordinateElement::send().
    // It is inlined here for speed (~15% speed increase).
#define SEND_VERTEX(_idx_) \
    if (is3d) glVertex3fv((const GLfloat*) (coords3d + _idx_)); \
    else glVertex4fv((const GLfloat*) (coords4d + _idx_));

    int matnr = 0;
    int texnr = 0;
    int mode = GL_POLYGON;
    int newmode;
    int n;

    SbVec3f dummynormal(0.0f, 0.0f, 1.0f);
    const SbVec3f * currnormal = &dummynormal;
    if (normals) currnormal = normals;
    if ((AttributeBinding)NormalBinding == OVERALL) {
      if (needNormals) glNormal3fv((const GLfloat *)currnormal);
    }

    while (ptr < end) {
      n = *ptr++;

      if (n < 3 || idx + n > numcoords) {
        static uint32_t current_errors = 0;
        if (current_errors < 1) {
          SoDebugError::postWarning("[nonindexedfaceset]::GLRender", "Erroneous "
                                    "number of coordinates specified: %d. Must "
                                    "be >= 3 and less than or equal to the number of "
                                    "coordinates available (which is: %d). Aborting "
                                    "rendering. This message will be shown only once, "
                                    "but more errors might be present", n, numcoords - idx);
        }

        current_errors++;
        break;
      }

      if (n == 3) newmode = GL_TRIANGLES;
      else if (n == 4) newmode = GL_QUADS;
      else newmode = GL_POLYGON;
      if (newmode != mode) {
        if (mode != GL_POLYGON) glEnd();
        mode = newmode;
        glBegin((GLenum) mode);
      }
      else if (mode == GL_POLYGON) glBegin(GL_POLYGON);

      if ((AttributeBinding)NormalBinding != OVERALL) {
        currnormal = normals++;
        glNormal3fv((const GLfloat *)currnormal);
      }
      if ((AttributeBinding)MaterialBinding != OVERALL) {
        mb->send(matnr++, TRUE);
      }
      if (TexturingEnabled == TRUE) {
        tb->send(texnr++, coords->get3(idx), *currnormal);
      }
      SEND_VERTEX(idx);
      idx++;
      while (--n) {
        if ((AttributeBinding)NormalBinding == PER_VERTEX) {
          currnormal = normals++;
          glNormal3fv((const GLfloat *)currnormal);
        }
        if ((AttributeBinding)MaterialBinding == PER_VERTEX) {
          mb->send(matnr++, TRUE);
        } else if ((AttributeBinding)MaterialBinding != OVERALL) {
          // only needed for nvidia color-per-face bug workaround
          mb->send(matnr-1, TRUE);
        }

        if (TexturingEnabled == TRUE) {
          tb->send(texnr++, coords->get3(idx), *currnormal);
        }
        SEND_VERTEX(idx);
        idx++;
      }
      if (mode == GL_POLYGON) glEnd();
    }
    if (mode != GL_POLYGON) glEnd();
#undef SEND_VERTEX
  }

} } } // namespace

/*!
  \copydetails SoNode::initClass(void)
*/
void
SoFaceSet::initClass(void)
{
  SO_NODE_INTERNAL_INIT_CLASS(SoFaceSet, SO_FROM_INVENTOR_1);
}

#define SOGL_FACESET_GLRENDER_CALL_FUNC(normalbinding, materialbinding, texturing, args) \
  SoGL::FaceSet::GLRender<normalbinding, materialbinding, texturing> args

#define SOGL_FACESET_GLRENDER_RESOLVE_ARG3(normalbinding, materialbinding, texturing, args) \
  if (texturing) {                                                       \
    SOGL_FACESET_GLRENDER_CALL_FUNC(normalbinding, materialbinding, TRUE, args); \
  } else {                                                               \
    SOGL_FACESET_GLRENDER_CALL_FUNC(normalbinding, materialbinding, FALSE, args); \
  }

#define SOGL_FACESET_GLRENDER_RESOLVE_ARG2(normalbinding, materialbinding, texturing, args) \
  switch (materialbinding) {                                             \
  case SoGL::FaceSet::OVERALL:                                           \
    SOGL_FACESET_GLRENDER_RESOLVE_ARG3(normalbinding, OVERALL, texturing, args); \
    break;                                                               \
  case SoGL::FaceSet::PER_FACE:                                          \
    SOGL_FACESET_GLRENDER_RESOLVE_ARG3(normalbinding, PER_FACE, texturing, args); \
    break;                                                               \
  case SoGL::FaceSet::PER_VERTEX:                                        \
    SOGL_FACESET_GLRENDER_RESOLVE_ARG3(normalbinding, PER_VERTEX, texturing, args); \
    break;                                                               \
  default:                                                               \
    assert(!"invalid materialbinding value");                            \
    break;                                                               \
  }

#define SOGL_FACESET_GLRENDER_RESOLVE_ARG1(normalbinding, materialbinding, texturing, args) \
  switch (normalbinding) {                                               \
  case SoGL::FaceSet::OVERALL:                                           \
    SOGL_FACESET_GLRENDER_RESOLVE_ARG2(OVERALL, materialbinding, texturing, args); \
    break;                                                               \
  case SoGL::FaceSet::PER_FACE:                                          \
    SOGL_FACESET_GLRENDER_RESOLVE_ARG2(PER_FACE, materialbinding, texturing, args); \
    break;                                                               \
  case SoGL::FaceSet::PER_VERTEX:                                        \
    SOGL_FACESET_GLRENDER_RESOLVE_ARG2(PER_VERTEX, materialbinding, texturing, args); \
    break;                                                               \
  default:                                                               \
    assert(!"invalid materialbinding value");                            \
    break;                                                               \
  }

#define SOGL_FACESET_GLRENDER(normalbinding, materialbinding, texturing, args) \
  SOGL_FACESET_GLRENDER_RESOLVE_ARG1(normalbinding, materialbinding, texturing, args)


// doc from parent
void
SoFaceSet::GLRender(SoGLRenderAction * action)
{
  int32_t dummyarray[1];
  const int32_t *ptr = this->numVertices.getValues(0);
  const int32_t *end = ptr + this->numVertices.getNum();
  if ((end-ptr == 1) && (ptr[0] == 0)) return; // nothing to render

  if (PRIVATE(this)->primitivetype == UNKNOWN_TYPE) {
    PRIVATE(this)->primitivetype = MIXED_TYPE;
    int numtriangles = 0;
    int numquads = 0;
    int numothers = 0;

    const int32_t * nv = this->numVertices.getValues(0);
    const int n = this->numVertices.getNum();
    for (int i = 0; i < n; i++) {
      switch (nv[i]) {
      case 3:
        numtriangles++;
        break;
      case 4:
        numquads++;
        break;
      default:
        numothers++;
        break;
      }
    }
    if (numtriangles && !numquads && !numothers) {
      PRIVATE(this)->primitivetype = GL_TRIANGLES;
    }
    else if (numquads && !numtriangles && !numothers) {
      PRIVATE(this)->primitivetype = GL_QUADS;
    }
  }

  SbBool didusevbo = FALSE;
  SoState * state = action->getState();
  this->fixNumVerticesPointers(state, ptr, end, dummyarray);

  SbBool storedinvalid = SoCacheElement::setInvalid(FALSE);
  state->push(); // for convex cache

  if (this->vertexProperty.getValue()) {
    state->push();
    this->vertexProperty.getValue()->GLRender(action);
  }

  if (!this->shouldGLRender(action)) {
    if (this->vertexProperty.getValue()) {
      state->pop();
    }
    // for convex cache
    (void) SoCacheElement::setInvalid(storedinvalid);
    state->pop();
    return;
  }

  if (!this->useConvexCache(action)) {
    // render normally
    const SoCoordinateElement * tmp;
    const SbVec3f * normals;
    SbBool doTextures;

    SoMaterialBundle mb(action);
    SoTextureCoordinateBundle tb(action, TRUE, FALSE);
    doTextures = tb.needCoordinates();

    SbBool needNormals = !mb.isColorOnly() || tb.isFunction();

    SoVertexShape::getVertexData(state, tmp, normals,
                                 needNormals);

    const SoGLCoordinateElement * coords = (SoGLCoordinateElement *)tmp;

    Binding mbind = this->findMaterialBinding(state);
    Binding nbind = this->findNormalBinding(state);

    if (!needNormals) nbind = OVERALL;

    SoNormalCache * nc = NULL;

    if (needNormals && normals == NULL) {
      nc = this->generateAndReadLockNormalCache(state);
      normals = nc->getNormals();
    }

    mb.sendFirst(); // make sure we have the correct material

    int32_t idx = this->startIndex.getValue();

    // Robustness test to see if the startindex is valid.  If it is
    // not, print error message and exit.
    if (idx < 0) {
      static uint32_t current_errors = 0;
      if (current_errors < 1) {
        SoDebugError::postWarning("SoFaceSet::GLRender", "startIndex == %d "
                                  "< 0, which is erroneous. This message will only "
                                  "be printed once, but more errors might be present",
                                  idx);
      }
      current_errors++;

      // Unlock resource if needed
      if (nc) {
        this->readUnlockNormalCache();
      }

      // Goto end of this method to clean up resources
      goto glrender_done;
    }

    const uint32_t contextid = action->getCacheContext();
    int numcoords = coords ? coords->getNum() : 0;
    SoGLLazyElement* lelem = NULL;
    // check if we can render things using glDrawArrays
    SbBool dova =
      SoVBO::shouldRenderAsVertexArrays(state, contextid, numcoords) &&
      ((PRIVATE(this)->primitivetype == GL_TRIANGLES) ||
       (PRIVATE(this)->primitivetype == GL_QUADS)) &&
      (nbind != PER_FACE) &&
      (mbind != PER_FACE) &&
      !tb.isFunction() &&
      SoGLDriverDatabase::isSupported(sogl_glue_instance(state), SO_GL_VERTEX_ARRAY);

    const SoGLVBOElement* vboelem = SoGLVBOElement::getInstance(state);
    SoVBO* colorvbo = NULL;

    if (dova && (mbind != OVERALL)) {
      dova = FALSE;
      if (mbind == PER_VERTEX) {
        lelem = (SoGLLazyElement*)SoLazyElement::getInstance(state);
        colorvbo = vboelem->getColorVBO();
        if (colorvbo) dova = TRUE;
        else {
          // we might be able to do VA-rendering, but need to check the
          // diffuse color type first.
          if (!lelem->isPacked() && lelem->getNumTransparencies() <= 1) {
            dova = TRUE;
          }
        }
      }
    }
    if (dova) {
      SbBool dovbo = this->startVertexArray(action,
                                            coords,
                                            nbind == PER_VERTEX ? normals : NULL,
                                            doTextures,
                                            mbind == PER_VERTEX);
      int numprimitives = this->numVertices.getNum();
      if (PRIVATE(this)->primitivetype == GL_TRIANGLES) numprimitives *= 3;
      else numprimitives *= 4; // quads
      cc_glglue_glDrawArrays(sogl_glue_instance(state), PRIVATE(this)->primitivetype,
                             idx, numprimitives); 
      this->finishVertexArray(action, dovbo, nbind == PER_VERTEX,
                              doTextures, (mbind == PER_VERTEX));
      
    }
    else {
      SOGL_FACESET_GLRENDER(nbind, mbind, doTextures, (coords,
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
    }

    if (nc) {
      this->readUnlockNormalCache();
    }
  }

 glrender_done:

  if (this->vertexProperty.getValue())
    state->pop();

  // needed for convex cache
  (void) SoCacheElement::setInvalid(storedinvalid);
  state->pop();

  int numv = this->numVertices.getNum();
  // send approx number of triangles for autocache handling
  sogl_autocache_update(state, numv ?
                        (this->numVertices[0]-2)*numv : 0, didusevbo);
}

#undef SOGL_FACESET_GLRENDER_CALL_FUNC
#undef SOGL_FACESET_GLRENDER_RESOLVE_ARG3
#undef SOGL_FACESET_GLRENDER_RESOLVE_ARG2
#undef SOGL_FACESET_GLRENDER_RESOLVE_ARG1
#undef SOGL_FACESET_GLRENDER

// doc from parent
SbBool
SoFaceSet::generateDefaultNormals(SoState * state, SoNormalCache * nc)
{
  SbBool ccw = TRUE;
  if (SoShapeHintsElement::getVertexOrdering(state) ==
      SoShapeHintsElement::CLOCKWISE) ccw = FALSE;

  SoNormalGenerator * gen =
    new SoNormalGenerator(ccw, this->numVertices.getNum() * 3);

  int32_t idx = startIndex.getValue();
  int32_t dummyarray[1];
  const int32_t *ptr = this->numVertices.getValues(0);
  const int32_t *end = ptr + this->numVertices.getNum();
  this->fixNumVerticesPointers(state, ptr, end, dummyarray);

  const SoCoordinateElement * coords =
    SoCoordinateElement::getInstance(state);

  int numcoords = coords->getNum();

  // Robustness test to see if the startindex is valid.  If it is
  // not, print error message and return FALSE.
  if (idx < 0) {
    static uint32_t current_errors = 0;
    if (current_errors < 1) {
      SoDebugError::postWarning("SoFaceSet::generateDefaultNormals", "startIndex == %d "
                                "< 0, which is erroneous. This message will only "
                                "be printed once, but more errors might be present",
                                idx);
    }
    current_errors++;

    // Unable to generate normals for illegal faceset
    return FALSE;
  }

  // Generate normals for the faceset
  while (ptr < end) {
    int num = *ptr++;
    // If a valid number of points for the faceset has been specified,
    // and the end index is below the number of points available, then
    // everything is okidoki, and a polygon is added to the normal
    // generator.
    if (num >= 3 && (idx + num) <= numcoords) {
      gen->beginPolygon();
      while (num--) {
        gen->polygonVertex(coords->get3(idx++));
      }
      gen->endPolygon();
    }
    // If an invalid polygon has been specified, print errormessage
    // and return FALSE.
    else {
      SoDebugError::postWarning("SoFaceSet::generateDefaultNormals", "Erroneous "
                                "number of coordinates: %d specified for FaceSet. "
                                "Legal value is >= 3, with %d coordinate(s) available",
                                num, numcoords - idx);

      // Not able to generate normals for invalid faceset
      return FALSE;
    }
  }

  switch (this->findNormalBinding(state)) {
  case PER_VERTEX:
    gen->generate(SoCreaseAngleElement::get(state));
    break;
  case PER_FACE:
    gen->generatePerFace();
    break;
  case OVERALL:
    gen->generateOverall();
    break;
  }
  nc->set(gen);
  return TRUE;
}

// Documented in superclass.
SbBool
SoFaceSet::generateDefaultNormals(SoState * /* state */,
                                  SoNormalBundle * /* nb */)
{
  // Normals are generated directly in normal cache for this shape.
  return FALSE;
}

// doc from parent
void
SoFaceSet::getPrimitiveCount(SoGetPrimitiveCountAction *action)
{
  if (this->numVertices.getNum() == 1 &&
      this->numVertices[0] == 0) return;

  if (!this->shouldPrimitiveCount(action)) return;

  int32_t dummyarray[1];
  const int32_t * ptr = numVertices.getValues(0);
  const int32_t * end = ptr + numVertices.getNum();
  this->fixNumVerticesPointers(action->getState(), ptr, end, dummyarray);

  if (action->canApproximateCount()) {
    const ptrdiff_t diff = end - ptr;
    action->addNumTriangles((int)(diff * 3));
  }
  else {
    int cnt = 0;
    while (ptr < end) {
      cnt += *ptr++ - 2;
    }
    action->addNumTriangles(cnt);
  }
}

// doc from parent
void
SoFaceSet::generatePrimitives(SoAction *action)
{
  if (this->numVertices.getNum() == 1 &&
      this->numVertices[0] == 0) return;

  SoState * state = action->getState();

  if (this->vertexProperty.getValue()) {
    state->push();
    this->vertexProperty.getValue()->doAction(action);
  }

  const SoCoordinateElement *coords;
  const SbVec3f * normals;
  SbBool doTextures;

  SbBool needNormals = TRUE;

  SoVertexShape::getVertexData(state, coords, normals,
                               needNormals);

  SoTextureCoordinateBundle tb(action, FALSE, FALSE);
  doTextures = tb.needCoordinates();

  Binding mbind = this->findMaterialBinding(state);
  Binding nbind = this->findNormalBinding(state);

  SoNormalCache * nc = NULL;

  if (needNormals && normals == NULL) {
    nc = this->generateAndReadLockNormalCache(state);
    normals = nc->getNormals();
  }

  int32_t idx = startIndex.getValue();
  int32_t dummyarray[1];
  const int32_t * ptr = this->numVertices.getValues(0);
  const int32_t * end = ptr + this->numVertices.getNum();
  this->fixNumVerticesPointers(state, ptr, end, dummyarray);

  int matnr = 0;
  int texnr = 0;
  int normnr = 0;
  TriangleShape mode = POLYGON;
  TriangleShape newmode;
  int n;

  SbVec3f dummynormal(0.0f, 0.0f, 1.0f);
  const SbVec3f * currnormal = &dummynormal;
  if (normals) currnormal = normals;

  SoPrimitiveVertex vertex;
  SoFaceDetail faceDetail;
  SoPointDetail pointDetail;

  vertex.setDetail(&pointDetail);
  vertex.setNormal(*currnormal);

  while (ptr < end) {
    n = *ptr++;
    if (n == 3) newmode = TRIANGLES;
    else if (n == 4) newmode = QUADS;
    else newmode = POLYGON;
    if (newmode != mode) {
      if (mode != POLYGON) this->endShape();
      mode = newmode;
      this->beginShape(action, mode, &faceDetail);
    }
    else if (mode == POLYGON) this->beginShape(action, mode, &faceDetail);
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

    while (--n) {
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
    }
    if (mode == POLYGON) this->endShape();
    faceDetail.incFaceIndex();
  }
  if (mode != POLYGON) this->endShape();

  if (nc) {
    this->readUnlockNormalCache();
  }

  if (this->vertexProperty.getValue())
    state->pop();
}

// Documented in superclass.
void
SoFaceSet::notify(SoNotList * l)
{
  // Overridden to invalidate convex cache.
  PRIVATE(this)->readLockConvexCache();
  if (PRIVATE(this)->convexCache) PRIVATE(this)->convexCache->invalidate();
  PRIVATE(this)->readUnlockConvexCache();
  SoField *f = l->getLastField();
  if (f == &this->numVertices) {
    PRIVATE(this)->concavestatus = STATUS_UNKNOWN;
    PRIVATE(this)->primitivetype = UNKNOWN_TYPE;
  }
  inherited::notify(l);
}

//
// internal method which checks if convex cache needs to be
// used or (re)created. Renders the shape if convex cache needs to be used.
//
SbBool
SoFaceSet::useConvexCache(SoAction * action)
{
  SoState * state = action->getState();
  if (SoShapeHintsElement::getFaceType(state) == SoShapeHintsElement::CONVEX)
    return FALSE;

  int32_t idx = this->startIndex.getValue();
  const int32_t * ptr = this->numVertices.getValues(0);;
  const int32_t * end = ptr + this->numVertices.getNum();
  int32_t dummyarray[1];
  this->fixNumVerticesPointers(state, ptr, end, dummyarray);

  if (PRIVATE(this)->concavestatus == STATUS_UNKNOWN) {
    const int32_t * tst = ptr;
    while (tst < end) {
      if (*tst > 3) break;
      tst++;
    }
    if (tst < end) PRIVATE(this)->concavestatus = STATUS_CONCAVE;
    else PRIVATE(this)->concavestatus = STATUS_CONVEX;
  }
  if (PRIVATE(this)->concavestatus == STATUS_CONVEX) {
    return FALSE;
  }

  PRIVATE(this)->readLockConvexCache();

  SbBool isvalid = PRIVATE(this)->convexCache && PRIVATE(this)->convexCache->isValid(state);

  SbMatrix modelmatrix;
  if (!isvalid) {
    PRIVATE(this)->readUnlockConvexCache();
    PRIVATE(this)->writeLockConvexCache();
    if (PRIVATE(this)->convexCache) PRIVATE(this)->convexCache->unref();

    // use nopush to avoid cache dependencies.
    SoModelMatrixElement * nopushelem = (SoModelMatrixElement*)
      state->getElementNoPush(SoModelMatrixElement::getClassStackIndex());

    // need to send matrix if we have some weird transformation
    modelmatrix = nopushelem->getModelMatrix();
    if (modelmatrix[3][0] == 0.0f &&
        modelmatrix[3][1] == 0.0f &&
        modelmatrix[3][2] == 0.0f &&
        modelmatrix[3][3] == 1.0f) modelmatrix = SbMatrix::identity();

    PRIVATE(this)->convexCache = new SoConvexDataCache(state);
    PRIVATE(this)->convexCache->ref();
    SoCacheElement::set(state, PRIVATE(this)->convexCache);
  }

  const SoCoordinateElement * tmp;
  const SbVec3f * normals;
  SbBool doTextures;

  SoMaterialBundle mb(action);

  SbBool needNormals = !mb.isColorOnly();

  SoVertexShape::getVertexData(state, tmp, normals,
                               needNormals);

  const SoGLCoordinateElement * coords = (SoGLCoordinateElement *)tmp;

  SoTextureCoordinateBundle tb(action, TRUE, FALSE);
  doTextures = tb.needCoordinates();

  SoConvexDataCache::Binding mbind;

  switch (this->findMaterialBinding(state)) {
  case OVERALL:
    mbind = SoConvexDataCache::NONE;
    break;
  case PER_VERTEX:
    mbind = SoConvexDataCache::PER_VERTEX;
    break;
  case PER_FACE:
    mbind = SoConvexDataCache::PER_FACE;
    break;
  default:
    mbind = SoConvexDataCache::NONE;
    break;
  }

  SoConvexDataCache::Binding nbind;
  switch (this->findNormalBinding(state)) {
  case OVERALL:
    nbind = SoConvexDataCache::NONE;
    break;
  case PER_VERTEX:
    nbind = SoConvexDataCache::PER_VERTEX;
    break;
  case PER_FACE:
    nbind = SoConvexDataCache::PER_FACE;
    break;
  default:
    nbind = SoConvexDataCache::NONE;
    break;
  }

  SoNormalCache * nc = NULL;

  if (needNormals && normals == NULL) {
    nc = this->generateAndReadLockNormalCache(state);
    normals = nc->getNormals();
  }
  else if (!needNormals) {
    nbind = SoConvexDataCache::NONE;
  }
  if (nbind == SoConvexDataCache::NONE && normals == NULL) {
    static SbVec3f dummynormal;
    dummynormal.setValue(0.0f, 0.0f, 1.0f);
    normals = &dummynormal;
  }

  SoConvexDataCache::Binding tbind = SoConvexDataCache::NONE;
  if (tb.needCoordinates()) tbind = SoConvexDataCache::PER_VERTEX;

  if (!isvalid) {
    SoCacheElement::set(state, NULL); // close cache
    // create an index table to be able to use convex cache.
    // should be fast compared to the tessellation
    const ptrdiff_t diff = end - ptr;
    SbList <int32_t> dummyidx((int)(diff * 4));
    const int32_t * tptr = ptr;
    while (tptr < end) {
      int num = *tptr++;
      while (num--) {
        dummyidx.append(idx++);
      }
      dummyidx.append(-1);
    }
    PRIVATE(this)->convexCache->generate(coords, modelmatrix,
                                dummyidx.getArrayPtr(), dummyidx.getLength(),
                                NULL, NULL, NULL,
                                mbind,
                                nbind,
                                tbind);

    PRIVATE(this)->writeUnlockConvexCache();
    PRIVATE(this)->readLockConvexCache();
  }

  mb.sendFirst(); // make sure we have the correct material

  // the convex data cache will change PER_VERTEX binding
  // to PER_VERTEX_INDEXED. We must do so also.
  int realmbind = (int) mbind;
  int realnbind = (int) nbind;

  // hack warning. We rely on PER_VERTEX_INDEXED == PER_VERTEX+1
  // and PER_FACE_INDEXED == PER_FACE+1 in SoGL.cpp
  if (mbind == SoConvexDataCache::PER_VERTEX ||
      mbind == SoConvexDataCache::PER_FACE) realmbind++;
  if (nbind == SoConvexDataCache::PER_VERTEX ||
      nbind == SoConvexDataCache::PER_FACE) realnbind++;

  SoVertexAttributeBundle vab(action, TRUE);
  SbBool doattribs = vab.doAttributes();

  SoVertexAttributeBindingElement::Binding attribbind = 
    SoVertexAttributeBindingElement::get(state);

    if (!doattribs) { 
      // for overall attribute binding we check for doattribs before
      // sending anything in SoGL::FaceSet::GLRender
      attribbind = SoVertexAttributeBindingElement::OVERALL;
    }

  // use the IndededFaceSet rendering method.
  sogl_render_faceset(coords,
                      PRIVATE(this)->convexCache->getCoordIndices(),
                      PRIVATE(this)->convexCache->getNumCoordIndices(),
                      normals,
                      PRIVATE(this)->convexCache->getNormalIndices(),
                      &mb,
                      PRIVATE(this)->convexCache->getMaterialIndices(),
                      &tb,
                      PRIVATE(this)->convexCache->getTexIndices(),
                      &vab,
                      realnbind,
                      realmbind,
                      attribbind,
                      doTextures ? 1 : 0,
                      doattribs ? 1 : 0);


  if (nc) {
    this->readUnlockNormalCache();
  }

  PRIVATE(this)->readUnlockConvexCache();

  return TRUE;
}

#undef PRIVATE
#undef STATUS_UNKNOWN
#undef STATUS_CONVEX
#undef STATUS_CONCAV
#undef UNKNOWN_TYPE
#undef MIXED_TYPE

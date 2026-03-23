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
  \class SoIndexedFaceSet SoIndexedFaceSet.h Inventor/nodes/SoIndexedFaceSet.h
  \brief The SoIndexedFaceSet class is used to handle generic indexed facesets.

  \ingroup coin_nodes

  Facesets are specified using the coordIndex field. Each face must be
  terminated by a negative (-1) index. Coordinates, normals, materials
  and texture coordinates from the current state (or from the
  vertexProperty node if set), can be indexed to create triangles,
  quads or polygons.

  Here's a usage example of just about the simplest possible use of
  this node, to show a single polygon face:

  \verbatim
  #Inventor V2.1 ascii
  
  Separator {
     Coordinate3 {
        point [ 0 0 0, 1 0 0, 1 1 0 ]
     }
     IndexedFaceSet {
        coordIndex [ 0, 1, 2, -1 ]
     }
  }
  \endverbatim

  Binding PER_VERTEX_INDEXED, PER_VERTEX, PER_FACE_INDEXED, PER_FACE
  or OVERALL can be set for material, and normals. The default
  material binding is OVERALL. The default normal binding is
  PER_VERTEX_INDEXED. When PER_VERTEX_INDEXED binding is used and the
  corresponding materialIndex, normalIndex, texCoordIndex field is
  empty, the coordIndex field will be used to index material, normal
  or texture coordinate. If you do specify indices for material,
  normals or texture coordinates for PER_VERTEX_INDEXED binding, make
  sure your index array matches the coordIndex array: there should be
  a -1 wherever there is a -1 in the coordIndex field. This is done to
  make this node more easily readable for humans.


  A fairly common request when rendering facesets is how to display a
  set of faces with different colors on the backside versus the
  frontside. There is not \e direct support for this in the API, but
  it can easily be implemented by duplicating all faces in both the
  SoShapeHints::COUNTERCLOCKWISE and the SoShapeHints::CLOCKWISE
  order. Here is a simple usage example, showing the technique for a
  single polygon, using two SoFaceSet nodes for rendering the same
  polygon from both sides:

  \verbatim
  #Inventor V2.1 ascii
  
  Separator {
     Coordinate3 { point [ 0 0 0, 1 0 0, 1 1 0 ] }
  
     Separator {
        Material { diffuseColor [ 1 1 0 ] }
        ShapeHints {
           vertexOrdering COUNTERCLOCKWISE
           shapeType SOLID
        }
        FaceSet { numVertices [ 3 ] }
     }
  
     Separator {
        Material { diffuseColor [ 1 0 0 ] }
        ShapeHints {
           vertexOrdering CLOCKWISE
           shapeType SOLID
        }
        FaceSet { numVertices [ 3 ] }
     }
  }
  \endverbatim

  The same effect can also be done in the following manner, using an
  SoIndexedFaceSet to explicitly render the polygon coordinates in
  both directions (clockwise and counterclockwise):

  \verbatim
  #Inventor V2.1 ascii
  
  Separator {
     Coordinate3 { point [ 0 0 0, 1 0 0, 1 1 0 ] }
  
     Material { diffuseColor [ 1 0 0, 1 1 0 ] }
     MaterialBinding { value PER_FACE_INDEXED }
  
     ShapeHints {
        vertexOrdering COUNTERCLOCKWISE
        shapeType SOLID
     }
  
     IndexedFaceSet {
        coordIndex [ 0, 1, 2, -1, 2, 1, 0, -1 ]
        materialIndex [ 0, 1 ]
     }
  }
  \endverbatim


  Another rather rare issue that might be interesting to know about is
  that to render polygons with concave borders, you should set up an
  SoShapeHints node with SoShapeHints::faceType set to
  SoShapeHints::UNKNOWN_FACE_TYPE in the scene graph before the
  SoIndexedFaceSet (or SoFaceSet) node. This needs to be done to force
  the rendering code to tessellate the polygons properly to triangles
  before sending it off to OpenGL. Without it, the polygons will be
  sent as-is to OpenGL, and the OpenGL implementation's tessellator is
  often not able to tessellate properly. Here is an example which
  usually fails without the SoShapeHints node (try commenting it out,
  and see what happens):

  \verbatim
  #Inventor V2.1 ascii
  
  ShapeHints { faceType UNKNOWN_FACE_TYPE }
  
  Coordinate3
  {
          point [ 2 0 0,
                  1 0 0,
                  1 1 0,
                  0 1 0,
                  0 2 0,
                  1 2 0,
                  2 2 0,
                  2 1 0 ]
  }
  
  FaceSet {}
  \endverbatim

  <b>FILE FORMAT/DEFAULTS:</b>
  \code
    IndexedFaceSet {
        vertexProperty NULL
        coordIndex 0
        materialIndex -1
        normalIndex -1
        textureCoordIndex -1
    }
  \endcode

  \sa SoFaceSet, SoIndexedTriangleStripSet
*/

// *************************************************************************

#include <Inventor/nodes/SoIndexedFaceSet.h>

#include <cassert>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H

#include <Inventor/SoPrimitiveVertex.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/actions/SoGetPrimitiveCountAction.h>
#include <Inventor/actions/SoRayPickAction.h>
#include <Inventor/bundles/SoMaterialBundle.h>
#include <Inventor/bundles/SoTextureCoordinateBundle.h>
#include <Inventor/bundles/SoVertexAttributeBundle.h>
#include <Inventor/caches/SoConvexDataCache.h>
#include <Inventor/caches/SoNormalCache.h>
#include <Inventor/details/SoFaceDetail.h>
#include <Inventor/elements/SoCacheElement.h>
#include <Inventor/elements/SoCoordinateElement.h>
#include <Inventor/elements/SoCreaseAngleElement.h>
#include <Inventor/elements/SoGLCacheContextElement.h>
#include <Inventor/elements/SoGLLazyElement.h>
#include <Inventor/elements/SoGLVBOElement.h>
#include <Inventor/elements/SoMaterialBindingElement.h>
#include <Inventor/elements/SoModelMatrixElement.h>
#include <Inventor/elements/SoMultiTextureCoordinateElement.h>
#include <Inventor/elements/SoMultiTextureEnabledElement.h>
#include <Inventor/elements/SoMultiTextureEnabledElement.h>
#include <Inventor/elements/SoNormalBindingElement.h>
#include <Inventor/elements/SoNormalElement.h>
#include <Inventor/elements/SoShapeHintsElement.h>
#include <Inventor/elements/SoTextureCoordinateBindingElement.h>
#include <Inventor/elements/SoMultiTextureCoordinateElement.h>
#include <Inventor/elements/SoVertexAttributeBindingElement.h>
#include <Inventor/errors/SoDebugError.h>
#include <Inventor/lists/SbList.h>
#include <Inventor/misc/SoGLDriverDatabase.h>
#include <Inventor/misc/SoState.h>
#include <Inventor/system/gl.h>
#include <Inventor/threads/SbMutex.h>
#include <Inventor/threads/SbRWMutex.h>

#include "nodes/SoSubNodeP.h"
#include "tidbitsp.h"
#include "threads/threadsutilp.h"
#include "rendering/SoVertexArrayIndexer.h"
#include "rendering/SoVBO.h"
#include "rendering/SoGL.h"

// *************************************************************************

// for concavestatus
#define STATUS_UNKNOWN 0
#define STATUS_CONVEX  1
#define STATUS_CONCAVE 2

#define LOCK_VAINDEXER(obj) SoBase::staticDataLock()
#define UNLOCK_VAINDEXER(obj) SoBase::staticDataUnlock()

// *************************************************************************

class SoIndexedFaceSetP {
public:
  SoIndexedFaceSetP(void) 
#ifdef COIN_THREADSAFE
    : convexmutex(SbRWMutex::READ_PRECEDENCE)
#endif // COIN_THREADSAFE
  { }

  SoVertexArrayIndexer * vaindexer;
  SoConvexDataCache * convexCache;
  int concavestatus;

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

#define PRIVATE(obj) ((obj)->pimpl)

// *************************************************************************

SO_NODE_SOURCE(SoIndexedFaceSet);

/*!
  Constructor.
*/
SoIndexedFaceSet::SoIndexedFaceSet()
{
  PRIVATE(this) = new SoIndexedFaceSetP;
  PRIVATE(this)->convexCache = NULL;
  PRIVATE(this)->vaindexer = NULL;
  PRIVATE(this)->concavestatus = STATUS_UNKNOWN;

  SO_NODE_INTERNAL_CONSTRUCTOR(SoIndexedFaceSet);
}


/*!
  Destructor.
*/
SoIndexedFaceSet::~SoIndexedFaceSet()
{
  delete PRIVATE(this)->vaindexer;
  if (PRIVATE(this)->convexCache) PRIVATE(this)->convexCache->unref();
  delete PRIVATE(this);
}

/*!
  \copydetails SoNode::initClass(void)
*/
void
SoIndexedFaceSet::initClass(void)
{
  SO_NODE_INTERNAL_INIT_CLASS(SoIndexedFaceSet, SO_FROM_INVENTOR_1|SoNode::VRML1);
}

//
// translates current material binding into the internal Binding enum.
//
SoIndexedFaceSet::Binding
SoIndexedFaceSet::findMaterialBinding(SoState * const state) const
{
  Binding binding = OVERALL;
  SoMaterialBindingElement::Binding matbind =
    SoMaterialBindingElement::get(state);

  switch (matbind) {
  case SoMaterialBindingElement::OVERALL:
    binding = OVERALL;
    break;
  case SoMaterialBindingElement::PER_VERTEX:
    binding = PER_VERTEX;
    break;
  case SoMaterialBindingElement::PER_VERTEX_INDEXED:
    binding = PER_VERTEX_INDEXED;
    break;
  case SoMaterialBindingElement::PER_PART:
  case SoMaterialBindingElement::PER_FACE:
    binding = PER_FACE;
    break;
  case SoMaterialBindingElement::PER_PART_INDEXED:
  case SoMaterialBindingElement::PER_FACE_INDEXED:
    binding = PER_FACE_INDEXED;
    break;
  default:
#if COIN_DEBUG
    SoDebugError::postWarning("SoIndexedFaceSet::findMaterialBinding",
                              "unknown material binding setting");
#endif // COIN_DEBUG
    break;
  }
  return binding;
}


//
// translates current normal binding into the internal Binding enum.
//
SoIndexedFaceSet::Binding
SoIndexedFaceSet::findNormalBinding(SoState * const state) const
{
  Binding binding = PER_VERTEX_INDEXED;
  SoNormalBindingElement::Binding normbind =
    (SoNormalBindingElement::Binding) SoNormalBindingElement::get(state);

  switch (normbind) {
  case SoNormalBindingElement::OVERALL:
    binding = OVERALL;
    break;
  case SoNormalBindingElement::PER_VERTEX:
    binding = PER_VERTEX;
    break;
  case SoNormalBindingElement::PER_VERTEX_INDEXED:
    binding = PER_VERTEX_INDEXED;
    break;
  case SoNormalBindingElement::PER_PART:
  case SoNormalBindingElement::PER_FACE:
    binding = PER_FACE;
    break;
  case SoNormalBindingElement::PER_PART_INDEXED:
  case SoNormalBindingElement::PER_FACE_INDEXED:
    binding = PER_FACE_INDEXED;
    break;
  default:
#if COIN_DEBUG
    SoDebugError::postWarning("SoIndexedFaceSet::findNormalBinding",
                              "unknown normal binding setting");
#endif // COIN_DEBUG
    break;
  }
  return binding;
}

// Documented in superclass.
void
SoIndexedFaceSet::notify(SoNotList * list)
{
  // Overridden to invalidate convex cache.
  PRIVATE(this)->readLockConvexCache();
  if (PRIVATE(this)->convexCache) PRIVATE(this)->convexCache->invalidate();
  PRIVATE(this)->readUnlockConvexCache();
  SoField *f = list->getLastField();
  if (f == &this->coordIndex) {
    PRIVATE(this)->concavestatus = STATUS_UNKNOWN;
    LOCK_VAINDEXER(this);
    delete PRIVATE(this)->vaindexer;
    PRIVATE(this)->vaindexer = NULL;
    UNLOCK_VAINDEXER(this);
  }
  inherited::notify(list);
}

// doc from parent
void
SoIndexedFaceSet::GLRender(SoGLRenderAction * action)
{
  if (this->coordIndex.getNum() < 3) return;
  SoState * state = action->getState();

  SbBool hasvp = FALSE;
  
  if (this->vertexProperty.getValue()) {
    hasvp = TRUE;
    state->push();
    this->vertexProperty.getValue()->GLRender(action);
  }

  if (!this->shouldGLRender(action)) {
    if (hasvp)
      state->pop();
    return;
  }

  Binding mbind = this->findMaterialBinding(state);
  Binding nbind = this->findNormalBinding(state);

  const SoCoordinateElement * coords;
  const SbVec3f * normals;
  const int32_t * cindices;
  int numindices;
  const int32_t * nindices;
  const int32_t * tindices;
  const int32_t * mindices;
  SbBool doTextures;
  SbBool normalCacheUsed;

  SoMaterialBundle mb(action);

  SoTextureCoordinateBundle tb(action, TRUE, FALSE);
  doTextures = tb.needCoordinates();
  SbBool sendNormals = !mb.isColorOnly() || tb.isFunction();

  this->getVertexData(state, coords, normals, cindices,
                      nindices, tindices, mindices, numindices,
                      sendNormals, normalCacheUsed);

  if (!sendNormals) nbind = OVERALL;
  else if (nbind == OVERALL) {
    if (normals) glNormal3fv(normals[0].getValue());
    else glNormal3f(0.0f, 0.0f, 1.0f);
  }
  else if (normalCacheUsed && nbind == PER_VERTEX) {
    nbind = PER_VERTEX_INDEXED;
  }
  else if (normalCacheUsed && nbind == PER_FACE_INDEXED) {
    nbind = PER_FACE;
  }
  if (this->getNodeType() == SoNode::VRML1) {
    // For VRML1, PER_VERTEX means per vertex in shape, not PER_VERTEX
    // on the state.
    if (mbind == PER_VERTEX) {
      mbind = PER_VERTEX_INDEXED;
      mindices = cindices;
    }
    if (nbind == PER_VERTEX) {
      nbind = PER_VERTEX_INDEXED;
      nindices = cindices;
    }
  }

  Binding tbind = NONE;
  if (doTextures) {
    if (tb.isFunction() && !tb.needIndices()) {
      tbind = NONE;
      tindices = NULL;
    }
    // FIXME: just call inherited::areTexCoordsIndexed() instead of
    // the if-check? 20020110 mortene.
    else if (SoTextureCoordinateBindingElement::get(state) ==
             SoTextureCoordinateBindingElement::PER_VERTEX) {
      tbind = PER_VERTEX;
      tindices = NULL;
    }
    else {
      tbind = PER_VERTEX_INDEXED;
      if (tindices == NULL) tindices = cindices;
    }
  }

  SbBool convexcacheused = FALSE;
  if (this->useConvexCache(action, normals, nindices, normalCacheUsed)) {
    cindices = PRIVATE(this)->convexCache->getCoordIndices();
    numindices = PRIVATE(this)->convexCache->getNumCoordIndices();
    mindices = PRIVATE(this)->convexCache->getMaterialIndices();
    nindices = PRIVATE(this)->convexCache->getNormalIndices();
    tindices = PRIVATE(this)->convexCache->getTexIndices();

    if (mbind == PER_VERTEX) mbind = PER_VERTEX_INDEXED;
    else if (mbind == PER_FACE) mbind = PER_FACE_INDEXED;
    if (nbind == PER_VERTEX) nbind = PER_VERTEX_INDEXED;
    else if (nbind == PER_FACE) nbind = PER_FACE_INDEXED;

    if (tbind != NONE) tbind = PER_VERTEX_INDEXED;
    convexcacheused = TRUE;
  }

  mb.sendFirst(); // make sure we have the correct material

#if 0
  fprintf(stderr,"numindices: %d, convex: %d, ncache: %d, nbind: %d, mbind: %d, tbind: %d, va: %d\n",
          numindices, convexcacheused, normalCacheUsed, nbind, mbind, tbind,
          cc_glglue_has_vertex_array(sogl_glue_instance(state)));
#endif

  const uint32_t contextid = action->getCacheContext();
  SoGLLazyElement * lelem = NULL;
  SbBool dova =
    SoVBO::shouldRenderAsVertexArrays(state, contextid, numindices) &&
    !convexcacheused && !normalCacheUsed &&
    ((nbind == OVERALL) || ((nbind == PER_VERTEX_INDEXED) && ((nindices == cindices) || (nindices == NULL)))) &&
    ((tbind == NONE && !tb.needCoordinates()) || // no 
     ((tbind == PER_VERTEX_INDEXED) && ((tindices == cindices) || (tindices == NULL)))) &&
    ((mbind == NONE) || ((mbind == PER_VERTEX_INDEXED) && ((mindices == cindices) || (mindices == NULL)))) &&
    SoGLDriverDatabase::isSupported(sogl_glue_instance(state), SO_GL_VERTEX_ARRAY);

  const SoGLVBOElement * vboelem = SoGLVBOElement::getInstance(state);
  SoVBO * colorvbo = NULL;

  SbBool didrenderasvbo = FALSE;
  if (dova && (mbind != OVERALL)) {
    dova = FALSE;
    if ((mbind == PER_VERTEX_INDEXED) && ((mindices == cindices) || (mindices == NULL))) {
      lelem = (SoGLLazyElement*) SoLazyElement::getInstance(state);
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
                                          (nbind != OVERALL) ? normals : NULL,
                                          doTextures,
                                          mbind != OVERALL);
    didrenderasvbo = dovbo;

    LOCK_VAINDEXER(this);
    if (PRIVATE(this)->vaindexer == NULL) {
      SoVertexArrayIndexer * indexer = new SoVertexArrayIndexer;
      int i = 0;
      while (i < numindices) {
        int cnt = 0;
        while (i + cnt < numindices && cindices[i+cnt] >= 0) cnt++;
        
        switch (cnt) {
        case 3:
          indexer->addTriangle(cindices[i],cindices[i+1], cindices[i+2]);
          break;
        case 4:
          indexer->addQuad(cindices[i],cindices[i+1],cindices[i+2],cindices[i+3]);
          break;
        default:
          if (cnt > 4) {
            indexer->beginTarget(GL_POLYGON);
            for (int j = 0; j < cnt; j++) {
              indexer->targetVertex(GL_POLYGON, cindices[i+j]);
            }
            indexer->endTarget(GL_POLYGON);
          }
        }
        i += cnt + 1;
      }
      indexer->close();
      if (indexer->getNumVertices()) {
        PRIVATE(this)->vaindexer = indexer;
      }
      else {
        delete indexer;
      }
#if 0
      fprintf(stderr,"XXX: create VertexArrayIndexer: %d\n", indexer->getNumVertices());
#endif
    }

    if (PRIVATE(this)->vaindexer) {
      PRIVATE(this)->vaindexer->render(state, dovbo, contextid);
    }
    UNLOCK_VAINDEXER(this);
    this->finishVertexArray(action,
                            dovbo,
                            (nbind != OVERALL),
                            doTextures,
                            mbind != OVERALL);
  }
  else {
    SoVertexAttributeBundle vab(action, TRUE);
    SbBool doattribs = vab.doAttributes();

    SoVertexAttributeBindingElement::Binding attribbind = 
      SoVertexAttributeBindingElement::get(state);

    if (!doattribs) { 
      // for overall attribute binding we check for doattribs before
      // sending anything in SoGL::FaceSet::GLRender
      attribbind = SoVertexAttributeBindingElement::OVERALL;
    }

    sogl_render_faceset((SoGLCoordinateElement *)coords,
                        cindices,
                        numindices,
                        normals,
                        nindices,
                        &mb,
                        mindices,
                        &tb,
                        tindices,
                        &vab,
                        (int)nbind,
                        (int)mbind,
                        (int)attribbind,
                        doTextures ? 1 : 0,
                        doattribs ? 1 : 0);
  }
  if (normalCacheUsed) {
    this->readUnlockNormalCache();
  }

  if (convexcacheused) {
    PRIVATE(this)->readUnlockConvexCache();
  }

  if (hasvp) {
    state->pop();
  }
  // send approx number of triangles for autocache handling
  sogl_autocache_update(state, this->coordIndex.getNum() / 4, didrenderasvbo);
}

  // this macro actually makes the code below more readable  :-)
#define DO_VERTEX(idx) \
  if (mbind == PER_VERTEX) {                  \
    pointDetail.setMaterialIndex(matnr);      \
    vertex.setMaterialIndex(matnr++);         \
  }                                           \
  else if (mbind == PER_VERTEX_INDEXED) {     \
    pointDetail.setMaterialIndex(*mindices); \
    vertex.setMaterialIndex(*mindices++); \
  }                                         \
  if (nbind == PER_VERTEX) {                \
    pointDetail.setNormalIndex(normnr);     \
    currnormal = &normals[normnr++];        \
    vertex.setNormal(*currnormal);          \
  }                                         \
  else if (nbind == PER_VERTEX_INDEXED) {   \
    pointDetail.setNormalIndex(*nindices);  \
    currnormal = &normals[*nindices++];     \
    vertex.setNormal(*currnormal);          \
  }                                        \
  if (tb.isFunction()) {                 \
    vertex.setTextureCoords(tb.get(coords->get3(idx), *currnormal)); \
    if (tb.needIndices()) pointDetail.setTextureCoordIndex(tindices ? *tindices++ : texidx++); \
  }                                         \
  else if (tbind != NONE) {                      \
    pointDetail.setTextureCoordIndex(tindices ? *tindices : texidx); \
    vertex.setTextureCoords(tb.get(tindices ? *tindices++ : texidx++)); \
  }                                         \
  vertex.setPoint(coords->get3(idx));        \
  pointDetail.setCoordinateIndex(idx);      \
  this->shapeVertex(&vertex);

// doc from parent
void
SoIndexedFaceSet::generatePrimitives(SoAction *action)
{
  if (this->coordIndex.getNum() < 3) return;

  SoState * state = action->getState();

  if (this->vertexProperty.getValue()) {
    state->push();
    this->vertexProperty.getValue()->doAction(action);
  }

  Binding mbind = this->findMaterialBinding(state);
  Binding nbind = this->findNormalBinding(state);

  const SoCoordinateElement * coords;
  const SbVec3f * normals;
  const int32_t * cindices;
  int numindices;
  const int32_t * nindices;
  const int32_t * tindices;
  const int32_t * mindices;
  SbBool doTextures;
  SbBool sendNormals;
  SbBool normalCacheUsed;

  sendNormals = TRUE; // always generate normals

  getVertexData(state, coords, normals, cindices,
                nindices, tindices, mindices, numindices,
                sendNormals, normalCacheUsed);

  SoTextureCoordinateBundle tb(action, FALSE, FALSE);
  doTextures = tb.needCoordinates();

  if (!sendNormals) nbind = OVERALL;
  else if (normalCacheUsed && nbind == PER_VERTEX) {
    nbind = PER_VERTEX_INDEXED;
  }
  else if (normalCacheUsed && nbind == PER_FACE_INDEXED) {
    nbind = PER_FACE;
  }

  if (this->getNodeType() == SoNode::VRML1) {
    // For VRML1, PER_VERTEX means per vertex in shape, not PER_VERTEX
    // on the state.
    if (mbind == PER_VERTEX) {
      mbind = PER_VERTEX_INDEXED;
      mindices = cindices;
    }
    if (nbind == PER_VERTEX) {
      nbind = PER_VERTEX_INDEXED;
      nindices = cindices;
    }
  }

  Binding tbind = NONE;
  if (doTextures) {
    if (tb.isFunction() && !tb.needIndices()) {
      tbind = NONE;
      tindices = NULL;
    }
    // FIXME: just call inherited::areTexCoordsIndexed() instead of
    // the if-check? 20020110 mortene.
    else if (SoTextureCoordinateBindingElement::get(state) ==
             SoTextureCoordinateBindingElement::PER_VERTEX) {
      tbind = PER_VERTEX;
      tindices = NULL;
    }
    else {
      tbind = PER_VERTEX_INDEXED;
      if (tindices == NULL) tindices = cindices;
    }
  }

  if (nbind == PER_VERTEX_INDEXED && nindices == NULL) {
    nindices = cindices;
  }
  if (mbind == PER_VERTEX_INDEXED && mindices == NULL) {
    mindices = cindices;
  }

  SbBool convexcacheused = FALSE;
  if (this->useConvexCache(action, normals, nindices, normalCacheUsed)) {
    cindices = PRIVATE(this)->convexCache->getCoordIndices();
    numindices = PRIVATE(this)->convexCache->getNumCoordIndices();
    mindices = PRIVATE(this)->convexCache->getMaterialIndices();
    nindices = PRIVATE(this)->convexCache->getNormalIndices();
    tindices = PRIVATE(this)->convexCache->getTexIndices();

    if (mbind == PER_VERTEX) mbind = PER_VERTEX_INDEXED;
    else if (mbind == PER_FACE) mbind = PER_FACE_INDEXED;
    if (nbind == PER_VERTEX) nbind = PER_VERTEX_INDEXED;
    else if (nbind == PER_FACE) nbind = PER_FACE_INDEXED;

    if (tbind != NONE) tbind = PER_VERTEX_INDEXED;
    convexcacheused = TRUE;
  }

  int texidx = 0;
  TriangleShape mode = POLYGON;
  TriangleShape newmode;
  const int32_t *viptr = cindices;
  const int32_t *viendptr = viptr + numindices;
  int32_t v1, v2, v3, v4, v5 = 0; // v5 init unnecessary, but kills a compiler warning.

  SoPrimitiveVertex vertex;
  SoPointDetail pointDetail;
  SoFaceDetail faceDetail;

  vertex.setDetail(&pointDetail);

  SbVec3f dummynormal(0,0,1);
  const SbVec3f *currnormal = &dummynormal;
  if (normals) currnormal = normals;
  vertex.setNormal(*currnormal);

  int matnr = 0;
  int normnr = 0;

  while (viptr + 2 < viendptr) {
    v1 = *viptr++;
    v2 = *viptr++;
    v3 = *viptr++;
    if (v1 < 0 || v2 < 0 || v3 < 0) {
#if COIN_DEBUG
      SoDebugError::postInfo("SoIndexedFaceSet::generatePrimitives",
                             "Polygon with less than three vertices detected. "
                             "Aborting current shape.");
#endif // COIN_DEBUG
      
      break;
    } 
    v4 = viptr < viendptr ? *viptr++ : -1;
    if (v4  < 0) newmode = TRIANGLES;
    else {
      v5 = viptr < viendptr ? *viptr++ : -1;
      if (v5 < 0) newmode = QUADS;
      else newmode = POLYGON;
    }
    if (newmode != mode) {
      if (mode != POLYGON) this->endShape();
      mode = newmode;
      this->beginShape(action, mode, &faceDetail);
    }
    else if (mode == POLYGON) this->beginShape(action, POLYGON, &faceDetail);

    // vertex 1 can't use DO_VERTEX
    if (mbind == PER_VERTEX || mbind == PER_FACE) {
      pointDetail.setMaterialIndex(matnr);
      vertex.setMaterialIndex(matnr++);
    }
    else if (mbind == PER_VERTEX_INDEXED || mbind == PER_FACE_INDEXED) {
      pointDetail.setMaterialIndex(*mindices);
      vertex.setMaterialIndex(*mindices++);
    }
    if (nbind == PER_VERTEX || nbind == PER_FACE) {
      pointDetail.setNormalIndex(normnr);
      currnormal = &normals[normnr++];
      vertex.setNormal(*currnormal);
    }
    else if (nbind == PER_FACE_INDEXED || nbind == PER_VERTEX_INDEXED) {
      pointDetail.setNormalIndex(*nindices);
      currnormal = &normals[*nindices++];
      vertex.setNormal(*currnormal);
    }

    if (tb.isFunction()) {
      vertex.setTextureCoords(tb.get(coords->get3(v1), *currnormal));
      if (tb.needIndices()) pointDetail.setTextureCoordIndex(tindices ? *tindices++ : texidx++); 
    }
    else if (tbind != NONE) {
      pointDetail.setTextureCoordIndex(tindices ? *tindices : texidx);
      vertex.setTextureCoords(tb.get(tindices ? *tindices++ : texidx++));
    }
    pointDetail.setCoordinateIndex(v1);
    vertex.setPoint(coords->get3(v1));
    this->shapeVertex(&vertex);

    DO_VERTEX(v2);
    DO_VERTEX(v3);

    if (mode != TRIANGLES) {
      DO_VERTEX(v4);
      if (mode == POLYGON) {
        DO_VERTEX(v5);
        v1 = viptr < viendptr ? *viptr++ : -1;
        while (v1 >= 0) {
          DO_VERTEX(v1);
          v1 = viptr < viendptr ? *viptr++ : -1;
        }
        this->endShape();
      }
    }
    faceDetail.incFaceIndex();
    faceDetail.incPartIndex();
    if (mbind == PER_VERTEX_INDEXED) {
      mindices++;
    }
    if (nbind == PER_VERTEX_INDEXED) {
      nindices++;
    }
    if (tindices) tindices++;
  }
  if (mode != POLYGON) this->endShape();

  if (normalCacheUsed) {
    this->readUnlockNormalCache();
  }
  if (convexcacheused) {
    PRIVATE(this)->readUnlockConvexCache();
  }

  if (this->vertexProperty.getValue()) {
    state->pop();
  }
}

#undef DO_VERTEX

// doc from parent
void
SoIndexedFaceSet::getPrimitiveCount(SoGetPrimitiveCountAction *action)
{
  if (!this->shouldPrimitiveCount(action)) return;

  int n = this->coordIndex.getNum();
  if (n < 3) return;

  if (action->canApproximateCount()) {
    action->addNumTriangles(n/4);
  }
  else {
    const int32_t * ptr = coordIndex.getValues(0);
    const int32_t * endptr = ptr + n;
    int cnt = 0;
    int add = 0;
    while (ptr < endptr) {
      if (*ptr++ >= 0) cnt++;
      else {
        add += cnt-2;
        cnt = 0;
      }
    }
    // in case index array wasn't terminated with a -1
    if (cnt >= 3) add += cnt-2;
    action->addNumTriangles(add);
  }
}

//
// internal method which checks if convex cache needs to be
// used or (re)created. Returns TRUE if convex cache must be
// used. this->convexCache is then guaranteed to be != NULL.
//
SbBool
SoIndexedFaceSet::useConvexCache(SoAction * action,
                                 const SbVec3f * COIN_UNUSED_ARG(normals),
                                 const int32_t * nindices,
                                 const SbBool normalsfromcache)
{
  // we only want to use the convex data cache when rendering. All
  // other actions should work on the original data to ensure correct
  // part and face indices.
  if (!action->isOfType(SoGLRenderAction::getClassTypeId())) return FALSE;
  
  SoState * state = action->getState();
  if (SoShapeHintsElement::getFaceType(state) == SoShapeHintsElement::CONVEX)
    return FALSE;
  
  if (PRIVATE(this)->concavestatus == STATUS_UNKNOWN) {
    const int32_t * ptr = this->coordIndex.getValues(0);
    const int32_t * endptr = ptr + this->coordIndex.getNum();
    int cnt = 0;
    PRIVATE(this)->concavestatus = STATUS_CONVEX;
    while (ptr < endptr) {
      if (*ptr++ >= 0) cnt++;
      else {
        if (cnt > 3) { PRIVATE(this)->concavestatus = STATUS_CONCAVE; break; }
        cnt = 0;
      }
    }
  }
  if (PRIVATE(this)->concavestatus == STATUS_CONVEX) return FALSE;

  PRIVATE(this)->readLockConvexCache();

  if (PRIVATE(this)->convexCache && PRIVATE(this)->convexCache->isValid(state))
    return TRUE;

  PRIVATE(this)->readUnlockConvexCache();
  PRIVATE(this)->writeLockConvexCache();

  if (PRIVATE(this)->convexCache) PRIVATE(this)->convexCache->unref();
  SbBool storedinvalid = SoCacheElement::setInvalid(FALSE);

  // need to send matrix if we have some weird transformation
  SbMatrix modelmatrix = SoModelMatrixElement::get(state);
  if (modelmatrix[3][0] == 0.0f &&
      modelmatrix[3][1] == 0.0f &&
      modelmatrix[3][2] == 0.0f &&
      modelmatrix[3][3] == 1.0f) modelmatrix = SbMatrix::identity();

  // push to create cache dependencies
  state->push();
  PRIVATE(this)->convexCache = new SoConvexDataCache(state);
  PRIVATE(this)->convexCache->ref();
  SoCacheElement::set(state, PRIVATE(this)->convexCache);
  if (this->vertexProperty.getValue()) this->vertexProperty.getValue()->doAction(action);
  const SoCoordinateElement * coords;
  const SbVec3f * dummynormals;
  const int32_t * cindices;
  int numindices;
  const int32_t * dummynindices;
  const int32_t * tindices;
  const int32_t * mindices;
  SbBool dummy;

  // normals was included as parameters to this function (to avoid
  // a double readLock on the normal cache), so tell getVertexData()
  // not to return normals.
  this->getVertexData(state, coords, dummynormals, cindices,
                      dummynindices, tindices, mindices, numindices,
                      FALSE, dummy);
  
  // force a cache-dependency on SoNormalElement
  (void) SoNormalElement::getInstance(state);

  Binding mbind = this->findMaterialBinding(state);
  Binding nbind = this->findNormalBinding(state);

  if (this->getNodeType() == SoNode::VRML1) {
    // For VRML1, PER_VERTEX means per vertex in shape, not PER_VERTEX
    // on the state.
    if (mbind == PER_VERTEX) {
      mbind = PER_VERTEX_INDEXED;
      mindices = cindices;
    }
    if (nbind == PER_VERTEX) {
      nbind = PER_VERTEX_INDEXED;
      nindices = cindices;
    }
  }

  if (normalsfromcache && nbind == PER_VERTEX) {
    nbind = PER_VERTEX_INDEXED;
  }

  Binding tbind = NONE;
  // FIXME: just call inherited::areTexCoordsIndexed() instead of
  // the if-check? 20020110 mortene.
  if (SoTextureCoordinateBindingElement::get(state) ==
      SoTextureCoordinateBindingElement::PER_VERTEX) {
    tbind = PER_VERTEX;
  }
  else {
    tbind = PER_VERTEX_INDEXED;
    if (tindices == NULL) tindices = cindices;
  }

  if (nbind == PER_VERTEX_INDEXED && nindices == NULL) {
    nindices = cindices;
  }
  if (mbind == PER_VERTEX_INDEXED && mindices == NULL) {
    mindices = cindices;
  }
  PRIVATE(this)->convexCache->generate(coords, modelmatrix,
                              cindices, numindices,
                              mindices, nindices, tindices,
                              (SoConvexDataCache::Binding)mbind,
                              (SoConvexDataCache::Binding)nbind,
                              (SoConvexDataCache::Binding)tbind);

  PRIVATE(this)->writeUnlockConvexCache();

  state->pop();
  SoCacheElement::setInvalid(storedinvalid);

  PRIVATE(this)->readLockConvexCache();

  return TRUE;
}

// Documented in superclass.
SbBool
SoIndexedFaceSet::generateDefaultNormals(SoState *, SoNormalBundle *)
{
  // Normals are generated in normal cache.
  return FALSE;
}

// Documented in superclass.
SbBool
SoIndexedFaceSet::generateDefaultNormals(SoState * state,
                                       SoNormalCache * nc)
{
  SbBool ccw = TRUE;
  if (SoShapeHintsElement::getVertexOrdering(state) ==
      SoShapeHintsElement::CLOCKWISE) ccw = FALSE;

  const SbVec3f * coords = SoCoordinateElement::getInstance(state)->getArrayPtr3();
  assert(coords);

  int numcoords = SoCoordinateElement::getInstance(state)->getNum();

  SoNormalBindingElement::Binding normbind =
    SoNormalBindingElement::get(state);

  switch (normbind) {
  case SoNormalBindingElement::PER_VERTEX:
  case SoNormalBindingElement::PER_VERTEX_INDEXED:
    nc->generatePerVertex(coords,
                          numcoords,
                          coordIndex.getValues(0),
                          coordIndex.getNum(),
                          SoCreaseAngleElement::get(state, this->getNodeType() == SoNode::VRML1),
                          NULL,
                          -1,
                          ccw);
    break;
  case SoNormalBindingElement::PER_FACE:
  case SoNormalBindingElement::PER_FACE_INDEXED:
  case SoNormalBindingElement::PER_PART:
  case SoNormalBindingElement::PER_PART_INDEXED:
    nc->generatePerFace(coords,
                        numcoords,
                        coordIndex.getValues(0),
                        coordIndex.getNum(),
                        ccw);
    break;
  default:
    break;
  }
  return TRUE;
}

#undef PRIVATE
#undef STATUS_UNKNOWN
#undef STATUS_CONVEX
#undef STATUS_CONCAVE
#undef LOCK_VAINDEXER
#undef UNLOCK_VAINDEXER

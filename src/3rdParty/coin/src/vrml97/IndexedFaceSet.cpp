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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H

#ifdef HAVE_VRML97

/*!
  \class SoVRMLIndexedFaceSet SoVRMLIndexedFaceSet.h Inventor/VRMLnodes/SoVRMLIndexedFaceSet.h
  \brief The SoVRMLIndexedFaceSet class is used for representing a generic 3D shape.

  \ingroup coin_VRMLnodes

  \WEB3DCOPYRIGHT

  \verbatim
  IndexedFaceSet {
    eventIn       MFInt32 set_colorIndex
    eventIn       MFInt32 set_coordIndex
    eventIn       MFInt32 set_normalIndex
    eventIn       MFInt32 set_texCoordIndex
    exposedField  SFNode  color             NULL
    exposedField  SFNode  coord             NULL
    exposedField  SFNode  normal            NULL
    exposedField  SFNode  texCoord          NULL
    field         SFBool  ccw               TRUE
    field         MFInt32 colorIndex        []        # [-1,)
    field         SFBool  colorPerVertex    TRUE
    field         SFBool  convex            TRUE
    field         MFInt32 coordIndex        []        # [-1,)
    field         SFFloat creaseAngle       0         # [0,)
    field         MFInt32 normalIndex       []        # [-1,)
    field         SFBool  normalPerVertex   TRUE
    field         SFBool  solid             TRUE
    field         MFInt32 texCoordIndex     []        # [-1,)
  }
  \endverbatim

  The IndexedFaceSet node represents a 3D shape formed by constructing
  faces (polygons) from vertices listed in the coord field. The coord
  field contains a Coordinate node that defines the 3D vertices
  referenced by the coordIndex field. IndexedFaceSet uses the indices
  in its coordIndex field to specify the polygonal faces by indexing
  into the coordinates in the Coordinate node. An index of "-1"
  indicates that the current face has ended and the next one
  begins. The last face may be (but does not have to be) followed by a
  "-1" index. If the greatest index in the coordIndex field is N, the
  Coordinate node shall contain N+1 coordinates (indexed as 0 to
  N). Each face of the IndexedFaceSet shall have:

  - at least three non-coincident vertices;
  - vertices that define a planar polygon;
  - vertices that define a non-self-intersecting polygon.

  Otherwise, The results are undefined.

  The IndexedFaceSet node is specified in the local coordinate system
  and is affected by the transformations of its ancestors.

  Descriptions of the coord, normal, and texCoord fields are provided
  in the SoVRMLCoordinate, SoVRMLNormal, and SoVRMLTextureCoordinate nodes,
  respectively.

  Details on lighting equations and the interaction
  between color field, normal field, textures, materials, and
  geometries are provided in 4.14, Lighting model.

  If the color field is not NULL, it shall contain a Color node whose
  colours are applied to the vertices or faces of the IndexedFaceSet
  as follows:

  - If colorPerVertex is FALSE, colours are applied to each face, as
    follows:

    - If the colorIndex field is not empty, then one colour is used
      for each face of the IndexedFaceSet. There shall be at least as many indices
      in the colorIndex field as there are faces in the IndexedFaceSet.
      If the greatest index in the colorIndex field is N, then there shall
      be N+1 colours in the Color node. The colorIndex field shall not
      contain any negative entries.

    - If the colorIndex field is empty, then the colours in the Color
      node are applied to each face of the IndexedFaceSet in order. There shall
      be at least as many colours in the Color node as there are faces.

  - If colorPerVertex is TRUE, colours are applied to each vertex,
    as follows:

    - If the colorIndex field is not empty, then colours are applied
      to each vertex of the IndexedFaceSet in exactly the same manner
      that the coordIndex field is used to choose coordinates for each
      vertex from the Coordinate node. The colorIndex field shall
      contain at least as many indices as the coordIndex field, and
      shall contain end-of-face markers (-1) in exactly the same places
      as the coordIndex field.  If the greatest index in the colorIndex
      field is N, then there shall be N+1 colours in the Color node.

    - If the colorIndex field is empty, then the coordIndex field is
      used to choose colours from the Color node. If the greatest index
      in the coordIndex field is N, then there shall be N+1 colours in
      the Color node.

  If the color field is NULL, the geometry shall be rendered normally
  using the Material and texture defined in the Appearance node (see
  4.14, Lighting model, for details
  http://www.web3d.org/documents/specifications/14772/V2.0/part1/concepts.html#4.6.5).

  If the normal field is not NULL, it shall contain a Normal node
  whose normals are applied to the vertices or faces of the
  IndexedFaceSet in a manner exactly equivalent to that described
  above for applying colours to vertices/faces (where normalPerVertex
  corresponds to colorPerVertex and normalIndex corresponds to
  colorIndex). If the normal field is NULL, the browser shall
  automatically generate normals, using creaseAngle to determine if
  and how normals are smoothed across shared vertices (see 4.6.3.5,
  Crease angle field).

  If the texCoord field is not NULL, it shall contain a
  TextureCoordinate node. The texture coordinates in that node are
  applied to the vertices of the IndexedFaceSet as follows: If the
  texCoordIndex field is not empty, then it is used to choose texture
  coordinates for each vertex of the IndexedFaceSet in exactly the
  same manner that the coordIndex field is used to choose coordinates
  for each vertex from the Coordinate node. The texCoordIndex field
  shall contain at least as many indices as the coordIndex field, and
  shall contain end-of-face markers (-1) in exactly the same places as
  the coordIndex field. If the greatest index in the texCoordIndex
  field is N, then there shall be N+1 texture coordinates in the
  TextureCoordinate node.

  If the texCoordIndex field is empty, then the coordIndex array is
  used to choose texture coordinates from the TextureCoordinate
  node. If the greatest index in the coordIndex field is N, then there
  shall be N+1 texture coordinates in the TextureCoordinate node.  If
  the texCoord field is NULL, a default texture coordinate mapping is
  calculated using the local coordinate system bounding box of the
  shape.  The longest dimension of the bounding box defines the S
  coordinates, and the next longest defines the T coordinates. If two
  or all three dimensions of the bounding box are equal, ties shall be
  broken by choosing the X, Y, or Z dimension in that order of
  preference. The value of the S coordinate ranges from 0 to 1, from
  one end of the bounding box to the other. The T coordinate ranges
  between 0 and the ratio of the second greatest dimension of the
  bounding box to the greatest dimension. Figure 6.10 illustrates the
  default texture coordinates for a simple box shaped IndexedFaceSet
  with an X dimension twice as large as the Z dimension and four times
  as large as the Y dimension. Figure 6.11 illustrates the original
  texture image used on the IndexedFaceSet used in Figure 6.10.

  <center>
  <img src="http://www.web3d.org/documents/specifications/14772/V2.0/Images/IFStexture.gif">
  Figure 6.10
  </center>

  <center>
  <img src="http://www.web3d.org/documents/specifications/14772/V2.0/Images/IFStexture2.gif">
  Figure 6.11
  </center>

  Subclause 4.6.3, Shapes and geometry
  (<http://www.web3d.org/documents/specifications/14772/V2.0/part1/concepts.html#4.6.5>),
  provides a description of the ccw, solid, convex, and creaseAngle
  fields.

*/

/*!
  \var SoSFBool SoVRMLIndexedFaceSet::ccw
  Specifies if vertex ordering is counterclockwise. Default value is TRUE.
*/

/*!
  \var SoSFBool SoVRMLIndexedFaceSet::solid
  Can be used to enable backface culling. Default value is TRUE.
*/

/*!
  \var SoSFBool SoVRMLIndexedFaceSet::convex
  Specifies if all polygons are convex. Default value is TRUE.
*/

/*!
  \var SoSFFloat SoVRMLIndexedFaceSet::creaseAngle
  Specifies the crease angle for the generated normals. Default value is 0.0.
*/

#include <Inventor/VRMLnodes/SoVRMLIndexedFaceSet.h>

#include <Inventor/SoPrimitiveVertex.h>
#include <Inventor/VRMLnodes/SoVRMLCoordinate.h>
#include <Inventor/VRMLnodes/SoVRMLMacros.h>
#include <Inventor/actions/SoCallbackAction.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/actions/SoGetPrimitiveCountAction.h>
#include <Inventor/bundles/SoMaterialBundle.h>
#include <Inventor/bundles/SoTextureCoordinateBundle.h>
#include <Inventor/bundles/SoVertexAttributeBundle.h>
#include <Inventor/caches/SoConvexDataCache.h>
#include <Inventor/caches/SoNormalCache.h>
#include <Inventor/details/SoFaceDetail.h>
#include <Inventor/elements/SoCacheElement.h>
#include <Inventor/elements/SoCoordinateElement.h>
#include <Inventor/elements/SoGLLazyElement.h>
#include <Inventor/elements/SoGLVBOElement.h>
#include <Inventor/elements/SoMaterialBindingElement.h>
#include <Inventor/elements/SoVertexAttributeBindingElement.h>
#include <Inventor/elements/SoModelMatrixElement.h>
#include <Inventor/elements/SoNormalBindingElement.h>
#include <Inventor/elements/SoOverrideElement.h>
#include <Inventor/errors/SoDebugError.h>
#include <Inventor/system/gl.h>
#include <Inventor/misc/SoGLDriverDatabase.h>
#ifdef HAVE_THREADS
#include <Inventor/threads/SbRWMutex.h>
#endif // HAVE_THREADS

#include "rendering/SoVBO.h"
#include "rendering/SoVertexArrayIndexer.h"
#include "glue/glp.h"
#include "rendering/SoGL.h"
#include "nodes/SoSubNodeP.h"

// *************************************************************************

// for concavestatus
#define STATUS_UNKNOWN 0
#define STATUS_CONVEX  1
#define STATUS_CONCAVE 2

#define LOCK_VAINDEXER(obj) SoBase::staticDataLock()
#define UNLOCK_VAINDEXER(obj) SoBase::staticDataUnlock()

// *************************************************************************

class SoVRMLIndexedFaceSetP {
public:
  SoVRMLIndexedFaceSetP(void) 
#ifdef COIN_THREADSAFE
    : convexmutex(SbRWMutex::READ_PRECEDENCE)
#endif // COIN_THREADSAFE 
  { }
  SoVertexArrayIndexer * vaindexer;
  SoConvexDataCache * convexCache;
  int concavestatus;

#ifdef COIN_THREADSAFE
  SbRWMutex convexmutex;
  void readLockConvexCache(void) { this->convexmutex.readLock(); }
  void readUnlockConvexCache(void) { this->convexmutex.readUnlock(); }
  void writeLockConvexCache(void) { this->convexmutex.writeLock(); }
  void writeUnlockConvexCache(void) { this->convexmutex.writeUnlock(); }
#else // !COIN_THREADSAFE
  void readLockConvexCache(void) { }
  void readUnlockConvexCache(void) { }
  void writeLockConvexCache(void) { }
  void writeUnlockConvexCache(void) { }
#endif // !COIN_THREADSAFE
};

#define PRIVATE(obj) ((obj)->pimpl)

// *************************************************************************

SO_NODE_SOURCE(SoVRMLIndexedFaceSet);

// *************************************************************************

/*!
  \copydetails SoNode::initClass(void)
*/
void
SoVRMLIndexedFaceSet::initClass(void) // static
{
  SO_NODE_INTERNAL_INIT_CLASS(SoVRMLIndexedFaceSet, SO_VRML97_NODE_TYPE);
}

/*!
  Constructor.
*/
SoVRMLIndexedFaceSet::SoVRMLIndexedFaceSet(void)
{
  PRIVATE(this) = new SoVRMLIndexedFaceSetP;
  PRIVATE(this)->convexCache = NULL;
  PRIVATE(this)->concavestatus = STATUS_UNKNOWN;
  PRIVATE(this)->vaindexer = NULL;

  SO_VRMLNODE_INTERNAL_CONSTRUCTOR(SoVRMLIndexedFaceSet);

  SO_VRMLNODE_ADD_FIELD(ccw, (TRUE));
  SO_VRMLNODE_ADD_FIELD(solid, (TRUE));
  SO_VRMLNODE_ADD_FIELD(convex, (TRUE));
  SO_VRMLNODE_ADD_FIELD(creaseAngle, (0.0f));

}

/*!
  Destructor.
*/
SoVRMLIndexedFaceSet::~SoVRMLIndexedFaceSet() // virtual, protected
{
  if (PRIVATE(this)->convexCache) PRIVATE(this)->convexCache->unref();
  delete PRIVATE(this)->vaindexer;
  delete PRIVATE(this);
}

//
// translates current material binding into the internal Binding enum.
//
SoVRMLIndexedFaceSet::Binding
SoVRMLIndexedFaceSet::findMaterialBinding(SoState * state) const
{
  Binding binding = OVERALL;

  if (SoOverrideElement::getMaterialBindingOverride(state)) {
    switch (SoMaterialBindingElement::get(state)) {
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
      SoDebugError::postWarning("SoVRMLIndexedFaceSet::findMaterialBinding",
                                "unknown material binding setting");
#endif // COIN_DEBUG
      break;
    }
  }
  else {
    if (this->color.getValue()) {
      if (this->colorPerVertex.getValue()) {
        binding = PER_VERTEX;
        if (this->colorIndex.getNum() && this->colorIndex[0] >= 0) 
          binding = PER_VERTEX_INDEXED;
      }
      else {
        binding = PER_FACE;
        if (this->colorIndex.getNum() && this->colorIndex[0] >= 0) 
          binding = PER_FACE_INDEXED;
      }
    }
  }
  return binding;
}


//
// translates current normal binding into the internal Binding enum.
//
SoVRMLIndexedFaceSet::Binding
SoVRMLIndexedFaceSet::findNormalBinding(SoState * state) const
{
  Binding binding = OVERALL;

  if (SoOverrideElement::getNormalBindingOverride(state)) {
    switch (SoNormalBindingElement::get(state)) {
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
      SoDebugError::postWarning("SoVRMLIndexedFaceSet::findNormalBinding",
                                "unknown normal binding setting");
#endif // COIN_DEBUG
      break;
    }
  }
  else {
    if (this->normalPerVertex.getValue()) {
      binding = PER_VERTEX_INDEXED;
      if (this->normal.getValue() && 
          (this->normalIndex.getNum() == 0 ||
           this->normalIndex[0] < 0)) binding = PER_VERTEX;
    }
    else {
      binding = PER_FACE;
      if (this->normalIndex.getNum() && this->normalIndex[0] >= 0) binding = PER_FACE_INDEXED;
    }
  }
  return binding;
}


// Doc in parent
void
SoVRMLIndexedFaceSet::GLRender(SoGLRenderAction * action)
{
  if (this->coordIndex.getNum() < 3 || this->coord.getValue() == NULL) return;
  SoState * state = action->getState();

  state->push();
  // update state with coordinates, normals and texture information
  SoVRMLVertexShape::GLRender(action);

  if (!this->shouldGLRender(action)) { 
    state->pop();
    return;
  }

  this->setupShapeHints(state, this->ccw.getValue(), this->solid.getValue());

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

  if (!sendNormals) {
    nbind = OVERALL;
    normals = NULL;
    nindices = NULL;
  }
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

  if (mbind == PER_VERTEX) {
    mbind = PER_VERTEX_INDEXED;
    mindices = cindices;
  }
  if (nbind == PER_VERTEX) {
    nbind = PER_VERTEX_INDEXED;
    nindices = cindices;
  }

  Binding tbind = NONE;
  if (doTextures) {
    if (tb.isFunction() && !tb.needIndices()) {
      tbind = NONE;
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

  SoGLLazyElement * lelem = NULL;
  const uint32_t contextid = action->getCacheContext();

  SbBool dova = 
    SoVBO::shouldRenderAsVertexArrays(state, contextid, numindices) &&
    !convexcacheused && !normalCacheUsed &&
    ((nbind == OVERALL) || ((nbind == PER_VERTEX_INDEXED) && ((nindices == cindices) || (nindices == NULL)))) &&
    ((tbind == NONE && !tb.needCoordinates()) || 
     ((tbind == PER_VERTEX_INDEXED) && ((tindices == cindices) || (tindices == NULL)))) &&
    ((mbind == NONE) || ((mbind == PER_VERTEX_INDEXED) && ((mindices == cindices) || (mindices == NULL)))) &&
    SoGLDriverDatabase::isSupported(sogl_glue_instance(state), SO_GL_VERTEX_ARRAY);

  const SoGLVBOElement * vboelem = SoGLVBOElement::getInstance(state);
  SoVBO * colorvbo = NULL;

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
  SbBool didrenderasvbo = FALSE;
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
      fprintf(stderr,"XXX: create VRML VertexArrayIndexer: %d\n", indexer->getNumVertices());
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

  // send approx number of triangles for autocache handling
  sogl_autocache_update(state, this->coordIndex.getNum() / 4, didrenderasvbo);

  state->pop();
}

// Doc in parent
void
SoVRMLIndexedFaceSet::getPrimitiveCount(SoGetPrimitiveCountAction * action)
{
  if (!this->shouldPrimitiveCount(action)) return;

  int n = this->coordIndex.getNum();
  if (n < 3) return;

  if (action->canApproximateCount()) {
    action->addNumTriangles(n/4);
  }
  else {
    const int32_t * ptr = this->coordIndex.getValues(0);
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

// Doc in parent
SbBool
SoVRMLIndexedFaceSet::generateDefaultNormals(SoState * COIN_UNUSED_ARG(s),
                                             SoNormalBundle * COIN_UNUSED_ARG(nb))
{
  return FALSE;
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
  }                                         \
  if (tb.isFunction()) {               \
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

// Doc in parent
void
SoVRMLIndexedFaceSet::generatePrimitives(SoAction * action)
{
  if (this->coordIndex.getNum() < 3) return;

  SoState * state = action->getState();

  state->push();
  SoVRMLVertexShape::doAction(action);

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

  this->getVertexData(state, coords, normals, cindices,
                      nindices, tindices, mindices, numindices,
                      sendNormals, normalCacheUsed);

  if (!sendNormals) {
    nbind = OVERALL;
    normals = NULL;
    nindices = NULL;
  }
  else if (normalCacheUsed && nbind == PER_VERTEX) {
    nbind = PER_VERTEX_INDEXED;
  }
  else if (normalCacheUsed && nbind == PER_FACE_INDEXED) {
    nbind = PER_FACE;
  }

  if (mbind == PER_VERTEX) {
    mbind = PER_VERTEX_INDEXED;
    mindices = cindices;
  }
  if (nbind == PER_VERTEX) {
    nbind = PER_VERTEX_INDEXED;
    nindices = cindices;
  }

  SoTextureCoordinateBundle tb(action, FALSE, FALSE);
  doTextures = tb.needCoordinates();

  Binding tbind = NONE;
  if (doTextures) {
    if (tb.isFunction() && !tb.needIndices()) {
      tbind = NONE;
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
    assert(v1 >= 0 && v2 >= 0 && v3 >= 0);
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
  state->pop();
}

#undef DO_VERTEX

// Doc in parent
SbBool
SoVRMLIndexedFaceSet::generateDefaultNormals(SoState * state,
                                             SoNormalCache * nc)
{
  SoVRMLCoordinate * node = (SoVRMLCoordinate*) this->coord.getValue();
  if (node == NULL) return TRUE; // ok, empty ifs

  const SbVec3f * coords = node->point.getValues(0);

  int numcoords = node->point.getNum();

  switch (this->findNormalBinding(state)) {
  case PER_VERTEX:
  case PER_VERTEX_INDEXED:
    nc->generatePerVertex(coords,
                          numcoords,
                          coordIndex.getValues(0),
                          coordIndex.getNum(),
                          this->creaseAngle.getValue(),
                          NULL, // face normals
                          0,    // num face normals
                          this->ccw.getValue());
    break;
  case PER_FACE:
  case PER_FACE_INDEXED:
    nc->generatePerFace(coords,
                        numcoords,
                        coordIndex.getValues(0),
                        coordIndex.getNum(),
                        this->ccw.getValue());
    break;
  default:
    break;
  }
  return TRUE;
}

// Doc in parent
void
SoVRMLIndexedFaceSet::notify(SoNotList * list)
{
  if (PRIVATE(this)->convexCache) PRIVATE(this)->convexCache->invalidate();
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

//
// internal method which checks if convex cache needs to be
// used or (re)created. Returns TRUE if convex cache must be
// used. PRIVATE(this)->convexCache is then guaranteed to be != NULL.
//
SbBool
SoVRMLIndexedFaceSet::useConvexCache(SoAction * action, 
                                     const SbVec3f * normals, 
                                     const int32_t * nindices,
                                     const SbBool normalsfromcache)
{
  SoState * state = action->getState();
  if (this->convex.getValue())
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

  if (PRIVATE(this)->convexCache && PRIVATE(this)->convexCache->isValid(state)) {
    // check if convex cache has normal indices. The convex cache
    // might be generated without normals.
    if (normals == NULL || PRIVATE(this)->convexCache->getNormalIndices()) {
      return TRUE;
    }
  }

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

  SoVRMLVertexShape::doAction(action);

  const SoCoordinateElement * coords;
  const int32_t * cindices;
  const SbVec3f * dummynormals;
  int numindices;
  const int32_t * dummynindices;
  const int32_t * tindices;
  const int32_t * mindices;
  SbBool dummy;

  this->getVertexData(state, coords, dummynormals, cindices,
                      dummynindices, tindices, mindices, numindices,
                      FALSE, dummy);

  Binding mbind = this->findMaterialBinding(state);
  Binding nbind = normals ? this->findNormalBinding(state) : OVERALL;
  
  if (normalsfromcache && nbind == PER_VERTEX) {
    nbind = PER_VERTEX_INDEXED;
  }

  if (mbind == PER_VERTEX) {
    mbind = PER_VERTEX_INDEXED;
    mindices = tindices;
  }
  if (nbind == PER_VERTEX) {
    nbind = PER_VERTEX_INDEXED;
    nindices = cindices;
  }

  Binding tbind = PER_VERTEX_INDEXED;
  if (tindices == NULL) tindices = cindices;

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

#undef PRIVATE
#undef STATUS_UNKNOWN
#undef STATUS_CONVEX
#undef STATUS_CONCAVE

#endif // HAVE_VRML97

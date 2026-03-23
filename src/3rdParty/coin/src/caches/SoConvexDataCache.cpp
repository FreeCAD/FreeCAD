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
  \class SoConvexDataCache SoConvexDataCache.h Inventor/caches/SoConvexDataCache.h
  \brief The SoConvexDataCache class is used to cache convexified polygons.

  \ingroup coin_caches

  SoConvexDataCache is used to speed up rendering of concave polygons
  by tessellating all polygons into triangles and storing the newly
  generated primitives in an internal cache.

  This class is not part of the original SGI Open Inventor v2.1
  API, but is a Coin extension.
*/

// *************************************************************************

#include <Inventor/caches/SoConvexDataCache.h>

#include <cassert>

#include <Inventor/SbMatrix.h>
#include <Inventor/SbTesselator.h>
#include <Inventor/elements/SoCoordinateElement.h>
#include <Inventor/errors/SoDebugError.h>
#include <Inventor/lists/SbList.h>

#include "tidbitsp.h"
#include "base/SbGLUTessellator.h"

// *************************************************************************

class SoConvexDataCacheP {
public:
  SbList <int32_t> coordIndices;
  SbList <int32_t> normalIndices;
  SbList <int32_t> materialIndices;
  SbList <int32_t> texIndices;
};

#define PRIVATE(obj) ((obj)->pimpl)

// *************************************************************************

/*!
  \enum SoConvexDataCache::Binding
  \brief The Binding enum is used to specify bindings.

  Binding applies to normals, materials and texture coordinates.
*/

// *************************************************************************

/*!
  Constructor with \a state being the current state.
*/
SoConvexDataCache::SoConvexDataCache(SoState * const state)
  : SoCache(state)
{
  PRIVATE(this) = new SoConvexDataCacheP;
#if COIN_DEBUG
  if (coin_debug_caching_level() > 0) {
    SoDebugError::postInfo("SoConvexDataCache::SoConvexDataCache",
                           "Cache created: %p", this);
    
  }
#endif // debug
}

/*!
  Destructor.
*/
SoConvexDataCache::~SoConvexDataCache()
{
#if COIN_DEBUG
  if (coin_debug_caching_level() > 0) {
    SoDebugError::postInfo("SoConvexDataCache::~SoConvexDataCache",
                           "Cache destructed: %p", this);
    
  }
#endif // debug
  delete PRIVATE(this);
}

/*!
  Returns a pointer to the convexified coordinate indices.
  \sa SoConvexDataCache::getNumCoordIndices()
*/
const int32_t *
SoConvexDataCache::getCoordIndices(void) const
{
  if (PRIVATE(this)->coordIndices.getLength()) return PRIVATE(this)->coordIndices.getArrayPtr();
  return NULL;
}

/*!
  Returns the number of coordinate indices.
  \sa SoConvexDataCache::getCoordIndices()
*/
int
SoConvexDataCache::getNumCoordIndices(void) const
{
  return PRIVATE(this)->coordIndices.getLength();
}

/*!
  Returns the convexified material indices.
  \sa SoConvexDataCache::getNumMaterialIndices()
*/
const int32_t *
SoConvexDataCache::getMaterialIndices(void) const
{
  if (PRIVATE(this)->materialIndices.getLength()) return PRIVATE(this)->materialIndices.getArrayPtr();
  return NULL;
}

/*!
  Returns the number of material indices.
  \sa SoConvexDataCache::getMaterialIndices()
*/
int
SoConvexDataCache::getNumMaterialIndices(void) const
{
  return PRIVATE(this)->materialIndices.getLength();
}

/*!
  Returns the convexified normal indices.
  \sa SoConvexDataCache::getNumNormalIndices()
*/
const int32_t *
SoConvexDataCache::getNormalIndices(void) const
{
  if (PRIVATE(this)->normalIndices.getLength()) return PRIVATE(this)->normalIndices.getArrayPtr();
  return NULL;
}

/*!
  Returns the number of normal indices.
  \sa SoConvexDataCache::getNormalIndices()
*/
int
SoConvexDataCache::getNumNormalIndices(void) const
{
  return PRIVATE(this)->normalIndices.getLength();
}

/*!
  Returns the convexified texture coordinate indices.
  \sa SoConvexDataCache::getNumTexIndices()
*/
const int32_t *
SoConvexDataCache::getTexIndices(void) const
{
  if (PRIVATE(this)->texIndices.getLength()) return PRIVATE(this)->texIndices.getArrayPtr();
  return NULL;
}

/*!
  Returns the number of texture coordinate indices.
  \sa SoConvexDataCache::getTexIndices()
*/
int
SoConvexDataCache::getNumTexIndices(void) const
{
  return PRIVATE(this)->texIndices.getLength();
}


typedef struct
{
  int  matnr;
  int  texnr;
  int  normnr;
  int  vertexnr;
} tVertexInfo;

// callback function
static void do_triangle(void *vo, void *v1, void *v2, void *data);

//
// struct used to hold data for the tessellator callback
//
typedef struct {
  SbBool firstvertex;
  tVertexInfo *vertexInfo;
  SoConvexDataCache::Binding matbind;
  SoConvexDataCache::Binding normbind;
  SoConvexDataCache::Binding texbind;

  SbList <int32_t> *vertexIndex;
  SbList <int32_t> *matIndex;
  SbList <int32_t> *normIndex;
  SbList <int32_t> *texIndex;
  int numvertexind;
  int nummatind;
  int numnormind;
  int numtexind;
} tTessData;

/*!
  Generates the convexified data. FIXME: doc
*/
void
SoConvexDataCache::generate(const SoCoordinateElement * const coords,
                            const SbMatrix & matrix,
                            const int32_t *vind,
                            const int numv,
                            const int32_t *mind, const int32_t *nind,
                            const int32_t *tind,
                            const Binding matbind, const Binding normbind,
                            const Binding texbind)
{
#if COIN_DEBUG && 0
  SoDebugError::postInfo("SoConvexDataCache::generate",
                         "generating convex data");
#endif

  SbBool identity = matrix == SbMatrix::identity();

  // remove old data
  PRIVATE(this)->coordIndices.truncate(0);
  PRIVATE(this)->materialIndices.truncate(0);
  PRIVATE(this)->normalIndices.truncate(0);
  PRIVATE(this)->texIndices.truncate(0);

  int matnr = 0;
  int texnr = 0;
  int normnr = 0;

  // initialize the struct with data needed during tessellation
  tTessData tessdata;
  tessdata.matbind = matbind;
  tessdata.normbind = normbind;
  tessdata.texbind = texbind;
  tessdata.numvertexind = 0;
  tessdata.nummatind = 0;
  tessdata.numnormind = 0;
  tessdata.numtexind = 0;
  // FIXME: stupid to have a separate struct for each coordIndex
  // should only allocate enough to hold the largest polygon
  tessdata.vertexInfo = new tVertexInfo[numv];
  tessdata.vertexIndex = NULL;
  tessdata.matIndex = NULL;
  tessdata.normIndex = NULL;
  tessdata.texIndex = NULL;
  tessdata.firstvertex = TRUE;

  // create tessellator
  SbGLUTessellator glutess(do_triangle, &tessdata);
  SbTesselator tess(do_triangle, &tessdata);
  const SbBool gt = SbGLUTessellator::preferred();

  // if PER_FACE binding, the binding must change to PER_FACE_INDEXED
  // if convexify data is used.
  tessdata.vertexIndex = &PRIVATE(this)->coordIndices;
  if (matbind != NONE)
    tessdata.matIndex = &PRIVATE(this)->materialIndices;
  if (normbind != NONE)
    tessdata.normIndex = &PRIVATE(this)->normalIndices;
  if (texbind != NONE)
    tessdata.texIndex = &PRIVATE(this)->texIndices;

  if (gt) { glutess.beginPolygon(); }
  else { tess.beginPolygon(); }
  for (int i = 0; i < numv; i++) {
    if (vind[i] < 0) {
      if (gt) { glutess.endPolygon(); }
      else { tess.endPolygon(); }
      if (matbind == PER_VERTEX_INDEXED || 
          matbind == PER_FACE ||
          matbind == PER_FACE_INDEXED) matnr++;
      if (normbind == PER_VERTEX_INDEXED ||
          normbind == PER_FACE ||
          normbind == PER_FACE_INDEXED) normnr++;
      if (texbind == PER_VERTEX_INDEXED) texnr++;
      if (i < numv - 1) { // if not last polygon
        if (gt) { glutess.beginPolygon(); }
        else { tess.beginPolygon(); }
      }
    }
    else {
      tessdata.vertexInfo[i].vertexnr = vind[i];
      if (mind)
        tessdata.vertexInfo[i].matnr = mind[matnr];
      else tessdata.vertexInfo[i].matnr = matnr;
      if (matbind >= PER_VERTEX) {
        matnr++;
      }
      if (nind)
        tessdata.vertexInfo[i].normnr = nind[normnr];
      else tessdata.vertexInfo[i].normnr = normnr;
      if (normbind >= PER_VERTEX)
        normnr++;
      if (tind)
        tessdata.vertexInfo[i].texnr = tind[texnr++];
      else
        tessdata.vertexInfo[i].texnr = texnr++;

      SbVec3f v = coords->get3(vind[i]);
      if (!identity) matrix.multVecMatrix(v,v);
      if (gt) { glutess.addVertex(v, static_cast<void *>(&tessdata.vertexInfo[i])); }
      else { tess.addVertex(v, static_cast<void *>(&tessdata.vertexInfo[i])); }
    }
  }
  
  // if last coordIndex != -1, terminate polygon
  if (numv > 0 && vind[numv-1] != -1) {
    if (gt) { glutess.endPolygon(); }
    else { tess.endPolygon(); }
  }

  delete [] tessdata.vertexInfo;

  PRIVATE(this)->coordIndices.fit();
  if (tessdata.matIndex) PRIVATE(this)->materialIndices.fit();
  if (tessdata.normIndex) PRIVATE(this)->normalIndices.fit();
  if (tessdata.texIndex) PRIVATE(this)->texIndices.fit();
}

//
// helper function for do_triangle() below
//
static void
vertex_tri(tVertexInfo *info, tTessData *tessdata)
{
  tessdata->vertexIndex->append(info->vertexnr);
  tessdata->numvertexind++;

  if (tessdata->matIndex &&
      (tessdata->firstvertex ||
       tessdata->matbind >= SoConvexDataCache::PER_VERTEX)) {
    tessdata->matIndex->append(info->matnr);
    tessdata->nummatind++;
  }

  if (tessdata->normIndex &&
      (tessdata->firstvertex ||
       tessdata->normbind >= SoConvexDataCache::PER_VERTEX)) {
    tessdata->normIndex->append(info->normnr);
    tessdata->numnormind++;
  }
  if (tessdata->texIndex &&
      tessdata->texbind != SoConvexDataCache::NONE) {
    tessdata->texIndex->append(info->texnr);
    tessdata->numtexind++;
  }
  tessdata->firstvertex = FALSE;
}

//
// handles callbacks from SbTesselator or SbGLUTessellator
//
static void
do_triangle(void *v0, void *v1, void *v2, void *data)
{
  tTessData *tessdata = static_cast<tTessData *>(data);
  tessdata->firstvertex = TRUE;
  vertex_tri(static_cast<tVertexInfo *>(v0), tessdata);
  vertex_tri(static_cast<tVertexInfo *>(v1), tessdata);
  vertex_tri(static_cast<tVertexInfo *>(v2), tessdata);

  tessdata->vertexIndex->append(-1);
  if (tessdata->matIndex &&
      tessdata->matbind >= SoConvexDataCache::PER_VERTEX) {
    tessdata->matIndex->append(-1);
    tessdata->nummatind++;
  }
  if (tessdata->normIndex &&
      tessdata->normbind >= SoConvexDataCache::PER_VERTEX) {
    tessdata->normIndex->append(-1);
    tessdata->numnormind++;
  }
  if (tessdata->texIndex &&
      tessdata->texbind != SoConvexDataCache::NONE) {
    tessdata->texIndex->append(-1);
    tessdata->numtexind++;
  }
}

#undef PRIVATE

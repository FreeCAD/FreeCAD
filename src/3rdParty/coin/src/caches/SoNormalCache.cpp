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
  \class SoNormalCache SoNormalCache.h Inventor/caches/SoNormalCache.h
  \brief The SoNormalCache class is used to hold cached normals.

  \ingroup coin_caches

  As an extension to the original SGI Open Inventor v2.1 API, it is
  also possible to generate normals using this class.

  It is more powerful and easier to use than the SoNormalGenerator
  class. It is possible to generate normals per vertex with indices
  (using much less memory than plain per vertex normals), and it
  contains special methods to generate normals for triangle strips and
  quads.
*/

// *************************************************************************

#include <Inventor/caches/SoNormalCache.h>

#include <cfloat> // FLT_EPSILON

#include <Inventor/misc/SoNormalGenerator.h>
#include <Inventor/lists/SbList.h>
#include <Inventor/errors/SoDebugError.h>

#include "tidbitsp.h"

// *************************************************************************

#ifndef DOXYGEN_SKIP_THIS
class SoNormalCacheP {
public:
  int numNormals;
  union {
    const SbVec3f *normals;
    SoNormalGenerator *generator;
  } normalData;
  SbList <int32_t> indices;
  SbList <SbVec3f> normalArray;
};
#endif // DOXYGEN_SKIP_THIS

//
// FIXME: add test to shrink normalArray.
//

#define NORMAL_EPSILON FLT_EPSILON

#define PRIVATE(obj) ((obj)->pimpl)

#define NORMALCACHE_DEBUG 0 // Set to one for debug output

// *************************************************************************

/*!
  Constructor with \a state being the current state.
*/
SoNormalCache::SoNormalCache(SoState * const state)
  : SoCache(state)
{
  PRIVATE(this) = new SoNormalCacheP;
  PRIVATE(this)->normalData.normals = NULL;
  PRIVATE(this)->numNormals = 0;

#if COIN_DEBUG
  if (coin_debug_caching_level() > 0) {
    SoDebugError::postInfo("SoNormalCache::SoNormalCache",
                           "Cache created: %p", this);
    
  }
#endif // debug
}

/*!
  Destructor
*/
SoNormalCache::~SoNormalCache()
{
#if COIN_DEBUG
  if (coin_debug_caching_level() > 0) {
    SoDebugError::postInfo("SoNormalCache::~SoNormalCache",
                           "Cache destructed: %p", this);
    
  }
#endif // debug

  this->clearGenerator();
  delete PRIVATE(this);
}

/*!
  Sets an array of normals for this cache. The normals will not
  be deleted when the instance is deleted.
*/
void
SoNormalCache::set(const int num, const SbVec3f * const normals)
{
  this->clearGenerator();
  PRIVATE(this)->numNormals = num;
  PRIVATE(this)->normalData.normals = normals;
  PRIVATE(this)->indices.truncate(0, TRUE);
  PRIVATE(this)->normalArray.truncate(0, TRUE);
}

/*!
  Uses a normal generator in this cache. The normal generator will
  be deleted when the cache is deleted or reset.
*/
void
SoNormalCache::set(SoNormalGenerator * generator)
{
  this->clearGenerator();
  PRIVATE(this)->indices.truncate(0, TRUE);
  PRIVATE(this)->normalArray.truncate(0, TRUE);
  PRIVATE(this)->numNormals = 0;
  PRIVATE(this)->normalData.generator = generator;
}

/*!
  Returns the number of normals in the cache.
*/
int
SoNormalCache::getNum(void) const
{
  if (PRIVATE(this)->numNormals == 0 && PRIVATE(this)->normalData.generator) {
    return PRIVATE(this)->normalData.generator->getNumNormals();
  }
  return PRIVATE(this)->numNormals;
}

/*!
  Return a pointer to the normals in this cache.
*/
const SbVec3f *
SoNormalCache::getNormals(void) const
{
  if (PRIVATE(this)->numNormals == 0 && PRIVATE(this)->normalData.generator) {
    return PRIVATE(this)->normalData.generator->getNormals();
  }
  return PRIVATE(this)->normalData.normals;
}

/*!
  Returns the number of indices in this cache. Normals are
  generated with PER_VERTEX_INDEXED binding.
*/
int
SoNormalCache::getNumIndices(void) const
{
  return PRIVATE(this)->indices.getLength();
}

/*!
  Returns the normal indices.
*/
const int32_t *
SoNormalCache::getIndices(void) const
{
  if (PRIVATE(this)->indices.getLength()) return PRIVATE(this)->indices.getArrayPtr();
  return NULL;
}

//
// calculates the normal vector for a vertex, based on the
// normal vectors of all incident faces
//
static void
calc_normal_vec(const SbVec3f * facenormals, const int facenum, 
                const int numfacenorm, SbList <int32_t> & faceArray, 
                const float threshold, SbVec3f & vertnormal)
{
  // start with face normal vector
  const SbVec3f * facenormal = & facenormals[facenum];
  vertnormal = *facenormal;

  int n = faceArray.getLength();
  int currface;

  for (int i = 0; i < n; i++) {
    currface = faceArray[i];
    if (currface != facenum) { // check all but this face
      if (currface < numfacenorm || numfacenorm == -1) { // -1 means: assume
        const SbVec3f & normal = facenormals[currface];  // everything is ok
        if ((normal.dot(*facenormal)) > threshold) {
          // smooth towards this face
          vertnormal += normal;
        }
      }
      else {
        static int calc_norm_error = 0;
        if (calc_norm_error < 1) {
          SoDebugError::postWarning("SoNormalCache::calc_normal_vec", "Normals "
                                    "have not been specified for all faces. "
                                    "this warning will only be shown once, "
                                    "but there might be more errors");
        }

        calc_norm_error++;
      }
    }
  }
}

/*!
  Generates normals for each vertex for each face. It is possible to
  specify face normals if these have been calculated somewhere else,
  otherwise the face normals will be calculated before the vertex
  normals are calculated. \a tristrip should be \c TRUE if the
  geometry consists of triangle strips.
*/
void
SoNormalCache::generatePerVertex(const SbVec3f * const coords,
                                 const unsigned int numcoords,
                                 const int32_t * vindex,
                                 const int numvi,
                                 const float crease_angle,
                                 const SbVec3f * facenormals,
                                 const int numfacenormals,
                                 const SbBool ccw,
                                 const SbBool tristrip)
{
#if NORMALCACHE_DEBUG && COIN_DEBUG
  SoDebugError::postInfo("SoNormalCache::generatePerVertex", "generating normals");
#endif

  this->clearGenerator();
  PRIVATE(this)->indices.truncate(0);
  PRIVATE(this)->normalArray.truncate(0);


#if COIN_DEBUG && 0 // debug
  SoDebugError::postInfo("SoNormalCache::generatePerVertex", "%d", numvi);
  for (int vrtidx=0; vrtidx < numvi; vrtidx++)
    fprintf(stdout, "%d ", vindex[vrtidx]);
  fprintf(stdout, "\n");
#endif // debug


  int numfacenorm = numfacenormals;
  SoNormalCache tempcache(NULL);
  const SbVec3f * facenorm = const_cast<SbVec3f *>(facenormals);
  if (facenorm == NULL) {
    // use a SoNormalCache to store temporary data
    if (tristrip) {
      tempcache.generatePerFaceStrip(coords, numcoords, vindex, numvi, ccw);
    }
    else {
      tempcache.generatePerFace(coords, numcoords, vindex, numvi, ccw);
    }

    facenorm = tempcache.getNormals();
    numfacenorm = tempcache.getNum();

    assert(facenorm && "Normals should be generated for all coords");
  }

  // find biggest vertex index
  int i;
  int maxi = 0;
  int temp;
  for (i = 0; i < numvi; i++) {
    temp = vindex[i]; // don't care about -1's
    if (temp > maxi) maxi = temp;
  }

  // for each vertex, store all faceindices the vertex is a part of
  SbList<int32_t> * vertexFaceArray = new SbList<int32_t>[maxi+1]; // [0, maxi]

  // for each vertex, store all normals that have been calculated
  SbList <int32_t>* vertexNormalArray = new SbList<int32_t>[maxi+1]; // [0, maxi]

  int numfaces = 0;

  if (tristrip) {
    // Find and save the faces belonging to the different vertices
    i = 0;
    while (i + 2 < numvi) {
      temp = vindex[i];
      if (temp >= 0 && static_cast<unsigned int>(temp) < numcoords) {
        vertexFaceArray[temp].append(numfaces);
      }
      else {
        i = i+1;
        numfaces++;
        continue;
      }

      temp = vindex[i+1];
      if (temp >= 0 && static_cast<unsigned int>(temp) < numcoords) {
        vertexFaceArray[temp].append(numfaces);
      }
      else {
        i = i+2;
        numfaces++;
        continue;
      }

      temp = vindex[i+2];
      if (temp >= 0 && static_cast<unsigned int>(temp) < numcoords) {
        vertexFaceArray[temp].append(numfaces);
      }
      else {
        i = i+3;
        numfaces++;
        continue;
      }

      temp = i+3 < numvi ? vindex[i+3] : -1;
      if (temp < 0 || static_cast<unsigned int>(temp) >= numcoords) {
        i = i + 4; // Jump to next possible face
        numfaces++;
        continue;
      }

      i++;
      numfaces++;
    }
  }
  else { // !tristrip
    for (i = 0; i < numvi; i++) {
      temp = vindex[i];
      if (temp >= 0 && static_cast<unsigned int>(temp) < numcoords) {
        vertexFaceArray[temp].append(numfaces);
      }
      else {
        numfaces++;
      }
    }
  }

  float threshold = static_cast<float>(cos(SbClamp(crease_angle, 0.0f, static_cast<float>(M_PI))));
  SbBool found;
  int currindex = 0; // current normal index
  int nindex = 0;
  int j, n ;
  int facenum = 0;
  int stripcnt = 0;

  for (i = 0; i < numvi; i++) {
    currindex = vindex[i];
    if (currindex >= 0 && static_cast<unsigned int>(currindex) < numcoords) {
      if (tristrip) {
        if (++stripcnt > 3) facenum++; // next face
      }
      // calc normal for this vertex
      SbVec3f tmpvec;
      calc_normal_vec(facenorm, facenum, numfacenorm, vertexFaceArray[currindex], 
                      threshold, tmpvec);

      // Be robust when it comes to erroneously specified triangles.
      if ((tmpvec.normalize() == 0.0f) && coin_debug_extra()) {
#if COIN_DEBUG
        static uint32_t normgenerrors_vertex = 0;
        if (normgenerrors_vertex < 1) {
          SoDebugError::postWarning("SoNormalCache::generatePerVertex","Unable to "
                                    "generate valid normal for face %d", facenum);
        }
        normgenerrors_vertex++;
#endif // COIN_DEBUG
      }
      // it's really ok to have a null vector for a face/vertex, and we
      // should not set it to some dummy vector. A null vector just
      // means that the face is empty, and that the face shouldn't be
      // considered when generating vertex normals.  
      // pederb, 2005-12-21

      if (PRIVATE(this)->normalArray.getLength() <= nindex)
        PRIVATE(this)->normalArray.append(tmpvec);
      else
        PRIVATE(this)->normalArray[nindex] = tmpvec;

      // try to find equal normal (total smoothing)
      SbList <int32_t> & array = vertexNormalArray[currindex];
      found = FALSE;
      n = array.getLength();
      int same_normal = -1;
      for (j = 0; j < n && !found; j++) {
        same_normal = array[j];
        found = PRIVATE(this)->normalArray[same_normal].equals(PRIVATE(this)->normalArray[nindex],
                                                      NORMAL_EPSILON);
      }
      if (found)
        PRIVATE(this)->indices.append(same_normal);
      // might be equal to the previous normal (when all normals for a face are equal)
      else if ((nindex > 0) &&
               PRIVATE(this)->normalArray[nindex].equals(PRIVATE(this)->normalArray[nindex-1],
                                                NORMAL_EPSILON)) {
        PRIVATE(this)->indices.append(nindex-1);
      }
      else {
        PRIVATE(this)->indices.append(nindex);
        array.append(nindex);
        nindex++;
      }
    }
    else { // new face
      facenum++;
      stripcnt = 0;
      PRIVATE(this)->indices.append(-1); // add a -1 for PER_VERTEX_INDEXED binding
    }
  }
  if (PRIVATE(this)->normalArray.getLength()) {
    PRIVATE(this)->normalData.normals = PRIVATE(this)->normalArray.getArrayPtr();
    PRIVATE(this)->numNormals = PRIVATE(this)->normalArray.getLength();
  }
#if NORMALCACHE_DEBUG && COIN_DEBUG
  SoDebugError::postInfo("SoNormalCache::generatePerVertex",
                         "generated normals per vertex: %p %d %d\n",
                         PRIVATE(this)->normalData.normals, PRIVATE(this)->numNormals, PRIVATE(this)->indices.getLength());
#endif
  delete [] vertexFaceArray;
  delete [] vertexNormalArray;
}

/*!
  Generates face normals for the faceset defined by \a coords
  and \a cind. 
*/
void
SoNormalCache::generatePerFace(const SbVec3f * const coords,
                               const unsigned int numcoords,
                               const int32_t * cind,
                               const int nv,
                               const SbBool ccw)
{
#if NORMALCACHE_DEBUG && COIN_DEBUG
    SoDebugError::postInfo("SoNormalCache::generatePerFace", "generating normals");
#endif

  this->clearGenerator();
  PRIVATE(this)->indices.truncate(0);
  PRIVATE(this)->normalArray.truncate(0, TRUE);

  const int32_t * cstart = cind;
  const int32_t * endptr = cind + nv;

  SbVec3f tmpvec;

  int maxcoordidx = numcoords - 1;

  while (cind + 2 < endptr) {
    int v0 = cind[0];
    int v1 = cind[1];
    int v2 = cind[2];

    if (v0 < 0 || v1 < 0 || v2 < 0 ||
        v0 > maxcoordidx || v1 > maxcoordidx || v2 > maxcoordidx) {
#if COIN_DEBUG
      SoDebugError::postWarning("SoNormalCache::generatePerFace",
                                "Polygon with less than three valid "
                                "vertices detected. (offset: %d, [%d %d %d]). "
                                "Should be within [0, %d].",
                                cind - cstart, v0, v1, v2, maxcoordidx);
#endif // COIN_DEBUG

       // Insert dummy normal for robustness
      SbVec3f dummynormal;
      dummynormal.setValue(0.0f, 0.0f, 0.0f);
      PRIVATE(this)->normalArray.append(dummynormal);

      // Skip ahead to next possible index
      if (cind[0] < 0 || cind[0] > maxcoordidx) {
        cind += 1;
      }
      else if (cind[1] < 0 || cind[1] > maxcoordidx) {
        cind += 2;
      }
      else if (cind + 3 < endptr && (cind[2] < 0 || cind[2] > maxcoordidx)) {
        cind += 3;
      }
      else {
        cind += 3; // For robustness check after while loop
        break;
      }

      continue;
    }
    
    if (cind + 3 >= endptr || cind[3] < 0 || cind[3] > maxcoordidx) { // triangle
      if (!ccw)
        tmpvec = (coords[v0] - coords[v1]).cross(coords[v2] - coords[v1]);
      else
        tmpvec = (coords[v2] - coords[v1]).cross(coords[v0] - coords[v1]);

      // Be robust when it comes to erroneously specified triangles.
      if ((tmpvec.normalize() == 0.0f) && coin_debug_extra()) {
        static uint32_t normgenerrors_face = 0;
        if (normgenerrors_face < 1) {
          SoDebugError::postWarning("SoNormalCache::generatePerFace",
                                    "Erroneous triangle specification in model "
                                    "(indices= [%d, %d, %d], "
                                    "coords=<%f, %f, %f>, <%f, %f, %f>, <%f, %f, %f>) "
                                    "(this warning will be printed only once, "
                                    "but there might be more errors).",
                                    v0, v1, v2,
                                    coords[v0][0], coords[v0][1], coords[v0][2],
                                    coords[v1][0], coords[v1][1], coords[v1][2],
                                    coords[v2][0], coords[v2][1], coords[v2][2]);
        }
        normgenerrors_face++;
      }
      
      PRIVATE(this)->normalArray.append(tmpvec);
      cind += 4; // goto next triangle/polygon
    }
    else { // more than 3 vertices
      // use Newell's method to calculate normal vector
      const SbVec3f * vert1, * vert2;
      tmpvec.setValue(0.0f, 0.0f, 0.0f);
      vert2 = coords + v0;
      cind++; // v0 is already read

      // The cind < endptr check makes us robust with regard to a
      // missing "-1" termination of the coordIndex field of the
      // IndexedShape nodetype.
      while (cind < endptr && *cind >= 0 && *cind <= maxcoordidx) {
        vert1 = vert2;
        vert2 = coords + *cind++;
        tmpvec[0] += ((*vert1)[1] - (*vert2)[1]) * ((*vert1)[2] + (*vert2)[2]);
        tmpvec[1] += ((*vert1)[2] - (*vert2)[2]) * ((*vert1)[0] + (*vert2)[0]);
        tmpvec[2] += ((*vert1)[0] - (*vert2)[0]) * ((*vert1)[1] + (*vert2)[1]);
      }

      vert1 = vert2;  // last edge (back to v0)
      vert2 = coords + v0;
      tmpvec[0] += ((*vert1)[1] - (*vert2)[1]) * ((*vert1)[2] + (*vert2)[2]);
      tmpvec[1] += ((*vert1)[2] - (*vert2)[2]) * ((*vert1)[0] + (*vert2)[0]);
      tmpvec[2] += ((*vert1)[0] - (*vert2)[0]) * ((*vert1)[1] + (*vert2)[1]);

      // Be robust when it comes to erroneously specified polygons.
      if ((tmpvec.normalize() == 0.0f) && coin_debug_extra()) {
        static uint32_t normgenerrors_face = 0;
        if (normgenerrors_face < 1) {
          SoDebugError::postWarning("SoNormalCache::generatePerFace",
                                    "Erroneous polygon specification in model. "
                                    "Unable to generate normal; using dummy normal. "
                                    "(this warning will be printed only once, "
                                    "but there might be more errors).");
        }
        normgenerrors_face++;
      }

      PRIVATE(this)->normalArray.append(ccw ? tmpvec : -tmpvec);
      cind++; // skip the -1
    }
  }

  if (endptr - cind > 0) {
#if COIN_DEBUG
    SoDebugError::postWarning("SoNormalCache::generatePerFace", "Face "
                              "specification did not end with a valid "
                              "polygon. Too few points");
#endif // COIN_DEBUG
    SbVec3f dummynormal;
    dummynormal.setValue(0.0f, 0.0f, 0.0f);
    PRIVATE(this)->normalArray.append(dummynormal);
  }

  if (PRIVATE(this)->normalArray.getLength()) {
    PRIVATE(this)->normalData.normals = PRIVATE(this)->normalArray.getArrayPtr();
    PRIVATE(this)->numNormals = PRIVATE(this)->normalArray.getLength();
  }

#if NORMALCACHE_DEBUG && COIN_DEBUG // debug
  SoDebugError::postInfo("SoNormalCache::generatePerFace",
                         "generated normals per face: %p %d",
                         PRIVATE(this)->normalData.normals, PRIVATE(this)->numNormals);
#endif // debug
}

/*!
  Generates face normals for triangle strips.
*/
void
SoNormalCache::generatePerFaceStrip(const SbVec3f * const coords,
                                    const unsigned int numcoords,
                                    const int32_t * cind,
                                    const int nv,
                                    const SbBool ccw)
{
#if NORMALCACHE_DEBUG && COIN_DEBUG
  SoDebugError::postInfo("SoNormalCache::generatePerFaceStrip", "generating normals");
#endif

  this->clearGenerator();
  PRIVATE(this)->indices.truncate(0);
  PRIVATE(this)->normalArray.truncate(0, TRUE);

  const int32_t * cstart = cind;
  const int32_t * endptr = cind + nv;

  const SbVec3f * c0, * c1, * c2;
  SbVec3f n;

  SbBool flip = ccw;

  const int maxcoordidx = numcoords - 1;

  while (cind + 2 < endptr) {
    if (cind[0] < 0 || cind[1] < 0 || cind[2] < 0 ||
        cind[0] > maxcoordidx || cind[1] > maxcoordidx || cind[2] > maxcoordidx) {
#if COIN_DEBUG
      SoDebugError::postWarning("SoNormalCache::generatePerFaceStrip", "Erroneous "
                                "coordinate index detected (offset: %d, [%d %d %d]). Should be "
                                "within [0, %d].",
                                cind - cstart, *(cind), *(cind+1), *(cind+2), maxcoordidx);
#endif // COIN_DEBUG

      // Insert dummy normal for robustness
      SbVec3f dummynormal;
      dummynormal.setValue(0.0, 0.0, 0.0);
      PRIVATE(this)->normalArray.append(dummynormal);

      // Skip to next possibly valid index
      if (cind[0] < 0 || cind[0] > maxcoordidx) {
        cind += 1;
      }
      else if (cind[1] < 0 || cind[1] > maxcoordidx) {
        cind += 2;
      }
      else if (cind + 3 < endptr && (cind[2] < 0 || cind[2] > maxcoordidx)) {
        cind += 3;
      }
      else {
        cind += 3; // For robustness check after while loop
        break;
      }

      continue;
    }

    flip = ccw;
    c0 = &coords[*cind++];
    c1 = &coords[*cind++];
    c2 = &coords[*cind++];

    if (!flip)
      n = (*c0 - *c1).cross(*c2 - *c1);
    else
      n = (*c2 - *c1).cross(*c0 - *c1);

    static uint32_t normgenerrors_facestrip = 0;
    if ((n.normalize() == 0.0f) && coin_debug_extra()) {
      if (normgenerrors_facestrip < 1) {
        SoDebugError::postWarning("SoNormalCache::generatePerFaceStrip",
                                  "Erroneous triangle specification in model "
                                  "(coords=<%f, %f, %f>, <%f, %f, %f>, <%f, %f, %f>) "
                                  "(this warning will be printed only once, "
                                  "but there might be more errors).",
                                  c0[0][0], c0[0][1], c0[0][2],
                                  c1[0][0], c1[0][1], c1[0][2],
                                  c2[0][0], c2[0][1], c2[0][2]);



      }
      normgenerrors_facestrip++;
    }
    
    PRIVATE(this)->normalArray.append(n);

    int idx = cind < endptr ? *cind++ : -1;
    while (idx >= 0 && idx <= maxcoordidx) {
      c0 = c1;
      c1 = c2;
      c2 = &coords[idx];
      flip = !flip;
      if (!flip)
        n = (*c0 - *c1).cross(*c2 - *c1);
      else
        n = (*c2 - *c1).cross(*c0 - *c1);

      if ((n.normalize() == 0.0f) && coin_debug_extra()) {
        if (normgenerrors_facestrip < 1) {
          SoDebugError::postWarning("SoNormalCache::generatePerFaceStrip",
                                    "Erroneous triangle specification in model "
                                    "(coords=<%f, %f, %f>, <%f, %f, %f>, <%f, %f, %f>) "
                                    "(this warning will be printed only once, "
                                    "but there might be more errors).",
                                    c0[0][0], c0[0][1], c0[0][2],
                                    c1[0][0], c1[0][1], c1[0][2],
                                    c2[0][0], c2[0][1], c2[0][2]);
        }
        normgenerrors_facestrip++;
      }

      PRIVATE(this)->normalArray.append(n);
      idx = cind < endptr ? *cind++ : -1;
    }
#if COIN_DEBUG
    if (idx > maxcoordidx) {
      static uint32_t normgenerrors_facestrip = 0;
      if (normgenerrors_facestrip < 1) {
        SoDebugError::postWarning("SoNormalCache::generatePerFaceStrip",
                                  "Erroneous polygon specification in model. "
                                  "Index out of bounds: %d. Max index: %d. "
                                  "(this warning will be printed only once, "
                                  "but there might be more errors).", 
                                  idx, maxcoordidx);
      }
      normgenerrors_facestrip++;
    }
#endif // COIN_DEBUG
  }

  if (endptr - cind > 0) {
#if COIN_DEBUG
    SoDebugError::postWarning("SoNormalCache::generatePerFaceStrip", "Strip "
                              "did not end with a valid polygon. Too few "
                              "points");
#endif // COIN_DEBUG
    SbVec3f dummynormal;
    dummynormal.setValue(0.0, 0.0, 0.0);
    PRIVATE(this)->normalArray.append(dummynormal);
  }

  if (PRIVATE(this)->normalArray.getLength()) {
    PRIVATE(this)->normalData.normals = PRIVATE(this)->normalArray.getArrayPtr();
    PRIVATE(this)->numNormals = PRIVATE(this)->normalArray.getLength();
  }

#if NORMALCACHE_DEBUG && COIN_DEBUG
  SoDebugError::postInfo("SoNormalCache::generatePerFaceStrip",
                         "generated tristrip normals per face: %p %d",
                         PRIVATE(this)->normalData.normals, PRIVATE(this)->numNormals);
#endif // debug

}

/*!
  Generates one normal per triangle strips (averages all triangle normals).
*/
void
SoNormalCache::generatePerStrip(const SbVec3f * const coords,
                                const unsigned int numcoords,
                                const int32_t * cind,
                                const int nv,
                                const SbBool ccw)
{
#if NORMALCACHE_DEBUG && COIN_DEBUG
  SoDebugError::postInfo("SoNormalCache::generatePerStrip", "generating normals");
#endif

  this->clearGenerator();
  PRIVATE(this)->indices.truncate(0);
  PRIVATE(this)->normalArray.truncate(0, TRUE);

  const int32_t * cstart = cind;
  const int32_t * endptr = cind + nv;

  const SbVec3f * c0, * c1, * c2;
  SbVec3f n;

  SbBool flip = ccw;

  const int maxcoordidx = numcoords - 1;

  while (cind + 2 < endptr) {
    if (cind[0] < 0 || cind[1] < 0 || cind[2] < 0 ||
        cind[0] > maxcoordidx || cind[1] > maxcoordidx || cind[2] > maxcoordidx) {
#if COIN_DEBUG
      SoDebugError::postWarning("SoNormalCache::generatePerStrip", "Erroneous "
                                "coordinate index detected (offset: %d, [%d %d %d]). Should be "
                                "within [0, %d].",
                                cind - cstart, *(cind), *(cind+1), *(cind+2), maxcoordidx);
#endif // COIN_DEBUG
      // Insert dummy normal for robustness
      SbVec3f dummynormal;
      dummynormal.setValue(0.0f, 0.0f, 0.0f);
      PRIVATE(this)->normalArray.append(dummynormal);

      // Skip to next possibly valid index
      if (cind[0] < 0 || cind[0] > maxcoordidx) {
        cind += 1;
      }
      else if (cind[1] < 0 || cind[1] > maxcoordidx) {
        cind += 2;
      }
      else if (cind + 3 < endptr && (cind[2] < 0 || cind[2] > maxcoordidx)) {
        cind += 3;
      }
      else {
        cind += 3; // For robustness check after while loop
        break;
      }

      continue;
    }
    
    flip = ccw;
    c0 = &coords[*cind++];
    c1 = &coords[*cind++];
    c2 = &coords[*cind++];

    if (!flip)
      n = (*c0 - *c1).cross(*c2 - *c1);
    else
      n = (*c2 - *c1).cross(*c0 - *c1);

    int idx = cind < endptr ? *cind++ : -1;
    while (idx >= 0 && idx <= maxcoordidx) {
      c0 = c1;
      c1 = c2;
      c2 = &coords[idx];
      flip = !flip;
      if (!flip)
        n += (*c0 - *c1).cross(*c2 - *c1);
      else
        n += (*c2 - *c1).cross(*c0 - *c1);
      idx = cind < endptr ? *cind++ : -1;
    }

#if COIN_DEBUG
    if (idx > maxcoordidx) {
      static uint32_t normgenerrors_strip = 0;
      if (normgenerrors_strip < 1) {
        SoDebugError::postWarning("SoNormalCache::generatePerStrip",
                                  "Erroneous polygon specification in model. "
                                  "Index out of bounds: %d. Max index: %d. "
                                  "(this warning will be printed only once, "
                                  "but there might be more errors).", 
                                  idx, maxcoordidx);
      }
      normgenerrors_strip++;
    }
#endif // COIN_DEBUG

    if ((n.normalize() == 0.0f) && coin_debug_extra()) {
      static uint32_t normgenerrors_strip = 0;
      if (normgenerrors_strip < 1) {
        SoDebugError::postWarning("SoNormalCache::generatePerStrip",
                                  "Erroneous polygon specification in model.  "
                                  "Unable to generate non-zero normal. Using "
                                  "dummy normal. "
                                  "(this warning will be printed only once, "
                                  "but there might be more errors).");
      }
      normgenerrors_strip++;
    }
    
    PRIVATE(this)->normalArray.append(n);
  }

  if (endptr - cind > 0) {
#if COIN_DEBUG
    SoDebugError::postWarning("SoNormalCache::generatePerStrip", "Strip did "
                              "not end with a valid polygon. Too few points");
#endif // COIN_DEBUG
    SbVec3f dummynormal;
    dummynormal.setValue(0.0, 0.0, 0.0);
    PRIVATE(this)->normalArray.append(dummynormal);
  }

  if (PRIVATE(this)->normalArray.getLength()) {
    PRIVATE(this)->normalData.normals = PRIVATE(this)->normalArray.getArrayPtr();
    PRIVATE(this)->numNormals = PRIVATE(this)->normalArray.getLength();
  }

#if NORMALCACHE_DEBUG && COIN_DEBUG
  SoDebugError::postInfo("SoNormalCache::generatePerStrip",
                         "generated normals per strip: %p %d\n",
                         PRIVATE(this)->normalData.normals, PRIVATE(this)->numNormals);
#endif

}

/*!
  Generates PER_VERTEX normals for quad data.
*/
void
SoNormalCache::generatePerVertexQuad(const SbVec3f * const coords,
                                     const unsigned int numcoords,
                                     const int vPerRow,
                                     const int vPerColumn,
                                     const SbBool ccw)
{
#if NORMALCACHE_DEBUG && COIN_DEBUG
  SoDebugError::postInfo("SoNormalCache::generatePerVertexQuad", "generating normals");
#endif

  this->clearGenerator();
  PRIVATE(this)->normalArray.truncate(0, TRUE);
  // avoid reallocations in growable array by setting the buffer size first
  PRIVATE(this)->normalArray.ensureCapacity(vPerRow * vPerColumn);

  SoNormalCache tempcache(NULL);
  tempcache.generatePerFaceQuad(coords, numcoords, vPerRow, vPerColumn, ccw);
  const SbVec3f * facenormals = tempcache.getNormals();
  int numfacenormals = tempcache.getNum(); // Used for extra robustness

#define IDX(r, c) ((r)*(vPerRow-1)+(c))

  for (int i = 0; i < vPerColumn; i++) {
    for (int j = 0; j < vPerRow; j++) {
      const int idx1 = IDX(i, j);
      const int idx2 = IDX(i-1, j);
      const int idx3 = IDX(i-1, j-1);
      const int idx4 = IDX(i, j-1);

      SbVec3f n(0, 0, 0);

      if (i < vPerColumn-1 && j < vPerRow-1 && idx1 < numfacenormals) n += facenormals[idx1];
      if (i > 0 && j < vPerRow-1 && idx2 < numfacenormals) n += facenormals[idx2];
      if (j > 0 && i > 0 && idx3 < numfacenormals) n += facenormals[idx3];
      if (j > 0 && i < vPerColumn-1 && idx4 < numfacenormals) n += facenormals[idx4];

      if ((n.normalize() == 0.0f) && coin_debug_extra()) {
        static uint32_t normgenerrors_vertexquad = 0;
        if (normgenerrors_vertexquad < 1) {
          SoDebugError::postWarning("SoNormalCache::generatePerVertexQuad",
                                    "Erroneous polygon specification in model. "
                                    "Unable to generate valid normal, adding dummy. "
                                    "(this warning will be printed only once, "
                                    "but there might be more errors).");
        }
        normgenerrors_vertexquad++;
      }        
      PRIVATE(this)->normalArray.append(ccw ? -n : n);
    }
  }

#undef IDX

  PRIVATE(this)->normalData.normals = PRIVATE(this)->normalArray.getArrayPtr();
  PRIVATE(this)->numNormals = PRIVATE(this)->normalArray.getLength();

#if NORMALCACHE_DEBUG && COIN_DEBUG
  SoDebugError::postInfo("SoNormalCache::generatePerVertexQuad",
                         "generated normals per vertex quad: %p %d\n",
                         PRIVATE(this)->normalData.normals, PRIVATE(this)->numNormals);
#endif
}

/*!
  Generates per face normals for quad data.
*/
void
SoNormalCache::generatePerFaceQuad(const SbVec3f * const coords,
                                   const unsigned int numcoords,
                                   const int vPerRow,
                                   const int vPerColumn,
                                   const SbBool ccw)
{
#if NORMALCACHE_DEBUG && COIN_DEBUG
  SoDebugError::postInfo("SoNormalCache::generatePerFaceQuad", "generating normals");
#endif

  this->clearGenerator();
  PRIVATE(this)->normalArray.truncate(0, TRUE);
  // avoid reallocations in growable array by setting the buffer size first
  PRIVATE(this)->normalArray.ensureCapacity((vPerRow-1)*(vPerColumn-1));
  
#if COIN_DEBUG
  if (vPerRow <= 1 || vPerColumn <= 1 || 
      static_cast<unsigned int>(vPerRow * vPerColumn) > numcoords) {

    SoDebugError::postWarning("SoNormalCache::generatePerFaceQuad", "Illegal "
                              "facequad dimension: [%d %d] with %d coordinates "
                              "available. verticesPerRow and verticesPerColumn "
                              "should be > 1, and verticesPerRow * verticesPerColumn "
                              "<= number of coordinates available.", 
                              vPerRow, vPerColumn, numcoords);
  }
#endif // COIN_DEBUG

#define IDX(r, c) ((r)*(vPerRow)+(c))

  for (int i = 0; i < vPerColumn-1; i++) {
    for (int j = 0; j < vPerRow-1; j++) {
      const unsigned int idx1 = IDX(i, j);
      const unsigned int idx2 = IDX(i+1, j);
      const unsigned int idx3 = IDX(i, j+1);

      if (idx2 < numcoords) { // Check the largest index only
        SbVec3f n = (coords[idx2] - coords[idx1]).cross(coords[idx3] - coords[idx1]);

        // Be robust when it comes to erroneously specified polygons.
        if ((n.normalize() == 0.0f) && coin_debug_extra())  {
          static uint32_t normgenerrors_facequad = 0;
          if (normgenerrors_facequad < 1) {
            SoDebugError::postWarning("SoNormalCache::generatePerFaceQuad",
                                      "Erroneous triangle specification in model "
                                      "(indices= [%d, %d, %d], "
                                      "coords=<%f, %f, %f>, <%f, %f, %f>, <%f, %f, %f>) "
                                      "(this warning will be printed only once, "
                                      "but there might be more errors).",
                                      idx1, idx2, idx3,
                                      coords[idx1][0], coords[idx1][1], coords[idx1][2],
                                      coords[idx2][0], coords[idx2][1], coords[idx2][2],
                                      coords[idx3][0], coords[idx3][1], coords[idx3][2]);
          }
          normgenerrors_facequad++;
        }
        
        PRIVATE(this)->normalArray.append(ccw ? -n : n);
      }
      else {
        // Generate normals even for invalid input
        SbVec3f dummynormal(0.0, 0.0, 0.0);
        PRIVATE(this)->normalArray.append(ccw ? -dummynormal : dummynormal);
      }
    }
  }

#undef IDX

  if (PRIVATE(this)->normalArray.getLength()) {
    PRIVATE(this)->normalData.normals = PRIVATE(this)->normalArray.getArrayPtr();
    PRIVATE(this)->numNormals = PRIVATE(this)->normalArray.getLength();
  }

#if NORMALCACHE_DEBUG && COIN_DEBUG
  SoDebugError::postInfo("SoNormalCache::generatePerFaceQuad",
                         "generated normals per face quad: %p %d\n",
                         PRIVATE(this)->normalData.normals, PRIVATE(this)->numNormals);
#endif

}

/*!
  Generates per row normals for quad data.
*/
void
SoNormalCache::generatePerRowQuad(const SbVec3f * const coords,
                                  const unsigned int numcoords,
                                  const int vPerRow,
                                  const int vPerColumn,
                                  const SbBool ccw)
{
#if NORMALCACHE_DEBUG && COIN_DEBUG
  SoDebugError::postInfo("SoNormalCache::generatePerRowQuad", "generating normals");
#endif

  this->clearGenerator();
  PRIVATE(this)->normalArray.truncate(0, TRUE);
  SbVec3f n;

#if COIN_DEBUG
  if (vPerRow <= 1 || vPerColumn <= 1 || 
      static_cast<unsigned int>(vPerRow * vPerColumn) > numcoords) {
    SoDebugError::postWarning("SoNormalCache::generatePerRowQuad", "Illegal "
                              "facequad dimension: [%d %d] with %d coordinates "
                              "available. verticesPerRow and verticesPerColumn "
                              "should be > 1, and verticesPerRow * verticesPerColumn "
                              "<= number of coordinates available.", 
                              vPerRow, vPerColumn, numcoords);
  }
#endif // COIN_DEBUG

#define IDX(r, c) ((r)*(vPerRow)+(c))

  for (int i = 0; i < vPerColumn-1; i++) {
    n.setValue(0.0f, 0.0f, 0.0f);
    for (int j = 0; j < vPerRow-1; j++) {
      const unsigned int idx1 = IDX(i, j);
      const unsigned int idx2 = IDX(i+1, j);
      const unsigned int idx3 = IDX(i, j+1);

      if (idx2 < numcoords) { // Check largest index only
        n += (coords[idx2] - coords[idx1]).cross(coords[idx3] - coords[idx1]);
      }
    }

    // Be robust when it comes to erroneously specified polygons.
    if ((n.normalize() == 0.0f) && coin_debug_extra()) {
      static uint32_t normgenerrors_rowquad = 0;
      if (normgenerrors_rowquad < 1) {
        SoDebugError::postWarning("SoNormalCache::generatePerRowQuad",
                                  "Erroneous polygon specification in model. "
                                  "Unable to generate valid normal, adding null vector. "
                                  "(this warning will be printed only once, "
                                  "but there might be more errors).");
      }
      normgenerrors_rowquad++;
    }    
    PRIVATE(this)->normalArray.append(ccw ? -n : n);
  }
  
#undef IDX

  if (PRIVATE(this)->normalArray.getLength()) {
    PRIVATE(this)->normalData.normals = PRIVATE(this)->normalArray.getArrayPtr();
    PRIVATE(this)->numNormals = PRIVATE(this)->normalArray.getLength();
  }

#if NORMALCACHE_DEBUG && COIN_DEBUG
  SoDebugError::postInfo("SoNormalCache::generatePerRowQuad",
                         "generated normals per row quad: %p %d\n",
                         PRIVATE(this)->normalData.normals, PRIVATE(this)->numNormals);
#endif

}

//
// frees generator and resets normal data.
//
void
SoNormalCache::clearGenerator(void)
{
  if (PRIVATE(this)->numNormals == 0 && PRIVATE(this)->normalData.generator) {
    delete PRIVATE(this)->normalData.generator;
  }
  PRIVATE(this)->normalData.normals = NULL;
  PRIVATE(this)->numNormals = 0;
}

#undef NORMAL_EPSILON
#undef NORMALCACHE_DEBUG
#undef PRIVATE

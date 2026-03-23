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
  \class SoNormalGenerator SoNormalGenerator.h include/Inventor/misc/SoNormalGenerator.h
  \brief The SoNormalGenerator class is used to generate normals.

  \ingroup coin_general

  FIXME: document properly
*/

#include <Inventor/misc/SoNormalGenerator.h>

#include <cstdio>

#include <Inventor/errors/SoDebugError.h>

#include "tidbitsp.h"
#include "coindefs.h" // COIN_OBSOLETED()

/*!
  Constructor with \a isccw indicating if polygons are specified
  in counterclockwise order. The \a approxVertices can be used
  to optimize normal generation.
*/
SoNormalGenerator::SoNormalGenerator(const SbBool isccw,
                                     const int approxVertices)
  : bsp(128, approxVertices),
    vertexList(approxVertices),
    vertexFace(approxVertices),
    faceNormals(approxVertices / 4),
    vertexNormals(approxVertices),
    ccw(isccw),
    perVertex(TRUE)
{
}

/*!
  Destructor.
*/
SoNormalGenerator::~SoNormalGenerator()
{
}

/*!
  Resets the normal generator, making it possible to reuse it without
  allocating a new one.

  \COIN_FUNCTION_EXTENSION

  \since Coin 2.0
*/
void
SoNormalGenerator::reset(const SbBool ccwarg)
{
  this->ccw = ccwarg;
  this->bsp.clear();
  this->vertexList.truncate(0);
  this->vertexFace.truncate(0);
  this->faceNormals.truncate(0);
  this->vertexNormals.truncate(0);
}

/*!
  Signals the start of a new polygon.

  \sa SoNormalGenerator::polygonVertex()
  \sa SoNormalGenerator::endPolygon()
*/
void
SoNormalGenerator::beginPolygon(void)
{
  this->currFaceStart = this->vertexList.getLength();
}

/*!
  Adds a vertex to the current polygon.
  \sa SoNormalGenerator::beginPolygon()
  \sa SoNormalGenerator::endPolygon()
*/
void
SoNormalGenerator::polygonVertex(const SbVec3f &v)
{
  this->vertexList.append(this->bsp.addPoint(v));
  this->vertexFace.append(this->faceNormals.getLength());
}

/*!
  Signals the end of a polygon.
  \sa SoNormalGenerator::beginPolygon()
  \sa SoNormalGenerator::polygonVertex()
*/
void
SoNormalGenerator::endPolygon(void)
{
  SbVec3f n = this->calcFaceNormal();
  this->faceNormals.append(n);
}

/*!
  Convenience method for adding a triangle.
*/
void
SoNormalGenerator::triangle(const SbVec3f &v0,
                            const SbVec3f &v1,
                            const SbVec3f &v2)
{
  this->beginPolygon();
  this->polygonVertex(v0);
  this->polygonVertex(v1);
  this->polygonVertex(v2);
  this->endPolygon();
}

/*!
  Convenience method for adding a quad
*/
void
SoNormalGenerator::quad(const SbVec3f &v0,
                        const SbVec3f &v1,
                        const SbVec3f &v2,
                        const SbVec3f &v3)
{
  this->beginPolygon();
  this->polygonVertex(v0);
  this->polygonVertex(v1);
  this->polygonVertex(v2);
  this->polygonVertex(v3);
  this->endPolygon();
}

//
// calculates the normal vector for a vertex, based on the
// normal vectors of all incident faces
//
static void
calc_normal_vec(const SbVec3f *facenormals, const int facenum,
                SbList <int32_t> &faceArray, const float threshold,
                SbVec3f &vertnormal)
{
  // start with face normal vector
  const SbVec3f * facenormal = &facenormals[facenum];
  vertnormal = *facenormal;

  int n = faceArray.getLength();
  int currface;

  for (int i = 0; i < n; i++) {
    currface = faceArray[i];
    if (currface != facenum) { // check all but this face
      const SbVec3f &normal = facenormals[currface];
      if ((normal.dot(*facenormal)) > threshold) {
        // smooth towards this face
        vertnormal += normal;
      }
    }
  }
}

/*!
  Triggers the normal generation. Normals are generated using
  \a creaseAngle to find which edges should be flat-shaded
  and which should be smooth-shaded.

  If normals are generated for triangle strips, the \a striplens and
  \a numstrips must be supplied. See src/nodes/SoTriangleStripSet.cpp
  (generateDefaultNormals()) for an example on how you send triangle
  strip information to this generator. It's not trivial, since you
  have to know how OpenGL/Coin generate triangles from triangle
  strips.

*/
void
SoNormalGenerator::generate(const float creaseAngle,
                            const int32_t *striplens,
                            const int numstrips)
{
  // just ignore the warnings from normalize(). A null vector just
  // means that we have an empty triangle which will be ignored by
  // OpenGL anyway. It's also common to have empty triangles in for
  // instance triangle strips (they're used as a trick to generate
  // longer triangle strips).

  int i;

  // for each vertex, store all faceindices the vertex is a part of
  SbList <int32_t> * vertexFaceArray = new SbList<int32_t>[bsp.numPoints()];

  int numvi = this->vertexList.getLength();

  for (i = 0; i < numvi; i++) {
    vertexFaceArray[vertexList[i]].append(this->vertexFace[i]);
  }

  float threshold = (float)cos(SbClamp(creaseAngle, 0.0f, (float) M_PI));

  if (striplens) {
    i = 0;
    for (int j = 0; j < numstrips; j++) {
      assert(i+2 < numvi);
      SbVec3f tmpvec;
      calc_normal_vec(this->faceNormals.getArrayPtr(),
                      this->vertexFace[i],
                      vertexFaceArray[vertexList[i]],
                      threshold, tmpvec);
      (void) tmpvec.normalize();
      this->vertexNormals.append(tmpvec);
      calc_normal_vec(this->faceNormals.getArrayPtr(),
                      this->vertexFace[i+1],
                      vertexFaceArray[vertexList[i+1]],
                      threshold, tmpvec);
      (void) tmpvec.normalize();
      this->vertexNormals.append(tmpvec);

      int num = striplens[j] - 2;

      while (num--) {
        i += 2;
        assert(i < numvi);
        calc_normal_vec(this->faceNormals.getArrayPtr(),
                        this->vertexFace[i],
                        vertexFaceArray[vertexList[i]],
                        threshold, tmpvec);
        (void) tmpvec.normalize();
        this->vertexNormals.append(tmpvec);
        i++;
      }
    }
  }
  else {
    for (i = 0; i < numvi; i++) {
      SbVec3f tmpvec;
      calc_normal_vec(this->faceNormals.getArrayPtr(),
                      this->vertexFace[i],
                      vertexFaceArray[vertexList[i]],
                      threshold, tmpvec);
      (void) tmpvec.normalize();
      this->vertexNormals.append(tmpvec);
    }
  }
  delete [] vertexFaceArray;
  this->vertexFace.truncate(0, TRUE);
  this->vertexList.truncate(0, TRUE);
  this->faceNormals.truncate(0, TRUE);
  this->bsp.clear();
  this->vertexNormals.fit();

  // return vertex normals
  this->perVertex = TRUE;
}

/*!
  Generates one normal per strip by averaging face normals.
*/
void
SoNormalGenerator::generatePerStrip(const int32_t * striplens,
                                    const int numstrips)
{
  int cnt = 0;
  for (int i = 0; i < numstrips; i++) {
    int n = striplens[i] - 2;
    SbVec3f acc(0.0f, 0.0f, 0.0f);
    while (n > 0) {
      acc += this->faceNormals[cnt++];
      n--;
    }
    (void) acc.normalize();
    // use face normal array to store strip normals
    this->faceNormals[i] = acc;
  }
  // strip normals can now be found in faceNormals array
  this->faceNormals.truncate(numstrips, TRUE);
  this->perVertex = FALSE;
}

/*!
  Generates the normals per face. Use this when PER_FACE normal
  binding is needed. This method is not part of the OIV API.
*/
void
SoNormalGenerator::generatePerFace(void)
{
  // face normals have already been generated. Just set flag.
  this->perVertex = FALSE;
  this->faceNormals.fit();
}

/*!
  Generates one overall normal by averaging all face
  normals. Use when normal binding is OVERALL. This method
  is not part of the OIV API.
*/
void
SoNormalGenerator::generateOverall(void)
{
  const int n = this->faceNormals.getLength();
  const SbVec3f * normals = this->faceNormals.getArrayPtr();
  SbVec3f acc(0.0f, 0.0f, 0.0f);
  for (int i = 0; i < n; i++) acc += normals[i];
  (void) acc.normalize();
  this->faceNormals.truncate(0, TRUE);
  this->faceNormals.append(acc);

  // normals are not per vertex
  this->perVertex = FALSE;
}

/*!
  Returns the number of normals generated.
*/
int
SoNormalGenerator::getNumNormals(void) const
{
  if (!this->perVertex) {
    return this->faceNormals.getLength();
  }
  return this->vertexNormals.getLength();
}

/*!
  Sets the number of generated normals. This method is not supported
  in Coin, and is provided for API compatibility only.
*/
void
SoNormalGenerator::setNumNormals(const int /* num */)
{
  COIN_OBSOLETED();
}

/*!
  Returns a pointer to the generated normals.
*/
const SbVec3f *
SoNormalGenerator::getNormals(void) const
{
  if (!this->perVertex) {
    if (this->faceNormals.getLength()) return this->faceNormals.getArrayPtr();
    return NULL;
  }
  if (this->vertexNormals.getLength())
    return this->vertexNormals.getArrayPtr();
  return NULL;
}

/*!
  Returns the normal at index \a i.
  \sa SoNormalGenerator::getNumNormals()
*/
const SbVec3f &
SoNormalGenerator::getNormal(const int32_t i) const
{
  assert(i >= 0 && i < this->getNumNormals());
  return this->getNormals()[i];
}

/*!
  Sets the normal at index \a index to \a normal. This method
  is not supported in Coin, and is provided for API compatibility
  only.
*/
void
SoNormalGenerator::setNormal(const int32_t /* index */,
                             const SbVec3f & /* normal */)
{
  COIN_OBSOLETED();
}

//
// Calculates the face normal to the current face.
//
SbVec3f
SoNormalGenerator::calcFaceNormal(void)
{
  const int num = this->vertexList.getLength() - this->currFaceStart;

  assert(num >= 3);
  const int * cind = (const int *) this->vertexList.getArrayPtr() + this->currFaceStart;
  const SbVec3f * coords = this->bsp.getPointsArrayPtr();
  SbVec3f ret;

  if (num == 3) { // triangle
    const SbVec3f v0 = coords[cind[0]] - coords[cind[1]];
    const SbVec3f v1 = coords[cind[2]] - coords[cind[1]];
    if (!this->ccw) { ret = v0.cross(v1); }
    else { ret = v1.cross(v0); }
  }
  else {
    // For non-triangle faces
    const SbVec3f *vert1, *vert2;
    ret.setValue(0.0f, 0.0f, 0.0f);
    vert2 = coords + cind[num-1];
    for (int i = 0; i < num; i++) {
      vert1 = vert2;
      vert2 = coords + cind[i];
      ret[0] += ((*vert1)[1] - (*vert2)[1]) * ((*vert1)[2] + (*vert2)[2]);
      ret[1] += ((*vert1)[2] - (*vert2)[2]) * ((*vert1)[0] + (*vert2)[0]);
      ret[2] += ((*vert1)[0] - (*vert2)[0]) * ((*vert1)[1] + (*vert2)[1]);
    }
    if (!this->ccw) ret = -ret;
  }

  if (ret.normalize() == 0.0f) {
#if COIN_DEBUG
    // make this an optional warning since it's really ok (in most
    // cases) to have empty triangles. pederb, 2005-12-21
    if (coin_debug_extra()) {
      SbString s;
      for (int i = 0; i < num; i++) {
        const SbVec3f v = coords[cind[i]];
        SbString c;
        c.sprintf(" <%f, %f, %f>", v[0], v[1], v[2]);
        s += c;
      }
      SoDebugError::postWarning("SoNormalGenerator::calcFaceNormal",
                                "Normal vector found to be of zero length "
                                "for face with vertex coordinates:%s",
                                s.getString());
    }
#endif // COIN_DEBUG      
    // set to (0,0,0) so that this face will not influence normal smoothing
    ret.setValue(0.0f, 0.0f, 0.0f);
  }
  return ret;
}

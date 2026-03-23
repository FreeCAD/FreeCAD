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
  \class SbTesselator SbTesselator.h Inventor/SbTesselator.h
  \brief The SbTesselator class is used to tessellate polygons into triangles.

  \ingroup coin_base

  SbTesselator is used within Coin to split polygons into
  triangles. It handles concave polygons, does Delaunay triangulation
  and avoids generating self-intersecting triangles.

  Here's a simple example which shows how to tessellate a quad polygon
  with corners in <0, 0, 0>, <1, 0, 0>, <1, 1, 0> and <0, 1, 0>.

  \code

  // Callback function for the tessellator. Called once for each
  // generated triangle with the vertices.
  static void
  tess_cb(void * v0, void * v1, void * v2, void * cbdata)
  {
    SbVec3f * vtx0 = (SbVec3f *)v0;
    SbVec3f * vtx1 = (SbVec3f *)v1;
    SbVec3f * vtx2 = (SbVec3f *)v2;
    (void) fprintf(stdout, "triangle: <%f, %f, %f> <%f, %f, %f> <%f, %f, %f>\n",
      (*vtx0)[0], (*vtx0)[1], (*vtx0)[2],
      (*vtx1)[0], (*vtx1)[1], (*vtx1)[2],
      (*vtx2)[0], (*vtx2)[1], (*vtx2)[2]);

    // Do stuff with triangle here.
  }

  static SbVec3f vertices[] = {
    SbVec3f(1, 0, 0), SbVec3f(1, 1, 0),
    SbVec3f(0, 1, 0), SbVec3f(0, 0, 0)
  };

  SbTesselator mytessellator(tess_cb, NULL);
  mytessellator.beginPolygon();
  for (int i=0; i < 4; i++) {
    mytessellator.addVertex(vertices[i], &vertices[i]);
  }
  mytessellator.endPolygon();

  \endcode

  The call to SbTesselator::endPolygon() triggers the SbTesselator to
  spring into action, calling the tess_cb() function for each triangle
  it generates.

  The reason we use 2 arguments to SbTesselator::addVertex() and passes
  void pointers for the vertices to the callback function is to make it
  possible to have more complex structures than just the coordinates
  themselves (as in the example above), like material information,
  lighting information or whatever other attributes your vertices have.

  This class is not part of the original Open Inventor API.


  Another option for tessellating polygons is the tessellator of the
  GLU library. It has some features not part of SbTesselator (like
  handling hulls), but the GLU library is known to have bugs in
  various implementations and doesn't do Delaunay triangulation. If
  you however still prefer to use the GLU tessellator instead of this
  one, that can be forced by setting an environment variable:

  \code
  (void) coin_setenv("COIN_PREFER_GLU_TESSELLATOR", "1", 1);
  \endcode
*/

// *************************************************************************

#include <Inventor/SbTesselator.h>

#include <cstdio>
#include <climits>
#include <cassert>
#include <cfloat>

#include <Inventor/C/base/heap.h>
#include <Inventor/SbBSPTree.h>
#include <Inventor/SbSphere.h>
#include <Inventor/lists/SbList.h>
#include <Inventor/errors/SoDebugError.h>

// *************************************************************************

/*!
  \typedef void SbTesselatorCB(void * v0, void * v1, void * v2, void * data)

  The type definition of the callback function which is called for each triangle
  returned by the tessellator.
*/

// *************************************************************************

class SbTesselator::PImpl {
public:
  struct Vertex {
    SbVec3f v;
    SbTesselator::PImpl * thisp;

    void * data;
    Vertex * prev, * next;

    double weight;
    int dirtyweight;
  };

  PImpl(void) : bsptree(256) { }
  cc_heap * heap;
  SbBSPTree bsptree;
  SbList <int> clippablelist;
  double epsilon;
  SbBox3f bbox;

  Vertex * newVertex(void);
  void cleanUp(void);

  int currVertex;
  SbList <struct Vertex *> vertexStorage;

  Vertex * headV, * tailV;
  int numVerts;
  SbVec3f polyNormal;
  int X, Y;
  int polyDir;
  void (*callback)(void * v0, void * v1, void * v2, void * data);
  void * callbackData;
  SbBool hasNormal;
  SbBool keepVertices;

  void emitTriangle(Vertex * v);
  void cutTriangle(Vertex * t);
  void calcPolygonNormal(void);

  double circleSize(Vertex * v);
  SbBool clippable(Vertex * v);
  SbBool pointInTriangle(Vertex * p, Vertex * t);
  SbBool pointInTriangle(Vertex * p, Vertex * v0, Vertex * v1, Vertex * v2);
  SbBool pointOnEdge(Vertex * p, Vertex * e0, Vertex * e1);
  double area(Vertex * t);

  static double heap_evaluate(void * v);
  static int heap_compare(void * v0, void * v1);
};

#define PRIVATE(obj) ((obj)->pimpl)

// *************************************************************************
// strict weak ordering is needed
int
SbTesselator::PImpl::heap_compare(void * h0, void * h1)
{
  return heap_evaluate(h0) < heap_evaluate(h1) ? 1 : 0;
}

double
SbTesselator::PImpl::heap_evaluate(void * v)
{
  Vertex * vertex = static_cast<Vertex *>(v);
  if (vertex->dirtyweight) {
    vertex->dirtyweight = 0;
    double area = vertex->thisp->area(vertex);
    bool istriangle = area * vertex->thisp->polyDir > 0.0f;
    bool isclippable = istriangle ? vertex->thisp->clippable(vertex) : false;
    if ((fabs(area) > vertex->thisp->epsilon) &&
        istriangle &&
        isclippable) {
#if 0 // testing code to avoid empty triangles
      vertex->weight = vertex->thisp->circleSize(vertex);
      Vertex *v2 = vertex->next;
      if (vertex->weight != FLT_MAX &&
          v2->thisp->keepVertices &&
          v2->thisp->numVerts > 4 &&
          fabs(v2->thisp->area(v2)) < v2->thisp->epsilon) {
        vertex->weight = 0.0f; // cut immediately!
      }

#else
      vertex->weight = vertex->thisp->circleSize(vertex);
#endif
    }
    else
      vertex->weight = FLT_MAX;
  }
  return vertex->weight;
}

// *************************************************************************

/*!
  Initializes a tessellator. The \a callback argument specifies a
  function which will be called for each triangle returned by the
  tessellator. The callback function will get three pointers to each
  vertex and the \a userdata pointer. The vertex pointers are
  specified in the SbTesselator::addVertex() method.
*/
SbTesselator::SbTesselator(SbTesselatorCB * func, void * data)
{
  this->setCallback(func, data);
  PRIVATE(this)->headV = PRIVATE(this)->tailV = NULL;
  PRIVATE(this)->currVertex = 0;

  PRIVATE(this)->heap =
    cc_heap_construct(256, reinterpret_cast<cc_heap_compare_cb *>(PImpl::heap_compare), TRUE);
  PRIVATE(this)->epsilon = FLT_EPSILON;
}

/*!
  Destructor.
*/
SbTesselator::~SbTesselator()
{
  PRIVATE(this)->cleanUp();
  int i, n = PRIVATE(this)->vertexStorage.getLength();
  for (i = 0; i < n; i++) { delete PRIVATE(this)->vertexStorage[i]; }

  cc_heap_destruct(PRIVATE(this)->heap);
}

// *************************************************************************

/*!
  Initializes new polygon.

  You can explicitly set the polygon normal if you know what it
  is. Otherwise it will be calculated internally.

  If \a keepVerts is \c TRUE, all vertices will be included in the
  returned triangles, even though this might lead to triangles without
  area.
*/
void
SbTesselator::beginPolygon(SbBool keepVerts, const SbVec3f & normal)
{
  PRIVATE(this)->cleanUp();
  PRIVATE(this)->keepVertices = keepVerts;
  if (normal != SbVec3f(0.0f, 0.0f, 0.0f)) {
    PRIVATE(this)->polyNormal = normal;
    PRIVATE(this)->hasNormal = TRUE;
  }
  else {
    PRIVATE(this)->hasNormal = FALSE;
  }
  PRIVATE(this)->headV = PRIVATE(this)->tailV = NULL;
  PRIVATE(this)->numVerts = 0;
  PRIVATE(this)->bbox.makeEmpty();
}

/*!
  Adds a new vertex to the polygon. \a data will be returned as a
  vertex in the callback-function.
*/
void
SbTesselator::addVertex(const SbVec3f & v, void * data)
{
  if (PRIVATE(this)->tailV &&
      !PRIVATE(this)->keepVertices &&
      v == PRIVATE(this)->tailV->v)
    return;

  PRIVATE(this)->bbox.extendBy(v);

  PImpl::Vertex *newv = PRIVATE(this)->newVertex();
  newv->v = v;
  newv->data = data;
  newv->next = NULL;
  newv->dirtyweight = 1;
  newv->weight = FLT_MAX;
  newv->prev = PRIVATE(this)->tailV;
  newv->thisp = &(PRIVATE(this).get());
  if (!PRIVATE(this)->headV) PRIVATE(this)->headV = newv;
  if (PRIVATE(this)->tailV) PRIVATE(this)->tailV->next = newv;
  PRIVATE(this)->tailV = newv;
  PRIVATE(this)->numVerts++;
}

/*!
  Signals the tessellator to begin tessellating. The callback function
  specified in the constructor (or set using the
  SbTesselator::setCallback() method) will be called for each triangle
  before returning.
*/
void
SbTesselator::endPolygon(void)
{
  // projection enums
  enum { OXY, OXZ, OYZ };

  // check for special case when last point equals the first point
  if (!PRIVATE(this)->keepVertices && PRIVATE(this)->numVerts >= 3) {
    PImpl::Vertex * first = PRIVATE(this)->headV;
    PImpl::Vertex * last = PRIVATE(this)->tailV;
    if (first->v == last->v) {
      PImpl::Vertex * newlast = last->prev;
      newlast->next = NULL;
      // don't delete old tail. We have some special memory handling
      // in this class
      PRIVATE(this)->tailV = newlast;
      PRIVATE(this)->numVerts--;
    }
  }

  if (PRIVATE(this)->numVerts > 3) {
    PRIVATE(this)->calcPolygonNormal();

    // Find best projection plane
    int projection;
    if (fabs(PRIVATE(this)->polyNormal[0]) > fabs(PRIVATE(this)->polyNormal[1]))
      if (fabs(PRIVATE(this)->polyNormal[0]) > fabs(PRIVATE(this)->polyNormal[2]))
        projection = OYZ;
      else
        projection = OXY;
    else
      if (fabs(PRIVATE(this)->polyNormal[1]) > fabs(PRIVATE(this)->polyNormal[2]))
        projection = OXZ;
      else
        projection = OXY;

    switch (projection) {
    case OYZ:
      PRIVATE(this)->X = 1;
      PRIVATE(this)->Y = 2;
      PRIVATE(this)->polyDir = PRIVATE(this)->polyNormal[0] > 0 ? 1 : -1;
      break;
    case OXY:
      PRIVATE(this)->X = 0;
      PRIVATE(this)->Y = 1;
      PRIVATE(this)->polyDir = PRIVATE(this)->polyNormal[2] > 0 ? 1 : -1;
      break;
    case OXZ:
      PRIVATE(this)->X = 2;
      PRIVATE(this)->Y = 0;
      PRIVATE(this)->polyDir = PRIVATE(this)->polyNormal[1] > 0 ? 1 : -1;
      break;
    }

    // find epsilon based on bbox
    SbVec3f d;
    PRIVATE(this)->bbox.getSize(d[0], d[1], d[2]);
    PRIVATE(this)->epsilon = SbMin(d[PRIVATE(this)->X], d[PRIVATE(this)->Y]) * FLT_EPSILON * FLT_EPSILON;

    //Make loop
    PRIVATE(this)->tailV->next = PRIVATE(this)->headV;
    PRIVATE(this)->headV->prev = PRIVATE(this)->tailV;

    // add all vertices to heap.
    cc_heap_clear(PRIVATE(this)->heap);
    PRIVATE(this)->bsptree.clear(256);

    // use two loops to add points to bsptree and heap, since the heap
    // requires that the bsptree is fully set up to evaluate
    // correctly.
    PImpl::Vertex* v = PRIVATE(this)->headV;
    do {
      PRIVATE(this)->bsptree.addPoint(SbVec3f(v->v[PRIVATE(this)->X],
                                              v->v[PRIVATE(this)->Y],
                                              0.0f), v);
      v = v->next;
    } while (v != PRIVATE(this)->headV);

    do {
      cc_heap_add(PRIVATE(this)->heap, v);
      v = v->next;
    } while (v != PRIVATE(this)->headV);

    while (PRIVATE(this)->numVerts > 4) {
      v = static_cast<PImpl::Vertex *>(cc_heap_get_top(PRIVATE(this)->heap));

      if (PImpl::heap_evaluate(v) == FLT_MAX)
        break;

      cc_heap_remove(PRIVATE(this)->heap, v->next);
      PRIVATE(this)->bsptree.removePoint(SbVec3f(v->next->v[PRIVATE(this)->X],
                                                 v->next->v[PRIVATE(this)->Y],
                                                 0.0f));
      PRIVATE(this)->emitTriangle(v); // will remove v->next
      PRIVATE(this)->numVerts--;

      v->prev->dirtyweight = 1;
      v->dirtyweight = 1;
      cc_heap_update(PRIVATE(this)->heap, v->prev);
      v->prev->dirtyweight = 1;
      v->dirtyweight = 1;
      cc_heap_update(PRIVATE(this)->heap, v);
    }

    // remember that headV and tailV are not valid anymore!

    //
    // must handle special case when only four vertices remain
    //
    if (PRIVATE(this)->numVerts == 4) {
      v->next->dirtyweight = 1;
      v->next->next->dirtyweight = 1;
      double v0 = SbMax(PImpl::heap_evaluate(v), PImpl::heap_evaluate(v->next->next));
      double v1 = SbMax(PImpl::heap_evaluate(v->next), PImpl::heap_evaluate(v->prev));

      // abort if vertices should not be kept
      if (v0 == v1 && v0 == FLT_MAX && !PRIVATE(this)->keepVertices) return;

      if (v0 < v1) {
        PRIVATE(this)->emitTriangle(v);
        PRIVATE(this)->emitTriangle(v);
      }
      else {
        v = v->next;
        PRIVATE(this)->emitTriangle(v);
        PRIVATE(this)->emitTriangle(v);
      }
      PRIVATE(this)->numVerts -= 2;
    }

    // Emit the empty triangles that might lay around
    if (PRIVATE(this)->keepVertices) {
      while (PRIVATE(this)->numVerts>=3) {
        PRIVATE(this)->emitTriangle(v);
        PRIVATE(this)->numVerts--;
      }
    }
  }
  else if (PRIVATE(this)->numVerts==3) {   //only one triangle
    PRIVATE(this)->emitTriangle(PRIVATE(this)->headV);
  }
}

// *************************************************************************

/*!
  Sets the callback function for this tessellator.
*/
void
SbTesselator::setCallback(SbTesselatorCB * func, void * data)
{
  PRIVATE(this)->callback = func;
  PRIVATE(this)->callbackData = data;
}

// *************************************************************************

//
// PRIVATE FUNCTIONS:
//

#if 1
//From http://totologic.blogspot.se/2014/01/accurate-point-in-triangle-test.html
//p is the testpoint, all other points are corners of the triangle
SbBool SbTesselator::PImpl::pointInTriangle(Vertex * pt, Vertex * t)
{
  const SbVec3f& p = pt->v;
  const SbVec3f& p1 = t->v;
  const SbVec3f& p2 = t->next->v;
  const SbVec3f& p3 = t->next->next->v;

  bool isWithinTriangle = FALSE;

  // Based on Barycentric coordinates
  float denominator = 1 / ((p2[Y] - p3[Y]) * (p1[X] - p3[X]) + (p3[X] - p2[X]) * (p1[Y] - p3[Y]));

  float a = ((p2[Y] - p3[Y]) * (p[X] - p3[X]) + (p3[X] - p2[X]) * (p[Y] - p3[Y])) * denominator;
  float b = ((p3[Y] - p1[Y]) * (p[X] - p3[X]) + (p1[X] - p3[X]) * (p[Y] - p3[Y])) * denominator;
  float c = 1 - a - b;

  // The point is within the triangle or on the border if 0 <= a <= 1 and 0 <= b <= 1 and 0 <= c <= 1
  if (a >= 0 && a <= 1 && b >= 0 && b <= 1 && c >= 0 && c <= 1)
  {
      isWithinTriangle = TRUE;
  }
  // The point is within the triangle
  //if (a > 0f && a < 1f && b > 0f && b < 1f && c > 0f && c < 1f)
  //{
  //  isWithinTriangle = TRUE;
  //}

  return isWithinTriangle;
}
#else
// Checks the distance of point pt(x,y) to edge e0->e1.
// The point is on the edge if its distance is less than epsilon.
// Algorithm from comp.graphics.algorithms FAQ
SbBool
SbTesselator::PImpl::pointOnEdge(Vertex * pt, Vertex * e0, Vertex * e1)
{
  const SbVec3f& p = pt->v;
  const SbVec3f& v0 = e0->v;
  const SbVec3f& v1 = e1->v;

  double C = v1[X] - v0[X];
  double D = v1[Y] - v0[Y];

  double dot = (p[X] - v0[X]) * C + (p[Y] - v0[Y]) * D;
  double norm = C * C + D * D;
  double t = -1;
  if (norm != 0.0)
    t = dot / norm;

  if (t < 0.0)
    t = 0.0;
  if (t > 1.0)
    t = 1.0;

  double px = v0[X] + t * C;
  double py = v0[Y] + t * D;
  double dx = p[X] - px;
  double dy = p[Y] - py;

  return fabs(dx * dx + dy * dy) < epsilon*epsilon;
}

// see: https://stackoverflow.com/questions/2049582/how-to-determine-if-a-point-is-in-a-2d-triangle
// p is the testpoint, and the other points are corners in the triangle
SbBool SbTesselator::PImpl::pointInTriangle(Vertex * pt, Vertex * v0, Vertex * v1, Vertex * v2)
{
  const SbVec3f& p = pt->v;
  const SbVec3f& a = v0->v;
  const SbVec3f& b = v1->v;
  const SbVec3f& c = v2->v;

  double x = p[X] - a[X];
  double y = p[Y] - a[Y];
  bool s_ab = (b[X] - a[X]) * y - (b[Y] - a[Y]) * x > 0;

  if ((c[X] - a[X]) * y - (c[Y] - a[Y]) * x > 0 == s_ab)
    return FALSE;

  if ((c[X] - b[X]) * (p[Y] - b[Y]) - (c[Y] - b[Y]) * (p[X] - b[X]) > 0 != s_ab)
    return FALSE;

  return TRUE;
}

SbBool SbTesselator::PImpl::pointInTriangle(Vertex * pt, Vertex * t)
{
  if (pointInTriangle(pt, t, t->next, t->next->next))
    return TRUE;

  // The pointInTriangle test might fail for vertices that are on one
  // of the triangle edges. Do a pointOnEdge test for all three
  // edges to handle this case.
  if (pointOnEdge(pt, t, t->next))
    return TRUE;
  if (pointOnEdge(pt, t->next, t->next->next))
    return TRUE;
  if (pointOnEdge(pt, t->next->next, t))
    return TRUE;

  return false;
}
#endif

//
// Check if there are no intersection to the triangle
// pointed to by t. (no other vertices are inside the triangle)
//
SbBool
SbTesselator::PImpl::clippable(Vertex * t)
{
  SbBox3f boundingbox;
  boundingbox.makeEmpty();
  boundingbox.extendBy(SbVec3f(t->v[X], t->v[Y], 0.0f));
  boundingbox.extendBy(SbVec3f(t->next->v[X], t->next->v[Y], 0.0f));
  boundingbox.extendBy(SbVec3f(t->next->next->v[X], t->next->next->v[Y], 0.0f));

  SbSphere sphere;
  sphere.circumscribe(boundingbox);

  SbList <int> & l = clippablelist;
  l.truncate(0);
  bsptree.findPoints(sphere, l);
  for (int i = 0; i < l.getLength(); i++) {
    Vertex * vtx = static_cast<Vertex*>(bsptree.getUserData(l[i]));
    if (vtx != t && vtx != t->next && vtx != t->next->next) {
      if (pointInTriangle(vtx, t))
        return FALSE;
    }
  }
  return TRUE;
}

//
// Call the callback-function for the triangle starting with t
//
void
SbTesselator::PImpl::emitTriangle(Vertex * t)
{
  assert(t);
  assert(t->next);
  assert(t->next->next);
  assert(callback);

  callback(t->data, t->next->data, t->next->next->data, callbackData);
  cutTriangle(t);
}

//
// Cuts t->next out of the triangle vertex list.
//
// FIXME: bad design, this should have been a method on
// Vertex. 20031007 mortene.
void
SbTesselator::PImpl::cutTriangle(Vertex * t)
{
  t->next->next->prev = t;
  t->next = t->next->next;
}

//
// Returns the two times the signed area of the triangle starting with t
//
double
SbTesselator::PImpl::area(Vertex * t)
{
  return 0.5*((t->next->v[X] - t->v[X]) * (t->next->next->v[Y] - t->v[Y]) -
              (t->next->v[Y] - t->v[Y]) * (t->next->next->v[X] - t->v[X]));
}

//
// Returns the square of the radius of the circum circle through points starting at vertex t in projection plane.
// Algorithm for calculating the center of the circle from comp.graphics.algorithms FAQ
//
double
SbTesselator::PImpl::circleSize(Vertex * t)
{
  const SbVec3f& a = t->v;
  const SbVec3f& b = t->next->v;
  const SbVec3f& c = t->next->next->v;

  double A = b[X] - a[X];
  double B = b[Y] - a[Y];
  double C = c[X] - a[X];
  double D = c[Y] - a[Y];

  double E = A * (a[X] + b[X]) + B * (a[Y] + b[Y]);
  double F = C * (a[X] + c[X]) + D * (a[Y] + c[Y]);

  double G = 2 * (A * (c[Y] - b[Y]) - B * (c[X] - b[X]));

  if (G != 0) {
    double val = 1 / G;
    double cx = (D * E - B * F) * val;
    double cy = (A * F - C * E) * val;
    double rx = a[X] - cx;
    double ry = a[Y] - cy;
    return rx * rx + ry * ry;
  }
  return FLT_MAX;
}

//
// Calculate surface normal using Newell's method.
// See https://www.khronos.org/opengl/wiki/Calculating_a_Surface_Normal
//
void
SbTesselator::PImpl::calcPolygonNormal()
{
  assert(numVerts > 3);

  polyNormal.setValue(0.0f, 0.0f, 0.0f);
  SbVec3f vert1, vert2;
  Vertex *currvertex = headV;
  vert2 = currvertex->v;

  while (currvertex->next != NULL && currvertex != tailV) {
    vert1 = vert2;
    vert2 = currvertex->next->v;
    polyNormal[0] += (vert1[1] - vert2[1]) * (vert1[2] + vert2[2]);
    polyNormal[1] += (vert1[2] - vert2[2]) * (vert1[0] + vert2[0]);
    polyNormal[2] += (vert1[0] - vert2[0]) * (vert1[1] + vert2[1]);
    currvertex = currvertex->next;
  }
  vert1 = vert2;
  vert2 = headV->v;
  polyNormal[0] += (vert1[1] - vert2[1]) * (vert1[2] + vert2[2]);
  polyNormal[1] += (vert1[2] - vert2[2]) * (vert1[0] + vert2[0]);
  polyNormal[2] += (vert1[0] - vert2[0]) * (vert1[1] + vert2[1]);

  if (polyNormal.normalize() == 0.0f) {
#if COIN_DEBUG
    SoDebugError::postWarning("SbTesselator::calcPolygonNormal",
                              "Polygon has no normal.");
#endif // COIN_DEBUG
  }
}

//
// makes sure Vertices are not allocated and deallocated
// all the time, by storing them in a growable array. This
// way, the Vertices will not be deleted until the tessellator
// is destructed, and Vertices can be reused.
//
SbTesselator::PImpl::Vertex *
SbTesselator::PImpl::newVertex()
{
  assert(currVertex <= vertexStorage.getLength());
  if (currVertex == vertexStorage.getLength()) {
    Vertex * v = new Vertex;
    vertexStorage.append(v);
  }
  return vertexStorage[currVertex++];
}

void
SbTesselator::PImpl::cleanUp()
{
  headV = tailV = NULL;
  currVertex = 0;
  numVerts = 0;
}

// *************************************************************************

#undef PRIVATE

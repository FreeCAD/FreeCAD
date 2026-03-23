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
  \class SbTri3f SbTri3f.h collision/SbTri3f.h
  \brief A class that at this point in time has one purpose - figuring out
  if two triangles intersect each other.

  \ingroup coin_base

  This class is so limited in functionality that it is not included in the
  public Coin API for now.

  The internals will probably be changed as well, as the a, b, c
  representation isn't very convenient for linear algebra purposes.  But
  as a public base class, the internal representation should be fixed, and
  made part of the private section of the public header.

  \since 2002-10-22
*/

// FIXME: clean up the code -- there's lots of stuff that has been
// commented out without any explanation, for instance. 20030603 mortene.

#include <cassert>
#include <cfloat>

#include <Inventor/errors/SoDebugError.h>
#include <Inventor/SbPlane.h>
#include <Inventor/SbLine.h>
#include <Inventor/SbBox3f.h>

#include "SbTri3f.h"

// Here's an idea for an alternate approach for this class:
//
// Let's say the triangle is defined as (0,0,0), (1,0,0), (1,1,0)
// transformed through a transformation matrix.
//
// With this representation, a triangle would take the 16 floats of
// an SbMatrix instead of 9 for the vertex positions.
//
// Finding the normal of the triangle is a multVecMatrix() call
// Finding a, b, and c are multVecMatrix() calls
// Finding the area is a multVecMatrix() and a getLength() call
//
// Doing intersection testing would be done by transforming one primitive
// into the local coordinate system of the triangle (by using the inverse
// matrix).  Given the simplicity of the a, b, and c coordinates of the
// local space triangle, the intersection testing ought to be a lot more
// trivial to perform than in fully flexible 3D space.  Especially the
// case of triangles in the same plane...

#define SBTRI_DEBUG 0

// *************************************************************************

class SbTri3fP {
public:
  SbTri3fP(void) {}
  SbTri3fP(SbTri3fP * t)
    : a(t->a), b(t->b), c(t->c) {}
  SbTri3fP(const SbVec3f & na, const SbVec3f & nb, const SbVec3f & nc)
    : a(na), b(nb), c(nc)
  {
    // FIXME: fix IDAction so this assert doesn't hit. 20030328 mortene.
    assert(a != b && a != c && b != c);
  }

  SbVec3f a;
  SbVec3f b;
  SbVec3f c;
};

// *************************************************************************

#define PRIVATE(obj) ((obj)->pimpl)

SbTri3f::SbTri3f(void)
  : pimpl(new SbTri3fP)
{
}

SbTri3f::SbTri3f(const SbTri3f & t)
  : pimpl(new SbTri3fP(PRIVATE(&t)))
{
}

SbTri3f::SbTri3f(const SbVec3f & a, const SbVec3f & b, const SbVec3f & c)
  : pimpl(new SbTri3fP(a, b, c))
{
}

SbTri3f::~SbTri3f(void)
{
  delete PRIVATE(this);
}

SbTri3f &
SbTri3f::setValue(const SbTri3f & t)
{
  PRIVATE(this)->a = PRIVATE(&t)->a;
  PRIVATE(this)->b = PRIVATE(&t)->b;
  PRIVATE(this)->c = PRIVATE(&t)->c;
  assert(PRIVATE(this)->a != PRIVATE(this)->b && PRIVATE(this)->a != PRIVATE(this)->c && PRIVATE(this)->b != PRIVATE(this)->c);
  return *this;
}

SbTri3f &
SbTri3f::setValue(const SbVec3f & a, const SbVec3f & b, const SbVec3f & c)
{
  assert(a != b && a != c && b != c);
  PRIVATE(this)->a = a;
  PRIVATE(this)->b = b;
  PRIVATE(this)->c = c;
  return *this;
}

void
SbTri3f::getValue(SbTri3f & t) const
{
  PRIVATE(&t)->a = PRIVATE(this)->a;
  PRIVATE(&t)->b = PRIVATE(this)->b;
  PRIVATE(&t)->c = PRIVATE(this)->c;
}

void
SbTri3f::getValue(SbVec3f & a, SbVec3f & b, SbVec3f & c) const
{
  a = PRIVATE(this)->a;
  b = PRIVATE(this)->b;
  c = PRIVATE(this)->c;
}

SbTri3f &
SbTri3f::operator = (const SbTri3f & t)
{
  PRIVATE(this)->a = PRIVATE(&t)->a;
  PRIVATE(this)->b = PRIVATE(&t)->b;
  PRIVATE(this)->c = PRIVATE(&t)->c;
  return *this;
}

// *************************************************************************

SbBool
SbTri3f::intersect(const SbTri3f & t) const
{
  // FIXME: remove all "programming logic" error messages and asserts from
  // this function when it is verified that those paths can't be taken.

  SbVec3f a1(PRIVATE(this)->a);
  SbVec3f b1(PRIVATE(this)->b);
  SbVec3f c1(PRIVATE(this)->c);
  SbPlane plane1(a1, b1, c1);

  SbVec3f a2(PRIVATE(&t)->a);
  SbVec3f b2(PRIVATE(&t)->b);
  SbVec3f c2(PRIVATE(&t)->c);
  SbPlane plane2(a2, b2, c2);

  // FIXME: can ((n1 == -n2) && (d1 == -d2)) really happen?
  if (SBTRI_DEBUG &&
       (plane1.getNormal() == -plane2.getNormal()) &&
       (plane1.getDistanceFromOrigin() == -plane2.getDistanceFromOrigin())) {
    SoDebugError::post("SbTri3f::intersect", "The (n1 == -n2 && d1 == -d2) case happened");
  }
  if (plane1.getNormal() == plane2.getNormal()) {
    // fprintf(stderr, "normals are equal\n");
    if (plane1.getDistanceFromOrigin() != plane2.getDistanceFromOrigin())
      return FALSE; // parallel planes
    // we work around coplanar intersection testing by making it a case of
    // biplanar intersection testing.

    int vertex = 1;
    float distance = a1.sqrLength();
    float d;
    if ((d = b1.sqrLength()) > distance) {
      distance = d;
      vertex = 2;
    } else if ((d = c1.sqrLength()) > distance) {
      distance = d;
      vertex = 3;
    } else if ((d = a2.sqrLength()) > distance) {
      distance = d;
      vertex = 4;
    } else if ((d = b2.sqrLength()) > distance) {
      distance = d;
      vertex = 5;
    } else if ((d = c2.sqrLength()) > distance) {
      distance = d;
      vertex = 6;
    }
    switch (vertex) {
    case 1:
      break;
    case 2:
      do { SbVec3f temp(a1); a1 = b1; b1 = c1; c1 = temp; } while (FALSE);
      break;
    case 3:
      do { SbVec3f temp(a1); a1 = c1; c1 = b1; b1 = temp; } while (FALSE);
      break;
    case 4:
      do { SbVec3f temp(a1); a1 = a2; a2 = temp; } while (FALSE);
      do { SbVec3f temp(b1); b1 = b2; b2 = temp; } while (FALSE);
      do { SbVec3f temp(c1); c1 = c2; c2 = temp; } while (FALSE);
      break;
    case 5:
      do { SbVec3f temp(a1); a1 = b2; b2 = temp; } while (FALSE);
      do { SbVec3f temp(b1); b1 = c2; c2 = temp; } while (FALSE);
      do { SbVec3f temp(c1); c1 = a2; a2 = temp; } while (FALSE);
      break;
    case 6:
      do { SbVec3f temp(a1); a1 = c2; c2 = temp; } while (FALSE);
      do { SbVec3f temp(b1); b1 = a2; a2 = temp; } while (FALSE);
      do { SbVec3f temp(c1); c1 = b2; b2 = temp; } while (FALSE);
      break;
    }
    vertex = 1;
    distance = (a2-a1).sqrLength();
    if ((d = (b2-a1).sqrLength()) > distance) {
      distance = d;
      vertex = 2;
    } else if ((d = (c2-a1).sqrLength()) > distance) {
      distance = d;
      vertex = 3;
    }
    switch (vertex) {
    case 1:
      break;
    case 2:
      do { SbVec3f temp(a2); a2 = b2; b2 = c2; c2 = temp; } while (FALSE);
      break;
    case 3:
      do { SbVec3f temp(a2); a2 = c2; c2 = b2; b2 = temp; } while (FALSE);
      break;
    }

    // FIXME: I'm not confident we've actually found the two vertices that should
    // be lifted up at this point.  I can think of cases that will give false
    // negatives.  20021024 larsa
    a1 = a1 + plane1.getNormal();
    a2 = a2 + plane1.getNormal();
    // regenerate planes
    plane1 = SbPlane(a1, b1, c1);
    plane2 = SbPlane(a2, b2, c2);
  // } else {
    // fprintf(stderr, "normals are different\n");
  }

  // set up point a on one side, and b and c on the other

  const SbBool a1hs = plane2.isInHalfSpace(a1);
  const SbBool b1hs = plane2.isInHalfSpace(b1);
  const SbBool c1hs = plane2.isInHalfSpace(c1);
  if ((a1hs == b1hs) && (a1hs == c1hs)) {
    // no intersection
    return FALSE;
  } else if (a1hs == c1hs) { // b is in other halfspace
    SbVec3f temp(a1); a1 = b1; b1 = c1; c1 = temp;
  } else if (a1hs == b1hs) { // c is in other halfspace
    SbVec3f temp(a1); a1 = c1; c1 = b1; b1 = temp;
  }

  const SbBool a2hs = plane1.isInHalfSpace(a2);
  const SbBool b2hs = plane1.isInHalfSpace(b2);
  const SbBool c2hs = plane1.isInHalfSpace(c2);
  if ((a2hs == b2hs) && (a2hs == c2hs)) {
    // no intersection
    return FALSE;
  } else if (a2hs == c2hs) { // b is in other halfspace
    SbVec3f temp(a2); a2 = b2; b2 = c2; c2 = temp;
  } else if (a2hs == b2hs) { // c is in other halfspace
    SbVec3f temp(a2); a2 = c2; c2 = b2; b2 = temp;
  }

  // find intersection points on line for triangles
  SbVec3f p11, p12;
  if (!plane2.intersect(SbLine(a1, b1), p11)) {
    // should really never happen
    if (SBTRI_DEBUG) {
      SoDebugError::post("SbTri3f::intersect", "programming logic error 1");
      SoDebugError::post("-", "SbVec3f a1(%g, %g, %g);", a1[0], a1[1], a1[2]);
      SoDebugError::post("-", "SbVec3f b1(%g, %g, %g);", b1[0], b1[1], b1[2]);
      SoDebugError::post("-", "SbVec3f c1(%g, %g, %g);", c1[0], c1[1], c1[2]);
      SoDebugError::post("-", "SbVec3f a2(%g, %g, %g);", a2[0], a2[1], a2[2]);
      SoDebugError::post("-", "SbVec3f b2(%g, %g, %g);", b2[0], b2[1], b2[2]);
      SoDebugError::post("-", "SbVec3f c2(%g, %g, %g);", c2[0], c2[1], c2[2]);
      assert(0);
    }
    return FALSE;
  }
  if (!plane2.intersect(SbLine(a1, c1), p12)) {
    // should never happen
    if (SBTRI_DEBUG) {
      SoDebugError::post("SbTri3f::intersect", "programming logic error 2");
      SoDebugError::post("-", "SbVec3f a1(%g, %g, %g);", a1[0], a1[1], a1[2]);
      SoDebugError::post("-", "SbVec3f b1(%g, %g, %g);", b1[0], b1[1], b1[2]);
      SoDebugError::post("-", "SbVec3f c1(%g, %g, %g);", c1[0], c1[1], c1[2]);
      SoDebugError::post("-", "SbVec3f a2(%g, %g, %g);", a2[0], a2[1], a2[2]);
      SoDebugError::post("-", "SbVec3f b2(%g, %g, %g);", b2[0], b2[1], b2[2]);
      SoDebugError::post("-", "SbVec3f c2(%g, %g, %g);", c2[0], c2[1], c2[2]);
      assert(0);
    }
    return FALSE;
  }

  SbVec3f p21, p22;
  if (!plane1.intersect(SbLine(a2, b2), p21)) {
    // should never happen
    // but since it does, it means something
    // possibly that a2 and b2 are in plane1, and halfspace values were wrong in
    // some way.  we should either return FALSE or set p21 to something
    if (SBTRI_DEBUG) {
      SoDebugError::post("SbTri3f::intersect", "programming logic error 3");
      SoDebugError::post("-", "SbVec3f a1(%g, %g, %g);", a1[0], a1[1], a1[2]);
      SoDebugError::post("-", "SbVec3f b1(%g, %g, %g);", b1[0], b1[1], b1[2]);
      SoDebugError::post("-", "SbVec3f c1(%g, %g, %g);", c1[0], c1[1], c1[2]);
      SoDebugError::post("-", "SbVec3f a2(%g, %g, %g);", a2[0], a2[1], a2[2]);
      SoDebugError::post("-", "SbVec3f b2(%g, %g, %g);", b2[0], b2[1], b2[2]);
      SoDebugError::post("-", "SbVec3f c2(%g, %g, %g);", c2[0], c2[1], c2[2]);
      assert(0);
    }
    return FALSE;
  }
  if (!plane1.intersect(SbLine(a2, c2), p22)) {
    // should never happen
    if (SBTRI_DEBUG) {
      SoDebugError::post("SbTri3f::intersect", "programming logic error 4\n");
      SoDebugError::post("-", "SbVec3f a1(%g, %g, %g);", a1[0], a1[1], a1[2]);
      SoDebugError::post("-", "SbVec3f b1(%g, %g, %g);", b1[0], b1[1], b1[2]);
      SoDebugError::post("-", "SbVec3f c1(%g, %g, %g);", c1[0], c1[1], c1[2]);
      SoDebugError::post("-", "SbVec3f a2(%g, %g, %g);", a2[0], a2[1], a2[2]);
      SoDebugError::post("-", "SbVec3f b2(%g, %g, %g);", b2[0], b2[1], b2[2]);
      SoDebugError::post("-", "SbVec3f c2(%g, %g, %g);", c2[0], c2[1], c2[2]);
      assert(0);
    }
    return FALSE;
  }

  // find end point of the four (the one furthest from origo would be an end point)
  // and the length of that line segment
  float distance, maxdistance;
  int vertex = 1;
  maxdistance = p11.sqrLength();
  distance = p12.sqrLength();
  if (distance > maxdistance) {
    vertex = 2;
    maxdistance = distance;
  }
  distance = p21.sqrLength();
  if (distance > maxdistance) {
    vertex = 3;
    maxdistance = distance;
  }
  distance = p22.sqrLength();
  if (distance > maxdistance) {
    vertex = 4;
    maxdistance = distance;
  }

  // check if a vertec from the other line segment is within the perimeter of the line
  SbVec3f p, e, p1, p2;
  switch (vertex) {
  case 1:
    p = p11; e = p12; p1 = p21; p2 = p22;
    break;
  case 2:
    p = p12; e = p11; p1 = p21; p2 = p22;
    break;
  case 3:
    p = p21; e = p22; p1 = p11; p2 = p12;
    break;
  case 4:
    p = p22; e = p21; p1 = p11; p2 = p12;
    break;
  default:
    if (SBTRI_DEBUG)
      SoDebugError::post("SbTri3f::intersect", "programming logic error 5\n");
    assert(0);
  }
  float pedistance = (e - p).sqrLength();
  if (pedistance > (p1-p).sqrLength()) return TRUE;
  if (pedistance > (p2-p).sqrLength()) return TRUE;
  return FALSE;
}

SbBool
SbTri3f::intersect(const SbTri3f & t, float e) const
{
  if (e == 0.0f) return this->intersect(t);
  if (this->getDistance(t) <= e) return TRUE;
  return FALSE;
}

SbVec3f
SbTri3f::getNormal() const
{
  SbVec3f p[3];
  this->getValue(p[0], p[1], p[2]);
  SbPlane pl(p[0], p[1], p[2]);
  return pl.getNormal();
}

/*!
  Returns the distance from the given point to this triangle.
*/
float 
SbTri3f::getDistance(const SbVec3f & p) const
{
  float dist = FLT_MAX;
  SbVec3f thisp[3];
  this->getValue(thisp[0], thisp[1], thisp[2]);
  SbPlane pl(thisp[0], thisp[1], thisp[2]);

  SbVec3f intersect;
  SbVec3f n = this->getNormal();
  SbLine line(p, p+n);
  if (pl.intersect(line, intersect)) {
    int i;
    for (i=0;i<3;i++) {
      SbPlane edgepl(thisp[i], thisp[i]+n, thisp[(i+1)%3]);
      if (!edgepl.isInHalfSpace(intersect)) break;
    }
    if (i == 3) dist = static_cast<float>(fabs(pl.getDistance(p)));
    else { // We didn't project inside triangle
      for (int j=0;j<3;j++) {
        float d = SbTri3f::getDistance(p, thisp[j], thisp[(j+1)%3]);
        if (d < dist) dist = d;
      }
    }
  }
  else {
    assert(FALSE);
  }
  return dist;
}

/*!
  Returns the distance from p to the line segment p1-p2.
*/
float 
SbTri3f::getDistance(const SbVec3f & p, 
                   const SbVec3f & p1, const SbVec3f & p2)
{
  SbVec3f normal = p2 - p1;
  SbPlane pl1(normal, p1);
  SbPlane pl2(-normal, p2);

  if (pl1.isInHalfSpace(p) && pl2.isInHalfSpace(p)) {
    SbLine line(p1, p2);
    return (line.getClosestPoint(p)-p).length();
  }
  else {
    float d1 = (p - p1).length();
    float d2 = (p - p2).length();
    return (d1<d2)?d1:d2;
  }
}

static const float gs_fTolerance = 1e-06f;

/*!
  Returns the distance from this triangle to the given line segment.
*/
float 
SbTri3f::getDistance(const SbVec3f & p1, const SbVec3f & p2) const
{
  SbVec3f kDiff = PRIVATE(this)->a - p1;
  SbVec3f edge0 = PRIVATE(this)->b - PRIVATE(this)->a;
  SbVec3f edge1 = PRIVATE(this)->c - PRIVATE(this)->a;
  float fA00 = (p2-p1).sqrLength();
  float fA01 = -(p2-p1).dot(edge0);
  float fA02 = -(p2-p1).dot(edge1);
  float fA11 = edge0.sqrLength();
  float fA12 = edge0.dot(edge1);
  float fA22 = edge1.dot(edge1);
  float fB0  = -kDiff.dot(p2-p1);
  float fB1  = kDiff.dot(edge0);
  float fB2  = kDiff.dot(edge1);

  float fSqrDist, fSqrDist0, fR, fS, fT, fR0, fS0, fT0;

  // Set up for a relative error test on the angle between ray direction
  // and triangle normal to determine parallel/nonparallel status.
  SbVec3f kN = edge0.cross(edge1);
  float fNSqrLen = kN.sqrLength();
  float fDot = (p2-p1).dot(kN);
  SbBool bNotParallel = (fDot*fDot >= gs_fTolerance*fA00*fNSqrLen);

  if (bNotParallel) {
    float fCof00 = fA11*fA22-fA12*fA12;
    float fCof01 = fA02*fA12-fA01*fA22;
    float fCof02 = fA01*fA12-fA02*fA11;
    float fCof11 = fA00*fA22-fA02*fA02;
    float fCof12 = fA02*fA01-fA00*fA12;
    float fCof22 = fA00*fA11-fA01*fA01;
    float fInvDet = 1.0f/(fA00*fCof00+fA01*fCof01+fA02*fCof02);
    float fRhs0 = -fB0*fInvDet;
    float fRhs1 = -fB1*fInvDet;
    float fRhs2 = -fB2*fInvDet;
    
    fR = fCof00*fRhs0+fCof01*fRhs1+fCof02*fRhs2;
    fS = fCof01*fRhs0+fCof11*fRhs1+fCof12*fRhs2;
    fT = fCof02*fRhs0+fCof12*fRhs1+fCof22*fRhs2;

    if (fR < 0.0f) {
      if (fS+fT <= 1.0f) {
        if (fS < 0.0f) {
          if (fT < 0.0f) {  // region 4m
            // min on face s=0 or t=0 or r=0
            fSqrDist = SbTri3f::sqrDistance(p1, p2, 
                                          PRIVATE(this)->a, PRIVATE(this)->c,
                                          &fR,&fT);
            fS = 0.0f;
            fSqrDist0 = SbTri3f::sqrDistance(p1, p2, 
                                           PRIVATE(this)->a, PRIVATE(this)->b,
                                           &fR0,&fS0);
            fT0 = 0.0f;
            if (fSqrDist0 < fSqrDist) {
              fSqrDist = fSqrDist0;
              fR = fR0;
              fS = fS0;
              fT = fT0;
            }
            fSqrDist0 = this->sqrDistance(p1,&fS0,&fT0);
            fR0 = 0.0f;
            if (fSqrDist0 < fSqrDist) {
              fSqrDist = fSqrDist0;
              fR = fR0;
              fS = fS0;
              fT = fT0;
            }
          }
          else {  // region 3m
            // min on face s=0 or r=0
            fSqrDist = SbTri3f::sqrDistance(p1, p2, PRIVATE(this)->a, PRIVATE(this)->c,&fR,&fT);
            fS = 0.0f;
            fSqrDist0 = this->sqrDistance(p1,&fS0,&fT0);
            fR0 = 0.0f;
            if (fSqrDist0 < fSqrDist) {
              fSqrDist = fSqrDist0;
              fR = fR0;
              fS = fS0;
              fT = fT0;
            }
          }
        }
        else if (fT < 0.0f) {  // region 5m
          // min on face t=0 or r=0
          fSqrDist = SbTri3f::sqrDistance(p1, p2, 
                                        PRIVATE(this)->a, PRIVATE(this)->b,
                                        &fR,&fS);
          fT = 0.0f;
          fSqrDist0 = this->sqrDistance(p1,&fS0,&fT0);
          fR0 = 0.0f;
          if (fSqrDist0 < fSqrDist) {
            fSqrDist = fSqrDist0;
            fR = fR0;
            fS = fS0;
            fT = fT0;
          }
        }
        else {  // region 0m
          // min on face r=0
          fSqrDist = this->sqrDistance(p1,&fS,&fT);
          fR = 0.0f;
        }
      }
      else {
        if (fS < 0.0f) {  // region 2m
          // min on face s=0 or s+t=1 or r=0
          fSqrDist = SbTri3f::sqrDistance(p1, p2, PRIVATE(this)->a, PRIVATE(this)->c,&fR,&fT);
          fS = 0.0f;
          fSqrDist0 = SbTri3f::sqrDistance(p1, p2, PRIVATE(this)->b, PRIVATE(this)->c,&fR0,&fT0);
          fS0 = 1.0f-fT0;
          if (fSqrDist0 < fSqrDist) {
            fSqrDist = fSqrDist0;
            fR = fR0;
            fS = fS0;
            fT = fT0;
          }
          fSqrDist0 = this->sqrDistance(p1,&fS0,&fT0);
          fR0 = 0.0f;
          if (fSqrDist0 < fSqrDist) {
            fSqrDist = fSqrDist0;
            fR = fR0;
            fS = fS0;
            fT = fT0;
          }
        }
        else if (fT < 0.0f) {  // region 6m
          // min on face t=0 or s+t=1 or r=0
          fSqrDist = SbTri3f::sqrDistance(p1, p2, 
                                        PRIVATE(this)->a, PRIVATE(this)->b,
                                        &fR,&fS);
          fT = 0.0f;
          fSqrDist0 = SbTri3f::sqrDistance(p1, p2, PRIVATE(this)->b, PRIVATE(this)->c,&fR0,&fT0);
          fS0 = 1.0f-fT0;
          if (fSqrDist0 < fSqrDist) {
            fSqrDist = fSqrDist0;
            fR = fR0;
            fS = fS0;
            fT = fT0;
          }
          fSqrDist0 = this->sqrDistance(p1,&fS0,&fT0);
          fR0 = 0.0f;
          if (fSqrDist0 < fSqrDist) {
            fSqrDist = fSqrDist0;
            fR = fR0;
            fS = fS0;
            fT = fT0;
          }
        }
        else {  // region 1m
          // min on face s+t=1 or r=0
          fSqrDist = SbTri3f::sqrDistance(p1, p2, PRIVATE(this)->b, PRIVATE(this)->c,&fR,&fT);
          fS = 1.0f-fT;
          fSqrDist0 = this->sqrDistance(p1,&fS0,&fT0);
          fR0 = 0.0f;
          if (fSqrDist0 < fSqrDist) {
            fSqrDist = fSqrDist0;
            fR = fR0;
            fS = fS0;
            fT = fT0;
          }
        }
      }
    }
    else if (fR <= 1.0f) {
      if (fS+fT <= 1.0f) {
        if (fS < 0.0f) {
          if (fT < 0.0f) {  // region 4
            // min on face s=0 or t=0
            fSqrDist = SbTri3f::sqrDistance(p1, p2, PRIVATE(this)->a, PRIVATE(this)->c,&fR,&fT);
            fS = 0.0f;
            fSqrDist0 = SbTri3f::sqrDistance(p1, p2, 
                                           PRIVATE(this)->a, PRIVATE(this)->b,
                                           &fR0,&fS0);
            fT0 = 0.0f;
            if (fSqrDist0 < fSqrDist) {
              fSqrDist = fSqrDist0;
              fR = fR0;
              fS = fS0;
              fT = fT0;
            }
          }
          else {  // region 3
            // min on face s=0
            fSqrDist = SbTri3f::sqrDistance(p1, p2, PRIVATE(this)->a, PRIVATE(this)->c,&fR,&fT);
            fS = 0.0f;
          }
        }
        else if (fT < 0.0f) {  // region 5
          // min on face t=0
          fSqrDist = SbTri3f::sqrDistance(p1, p2, PRIVATE(this)->a, PRIVATE(this)->b,&fR,&fS);
          fT = 0.0f;
        }
        else {  // region 0
          // global minimum is interior, done
          fSqrDist = 0.0f;
        }
      }
      else {
        if (fS < 0.0f) {  // region 2
          // min on face s=0 or s+t=1
          fSqrDist = SbTri3f::sqrDistance(p1, p2, PRIVATE(this)->a, PRIVATE(this)->c,&fR,&fT);
          fS = 0.0f;
          fSqrDist0 = SbTri3f::sqrDistance(p1, p2, PRIVATE(this)->b, PRIVATE(this)->c,&fR0,&fT0);
          fS0 = 1.0f-fT0;
          if (fSqrDist0 < fSqrDist) {
            fSqrDist = fSqrDist0;
            fR = fR0;
            fS = fS0;
            fT = fT0;
          }
        }
        else if (fT < 0.0f) {  // region 6
          // min on face t=0 or s+t=1
          fSqrDist = SbTri3f::sqrDistance(p1, p2, PRIVATE(this)->a, PRIVATE(this)->b,&fR,&fS);
          fT = 0.0f;
          fSqrDist0 = SbTri3f::sqrDistance(p1, p2, PRIVATE(this)->b, PRIVATE(this)->c,&fR0,&fT0);
          fS0 = 1.0f-fT0;
          if (fSqrDist0 < fSqrDist) {
            fSqrDist = fSqrDist0;
            fR = fR0;
            fS = fS0;
            fT = fT0;
          }
        }
        else {  // region 1
          // min on face s+t=1
          fSqrDist = SbTri3f::sqrDistance(p1, p2, PRIVATE(this)->b, PRIVATE(this)->c,&fR,&fT);
          fS = 1.0f-fT;
        }
      }
    }
    else {  // fR > 1
      if (fS+fT <= 1.0f) {
        if (fS < 0.0f) {
          if (fT < 0.0f) {  // region 4p
            // min on face s=0 or t=0 or r=1
            fSqrDist = SbTri3f::sqrDistance(p1, p2, PRIVATE(this)->a, PRIVATE(this)->c,&fR,&fT);
            fS = 0.0f;
            fSqrDist0 = SbTri3f::sqrDistance(p1, p2, PRIVATE(this)->a, PRIVATE(this)->b,&fR0,&fS0);
            fT0 = 0.0f;
            if (fSqrDist0 < fSqrDist) {
              fSqrDist = fSqrDist0;
              fR = fR0;
              fS = fS0;
              fT = fT0;
            }
            fSqrDist0 = this->sqrDistance(p2,&fS0,&fT0);
            fR0 = 1.0f;
            if (fSqrDist0 < fSqrDist) {
              fSqrDist = fSqrDist0;
              fR = fR0;
              fS = fS0;
              fT = fT0;
            }
          }
          else {  // region 3p
            // min on face s=0 or r=1
            fSqrDist = SbTri3f::sqrDistance(p1, p2, PRIVATE(this)->a, PRIVATE(this)->c,&fR,&fT);
            fS = 0.0f;
            fSqrDist0 = this->sqrDistance(p2,&fS0,&fT0);
            fR0 = 1.0f;
            if (fSqrDist0 < fSqrDist) {
              fSqrDist = fSqrDist0;
              fR = fR0;
              fS = fS0;
              fT = fT0;
            }
          }
        }
        else if (fT < 0.0f) {  // region 5p
          // min on face t=0 or r=1
          fSqrDist = SbTri3f::sqrDistance(p1, p2, PRIVATE(this)->a, PRIVATE(this)->b,&fR,&fS);
          fT = 0.0f;
          fSqrDist0 = this->sqrDistance(p2,&fS0,&fT0);
          fR0 = 1.0f;
          if (fSqrDist0 < fSqrDist) {
            fSqrDist = fSqrDist0;
            fR = fR0;
            fS = fS0;
            fT = fT0;
          }
        }
        else {  // region 0p
          // min face on r=1
          fSqrDist = this->sqrDistance(p2,&fS,&fT);
          fR = 1.0f;
        }
      }
      else {
        if (fS < 0.0f) {  // region 2p
          // min on face s=0 or s+t=1 or r=1
          fSqrDist = SbTri3f::sqrDistance(p1, p2, PRIVATE(this)->a, PRIVATE(this)->c,&fR,&fT);
          fS = 0.0f;
          fSqrDist0 = SbTri3f::sqrDistance(p1, p2, PRIVATE(this)->b, PRIVATE(this)->c,&fR0,&fT0);
          fS0 = 1.0f-fT0;
          if (fSqrDist0 < fSqrDist) {
            fSqrDist = fSqrDist0;
            fR = fR0;
            fS = fS0;
            fT = fT0;
          }
          fSqrDist0 = this->sqrDistance(p2,&fS0,&fT0);
          fR0 = 1.0f;
          if (fSqrDist0 < fSqrDist) {
            fSqrDist = fSqrDist0;
            fR = fR0;
            fS = fS0;
            fT = fT0;
          }
        }
        else if (fT < 0.0f) {  // region 6p
          // min on face t=0 or s+t=1 or r=1
          fSqrDist = SbTri3f::sqrDistance(p1, p2, PRIVATE(this)->a, PRIVATE(this)->b,&fR,&fS);
          fT = 0.0f;
          fSqrDist0 = SbTri3f::sqrDistance(p1, p2, PRIVATE(this)->b, PRIVATE(this)->c,&fR0,&fT0);
          fS0 = 1.0f-fT0;
          if (fSqrDist0 < fSqrDist) {
            fSqrDist = fSqrDist0;
            fR = fR0;
            fS = fS0;
            fT = fT0;
          }
          fSqrDist0 = this->sqrDistance(p2,&fS0,&fT0);
          fR0 = 1.0f;
          if (fSqrDist0 < fSqrDist) {
            fSqrDist = fSqrDist0;
            fR = fR0;
            fS = fS0;
            fT = fT0;
          }
        }
        else {  // region 1p
          // min on face s+t=1 or r=1
          fSqrDist = SbTri3f::sqrDistance(p1, p2, PRIVATE(this)->b, PRIVATE(this)->c,&fR,&fT);
          fS = 1.0f-fT;
          fSqrDist0 = this->sqrDistance(p2,&fS0,&fT0);
          fR0 = 1.0f;
          if (fSqrDist0 < fSqrDist) {
            fSqrDist = fSqrDist0;
            fR = fR0;
            fS = fS0;
            fT = fT0;
          }
        }
      }
    }
  }
  else {
    // segment and triangle are parallel
    fSqrDist = SbTri3f::sqrDistance(p1, p2, PRIVATE(this)->a, PRIVATE(this)->b,&fR,&fS);
    fT = 0.0f;

    fSqrDist0 = SbTri3f::sqrDistance(p1, p2, PRIVATE(this)->a, PRIVATE(this)->c,&fR0,&fT0);
    fS0 = 0.0f;
    if (fSqrDist0 < fSqrDist) {
      fSqrDist = fSqrDist0;
      fR = fR0;
      fS = fS0;
      fT = fT0;
    }

    fSqrDist0 = SbTri3f::sqrDistance(p1, p2, PRIVATE(this)->b, PRIVATE(this)->c,&fR0,&fT0);
    fS0 = 1.0f-fT0;
    if (fSqrDist0 < fSqrDist) {
      fSqrDist = fSqrDist0;
      fR = fR0;
      fS = fS0;
      fT = fT0;
    }

    fSqrDist0 = this->sqrDistance(p1,&fS0,&fT0);
    fR0 = 0.0f;
    if (fSqrDist0 < fSqrDist)
      {
        fSqrDist = fSqrDist0;
        fR = fR0;
        fS = fS0;
        fT = fT0;
      }

    fSqrDist0 = this->sqrDistance(p2,&fS0,&fT0);
    fR0 = 1.0f;
    if (fSqrDist0 < fSqrDist)
      {
        fSqrDist = fSqrDist0;
        fR = fR0;
        fS = fS0;
        fT = fT0;
      }
  }

//    if (pfSegP) *pfSegP = fR;
//    if (pfTriP0) *pfTriP0 = fS;
//    if (pfTriP1) *pfTriP1 = fT;

  return static_cast<float>(sqrt(fSqrDist));
}

/*!
  Returns the distance between the two line segments.
*/
float 
SbTri3f::sqrDistance(const SbVec3f & a1, const SbVec3f & a2,
                   const SbVec3f & b1, const SbVec3f & b2,
                   float * pfSegP0, float * pfSegP1)
{
  SbVec3f kDiff = a1 - b1;
  SbVec3f dir0 = a2 - a1;
  SbVec3f dir1 = b2 - b1;
  float fA00 = dir0.sqrLength();
  float fA01 = -dir0.dot(dir1);
  float fA11 = dir1.sqrLength();
  float fB0 = kDiff.dot(dir0);
  float fC = kDiff.sqrLength();
  float fDet = static_cast<float>(fabs(fA00*fA11-fA01*fA01));
  float fB1, fS, fT, fSqrDist, fTmp;

  if (fDet >= gs_fTolerance) {
    // line segments are not parallel
    fB1 = -kDiff.dot(dir1);
    fS = fA01*fB1-fA11*fB0;
    fT = fA01*fB0-fA00*fB1;
        
    if (fS >= 0.0f) {
      if (fS <= fDet) {
        if (fT >= 0.0f) {
          if (fT <= fDet) {  // region 0 (interior)
            // minimum at two interior points of 3D lines
            float fInvDet = 1.0f/fDet;
            fS *= fInvDet;
            fT *= fInvDet;
            fSqrDist = fS*(fA00*fS+fA01*fT+2.0f*fB0) +
              fT*(fA01*fS+fA11*fT+2.0f*fB1)+fC;
          }
          else {  // region 3 (side)
            fT = 1.0f;
            fTmp = fA01+fB0;
            if (fTmp >= 0.0f) {
              fS = 0.0f;
              fSqrDist = fA11+2.0f*fB1+fC;
            }
            else if (-fTmp >= fA00) {
              fS = 1.0f;
              fSqrDist = fA00+fA11+fC+2.0f*(fB1+fTmp);
            }
            else {
              fS = -fTmp/fA00;
              fSqrDist = fTmp*fS+fA11+2.0f*fB1+fC;
            }
          }
        }
        else {  // region 7 (side)
          fT = 0.0f;
          if (fB0 >= 0.0f) {
            fS = 0.0f;
            fSqrDist = fC;
          }
          else if (-fB0 >= fA00) {
            fS = 1.0f;
            fSqrDist = fA00+2.0f*fB0+fC;
          }
          else {
            fS = -fB0/fA00;
            fSqrDist = fB0*fS+fC;
          }
        }
      }
      else {
        if (fT >= 0.0) {
          if (fT <= fDet) {  // region 1 (side)
            fS = 1.0f;
            fTmp = fA01+fB1;
            if (fTmp >= 0.0f) {
              fT = 0.0f;
              fSqrDist = fA00+2.0f*fB0+fC;
            }
            else if (-fTmp >= fA11) {
              fT = 1.0f;
              fSqrDist = fA00+fA11+fC+2.0f*(fB0+fTmp);
            }
            else {
              fT = -fTmp/fA11;
              fSqrDist = fTmp*fT+fA00+2.0f*fB0+fC;
            }
          }
          else {  // region 2 (corner)
            fTmp = fA01+fB0;
            if (-fTmp <= fA00) {
              fT = 1.0f;
              if (fTmp >= 0.0f) {
                fS = 0.0f;
                fSqrDist = fA11+2.0f*fB1+fC;
              }
              else {
                fS = -fTmp/fA00;
                fSqrDist = fTmp*fS+fA11+2.0f*fB1+fC;
              }
            }
            else {
              fS = 1.0f;
              fTmp = fA01+fB1;
              if (fTmp >= 0.0f) {
                fT = 0.0f;
                fSqrDist = fA00+2.0f*fB0+fC;
              }
              else if (-fTmp >= fA11) {
                fT = 1.0f;
                fSqrDist = fA00+fA11+fC+2.0f*(fB0+fTmp);
              }
              else {
                fT = -fTmp/fA11;
                fSqrDist = fTmp*fT+fA00+2.0f*fB0+fC;
              }
            }
          }
        }
        else {  // region 8 (corner)
          if (-fB0 < fA00) {
            fT = 0.0f;
            if (fB0 >= 0.0f) {
              fS = 0.0f;
              fSqrDist = fC;
            }
            else {
              fS = -fB0/fA00;
              fSqrDist = fB0*fS+fC;
            }
          }
          else {
            fS = 1.0f;
            fTmp = fA01+fB1;
            if (fTmp >= 0.0f) {
              fT = 0.0f;
              fSqrDist = fA00+2.0f*fB0+fC;
            }
            else if (-fTmp >= fA11) {
              fT = 1.0f;
              fSqrDist = fA00+fA11+fC+2.0f*(fB0+fTmp);
            }
            else {
              fT = -fTmp/fA11;
              fSqrDist = fTmp*fT+fA00+2.0f*fB0+fC;
            }
          }
        }
      }
    }
    else {
      if (fT >= 0.0f) {
        if (fT <= fDet) {  // region 5 (side)
          fS = 0.0f;
          if (fB1 >= 0.0f) {
            fT = 0.0f;
            fSqrDist = fC;
          }
          else if (-fB1 >= fA11) {
            fT = 1.0f;
            fSqrDist = fA11+2.0f*fB1+fC;
          }
          else {
            fT = -fB1/fA11;
            fSqrDist = fB1*fT+fC;
          }
        }
        else {  // region 4 (corner)
          fTmp = fA01+fB0;
          if (fTmp < 0.0f) {
            fT = 1.0f;
            if (-fTmp >= fA00) {
              fS = 1.0f;
              fSqrDist = fA00+fA11+fC+2.0f*(fB1+fTmp);
            }
            else {
              fS = -fTmp/fA00;
              fSqrDist = fTmp*fS+fA11+2.0f*fB1+fC;
            }
          }
          else {
            fS = 0.0f;
            if (fB1 >= 0.0f) {
              fT = 0.0f;
              fSqrDist = fC;
            }
            else if (-fB1 >= fA11) {
              fT = 1.0f;
              fSqrDist = fA11+2.0f*fB1+fC;
            }
            else {
              fT = -fB1/fA11;
              fSqrDist = fB1*fT+fC;
            }
          }
        }
      }
      else {   // region 6 (corner)
        if (fB0 < 0.0f) {
          fT = 0.0f;
          if (-fB0 >= fA00) {
            fS = 1.0f;
            fSqrDist = fA00+2.0f*fB0+fC;
          }
          else {
            fS = -fB0/fA00;
            fSqrDist = fB0*fS+fC;
          }
        }
        else {
          fS = 0.0f;
          if (fB1 >= 0.0f) {
            fT = 0.0f;
            fSqrDist = fC;
          }
          else if (-fB1 >= fA11) {
            fT = 1.0f;
            fSqrDist = fA11+2.0f*fB1+fC;
          }
          else {
            fT = -fB1/fA11;
            fSqrDist = fB1*fT+fC;
          }
        }
      }
    }
  }
  else {
    // line segments are parallel
    if (fA01 > 0.0f) {
      // direction vectors form an obtuse angle
      if (fB0 >= 0.0f) {
        fS = 0.0f;
        fT = 0.0f;
        fSqrDist = fC;
      }
      else if (-fB0 <= fA00) {
        fS = -fB0/fA00;
        fT = 0.0f;
        fSqrDist = fB0*fS+fC;
      }
      else {
        fB1 = -kDiff.dot(dir1);
        fS = 1.0f;
        fTmp = fA00+fB0;
        if (-fTmp >= fA01) {
          fT = 1.0f;
          fSqrDist = fA00+fA11+fC+2.0f*(fA01+fB0+fB1);
        }
        else {
          fT = -fTmp/fA01;
          fSqrDist = fA00+2.0f*fB0+fC+fT*(fA11*fT+2.0f*(fA01+fB1));
        }
      }
    }
    else {
      // direction vectors form an acute angle
      if (-fB0 >= fA00) {
        fS = 1.0f;
        fT = 0.0f;
        fSqrDist = fA00+2.0f*fB0+fC;
      }
      else if (fB0 <= 0.0f) {
        fS = -fB0/fA00;
        fT = 0.0f;
        fSqrDist = fB0*fS+fC;
      }
      else {
        fB1 = -kDiff.dot(dir1);
        fS = 0.0f;
        if (fB0 >= -fA01) {
          fT = 1.0f;
          fSqrDist = fA11+2.0f*fB1+fC;
        }
        else {
          fT = -fB0/fA01;
          fSqrDist = fC+fT*(2.0f*fB1+fA11*fT);
        }
      }
    }
  }

  if (pfSegP0) *pfSegP0 = fS;

  if (pfSegP1) *pfSegP1 = fT;

  return static_cast<float>(fabs(fSqrDist));

  //FIXME: Old code. not for segments but lines.
#if 0
  SbVec3f kDiff = a1 - b1;
  SbVec3f dir0 = a2 - a1;
  SbVec3f dir1 = b2 - b1;
  float fA00 = dir0.sqrLength();
  float fA01 = -dir0.dot(dir1);
  float fA11 = dir1.sqrLength();
  float fB0 = kDiff.dot(dir0);
  float fC = kDiff.sqrLength();
  float fDet = fabs(fA00*fA11-fA01*fA01);
  float fB1, fS, fT, fSqrDist;
  
  if (fDet >= gs_fTolerance) {
    // lines are not parallel
    fB1 = -kDiff.dot(dir1);
    float fInvDet = 1.0f/fDet;
    fS = (fA01*fB1-fA11*fB0)*fInvDet;
    fT = (fA01*fB0-fA00*fB1)*fInvDet;
    fSqrDist = fS*(fA00*fS+fA01*fT+2.0f*fB0) +
      fT*(fA01*fS+fA11*fT+2.0f*fB1)+fC;
  }
  else {
    // lines are parallel, select any closest pair of points
    fS = -fB0/fA00;
    fT = 0.0f;
    fSqrDist = fB0*fS+fC;
  }
  
  if (linP0) *linP0 = fS;
  
  if (linP1) *linP1 = fT;

  return fabs(fSqrDist);
#endif
}

/*!
  Returns the minimum distance from this triangle to the given triangle.
*/
float 
SbTri3f::getDistance(const SbTri3f & t) const
{
  float dist = FLT_MAX;
  SbVec3f p[3];
  t.getValue(p[0], p[1], p[2]);
  int i;
  for (i=0;i<3;i++) {
    float d = this->getDistance(p[i], p[(i+1)%3]);
    if (d < dist) dist = d;
  }
  this->getValue(p[0], p[1], p[2]);
  for (i=0;i<3;i++) {
    float d = t.getDistance(p[i], p[(i+1)%3]);
    if (d < dist) dist = d;
  }

#if 0 // debug output
  if (dist <= 1) {
    this->getValue(p[0], p[1], p[2]);
    for (int i=0;i<3;i++) {
      printf("%f %f %f (%x %x %x)\n", 
             p[i][0], p[i][1], p[i][2], 
             *(void **)(&p[i][0]), *(void **)(&p[i][1]), *(void **)(&p[i][2]));
    }
    t.getValue(p[0], p[1], p[2]);
    for (int i=0;i<3;i++) {
      printf("%f %f %f (%x %x %x)\n", 
             p[i][0], p[i][1], p[i][2], 
             *(void **)(&p[i][0]), *(void **)(&p[i][1]), *(void **)(&p[i][2]));
    }
    printf("Dist: %f\n", dist);
  }
#endif

  return dist;

  //FIXME: Old code. Kept for reference (kintel 20021121)
#if 0
  float dist = FLT_MAX;
  SbVec3f p[3];
  t.getValue(p[0], p[1], p[2]);
  int i;
  for (i=0;i<3;i++) {
    float d = this->getDistance(p[i]);
    if (d < dist) dist = d;
  }
  this->getValue(p[0], p[1], p[2]);
  for (i=0;i<3;i++) {
    float d = t.getDistance(p[i]);
    if (d < dist) dist = d;
  }

  return dist;
#endif
}

float
SbTri3f::sqrDistance (const SbVec3f & p1, 
                    float * pfSParam, float * pfTParam) const
{
  SbVec3f kDiff = PRIVATE(this)->a - p1;
  SbVec3f edge0 = PRIVATE(this)->b - PRIVATE(this)->a;
  SbVec3f edge1 = PRIVATE(this)->c - PRIVATE(this)->a;
  float fA00 = edge0.sqrLength();
  float fA01 = edge0.dot(edge1);
  float fA11 = edge1.sqrLength();
  float fB0 = kDiff.dot(edge0);
  float fB1 = kDiff.dot(edge1);
  float fC = kDiff.sqrLength();
  float fDet = static_cast<float>(fabs(fA00*fA11-fA01*fA01));
  float fS = fA01*fB1-fA11*fB0;
  float fT = fA01*fB0-fA00*fB1;
  float fSqrDist;

  if (fS + fT <= fDet)
    {
      if (fS < 0.0f)
        {
          if (fT < 0.0f)  // region 4
            {
              if (fB0 < 0.0f)
                {
                  fT = 0.0f;
                  if (-fB0 >= fA00)
                    {
                      fS = 1.0f;
                      fSqrDist = fA00+2.0f*fB0+fC;
                    }
                  else
                    {
                      fS = -fB0/fA00;
                      fSqrDist = fB0*fS+fC;
                    }
                }
              else
                {
                  fS = 0.0f;
                  if (fB1 >= 0.0f)
                    {
                      fT = 0.0f;
                      fSqrDist = fC;
                    }
                  else if (-fB1 >= fA11)
                    {
                      fT = 1.0f;
                      fSqrDist = fA11+2.0f*fB1+fC;
                    }
                  else
                    {
                      fT = -fB1/fA11;
                      fSqrDist = fB1*fT+fC;
                    }
                }
            }
          else  // region 3
            {
              fS = 0.0f;
              if (fB1 >= 0.0f)
                {
                  fT = 0.0f;
                  fSqrDist = fC;
                }
              else if (-fB1 >= fA11)
                {
                  fT = 1.0f;
                  fSqrDist = fA11+2.0f*fB1+fC;
                }
              else
                {
                  fT = -fB1/fA11;
                  fSqrDist = fB1*fT+fC;
                }
            }
        }
      else if (fT < 0.0f)  // region 5
        {
          fT = 0.0f;
          if (fB0 >= 0.0f)
            {
              fS = 0.0f;
              fSqrDist = fC;
            }
          else if (-fB0 >= fA00)
            {
              fS = 1.0f;
              fSqrDist = fA00+2.0f*fB0+fC;
            }
          else
            {
              fS = -fB0/fA00;
              fSqrDist = fB0*fS+fC;
            }
        }
      else  // region 0
        {
          // minimum at interior point
          float fInvDet = 1.0f/fDet;
          fS *= fInvDet;
          fT *= fInvDet;
          fSqrDist = fS*(fA00*fS+fA01*fT+2.0f*fB0) +
            fT*(fA01*fS+fA11*fT+2.0f*fB1)+fC;
        }
    }
  else
    {
      float fTmp0, fTmp1, fNumer, fDenom;

      if (fS < 0.0f)  // region 2
        {
          fTmp0 = fA01 + fB0;
          fTmp1 = fA11 + fB1;
          if (fTmp1 > fTmp0)
            {
              fNumer = fTmp1 - fTmp0;
              fDenom = fA00-2.0f*fA01+fA11;
              if (fNumer >= fDenom)
                {
                  fS = 1.0f;
                  fT = 0.0f;
                  fSqrDist = fA00+2.0f*fB0+fC;
                }
              else
                {
                  fS = fNumer/fDenom;
                  fT = 1.0f - fS;
                  fSqrDist = fS*(fA00*fS+fA01*fT+2.0f*fB0) +
                    fT*(fA01*fS+fA11*fT+2.0f*fB1)+fC;
                }
            }
          else
            {
              fS = 0.0f;
              if (fTmp1 <= 0.0f)
                {
                  fT = 1.0f;
                  fSqrDist = fA11+2.0f*fB1+fC;
                }
              else if (fB1 >= 0.0f)
                {
                  fT = 0.0f;
                  fSqrDist = fC;
                }
              else
                {
                  fT = -fB1/fA11;
                  fSqrDist = fB1*fT+fC;
                }
            }
        }
      else if (fT < 0.0f)  // region 6
        {
          fTmp0 = fA01 + fB1;
          fTmp1 = fA00 + fB0;
          if (fTmp1 > fTmp0)
            {
              fNumer = fTmp1 - fTmp0;
              fDenom = fA00-2.0f*fA01+fA11;
              if (fNumer >= fDenom)
                {
                  fT = 1.0f;
                  fS = 0.0f;
                  fSqrDist = fA11+2.0f*fB1+fC;
                }
              else
                {
                  fT = fNumer/fDenom;
                  fS = 1.0f - fT;
                  fSqrDist = fS*(fA00*fS+fA01*fT+2.0f*fB0) +
                    fT*(fA01*fS+fA11*fT+2.0f*fB1)+fC;
                }
            }
          else
            {
              fT = 0.0f;
              if (fTmp1 <= 0.0f)
                {
                  fS = 1.0f;
                  fSqrDist = fA00+2.0f*fB0+fC;
                }
              else if (fB0 >= 0.0f)
                {
                  fS = 0.0f;
                  fSqrDist = fC;
                }
              else
                {
                  fS = -fB0/fA00;
                  fSqrDist = fB0*fS+fC;
                }
            }
        }
      else  // region 1
        {
          fNumer = fA11 + fB1 - fA01 - fB0;
          if (fNumer <= 0.0f)
            {
              fS = 0.0f;
              fT = 1.0f;
              fSqrDist = fA11+2.0f*fB1+fC;
            }
          else
            {
              fDenom = fA00-2.0f*fA01+fA11;
              if (fNumer >= fDenom)
                {
                  fS = 1.0f;
                  fT = 0.0f;
                  fSqrDist = fA00+2.0f*fB0+fC;
                }
              else
                {
                  fS = fNumer/fDenom;
                  fT = 1.0f - fS;
                  fSqrDist = fS*(fA00*fS+fA01*fT+2.0f*fB0) +
                    fT*(fA01*fS+fA11*fT+2.0f*fB1)+fC;
                }
            }
        }
    }

  if (pfSParam)
    *pfSParam = fS;

  if (pfTParam)
    *pfTParam = fT;

  return static_cast<float>(fabs(fSqrDist));
}

// *************************************************************************

/*!
  Returns bounding box fully enclosing the triangle.
*/
const SbBox3f
SbTri3f::getBoundingBox(void) const
{
  // FIXME: this involves quite a lot of function calls, and can
  // probably be optimized simply by expanding the code. 20030328 mortene.

  SbBox3f b;
  b.extendBy(PRIVATE(this)->a);
  b.extendBy(PRIVATE(this)->b);
  b.extendBy(PRIVATE(this)->c);
  return b;
}

// *************************************************************************

#undef SBTRI_DEBUG
#undef PRIVATE

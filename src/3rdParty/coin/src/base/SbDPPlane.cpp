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
  \class SbDPPlane SbDPPlane.h Inventor/SbLinear.h
  \brief The SbDPPlane class represents a plane in 3D space.

  \ingroup coin_base

  SbDPPlane is used by many other classes in Coin.  It provides a way of
  representing a plane, specified by a plane normal vector and a
  distance from the origin of the coordinate system.

  \COIN_CLASS_EXTENSION

  \since Coin 2.0
*/

#include <cassert>
#include <cstdio>
#include <Inventor/SbDPPlane.h>
#include <Inventor/SbDPLine.h>
#include <Inventor/SbDPMatrix.h>
#include <cfloat>

#if COIN_DEBUG
#include <Inventor/errors/SoDebugError.h>
#endif // COIN_DEBUG


/*!
  An SbDPPlane instantiated with the default constructor will be
  uninitialized.
*/
SbDPPlane::SbDPPlane(void)
{
}

/*!
  Construct an SbDPPlane instance with a normal pointing in the given
  direction and the given shortest distance from the origin of the
  coordinate system to a point in the plane.

  \a normal must not be a null vector.
*/
SbDPPlane::SbDPPlane(const SbVec3d & normalref, const double D)
{
#if COIN_DEBUG
  if(!(normalref.length() != 0.0f))
    SoDebugError::postWarning("SbDPPlane::SbDPPlane",
                              "Plane normal vector is a null vector.");
#endif // COIN_DEBUG

  this->normal = normalref;
  // we test for a null vector above, just normalize
  (void) this->normal.normalize();
  this->distance = D;
}

/*!
  Construct an SbDPPlane with three points laying in the plane.  Make
  sure \a p0, \a p1 and \a p2 are actually three distinct points when
  using this constructor.
*/
SbDPPlane::SbDPPlane(const SbVec3d & p0, const SbVec3d & p1, const SbVec3d & p2)
{
#if COIN_DEBUG
  if(!(p0 != p1 && p1 != p2 && p0 != p2))
    SoDebugError::postWarning("SbDPPlane::SbDPPlane",
                              "The three points defining the plane cannot "
                              "be coincident.");
#endif // COIN_DEBUG

  this->normal = (p1 - p0).cross(p2 - p0);

  // we test and warn about a null vector above
  (void) this->normal.normalize();

  //     N·point
  // d = -------, |N| == 1
  //       |N|²

  this->distance = this->normal.dot(p0);
}

/*!
  Construct an SbDPPlane from a normal and a point laying in the plane.

  \a normal must not be a null vector.
*/
SbDPPlane::SbDPPlane(const SbVec3d & normalref, const SbVec3d & point)
{
#if COIN_DEBUG
  if(!(normalref.length() != 0.0f))
    SoDebugError::postWarning("SbDPPlane::SbDPPlane",
                              "Plane normal vector is a null vector.");
#endif // COIN_DEBUG

  this->normal = normalref;
  // we test and warn about a null vector above
  (void) this->normal.normalize();

  //     N·point
  // d = -------, |N| == 1
  //       |N|²

  this->distance = this->normal.dot(point);
}


/*!
  Add the given offset \a d to the plane distance from the origin.
*/
void
SbDPPlane::offset(const double d)
{
  this->distance += d;
}

/*!
  Find the point on given line \a l intersecting the plane and return
  it in \a intersection. If the line is parallel to the plane,
  we return \c FALSE, otherwise \c TRUE.

  Do not pass an invalid line for the \a l parameter (i.e. with a
  null direction vector).
*/
SbBool
SbDPPlane::intersect(const SbDPLine & l, SbVec3d & intersection) const
{
#if COIN_DEBUG
  if(!(normal.length() != 0.0f))
    SoDebugError::postWarning("SbDPPlane::intersect",
                              "Intersecting line doesn't have a direction.");
#endif // COIN_DEBUG

  // Check if the line is parallel to the plane.
  if(fabs(l.getDirection().dot(this->normal)) < DBL_EPSILON) return FALSE;

  // From the discussion on SbDPLine::getClosestPoint() we know that
  // any point on the line can be expressed as:
  //                    Q = P + t*D    (1)
  //
  // We can also easily see that a point must satisfy this equation to lie
  // in the plane:
  //                    N·(Q - d*N) = 0, where N is the normal vector,
  //                                     Q is the point and d the offset
  //                                     from the origin.
  //
  // Combining these two equations and simplifying we get:
  //
  //                          d*|N|² - N·P
  //                    t = ----------------, |N| == 1
  //                               N·D
  //
  // Substituting t back in (1), we've solved the problem.
  //                                                         19980816 mortene.

  double t =
    (this->distance - this->normal.dot(l.getPosition()))
    / this->normal.dot(l.getDirection());

  intersection = l.getPosition() + t * l.getDirection();

  return TRUE;
}

/*!
  Transform the plane by \a matrix.

  \sa offset()
*/
void
SbDPPlane::transform(const SbDPMatrix & matrix)
{
  SbVec3d ptInPlane = this->normal * this->distance;

  // according to discussions on comp.graphics.algorithms, the inverse
  // transpose matrix should be used to rotate the plane normal.
  SbDPMatrix invtransp = matrix.inverse().transpose();
  invtransp.multDirMatrix(this->normal, this->normal);

  // the point should be transformed using the original matrix
  matrix.multVecMatrix(ptInPlane, ptInPlane);

  if (this->normal.normalize() == 0.0f) {
#if COIN_DEBUG
    SoDebugError::postWarning("SbDPPlane::transform",
                              "The transformation invalidated the plane.");
#endif // COIN_DEBUG
  }
  this->distance = this->normal.dot(ptInPlane);
}

/*!
  Check if the given point lies in the halfspace of the plane which the
  plane normal vector is pointing.
*/
SbBool
SbDPPlane::isInHalfSpace(const SbVec3d & point) const
{
  // This one is dead easy, we just take the dot product of the normal
  // vector and the vector going from the plane base point to the
  // point we're checking against, and see if the angle between the
  // vectors are within 90° (which is the same as checking the sign
  // of the dot product).
  //                                                    19980816 mortene.
#if 0 // not very efficient code, disabled 19991012 pederb
  SbVec3d pointToPlaneBase = point - (this->normal * this->distance);
  double dotWithNormal = this->normal.dot(pointToPlaneBase);
  if(dotWithNormal >= 0.0f) return TRUE;
  return FALSE;
#else // this code uses distance to plane instead
  return this->getDistance(point) >= 0.0f;
#endif // new code
}

/*!
  Return the distance from \a point to plane. Positive distance means
  the point is in the plane's halfspace.

  This method is an extension specific to Coin versus the original SGI
  Inventor API.
*/
double
SbDPPlane::getDistance(const SbVec3d & point) const
{
  return point.dot(this->normal) - this->distance;
}

/*!
  Return the plane's normal vector, which indicates which direction the plane
  is oriented.

  \sa getDistanceFromOrigin().
*/
const SbVec3d&
SbDPPlane::getNormal(void) const
{
  return this->normal;
}

/*!
  Return distance from origin of coordinate system to the point in the plane
  which is closest to the origin.

  \sa getNormal().
*/
double
SbDPPlane::getDistanceFromOrigin(void) const
{
  return this->distance;
}

/*!
  Intersect this plane with \a pl, and return the resulting line in \a
  line. Returns \c TRUE if an intersection line can be found, and \c
  FALSE if the planes are parallel.

  \COIN_FUNCTION_EXTENSION

  \since Coin 2.0
*/
SbBool
SbDPPlane::intersect(const SbDPPlane & pl, SbDPLine & line) const
{
  // Based on code from Graphics Gems III, Plane-to-Plane Intersection
  // by Priamos Georgiades

  double invdet;  // inverse of 2x2 matrix determinant
  SbVec3d dir2;  // holds the squares of the coordinates of xdir

  SbVec3d xpt;
  SbVec3d xdir;
  xdir = this->normal.cross(pl.normal);

  dir2[0] = xdir[0] * xdir[0];
  dir2[1] = xdir[1] * xdir[1];
  dir2[2] = xdir[2] * xdir[2];

  const SbVec3d & pl1n = this->normal;
  const SbVec3d & pl2n = pl.normal;
  const double pl1w = - this->distance;
  const double pl2w = - pl.distance;

  if (dir2[2] > dir2[1] && dir2[2] > dir2[0] && dir2[2] > DBL_EPSILON) {
    // then get a point on the XY plane
    invdet = 1.0f / xdir[2];
    xpt = SbVec3d(pl1n[1] * pl2w - pl2n[1] * pl1w,
                  pl2n[0] * pl1w - pl1n[0] * pl2w, 0.0f);
  }
  else if (dir2[1] > dir2[0] && dir2[1] > DBL_EPSILON) {
    // then get a point on the XZ plane
    invdet = -1.0f / xdir[1];
    xpt = SbVec3d(pl1n[2] * pl2w - pl2n[2] * pl1w, 0.0f,
                  pl2n[0] * pl1w - pl1n[0] * pl2w);
  }
  else if (dir2[0] > DBL_EPSILON) {
    // then get a point on the YZ plane
    invdet = 1.0f / xdir[0];
    xpt = SbVec3d(0.0f, pl1n[2] * pl2w - pl2n[2] * pl1w,
                  pl2n[1] * pl1w - pl1n[1] * pl2w);
  }
  else // xdir is zero, then no point of intersection exists
    return FALSE;

  xpt *= invdet;
  invdet = 1.0f / static_cast<double>(sqrt(dir2[0] + dir2[1] + dir2[2]));

  xdir *= invdet;
  line.setPosDir(xpt, xdir);
  return TRUE;
}

/*!
  \relates SbDPPlane

  Check the two given planes for equality.
*/
int
operator ==(const SbDPPlane & p1, const SbDPPlane & p2)
{
  if(p1.getDistanceFromOrigin() == p2.getDistanceFromOrigin() &&
     p1.getNormal() == p2.getNormal()) return TRUE;
  return FALSE;
}

/*!
  \relates SbDPPlane

  Check the two given planes for inequality.
*/
int
operator !=(const SbDPPlane & p1, const SbDPPlane & p2)
{
  return !(p1 == p2);
}

/*!
  Dump the state of this object to the \a fp file stream. Only works in
  debug version of library, method does nothing in an optimized build.
*/
void
SbDPPlane::print(FILE * fp) const
{
#if COIN_DEBUG
  this->getNormal().print(fp);
  (void)fprintf(fp, "  %f", this->getDistanceFromOrigin());
#endif // COIN_DEBUG
}

#ifdef COIN_TEST_SUITE
#include <Inventor/SbDPLine.h>
#include <Inventor/SbDPPlane.h>
#include <Inventor/SbLine.h>
#include <Inventor/SbPlane.h>
#include <Inventor/SbTypeInfo.h>

#include <cfloat>
#include <algorithm>
#include <cmath>

using namespace SIM::Coin::TestSuite;

float slew(float Start, float End, int steps, int step) {
  const float S = log(Start<0?-Start:Start);
  const float E = log(End<0?-End:End);

  --steps;

  float res=0;

  assert(Start<End);

  if (Start<0 && End < 0 ) {
    res=-exp(S+step*(E-S)/steps);
  }
  else if(Start>0 && End>0) {
    res=exp(S+step*(E-S)/steps);
  }
  else {
    float ZeroPoint = S*steps/(E+S);
    if(step<ZeroPoint) {
      res=-exp(S-2*step*S/ZeroPoint);
    }
    else if (step==ZeroPoint) {
      res=0;
    }
    else {
      res=exp(2*E*(step-ZeroPoint)/(steps-ZeroPoint)-E);
    }
  }

  return res;
}

BOOST_AUTO_TEST_CASE(signCorrect)
{
  SbDPPlane plane1(SbVec3d(0.0, 0.0, 1.0), 3.0);
  SbDPPlane plane2(SbVec3d(1.0, 0.0, 0.0), 21.0);
  SbDPLine line;
  plane1.intersect(plane2, line); 

  SbVec3d intersect = line.getPosition();
  SbVec3d vec(21, 0, 3);

  check_compare(intersect,vec, "SbDPPlane SignCorrect", .1f);
}

BOOST_AUTO_TEST_CASE(equalityToFloatPlane)
{
  const float delX = 1;
  const float delY = .1f;

  const float XMax = (float)pow(2.,FLT_MAX_EXP/3.);
  const float XMin = -XMax;

  const float YMax = (float)pow(2.,FLT_MAX_EXP/3.);
  const float YMin = -YMax;

#ifdef TEST_SUITE_QUICK
  const int XSteps = 4;
  const int YSteps = 4;
#endif //TEST_SUITE_QUICK
#ifdef TEST_SUITE_THOROUG
  const int XSteps = 6;
  const int YSteps = 6;
#endif //TEST_SUITE_THOROUG
#ifdef TEST_SUITE_EXPANSIVE
  const int XSteps = 10;
  const int YSteps = 10;
#endif //TEST_SUITE_EXPANSIVE

  int count=0;
 
  for (int x1=0;x1<XSteps;++x1) {
    float X1=slew(XMin,XMax,XSteps,x1);
    for (int x2=0;x2<XSteps;++x2) {
      float X2=slew(XMin,XMax,XSteps,x2);
      for (int x3=0;x3<XSteps;++x3) {
        float X3=slew(XMin,XMax,XSteps,x3);
        SbVec3f fv1(X1,X2,X3);
        SbVec3d dv1(X1,X2,X3);
        
        for (int x4=0;x4<XSteps;++x4) {
          float X4=slew(XMin,XMax,XSteps,x4);

          SbPlane fp1(fv1,X4);
          SbDPPlane dp1(dv1,X4);

          check_compare(fp1.getDistanceFromOrigin(),dp1.getDistanceFromOrigin(), "Distance from origin differs", 64);
          check_compare(fp1.getNormal(),dp1.getNormal(),"Comparing normals yields different results",.000001f);
          for (int y1=0;y1<YSteps;++y1) {
            float Y1=slew(YMin,YMax,YSteps,y1);
            for (int y2=0;y2<YSteps;++y2) {
              float Y2=slew(YMin,YMax,YSteps,y2);
              for (int y3=0;y3<YSteps;++y3) {
                float Y3=slew(YMin,YMax,YSteps,y3);
                SbVec3f fv2(Y1,Y2,Y3);
                SbVec3d dv2(Y1,Y2,Y3);

                //A bit arbitrary, this holds
                const float tol = .03f;
                BOOST_CHECK_MESSAGE(
                                    floatEquals(fp1.getDistance(fv2),(float)dp1.getDistance(dv2),tol)||
                                    fabs(fp1.getDistance(fv2)-dp1.getDistance(dv2))/fabs(dp1.getDistanceFromOrigin())<tol,
                                    "Distance from plane is significantly different");
                for (int y4=0;y4<YSteps;++y4) {
                  float Y4=slew(YMin,YMax,YSteps,y3);
                  SbPlane fp2(fv2,Y4);
                  SbDPPlane dp2(dv2,Y4);

                  SbLine fLine;
                  SbDPLine dLine;
                  bool failed=false;
                  if (!fp1.intersect(fp2, fLine)) {
                    failed = true;
                  }
                  if (!dp1.intersect(dp2, dLine)) {
                    BOOST_CHECK_MESSAGE(failed,"Float intersection worked, but double intersection failed");
                    failed = true;
                  }
                  if (failed)
                    continue;


                  SbVec3f fDir(fLine.getDirection());
                  SbVec3d dDir(dLine.getDirection());

                  check_compare(fDir,dDir, "Intersection direction differs", .004f);
                }
              }
            }
          }
        }
      }
    }
  }
  
}

#endif //COIN_TEST_SUITE

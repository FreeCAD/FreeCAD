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
  \class SbXfBox3d SbXfBox3d.h Inventor/SbXfBox3d.h
  \brief The SbXfBox3d class is a 3 dimensional box with double precision coordinates and an attached transformation.

  \ingroup coin_base

  It provides storage for two box corners with double precision floating
  point coordinates, and for a double precision 4x4 transformation matrix.

  \sa SbBox3d, SbDPMatrix, SbXfBox3f
*/

#include <Inventor/SbXfBox3d.h>

#include <limits>

#include <Inventor/errors/SoDebugError.h>

// this value is used to signal an invalid inverse matrix
#define INVALID_TAG (std::numeric_limits<double>::max())

static SbVec3d
SbXfBox3d_get_scaled_span_vec(const SbXfBox3d & xfbox)
{
  const SbDPMatrix & m = xfbox.getTransform();

  // FIXME: is this really correct? Won't we get the wrong result if
  // there are rotations in the transformation matrix? 20020209 mortene.
  double scalex = sqrt(m[0][0] * m[0][0] +
                       m[1][0] * m[1][0] +
                       m[2][0] * m[2][0]);
  double scaley = sqrt(m[0][1] * m[0][1] +
                       m[1][1] * m[1][1] +
                       m[2][1] * m[2][1]);
  double scalez = sqrt(m[0][2] * m[0][2] +
                       m[1][2] * m[1][2] +
                       m[2][2] * m[2][2]);

  SbVec3d min, max;
  xfbox.getBounds(min, max);

  return SbVec3d((max[0] - min[0]) * scalex,
                 (max[1] - min[1]) * scaley,
                 (max[2] - min[2]) * scalez);
}


/*!
  The default constructor makes an empty box and identity matrix.
*/
SbXfBox3d::SbXfBox3d(void)
{
  this->matrix.makeIdentity();
  this->invertedmatrix.makeIdentity();
}

/*!
  Constructs a box with the given corners.

  The coordinates of \a min should be less than the coordinates of
  \a max if you want to make a valid box.
 */
SbXfBox3d::SbXfBox3d(const SbVec3d & boxmin, const SbVec3d & boxmax)
  : inherited(boxmin, boxmax)
{
  this->matrix.makeIdentity();
  this->invertedmatrix.makeIdentity();
}

/*!
  Constructs a box from the given SbBox3d.

  The transformation is set to the identity matrix.
*/
SbXfBox3d::SbXfBox3d(const SbBox3d & box)
  : inherited(box)
{
  this->matrix.makeIdentity();
  this->invertedmatrix.makeIdentity();
}

/*!
  Overridden from SbBox3d, as the transformations are to be kept
  separate from the box in the SbXfBox3d class.
 */
void
SbXfBox3d::transform(const SbDPMatrix & m)
{
  this->setTransform(this->matrix.multRight(m));
}

/*!
  Sets the transformation to the given SbMatrix.
*/
void
SbXfBox3d::setTransform(const SbDPMatrix & m)
{
  this->matrix = m;
  this->makeInvInvalid(); // invalidate current inverse
}

/*!
  \fn const SbDPMatrix & SbXfBox3d::getTransform(void) const
  Returns the current transformation matrix.
*/

/*!
  Returns the inverse of the current transformation matrix.
*/
const SbDPMatrix &
SbXfBox3d::getInverse(void) const
{
  this->calcInverse();
  return this->invertedmatrix;
}

/*!
  Return the transformed center point of the box.
*/
SbVec3d
SbXfBox3d::getCenter(void) const
{
  SbVec3d center = SbBox3d::getCenter();
  this->matrix.multVecMatrix(center, center);
  return center;
}

/*!
  Extend the boundaries of the box by the given point, i.e. make the
  point fit inside the box if it isn't already so.

  The point is assumed to be in transformed space.
*/
void
SbXfBox3d::extendBy(const SbVec3d & pt)
{
  if (this->isEmpty()) {
    this->matrix.makeIdentity();
    this->invertedmatrix.makeIdentity();
  }

  const SbDPMatrix & im = this->getInverse();
  SbVec3d trans;
  im.multVecMatrix(pt, trans);
  SbBox3d::extendBy(trans);
}

/*!
  Extend the boundaries of the box by the given \a bb parameter.
  The given box is assumed to be in transformed space.

  The two given boxes will be combined in such a way so that the resultant
  bounding box always has the smallest possible volume. To accomplish this,
  the transformation on this SbXfBox3f will sometimes be flattened before
  it is combined with \a bb.
*/
void
SbXfBox3d::extendBy(const SbBox3d & bb)
{
#if COIN_DEBUG
  if (bb.isEmpty()) {
    SoDebugError::postWarning("SbXfBox3f::extendBy",
                              "Extending box is empty.");
    return;
  }
#endif // COIN_DEBUG

  if (this->isEmpty()) {
    *this = bb;
    this->matrix.makeIdentity();
    this->invertedmatrix.makeIdentity();
    return;
  }

  SbVec3d points[2] = { bb.getMin(), bb.getMax() };

  // Combine bboxes while keeping the transformation matrix.
  SbBox3d box1 = *this;
  {
    SbDPMatrix im = this->getInverse();
    // Transform all the corners and include them into the new box.
    for (int i=0; i < 8; i++) {
      SbVec3d corner, dst;
      // Find all corners the "binary" way :-)
      corner.setValue(points[(i&4)>>2][0],
                      points[(i&2)>>1][1],
                      points[i&1][2]);
      // Don't try to optimize the transformation out of the loop,
      // it's not as easy as it seems.
      im.multVecMatrix(corner, dst);
#if 0 // debug
      SoDebugError::postInfo("SbXfBox3f::extendBy",
                             "point: <%f, %f, %f> -> <%f, %f, %f>",
                             corner[0], corner[1], corner[2],
                             dst[0], dst[1], dst[2]);
#endif // debug
      box1.extendBy(dst);
    }
  }


  // Combine bboxes with a flattened transformation matrix.
  SbBox3d box2 = this->project();
  {
    for (int j=0;j<8;j++) {
      SbVec3d corner;
      corner.setValue(points[(j&4)>>2][0],
                      points[(j&2)>>1][1],
                      points[j&1][2]);
      box2.extendBy(corner);
    }
  }

  SbXfBox3d xfbox(box1);
  xfbox.setTransform(this->matrix);
#if 0 // debug
  SoDebugError::postInfo("SbXfBox3f::extendBy",
                         "kintel-volume: %f, mortene-volume: %f",
                         xfbox.getVolume(), box2.getVolume());
#endif // debug

  // Choose result from one of the two techniques based on the volume
  // of the resultant bbox.
  SbBool firstsmaller;
  double vol1 = xfbox.getVolume(), vol2 = box2.getVolume();
  if ((vol1 != 0.0f) || (vol2 != 0.0f)) {
    firstsmaller = (vol1 < vol2);
  }
  // If one dimension has zero span, we need to compare area (or
  // length, if two dimensions have zero span).
  else {
    SbVec3d s1 = SbXfBox3d_get_scaled_span_vec(xfbox);
    SbVec3d s2 = SbXfBox3d_get_scaled_span_vec(box2);

    double v1 = fabs((s1[0] != 0.0f ? s1[0] : 1.0f) *
                     (s1[1] != 0.0f ? s1[1] : 1.0f) *
                     (s1[2] != 0.0f ? s1[2] : 1.0f));
    double v2 = fabs((s2[0] != 0.0f ? s2[0] : 1.0f) *
                     (s2[1] != 0.0f ? s2[1] : 1.0f) *
                     (s2[2] != 0.0f ? s2[2] : 1.0f));

    firstsmaller = (v1 < v2);
  }

  if (firstsmaller) {
    this->setBounds(box1.getMin(), box1.getMax());
  }
  else {
    this->setBounds(box2.getMin(), box2.getMax());
    this->matrix.makeIdentity();
    this->invertedmatrix.makeIdentity();
  }
}

/*!
  Extend the boundaries of the box by the given \a bb parameter.

  The given box is assumed to be in transformed space.

  Note: is not guaranteed to give an optimal result if used for bounding box
  calculation since the transformation matrix might change. See
  documentation in SoGetBoundingBoxAction for more details.
*/
void
SbXfBox3d::extendBy(const SbXfBox3d & bb)
{
#if COIN_DEBUG
  if (bb.isEmpty()) {
    SoDebugError::postWarning("SbXfBox3f::extendBy",
                              "Extending box is empty.");
    return;
  }
#endif // COIN_DEBUG

  if (this->isEmpty()) {
    *this = bb;
    return;
  }

#if 0 // debug
  SoDebugError::postInfo("SbXfBox3d::extendBy",
                         "bb: <%f, %f, %f>, <%f, %f, %f>",
                         bb.getMin()[0],
                         bb.getMin()[1],
                         bb.getMin()[2],
                         bb.getMax()[0],
                         bb.getMax()[1],
                         bb.getMax()[2]);
#endif // debug

  // Try extending while keeping the transform on "this" first.
  SbXfBox3d box1 = *this;
  {
    SbVec3d points[2] = { bb.getMin(), bb.getMax() };
    {
      SbDPMatrix m = bb.getTransform();
      m.multRight(box1.getInverse());

      for (int i=0; i < 8; i++) {
        SbVec3d corner, dst;
        corner.setValue(points[(i&4)>>2][0],
                        points[(i&2)>>1][1],
                        points[i&1][2]);
        m.multVecMatrix(corner, dst);
#if 0 // debug
        SoDebugError::postInfo("SbXfBox3d::extendBy",
                               "corner: <%f, %f, %f>, dst <%f, %f, %f>",
                               corner[0], corner[1], corner[2],
                               dst[0], dst[1], dst[2]);
#endif // debug
        static_cast<SbBox3d *>(&box1)->extendBy(dst);
#if 0 // debug
        SoDebugError::postInfo("SbXfBox3d::extendBy",
                               "dst: <%f, %f, %f>  ->   "
                               "box1: <%f, %f, %f>, <%f, %f, %f>",
                               dst[0], dst[1], dst[2],
                               box1.getMin()[0],
                               box1.getMin()[1],
                               box1.getMin()[2],
                               box1.getMax()[0],
                               box1.getMax()[1],
                               box1.getMax()[2]);
#endif // debug
      }
    }
  }

  // Try extending while keeping the transform on bb.
  SbXfBox3d box2 = bb;
  {
    SbVec3d points[2] = { this->getMin(), this->getMax() };
    {
      SbDPMatrix m = this->getTransform();
      m.multRight(box2.getInverse());

      for (int i=0; i < 8; i++) {
        SbVec3d corner, dst;
        corner.setValue(points[(i&4)>>2][0],
                        points[(i&2)>>1][1],
                        points[i&1][2]);
        m.multVecMatrix(corner, dst);
#if 0 // debug
        SoDebugError::postInfo("SbXfBox3d::extendBy",
                               "corner: <%f, %f, %f>, dst <%f, %f, %f>",
                               corner[0], corner[1], corner[2],
                               dst[0], dst[1], dst[2]);
#endif // debug
        static_cast<SbBox3d *>(&box2)->extendBy(dst);
#if 0 // debug
        SoDebugError::postInfo("SbXfBox3d::extendBy",
                               "dst: <%f, %f, %f>  ->   "
                               "box2: <%f, %f, %f>, <%f, %f, %f>",
                               dst[0], dst[1], dst[2],
                               box2.getMin()[0],
                               box2.getMin()[1],
                               box2.getMin()[2],
                               box2.getMax()[0],
                               box2.getMax()[1],
                               box2.getMax()[2]);
#endif // debug
      }
    }
  }

#if 0 // debug
  SoDebugError::postInfo("SbXfBox3d::extendBy",
                         "box1-volume: %f, box2-volume: %f",
                         box1.getVolume(), box2.getVolume());
#endif // debug

  // Compare volumes and pick the smallest bounding box.
  SbBool firstsmaller;
  double vol1 = box1.getVolume(), vol2 = box2.getVolume();
  if ((vol1 != 0.0f) || (vol2 != 0.0f)) {
    firstsmaller = (vol1 < vol2);
  }
  // If one dimension has zero span, we need to compare area (or
  // length, if two dimensions have zero span).
  else {
    SbVec3d s1 = SbXfBox3d_get_scaled_span_vec(box1);
    SbVec3d s2 = SbXfBox3d_get_scaled_span_vec(box2);

    double v1 = fabs((s1[0] != 0.0f ? s1[0] : 1.0f) *
                     (s1[1] != 0.0f ? s1[1] : 1.0f) *
                     (s1[2] != 0.0f ? s1[2] : 1.0f));
    double v2 = fabs((s2[0] != 0.0f ? s2[0] : 1.0f) *
                     (s2[1] != 0.0f ? s2[1] : 1.0f) *
                     (s2[2] != 0.0f ? s2[2] : 1.0f));

    firstsmaller = (v1 < v2);
  }
  *this = (firstsmaller ? box1 : box2);
}

/*!
  Check if the given point lies within the boundaries of this box.

  The point is assumed to be in transformed space.
*/
SbBool
SbXfBox3d::intersect(const SbVec3d & pt) const
{
  this->calcInverse();
  SbVec3d trans;
  this->invertedmatrix.multVecMatrix(pt, trans);
  return SbBox3d::intersect(trans);
}

//
// tests for intersection between an axis aligned box and the
// 12 edges defined by the 8 points in the 'points' array.
//
static
SbBool intersect_box_edges(const SbVec3d & min,
                           const SbVec3d & max,
                           const SbVec3d * const points)
{
  // lookup table for edges in the cube.
  static int lines[12*2] =
  {
    0,1,
    0,2,
    0,4,
    1,3,
    1,5,
    2,3,
    2,6,
    3,7,
    4,5,
    4,6,
    5,7,
    6,7
  };

  // need this for the innermost loop
  SbVec3d boxpts[2];
  boxpts[0] = min;
  boxpts[1] = max;

  // test for edge intersection
  for (int i = 0; i < 12; i++) { // 12 edges in a cube
    SbVec3d l1 = points[lines[i*2]];
    SbVec3d l2 = points[lines[i*2+1]];
    // possible optimization: reuse directional vectors
    SbVec3d dir = l2 - l1;
    // if the direction is a nil-vector, this means that the bounding
    // box is flat (2D or 1D) or empty and we can just skip this vector.
    if (dir.normalize() == 0.0) continue;
    SbVec3d lmin(SbMin(l1[0], l2[0]),
                 SbMin(l1[1], l2[1]),
                 SbMin(l1[2], l2[2]));
    SbVec3d lmax(SbMax(l1[0], l2[0]),
                 SbMax(l1[1], l2[1]),
                 SbMax(l1[2], l2[2]));

    // the bbox to test against is axis-aligned, and this makes it
    // quite simple.
    for (int j = 0; j < 3; j++) { // test planes in all three dimensions
      for (int k = 0; k < 2; k++) { // test both min and max planes
        // check if line crosses current plane
        if (dir[j] != 0.0f &&
            lmin[j] <= boxpts[k][j] && lmax[j] >= boxpts[k][j]) {
          // find the two other coordinates
          int t1 = j+1;
          int t2 = j+2;
          // do this instead of modulo 3
          if (t1 >= 3) t1 -= 3;
          if (t2 >= 3) t2 -= 3;

          // find what we need to multiply coordinate j by to
          // put it onto the current plane
          double delta = fabs((boxpts[k][j] - l1[j]) / dir[j]);
          // calculate the two other coordinates
          double v1 = l1[t1] + delta*dir[t1];
          double v2 = l1[t2] + delta*dir[t2];
          if (v1 > boxpts[0][t1] && v1 < boxpts[1][t1] &&
              v2 > boxpts[0][t2] && v2 < boxpts[1][t2]) {
            return TRUE;
          }
        }
      }
    }
  }
  return FALSE;
}

//
// weak box-box intersection test: min, max defines an axis-aligned
// box, while boxmin, boxmax defines an box that should be transformed
// by matrix. This function only tests whether any of the 8
// (transformed) points in (boxmin, boxmax) is inside (min, max),
// and if any of the 12 edges in (boxmin, boxmax) intersects any of the
// planes in the box defined by (min, max).
//
// Use this function twice to cover all intersection cases.
//
static SbBool
intersect_box_box(const SbVec3d & min,
                  const SbVec3d & max,
                  const SbVec3d & boxmin,
                  const SbVec3d & boxmax,
                  const SbDPMatrix & matrix,
                  SbBool & alignedIntersect)
{
  SbVec3d transpoints[8];
  SbBox3d alignedBox;
  for (int i = 0;  i < 8; i++) {
    SbVec3d tmp, tmp2;
    tmp.setValue((i&4) ? boxmin[0] : boxmax[0],
                 (i&2) ? boxmin[1] : boxmax[1],
                 (i&1) ? boxmin[2] : boxmax[2]);
    matrix.multVecMatrix(tmp, tmp2);
    // is point inside
    if (tmp2[0] >= min[0] &&
        tmp2[0] <= max[0] &&
        tmp2[1] >= min[1] &&
        tmp2[1] <= max[1] &&
        tmp2[2] >= min[2] &&
        tmp2[2] <= max[2]) {
      return TRUE;
    }
    alignedBox.extendBy(tmp2);
    transpoints[i] = tmp2;
  }
  // this is just an optimization:
  // if the axis aligned box doesn't intersect the box, there
  // is no chance for any intersection.
  SbBox3d thisbox(min, max);
  alignedIntersect = thisbox.intersect(alignedBox);

  // only test edge intersection if aligned boxes intersect
  if (alignedIntersect)
    return intersect_box_edges(min, max, transpoints);
  return FALSE;
}

/*!
  Check if the given \a box lies entirely or partially within the boundaries
  of this box.

  The given box is assumed to be in transformed space.
*/
SbBool
SbXfBox3d::intersect(const SbBox3d & bb) const
{
  if (this->isEmpty() || bb.isEmpty()) {
#if COIN_DEBUG
    SoDebugError::postWarning("SbXfBox3d::intersect",
                              "%s is an empty / uninitialized box",
                              this->isEmpty() ? "this" : "input argument");
#endif // COIN_DEBUG
    return FALSE;
  }

  if (this->matrix == SbDPMatrix::identity()) return SbBox3d::intersect(bb);

  //
  // do double-test to get all intersection cases
  //
  SbBool alignedIntersect;

  if (intersect_box_box(bb.getMin(), bb.getMax(),
                        this->getMin(), this->getMax(),
                        this->matrix, alignedIntersect)) return TRUE;

  if (!alignedIntersect) return FALSE;

  // will need the inverse matrix here
  this->calcInverse();
  return intersect_box_box(this->getMin(), this->getMax(),
                           bb.getMin(), bb.getMax(),
                           this->invertedmatrix,
                           alignedIntersect);
}

/*!
  Check if two transformed boxes intersect.

  \COIN_FUNCTION_EXTENSION

  \since Coin 2.0
*/

SbBool
SbXfBox3d::intersect(const SbXfBox3d & xfbb) const
{
  const SbBox3d & bbr = xfbb;
  SbBox3d bb(bbr);
  SbXfBox3d me(*this);
  me.transform(xfbb.getInverse());
  return me.intersect(bb);
}


/*!
  Find the span of the box in the given direction (i.e. how much room
  in the given direction the box needs). The distance is returned as
  the minimum and maximum distance from origo to the closest and
  furthest plane defined by the direction vector and each of the box'
  corners. The difference between these values gives the span.
*/
void
SbXfBox3d::getSpan(const SbVec3d & direction, double & dMin, double & dMax) const
{
  this->project().getSpan(direction, dMin, dMax);
}

/*!
  Project the SbXfBox3d into a SbBox3d.

  This gives the same resulting SbBox3d as doing a SbBox3d::transform()
  with this transformation matrix as parameter.
*/
SbBox3d
SbXfBox3d::project(void) const
{
  SbBox3d box(this->getMin(), this->getMax());
  if (!box.isEmpty()) box.transform(this->matrix);
  return box;
}

/*!
  Check if \a b1 and \a b2 are equal. Return 1 if they are equal,
  or 0 if they are not equal. Note that the method will do a dumb
  component by component comparison.
*/
int
operator == (const SbXfBox3d & b1, const SbXfBox3d & b2)
{
  const SbBox3d & box1 = b1;
  const SbBox3d & box2 = b2;
  return (box1.getMin() == box2.getMin()) &&
         (box1.getMax() == box2.getMax()) &&
         (b1.getTransform() == b2.getTransform());
}

/*!
  Check if \a b1 and \a b2 are not equal. Return 0 if they are equal,
  or 1 if they are not equal. See the note on operator==().
 */
int
operator != (const SbXfBox3d & b1, const SbXfBox3d & b2)
{
  return !(b1 == b2);
}

/*!
  Return box volume. Overridden from parent class to take into account
  the possibility of scaling in the transformation matrix.
*/
double
SbXfBox3d::getVolume(void) const
{
  if (!this->hasVolume()) return 0.0f;
  
  // The determinant of the upper-left 3x3 matrix can be used to
  // calculate the volume of the transformed box.
  //
  // By Doctor Tom at the Math Forum:
  // ----------------------------------------------------------------
  // <URL:http://mathforum.org/dr.math/problems/carlino11.16.97.html>
  //
  // Date: 11/17/97 at 19:57:10 
  // From: Doctor Tom 
  // Subject: Re:Explaining the determinant 
  // 
  // Hello Jeremy, 
  //
  // I always think of it geometrically. Let's look in
  // two dimensions, at the determinant of the following: 
  //
  //    | x0 y0 | = x0*y1 - x1*y0 
  //    | x1 y1 | 
  //
  // Now imagine the two vectors (x0, y0) and (x1, y1) drawn in the
  // x-y plane from the origin. If you consider them to be two sides
  // of a parallelogram, then the determinant is the area of the
  // parallelogram.  Well, not exactly the area, the "signed" area,
  // in the sense that if you sweep the area clockwise, you get one
  // sign, and the opposite sign if you sweep it in the other
  // direction. It's just as useful a concept as considering area
  // below the x-axis as negative in your calculus course. Swapping
  // the vectors swaps the sign, in the same way that swapping the
  // rows of the determinant swaps the sign. In one dimension, the
  // determinant is just the number, but if you "plot" that number on
  // a number line, it's the (signed) length of the line. If it goes
  // in the positive direction from the origin, it is positive, and
  // negative otherwise. In three dimensions, consider three vectors
  // (x0,y0,z0), (x1,y1,z1), and (x2,y2,z2). If you draw them from
  // the origin, they form the principle edges of a parallelepiped,
  // and the determinant of: 
  //
  //    | x0 y0 z0 | 
  //    | x1 y1 z1 | 
  //    | x2 y2 z2 |
  //
  //  is the volume of that parallelepiped.
  // --------------------------------------------------------------
  //
  // this means that the determinant is the volume of a unit size cube
  // in the coordinate system specified by a 3x3 matrix, and that we
  // can find the volume of our box by multiplying the volume of the
  // orthogonal box with the determinant of the upper-left 3x3 matrix.

  double volume = (SbBox3d::getVolume() * this->matrix.det3());

  // The determinant might be negative if e.g. negative scaling has
  // been performed on the matrix. To rectify this, we make sure the
  // returned volume is positive.
  return (volume > 0) ? volume : -volume;
}

void
SbXfBox3d::calcInverse(void) const
{
  // det4() is checked against VALID_LIMIT to determine if the inverse
  // matrix can be calculated.
  const float VALID_LIMIT = (1.0e-12f);
  
  if (this->invertedmatrix[0][0] == INVALID_TAG) {
    if (SbAbs(this->matrix.det4()) > VALID_LIMIT) {
      this->invertedmatrix = this->matrix.inverse();
    }
    else {
#if COIN_DEBUG && 0 // disabled
      const SbDPMatrix & m = this->matrix;
      SoDebugError::postWarning("SbXfBox3d::setTransform",
                                "invalid matrix (can't be inverted)");
      SoDebugError::postWarning("SbXfBox3d::setTransform",
                                "%f %f %f %f",
                                m[0][0], m[0][1], m[0][2], m[0][3]);
      SoDebugError::postWarning("SbXfBox3d::setTransform",
                                "%f %f %f %f",
                                m[1][0], m[1][1], m[1][2], m[1][3]);
      SoDebugError::postWarning("SbXfBox3d::setTransform",
                                "%f %f %f %f",
                                m[2][0], m[2][1], m[2][2], m[2][3]);
      SoDebugError::postWarning("SbXfBox3d::setTransform",
                                "%f %f %f %f",
                                m[3][0], m[3][1], m[3][2], m[3][3]);
#endif // COIN_DEBUG

      // Degenerate transforms are fixed by projecting box. This will
      // transform the min and max points (using the normal matrix,
      // not the inverse), and leave us with an identity transform.
      SbXfBox3d * thisp = const_cast<SbXfBox3d *>(this);
      *thisp = SbXfBox3d(this->project());

      // FIXME: this degenerate-transform fix looks like bad
      // engineering. It's the caller who does something wrong when
      // combining transforms into SbXfBox3d to make a non-inversible
      // matrix. This will for instance happen when calculating bboxes
      // for a scene with scale transforms with 0 components.
      // 20010627 mortene.
    }
  }
}

void
SbXfBox3d::makeInvInvalid(void)
{
  this->invertedmatrix[0][0] = INVALID_TAG;
}

#undef INVALID_TAG

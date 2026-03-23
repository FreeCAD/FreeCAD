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
  \class SbDPViewVolume SbDPViewVolume.h Inventor/SbDPViewVolume.h
  \brief The SbDPViewVolume class is a double precision viewing volume in 3D space.

  \ingroup coin_base

  This class contains the necessary information for storing a view
  volume.  It has methods for projection of primitives from or into
  the 3D volume, doing camera transforms, view volume transforms etc.

  \COIN_CLASS_EXTENSION

  \sa SbViewportRegion
  \since Coin 2.0
*/

#include <Inventor/SbDPViewVolume.h>
#include <Inventor/SbViewVolume.h>
#include <Inventor/SbDPRotation.h>
#include <Inventor/SbDPLine.h>
#include <Inventor/SbLine.h>
#include <Inventor/SbDPMatrix.h>
#include <Inventor/SbPlane.h>
#include <Inventor/SbBox2f.h>
#include <Inventor/SbBox3f.h>
#include <Inventor/SbVec2d.h>
#include <Inventor/SbVec3d.h>
#include <Inventor/SbDPPlane.h>
#if COIN_DEBUG
#include <Inventor/errors/SoDebugError.h>
#endif // COIN_DEBUG

#include <coindefs.h> // COIN_OBSOLETED()
#include <cassert>

/*!
  \enum SbDPViewVolume::ProjectionType

  An SbDPViewVolume instance can represent either an orthogonal projection
  volume or a perspective projection volume.

  \sa ortho(), perspective(), getProjectionType().
*/

/*!
  \var SbDPViewVolume::ProjectionType SbDPViewVolume::ORTHOGRAPHIC
  Orthographic projection.
*/

/*!
  \var SbDPViewVolume::ProjectionType SbDPViewVolume::PERSPECTIVE
  Perspective projection.
*/

/*!
  \var SbDPViewVolume::ProjectionType SbDPViewVolume::type
  \COININTERNAL
*/

/*!
  \var SbVec3d SbDPViewVolume::projPoint
  \COININTERNAL
*/

/*!
  \var SbVec3d SbDPViewVolume::projDir
  \COININTERNAL
*/

/*!  
  \var double SbDPViewVolume::nearDist
  \COININTERNAL
*/

/*!
  \var double SbDPViewVolume::nearToFar
  \COININTERNAL
*/

/*!
  \var SbVec3d SbDPViewVolume::llf
  \COININTERNAL
*/

/*!
  \var SbVec3d SbDPViewVolume::lrf
  \COININTERNAL
*/

/*! 
  \var SbVec3d SbDPViewVolume::ulf
  \COININTERNAL
*/

//
// some convenience function for converting between single precision
// and double precision classes.
//
static SbVec3f 
dp_to_sbvec3f(const SbVec3d & v)
{
  return SbVec3f(static_cast<float>(v[0]), static_cast<float>(v[1]), static_cast<float>(v[2]));
}

static SbVec3d 
dp_to_sbvec3d(const SbVec3f & v)
{
  return SbVec3d(static_cast<double>(v[0]), static_cast<double>(v[1]), static_cast<double>(v[2]));
}

/*!
  Constructor. Note that the SbDPViewVolume instance will be uninitialized
  until you explicitly call \a ortho() or \a perspective().

  \sa ortho(), perspective().
 */
SbDPViewVolume::SbDPViewVolume(void)
{
}

/*!
  Destructor.
 */
SbDPViewVolume::~SbDPViewVolume(void)
{
}

// Perspective projection matrix. From the "OpenGL Programming Guide,
// release 1", Appendix G (but with row-major mode).
static SbDPMatrix
get_perspective_projection(const double rightminusleft, const double rightplusleft,
                           const double topminusbottom, const double topplusbottom,
                           const double nearval, const double farval)
{
#if COIN_DEBUG
  if (nearval * farval <= 0.0) {
    SoDebugError::postWarning("SbDPViewVolume::get_perspective_projection",
                              "Projection frustum crosses zero. Rendering is unpredictable.");
  }
#endif // COIN_DEBUG
  SbDPMatrix proj;

  proj[0][0] = 2.0*nearval/rightminusleft;
  proj[0][1] = 0.0;
  proj[0][2] = 0.0;
  proj[0][3] = 0.0;
  proj[1][0] = 0.0;
  proj[1][1] = 2.0*nearval/topminusbottom;
  proj[1][2] = 0.0;
  proj[1][3] = 0.0;
  proj[2][0] = rightplusleft/rightminusleft;
  proj[2][1] = topplusbottom/topminusbottom;
  proj[2][2] = -(farval+nearval)/(farval-nearval);
  proj[2][3] = -1.0;
  proj[3][0] = 0.0;
  proj[3][1] = 0.0;
  proj[3][2] = -2.0*farval*nearval/(farval-nearval);
  proj[3][3] = 0.0;

  // special handling for reverse perspective projection (see SoPerspectiveCamera documentation)
  if (nearval < 0.0) {
    // OpenGL performs clipping in homogeneous space (before computing the perspective division).
    // i.e. instead of testing for -1 <= z/w <= +1, it checks for -w <= z <= +w. Both conditions
    // are only equivalent if w > 0.
    // In the reverse perspective case the projection matrix above leads to negative w values,
    // but this can be compensated by multiplying the whole matrix by -1.
    proj[0][0] *= -1.0;
    proj[1][1] *= -1.0;
    proj[2][0] *= -1.0;
    proj[2][1] *= -1.0;
    proj[2][2] *= -1.0;
    proj[2][3] *= -1.0;
    proj[3][2] *= -1.0;
  }

  return proj;
}


// Orthographic projection matrix. From the "OpenGL Programming Guide,
// release 1", Appendix G (but with row-major mode).
static SbDPMatrix
get_ortho_projection(const double rightminusleft, const double rightplusleft,
                     const double topminusbottom, const double topplusbottom,
                     const double nearval, const double farval)
{
  SbDPMatrix proj;
  proj[0][0] = 2.0/rightminusleft;
  proj[0][1] = 0.0;
  proj[0][2] = 0.0;
  proj[0][3] = 0.0;
  proj[1][0] = 0.0;
  proj[1][1] = 2.0/topminusbottom;
  proj[1][2] = 0.0;
  proj[1][3] = 0.0;
  proj[2][0] = 0.0;
  proj[2][1] = 0.0;
  proj[2][2] = -2.0/(farval-nearval);
  proj[2][3] = 0.0;
  proj[3][0] = -rightplusleft/rightminusleft;
  proj[3][1] = -topplusbottom/topminusbottom;
  proj[3][2] = -(farval+nearval)/(farval-nearval);
  proj[3][3] = 1.0;

  return proj;

}


/*!
  Returns the view volume's affine matrix and projection matrix.

  \sa getMatrix(), getCameraSpaceMatrix()
 */
void
SbDPViewVolume::getMatrices(SbDPMatrix& affine, SbDPMatrix& proj) const
{
  SbVec3d upvec = this->ulf - this->llf;
#if COIN_DEBUG
  if (upvec == SbVec3d(0.0, 0.0, 0.0)) {
    SoDebugError::postWarning("SbDPViewVolume::getMatrices",
                              "empty frustum!");
    affine = SbDPMatrix::identity();
    proj = SbDPMatrix::identity();
    return;
  }
#endif // COIN_DEBUG
  SbVec3d rightvec = this->lrf - this->llf;

#if COIN_DEBUG
  if (rightvec == SbVec3d(0.0, 0.0, 0.0)) {
    SoDebugError::postWarning("SbDPViewVolume::getMatrices",
                              "empty frustum!");
    affine = SbDPMatrix::identity();
    proj = SbDPMatrix::identity();
    return;
  }
#endif // COIN_DEBUG

  // we test vectors above, just normalize
  (void) upvec.normalize();
  (void) rightvec.normalize();

  // build matrix that will transform into camera coordinate system
  SbDPMatrix mat;
  mat[0][0] = rightvec[0];
  mat[0][1] = rightvec[1];
  mat[0][2] = rightvec[2];
  mat[0][3] = 0.0f;

  mat[1][0] = upvec[0];
  mat[1][1] = upvec[1];
  mat[1][2] = upvec[2];
  mat[1][3] = 0.0f;

  mat[2][0] = -this->projDir[0];
  mat[2][1] = -this->projDir[1];
  mat[2][2] = -this->projDir[2];
  mat[2][3] = 0.0f;

  mat[3][0] = this->projPoint[0];
  mat[3][1] = this->projPoint[1];
  mat[3][2] = this->projPoint[2];
  mat[3][3] = 1.0f;

  // the affine matrix is the inverse of the camera coordinate system
  affine = mat.inverse();

  // rotate frustum points back to an axis-aligned view volume to
  // calculate parameters for the projection matrix
  SbVec3d nlrf, nllf, nulf;

  affine.multDirMatrix(this->lrf, nlrf);
  affine.multDirMatrix(this->llf, nllf);
  affine.multDirMatrix(this->ulf, nulf);

  double rml = nlrf[0] - nllf[0];
  double rpl = nlrf[0] + nllf[0];
  double tmb = nulf[1] - nllf[1];
  double tpb = nulf[1] + nllf[1];
  double n = this->getNearDist();
  double f = n + this->getDepth();

#if COIN_DEBUG
  if (rml <= 0.0f || tmb <= 0.0f || n >= f) {
    SoDebugError::postWarning("SbDPViewVolume::getMatrices",
                              "invalid frustum");
    proj = SbDPMatrix::identity();
    return;
  }
#endif // COIN_DEBUG


  if(this->type == SbDPViewVolume::ORTHOGRAPHIC)
    proj = get_ortho_projection(rml, rpl, tmb, tpb, n, f);
  else
    proj = get_perspective_projection(rml, rpl, tmb, tpb, n, f);
}

/*!
  Returns the combined affine and projection matrix.

  \sa getMatrices(), getCameraSpaceMatrix()
 */
SbDPMatrix
SbDPViewVolume::getMatrix(void) const
{
  SbDPMatrix affine, proj;
  this->getMatrices(affine, proj);
  return affine.multRight(proj);
}

/*!
  Returns a matrix which will translate the view volume camera back to
  origo, and rotate the camera so it'll point along the negative Z-axis.

  Note that the matrix will \a not include the rotation necessary to
  make the camera up vector point along the positive Y-axis (i.e.
  camera roll is not accounted for).

  \sa getMatrices(), getMatrix()
 */
SbDPMatrix
SbDPViewVolume::getCameraSpaceMatrix(void) const
{
  // Find rotation of projection direction.
  SbDPRotation pdrot =
    SbDPRotation(this->projDir, SbVec3d(0.0f, 0.0f, -1.0f));

  // Combine transforms.
  SbDPMatrix mat, tmp;
  mat.setTranslate(-this->projPoint);
  tmp.setRotate(pdrot);
  mat.multRight(tmp);

  return mat;
}

/*!
  Project the given 2D point from the projection plane into a 3D line.

  \a pt coordinates should be normalized to be within [0, 1].
 */
void
SbDPViewVolume::projectPointToLine(const SbVec2d& pt, SbDPLine& line) const
{
  SbVec3d pt0, pt1;
  this->projectPointToLine(pt, pt0, pt1);
  line.setValue(pt0, pt1);
}

/*!
  Project the given 2D point from the projection plane into two points
  defining a 3D line. The first point, \a line0, will be the
  corresponding point for the projection on the near plane, while \a line1
  will be the line endpoint, lying in the far plane.
 */
void
SbDPViewVolume::projectPointToLine(const SbVec2d& pt,
                                   SbVec3d & line0, SbVec3d & line1) const
{
  SbVec3d dx = this->lrf - this->llf;
  SbVec3d dy = this->ulf - this->llf;

#if COIN_DEBUG
  if (dx.sqrLength() == 0.0f || dy.sqrLength() == 0.0f) {
    SoDebugError::postWarning("SbDPViewVolume::projectPointToLine",
                              "invalid frustum");
    return;
  }
#endif // COIN_DEBUG

  line0 = this->projPoint + this->llf + dx*pt[0] + dy*pt[1];
  SbVec3d dir;
  if (this->type == PERSPECTIVE) {
    dir = line0 - this->projPoint;
    // a null vector is ok here, just normalize
    (void) dir.normalize();
    line1 = line0 + dir * this->getDepth() / dir.dot(this->projDir);
  }
  else {
    dir = this->projDir;
    line1 = line0 + dir*this->getDepth();
  }
}

/*!
  Project the \a src point to a normalized set of screen coordinates in
  the projection plane and place the result in \a dst.

  It is safe to let \a src and \a dst be the same SbVec3d instance.
  
  The z-coordinate of \a dst is monotonically increasing for points
  closer to the far plane. Note however that this is not a linear
  relationship, the \a dst z-coordinate is calculated as follows:
  
  Orthogonal view:  DSTz = (-2 * SRCz - far - near) / (far - near),
  Perspective view:  DSTz = (-SRCz * (far - near) - 2*far*near) / (far - near)

  The returned coordinates (\a dst) are normalized to be in range [0, 1].
*/
void
SbDPViewVolume::projectToScreen(const SbVec3d& src, SbVec3d& dst) const
{
  this->getMatrix().multVecMatrix(src, dst);
  
  // coordinates are in range [-1, 1], normalize to [0,1]
  dst *= 0.5f;
  dst += SbVec3d(0.5f, 0.5f, 0.5f);
}

/*!
  Returns an SbPlane instance which has a normal vector in the opposite
  direction of which the camera is pointing. This means the
  plane will be parallel to the near and far clipping planes.

  \sa getSightPoint()
 */
SbPlane
SbDPViewVolume::getPlane(const double distFromEye) const
{
  return SbPlane(dp_to_sbvec3f(-this->projDir),
                 dp_to_sbvec3f(this->projPoint + distFromEye * this->projDir));
}

/*!
  Returns the point on the center line-of-sight from the camera position
  with the given distance.

  \sa getPlane()
 */
SbVec3d
SbDPViewVolume::getSightPoint(const double distFromEye) const
{
  return this->projPoint + this->projDir * distFromEye;
}

/*!
  Return the 3D point which projects to \a normPoint and lies on the
  plane perpendicular to the camera direction and \a distFromEye
  distance away from the camera position.

  \a normPoint should be given in normalized coordinates, where the
  visible render canvas is covered by the range [0.0, 1.0].
 */
SbVec3d
SbDPViewVolume::getPlanePoint(const double distFromEye,
                              const SbVec2d & normPoint) const
{
  SbVec3d volpt;

  if(this->getProjectionType() == SbDPViewVolume::ORTHOGRAPHIC) {
    SbVec3d scr(normPoint[0], normPoint[1], -1.0f);

    scr[0] -= 0.5f;
    scr[1] -= 0.5f;
    scr[0] *= 2.0f;
    scr[1] *= 2.0f;

    SbDPMatrix m = this->getMatrix().inverse();
    m.multVecMatrix(scr, volpt);
    volpt += (distFromEye - this->getNearDist()) *
      this->getProjectionDirection();
  }
  else {
    // Find vector pointing in the direction of the normalized 2D
    // point.
    SbVec3d dvec =
      this->llf +
      (this->lrf - this->llf) * normPoint[0] +
      (this->ulf - this->llf) * normPoint[1];

    if (dvec.normalize() == 0.0) {
#if COIN_DEBUG
      SoDebugError::postWarning("SbDPViewVolume::getPlanePoint",
                                "Frustum is invalid, point set to the projection point.");
#endif // COIN_DEBUG
      // just set volpt to the projection point
      volpt = this->getProjectionPoint();
    }
    else {
      // Distance to point.
      double d = distFromEye/dvec.dot(this->getProjectionDirection());
      
      volpt = d * dvec + this->getProjectionPoint();
    }
  }

  return volpt;
}

/*!
  Returns a rotation that aligns an object so that its positive x-axis
  is to the right and its positive y-axis is up in the view volume.
  
  If rightangleonly is TRUE, it will create a rotation that aligns the
  x and y-axis with the closest orthogonal axes to right and up.
*/
SbDPRotation
SbDPViewVolume::getAlignRotation(SbBool rightangleonly) const
{
  SbVec3d x,y,z;
  if (rightangleonly) {
    y = this->ulf - this->llf;
    y = y.getClosestAxis();

    x = this->lrf - this->llf;
    x = x.getClosestAxis();
    
    z = x.cross(y);
    (void) z.normalize();
  }    
  else {
    y = this->ulf - this->llf;
    (void) y.normalize();
    x = this->lrf - this->llf;
    (void) x.normalize();
    z = x.cross(y);
    (void) z.normalize();
  }
  
  SbDPMatrix m = SbDPMatrix::identity();
  m[0][0] = x[0];
  m[0][1] = x[1];
  m[0][2] = x[2];
  
  m[1][0] = y[0];
  m[1][1] = y[1];
  m[1][2] = y[2];
  
  m[2][0] = z[0];
  m[2][1] = z[1];
  m[2][2] = z[2];
  
  return SbDPRotation(m);
}

/*!
  Given a sphere with center in \a worldCenter and an initial radius of \a 1.0,
  return the scale factor needed to make this sphere have a \a normRadius
  radius when projected onto the near clipping plane.
 */
double
SbDPViewVolume::getWorldToScreenScale(const SbVec3d& worldCenter,
                                    double normRadius) const
{
#if COIN_DEBUG
  if (normRadius < 0.0f) {
    SoDebugError::postWarning("SbDPViewVolume::getWorldToScreenScale",
                              "normRadius (%f) should be >=0.0f.", normRadius);
    return 1.0f;
  }
  if (this->getWidth() == 0.0f || this->getHeight() == 0.0f) {
    SoDebugError::postWarning("SbDPViewVolume::getWorldToScreenScale",
                              "invalid frustum <%f, %f>",
                              this->getWidth(), this->getHeight());
    return 1.0f;
  }
#endif // COIN_DEBUG

  if(this->getProjectionType() == SbDPViewVolume::ORTHOGRAPHIC) {
    SbVec3d rightvec = this->lrf - this->llf;
    return (normRadius * rightvec).length();
  }
  else {
    // Find screen space coordinates of sphere center point and tangent
    // point.
    SbVec3d center_scr;
    this->projectToScreen(worldCenter, center_scr);
    center_scr[0] += normRadius;

    // Vectors spanning the projection plane.
    SbVec3d upvec = this->ulf - this->llf;
    SbVec3d rightvec = this->lrf - this->llf;

    // Find projection plane point for the sphere tangent touch point,
    // which is then used to define the sphere tangent line.
    SbVec3d ppp = this->projPoint + 
      this->llf + center_scr[0] * rightvec + center_scr[1] * upvec;
    SbLine tl(dp_to_sbvec3f(this->getProjectionPoint()), dp_to_sbvec3f(ppp));

    // Define the plane which is cutting the sphere in half and is normal
    // to the camera direction.
    SbVec3d sphere_camera_vec = worldCenter - this->getProjectionPoint();
    SbPlane p = SbPlane(dp_to_sbvec3f(sphere_camera_vec), dp_to_sbvec3f(worldCenter));

    // Find tangent point of sphere.
    SbVec3f tangentpt;
    SbBool result = p.intersect(tl, tangentpt);
    assert(result != FALSE);

    // Return radius (which is equal to the scale factor, since we're
    // dealing with a unit sphere).
    return (dp_to_sbvec3d(tangentpt) - worldCenter).length();
  }
}

/*!
  Projects the given box onto the projection plane and returns the
  normalized screen space it occupies.
 */
SbVec2d
SbDPViewVolume::projectBox(const SbBox3f& box) const
{
#if COIN_DEBUG
  if (box.isEmpty()) {
    SoDebugError::postWarning("SbDPViewVolume::projectBox",
                              "Box is empty.");
  }
#endif // COIN_DEBUG

  SbVec3d mincorner = dp_to_sbvec3d(box.getMin());
  SbVec3d maxcorner = dp_to_sbvec3d(box.getMax());
  SbBox2f span;

  for(int i=0; i < 2; i++) {
    for(int j=0; j < 2; j++) {
      for(int k=0; k < 2; k++) {
        SbVec3d corner(i ? mincorner[0] : maxcorner[0],
                       j ? mincorner[1] : maxcorner[1],
                       k ? mincorner[2] : maxcorner[2]);
        this->projectToScreen(corner, corner);
        span.extendBy(SbVec2f(static_cast<float>(corner[0]), static_cast<float>(corner[1])));
      }
    }
  }

  return SbVec2d(span.getMax()[0] - span.getMin()[0],
                 span.getMax()[1] - span.getMin()[1]);
}

/*!
  Returns a narrowed version of the view volume which is within the
  given [0, 1] normalized coordinates. The coordinates are taken to be
  corner points of a normalized "view window" on the near clipping
  plane.  I.e.:

  \code
  SbDPViewVolume view;
  view.ortho(0, 100, 0, 100, 0.1, 1000);
  view = view.narrow(0.25, 0.5, 0.75, 1.0);
  \endcode

  ..will give a view volume with corner points <25, 75> and <50, 100>.

  \sa scale(), scaleWidth(), scaleHeight()
 */
SbDPViewVolume
SbDPViewVolume::narrow(double left, double bottom,
                     double right, double top) const
{
#if COIN_DEBUG && 0 // debug test disabled, 2001-02-16, pederb
  if (left<0.0f) {
    SoDebugError::postWarning("SbDPViewVolume::narrow",
                              "left coordinate (%f) should be >=0.0f. "
                              "Clamping to 0.0f.",left);
    left=0.0f;
  }
  if (right>1.0f) {
    SoDebugError::postWarning("SbDPViewVolume::narrow",
                              "right coordinate (%f) should be <=1.0f. "
                              "Clamping to 1.0f.",right);
    right=1.0f;
  }
  if (bottom<0.0f) {
    SoDebugError::postWarning("SbDPViewVolume::narrow",
                              "bottom coordinate (%f) should be >=0.0f. "
                              "Clamping to 0.0f.",bottom);
    bottom=0.0f;
  }
  if (top>1.0f) {
    SoDebugError::postWarning("SbDPViewVolume::narrow",
                              "top coordinate (%f) should be <=1.0f. "
                              "Clamping to 1.0f.",top);
    top=1.0f;
  }
  if (left>right) {
    SoDebugError::postWarning("SbDPViewVolume::narrow",
                              "right coordinate (%f) should be larger than "
                              "left coordinate (%f). Swapping left/right.",
                              right,left);
    double tmp=right;
    right=left;
    left=tmp;
  }
  if (bottom>top) {
    SoDebugError::postWarning("SbDPViewVolume::narrow",
                              "top coordinate (%f) should be larger than "
                              "bottom coordinate (%f). Swapping top/bottom.",
                              top,bottom);
    double tmp=top;
    top=bottom;
    bottom=tmp;
  }
#endif // COIN_DEBUG

  SbDPViewVolume nvw = *this;

  double w = nvw.getWidth();
  double h = nvw.getHeight();

  SbVec3d xvec = this->lrf - this->llf;
  SbVec3d yvec = this->ulf - this->llf;
  
  if (yvec.normalize() == 0.0 || xvec.normalize() == 0.0) {
#if COIN_DEBUG
    SoDebugError::postWarning("SbDPViewVolume::narrow",
                              "View volume was empty before narrowing.");
    
#endif // COIN_DEBUG
  }

  nvw.ulf = nvw.llf + (xvec * left * w + yvec * top * h);
  nvw.lrf =
    nvw.llf + (xvec * right * w + yvec * bottom * h);
  nvw.llf += xvec * left * w + yvec * bottom * h;

  return nvw;
}

/*!

  Returns a narrowed version of the view volume which is within the
  given [0, 1] normalized coordinates. The box x and y coordinates are
  taken to be corner points of a normalized "view window" on the near
  clipping plane. The box z coordinates are used to adjust the near
  and far clipping planes, and should be relative to the current
  clipping planes. A value of 1.0 is at the current near plane. A
  value of 0.0 is at the current far plane.
*/
SbDPViewVolume
SbDPViewVolume::narrow(const SbBox3f & box) const
{
  SbVec3d bmin = dp_to_sbvec3d(box.getMin());
  SbVec3d bmax = dp_to_sbvec3d(box.getMax());
  return this->narrow(bmin[0], bmin[1], bmax[0], bmax[1]).zNarrow(bmax[2], bmin[2]);
}

// FIXME: bitmap-illustration for function doc which shows how the
// frustum is set up wrt the input arguments. 20010919 mortene.
/*!
  Set up the view volume as a rectangular box for orthographic
  parallel projections. The line of sight will be along the negative
  Z-axis, through the center of the plane defined by the point
  <(right+left)/2, (top+bottom)/2, 0>.

  \sa perspective().
*/
void
SbDPViewVolume::ortho(double left, double right,
                    double bottom, double top,
                    double nearval, double farval)
{
#if defined(COIN_DEBUG) && 0 // disabled 2002-08-30 pederb

  // These parameter tests are probably incorrect. It is possible to
  // set left > right etc. in SGI/TGS Inventor, and it should be
  // possible to do this in Coin also.  pederb.

  // (Yes, we've actually had user requests for making this possible
  // after first disallowing it. I don't know what kind of effects
  // they are using this for, and I'm not sure I want to know.. :-})
  // mortene.


  if (left>right) {
    SoDebugError::postWarning("SbDPViewVolume::ortho",
                              "right coordinate (%f) should be larger than "
                              "left coordinate (%f). Swapping left/right.",
                              right,left);
    double tmp=right;
    right=left;
    left=tmp;
  }
  if (bottom>top) {
    SoDebugError::postWarning("SbDPViewVolume::ortho",
                              "top coordinate (%f) should be larger than "
                              "bottom coordinate (%f). Swapping bottom/top.",
                              top,bottom);
    double tmp=top;
    top=bottom;
    bottom=tmp;
  }
  if (nearval>farval) {
    SoDebugError::postWarning("SbDPViewVolume::ortho",
                              "far coordinate (%f) should be larger than near "
                              "coordinate (%f). Swapping near/far.",farval,nearval);
    double tmp=farval;
    farval=nearval;
    nearval=tmp;
  }
#endif // disabled

  this->type = SbDPViewVolume::ORTHOGRAPHIC;
  this->projPoint.setValue(0.0f, 0.0f, 0.0f);
  this->projDir.setValue(0.0f, 0.0f, -1.0f);
  this->nearDist = nearval;
  this->nearToFar = farval - nearval;
  this->llf.setValue(left, bottom, -nearval);
  this->lrf.setValue(right, bottom, -nearval);
  this->ulf.setValue(left, top, -nearval);
}

// FIXME: bitmap-illustration for function doc which shows how the
// frustum is set up wrt the input arguments. 20010919 mortene.
/*!
  Set up the view volume for perspective projections. The line of
  sight will be through origo along the negative Z-axis.

  \sa ortho().
*/
void
SbDPViewVolume::perspective(double fovy, double aspect,
                            double nearval, double farval)
{
#if COIN_DEBUG
  if (fovy<0.0f || fovy > M_PI) {
    SoDebugError::postWarning("SbDPViewVolume::perspective",
                              "Field of View 'fovy' (%f) is out of bounds "
                              "[0,PI]. Clamping to be within bounds.",fovy);
    if (fovy<0.0f) fovy=0.0f;
    else if (fovy>M_PI) fovy=M_PI;
  }

#if 0 // obsoleted 2003-02-03 pederb. A negative aspect ratio is ok
  if (aspect<0.0f) {
    SoDebugError::postWarning("SbDPViewVolume::perspective",
                              "Aspect ratio 'aspect' (%d) should be >=0.0f. "
                              "Clamping to 0.0f.",aspect);
    aspect=0.0f;
  }
#endif // obsoleted

  if (nearval>farval) {
    SoDebugError::postWarning("SbDPViewVolume::perspective",
                              "far coordinate (%f) should be larger than "
                              "near coordinate (%f). Swapping near/far.",
                              farval,nearval);
    double tmp=farval;
    farval=nearval;
    nearval=tmp;
  }
#endif // COIN_DEBUG

  this->type = SbDPViewVolume::PERSPECTIVE;
  this->projPoint.setValue(0.0f, 0.0f, 0.0f);
  this->projDir.setValue(0.0f, 0.0f, -1.0f);
  this->nearDist = nearval;
  this->nearToFar = farval - nearval;

  double top = nearval * double(tan(fovy/2.0f));
  double bottom = -top;
  double left = bottom * aspect;
  double right = -left;

  this->llf.setValue(left, bottom, -nearval);
  this->lrf.setValue(right, bottom, -nearval);
  this->ulf.setValue(left, top, -nearval);
}

/*! 
  Set up the frustum for perspective projection. This is an
  alternative to perspective() that lets you specify any kind of view
  volumes (e.g. off center volumes). It has the same arguments and
  functionality as the corresponding OpenGL glFrustum() function.

  \sa perspective() 
*/
void 
SbDPViewVolume::frustum(double left, double right,
                        double bottom, double top,
                        double nearval, double farval)
{
  this->type = SbDPViewVolume::PERSPECTIVE;
  this->projPoint.setValue(0.0f, 0.0f, 0.0f);
  this->projDir.setValue(0.0f, 0.0f, -1.0f);
  this->nearDist = nearval;
  this->nearToFar = farval - nearval;

  this->llf.setValue(left, bottom, -nearval);
  this->lrf.setValue(right, bottom, -nearval);
  this->ulf.setValue(left, top, -nearval);
}

/*!
  Rotate the direction which the camera is pointing in.

  \sa translateCamera().
 */
void
SbDPViewVolume::rotateCamera(const SbDPRotation& q)
{
  SbDPMatrix mat;
  mat.setRotate(q);

  mat.multDirMatrix(this->projDir, this->projDir);
  mat.multDirMatrix(this->llf, this->llf);
  mat.multDirMatrix(this->lrf, this->lrf);
  mat.multDirMatrix(this->ulf, this->ulf);
}

/*!
  Translate the camera position of the view volume.

  \sa rotateCamera().
 */
void
SbDPViewVolume::translateCamera(const SbVec3d & v)
{
  this->projPoint += v;
}

/*!
  Return the vector pointing from the center of the view volume towards
  the camera. This is just the vector pointing in the opposite direction
  of \a getProjectionDirection().

  \sa getProjectionDirection().
 */
SbVec3d
SbDPViewVolume::zVector(void) const
{
  return -this->projDir;
}

/*!
  Return a copy SbDPViewVolume with narrowed depth by supplying parameters
  for new near and far clipping planes.

  \a nearval and \a farval should be relative to the current clipping
  planes. A value of 1.0 is at the current near plane. A value of
  0.0 is at the current far plane.

  \sa zVector().
*/
SbDPViewVolume
SbDPViewVolume::zNarrow(double nearval, double farval) const
{
  SbDPViewVolume narrowed = *this;

  narrowed.nearDist = this->nearDist + (1.0f - nearval) * this->nearToFar;
  narrowed.nearToFar = this->nearDist + this->nearToFar * (1.0f - farval);

  SbVec3d dummy;
  this->getPlaneRectangle(narrowed.nearDist - this->nearDist,
                          narrowed.llf,
                          narrowed.lrf,
                          narrowed.ulf,
                          dummy);
  return narrowed;
}

/*!
  Scale width and height of viewing frustum by the given ratio around the
  projection plane center axis.

  \sa scaleWidth(), scaleHeight().
 */
void
SbDPViewVolume::scale(double factor)
{
#if COIN_DEBUG
  if (factor<0.0f) {
    SoDebugError::postWarning("SbDPViewVolume::scale",
                              "Scale factor (%f) should be >=0.0f. Clamping "
                              "to 0.0f.",factor);
    factor=0.0f;
  }
#endif // COIN_DEBUG

  this->scaleWidth(factor);
  this->scaleHeight(factor);
}

/*!
  Scale width of viewing frustum by the given ratio around the vertical
  center axis in the projection plane.

  \sa scale(), scaleHeight().
 */
void
SbDPViewVolume::scaleWidth(double ratio)
{
#if COIN_DEBUG
  if (ratio<0.0f) {
    SoDebugError::postWarning("SbDPViewVolume::scaleWidth",
                              "Scale factor (%f) should be >=0.0f. "
                              "Clamping to 0.0f.",ratio);
    ratio=0.0f;
  }
#endif // COIN_DEBUG

  double w = this->getWidth();
  double neww = w * ratio;
  double wdiff = (neww - w)/2.0f;

  SbVec3d xvec = this->lrf - this->llf;
  if (xvec.normalize() == 0.0) {
#if COIN_DEBUG
    SoDebugError::postWarning("SbDPViewVolume::scaleWidth",
                              "View volume had no width before scaling.");
#endif // COIN_DEBUG
  }
  SbVec3d diffvec = xvec * wdiff;

  this->llf -= diffvec;
  this->ulf -= diffvec;
  this->lrf += diffvec;
}

/*!
  Scale height of viewing frustum by the given ratio around the horizontal
  center axis in the projection plane.

  \sa scale(), scaleWidth().
 */
void
SbDPViewVolume::scaleHeight(double ratio)
{
#if COIN_DEBUG
  if (ratio<0.0f) {
    SoDebugError::postWarning("SbDPViewVolume::scaleHeight",
                              "Scale factor (%f) should be >=0.0f. "
                              "Clamping to 0.0f.",ratio);
    ratio=0.0f;
  }
#endif // COIN_DEBUG

  double h = this->getHeight();
  double newh = h * ratio;
  double hdiff = (newh - h)/2.0f;

  SbVec3d upvec = this->ulf - this->llf;
  if (upvec.normalize() == 0.0) {
#if COIN_DEBUG
    SoDebugError::postWarning("SbDPViewVolume::scaleHeight",
                              "View volume had no height before scaling.");
#endif // COIN_DEBUG
  }
  SbVec3d diffvec = upvec * hdiff;

  this->llf -= diffvec;
  this->ulf += diffvec;
  this->lrf -= diffvec;
}

/*!
  Return current view volume projection type, which can be
  either \a ORTHOGRAPHIC or \a PERSPECTIVE.

  \sa SbDPViewVolume::ProjectionType
 */
SbDPViewVolume::ProjectionType
SbDPViewVolume::getProjectionType(void) const
{
  return this->type;
}

/*!
  Returns coordinates of center point in the projection plane.
 */
const SbVec3d&
SbDPViewVolume::getProjectionPoint(void) const
{
  return this->projPoint;
}

/*!
  Returns the direction of projection, i.e. the direction the camera is
  pointing.

  \sa getNearDist().
 */
const SbVec3d&
SbDPViewVolume::getProjectionDirection(void) const
{
  return this->projDir;
}

/*!
  Returns distance from projection plane to near clipping plane.

  \sa getProjectionDirection().
 */
double
SbDPViewVolume::getNearDist(void) const
{
  return this->nearDist;
}

/*!
  Returns width of viewing frustum in the projection plane.

  \sa getHeight(), getDepth().
*/
double
SbDPViewVolume::getWidth(void) const
{
  return (this->lrf - this->llf).length();
}

/*!
  Returns height of viewing frustum in the projection plane.

  \sa getWidth(), getDepth().
*/
double
SbDPViewVolume::getHeight(void) const
{
  return (this->ulf - this->llf).length();
}

/*!
  Returns depth of viewing frustum, i.e. the distance from the near clipping
  plane to the far clipping plane.

  \sa getWidth(), getHeight().
 */
double
SbDPViewVolume::getDepth(void) const
{
  return this->nearToFar;
}

/*!
  Dump the state of this object to the \a fp file stream. Only works in
  debug version of library, method does nothing in an optimized build.
 */
void
SbDPViewVolume::print(FILE * fp) const
{
#if COIN_DEBUG
  fprintf( fp, "  projtype: %d\n", static_cast<int>(this->getProjectionType()) );
  fprintf( fp, "  projpt:   " );
  this->getProjectionPoint().print(fp);
  fprintf( fp, "\n" );
  fprintf( fp, "  projdir:  " );
  this->getProjectionDirection().print(fp);
  fprintf( fp, "\n" );
  fprintf( fp, "  neardist: %f\n", this->getNearDist() );
  fprintf( fp, "  width:    %f\n", this->getWidth() );
  fprintf( fp, "  height:   %f\n", this->getHeight() );
  fprintf( fp, "  depth:    %f\n", this->getDepth() );
  fprintf( fp, "    llf:    " );
  this->llf.print(fp);
  fprintf( fp, "\n" );
  fprintf( fp, "    lrf:    " );
  this->lrf.print(fp);
  fprintf( fp, "\n" );
  fprintf( fp, "    ulf:    " );
  this->ulf.print(fp);
  fprintf( fp, "\n" );
#endif // COIN_DEBUG
}

/*!
  Returns the six planes defining the view volume in the following
  order: left, bottom, right, top, near, far. Plane normals are
  directed into the view volume.

  This method is an extension for Coin, and is not available in the
  original Open Inventor.
*/
void
SbDPViewVolume::getViewVolumePlanes(SbPlane planes[6]) const
{
  SbVec3d far_ll;
  SbVec3d far_lr;
  SbVec3d far_ul;
  SbVec3d far_ur;

  this->getPlaneRectangle(this->nearToFar, far_ll, far_lr, far_ul, far_ur);
  SbVec3d near_ur = this->ulf + (this->lrf-this->llf);

  SbVec3f f_ulf = dp_to_sbvec3f(this->ulf + this->projPoint);
  SbVec3f f_llf = dp_to_sbvec3f(this->llf + this->projPoint);
  SbVec3f f_lrf = dp_to_sbvec3f(this->lrf + this->projPoint);
  SbVec3f f_near_ur = dp_to_sbvec3f(near_ur + this->projPoint);
  SbVec3f f_far_ll = dp_to_sbvec3f(far_ll + this->projPoint);
  SbVec3f f_far_lr = dp_to_sbvec3f(far_lr + this->projPoint);
  SbVec3f f_far_ul = dp_to_sbvec3f(far_ul + this->projPoint);
  SbVec3f f_far_ur = dp_to_sbvec3f(far_ur + this->projPoint);
  
  planes[0] = SbPlane(f_ulf, f_llf, f_far_ll);  // left
  planes[1] = SbPlane(f_llf, f_lrf, f_far_lr); // bottom
  planes[2] = SbPlane(f_lrf, f_near_ur, f_far_ur); // right
  planes[3] = SbPlane(f_near_ur, f_ulf, f_far_ul); // top
  planes[4] = SbPlane(f_ulf, f_near_ur, f_lrf); // near
  planes[5] = SbPlane(f_far_ll, f_far_lr, f_far_ur); // far

  // check for inverted view volume (negative aspectRatio)
  if (!planes[0].isInHalfSpace(f_lrf)) {
    SbVec3f n;
    float D;

    n = planes[0].getNormal();
    D = planes[0].getDistanceFromOrigin();    
    planes[0] = SbPlane(-n, -D);

    n = planes[2].getNormal();
    D = planes[2].getDistanceFromOrigin();    
    planes[2] = SbPlane(-n, -D);
  }
  if (!planes[1].isInHalfSpace(f_near_ur)) {
    SbVec3f n;
    float D;

    n = planes[1].getNormal();
    D = planes[1].getDistanceFromOrigin();    
    planes[1] = SbPlane(-n, -D);

    n = planes[3].getNormal();
    D = planes[3].getDistanceFromOrigin();    
    planes[3] = SbPlane(-n, -D);
    
  }

  if (!planes[4].isInHalfSpace(f_far_ll)) {
    SbVec3f n;
    float D;

    n = planes[4].getNormal();
    D = planes[4].getDistanceFromOrigin();    
    planes[4] = SbPlane(-n, -D);

    n = planes[5].getNormal();
    D = planes[5].getDistanceFromOrigin();    
    planes[5] = SbPlane(-n, -D);
    
  }

}

/*!
  Transform the viewing volume by \a matrix.
 */
void
SbDPViewVolume::transform(const SbDPMatrix & matrix)
{
  SbVec3d oldprojpt = this->projPoint;
  SbVec3d newprojpt;
  SbVec3d newllf;
  SbVec3d newlrf;
  SbVec3d newulf;
  matrix.multVecMatrix(oldprojpt, newprojpt);
  

  // need to translate frustum point with the projection point before
  // transforming, then translate back afterwards.
  matrix.multVecMatrix(this->llf+oldprojpt, newllf);
  newllf -= newprojpt;

  matrix.multVecMatrix(this->lrf+oldprojpt, newlrf);
  newlrf -= newprojpt;

  matrix.multVecMatrix(this->ulf+oldprojpt, newulf);
  newulf -= newprojpt;

  // Construct and transform nearpt and farpt to find the new near and
  // far values.
  SbVec3d nearpt;
  SbVec3d farpt;
  matrix.multVecMatrix(oldprojpt + this->nearDist * this->projDir, 
                       nearpt);

  double fardist = this->nearDist + this->nearToFar;
  matrix.multVecMatrix(oldprojpt + fardist * this->projDir, farpt);

  matrix.multDirMatrix(this->projDir, this->projDir);
  this->projPoint = newprojpt;
  this->llf = newllf;
  this->ulf = newulf;
  this->lrf = newlrf;
  SbDPPlane projPlane(this->projDir, this->projPoint);
  this->nearDist = projPlane.getDistance(nearpt);
  this->nearToFar = (farpt-nearpt).length();
}

/*!
  Returns the view up vector for this view volume. It's a vector
  which is perpendicular to the projection direction, and parallel and
  oriented in the same direction as the vector from the lower left
  corner to the upper left corner of the near plane.
*/
SbVec3d
SbDPViewVolume::getViewUp(void) const
{
  SbVec3d v = this->ulf - this->llf;
  if (v.normalize() == 0.0) {
#if COIN_DEBUG
    SoDebugError::postWarning("SbDPViewVolume::getViewUp",
                              "View volume is empty.");
#endif // COIN_DEBUG
  }
  return v;
}

//
// Returns the four points defining the view volume rectangle at the
// specified distance from the near plane, towards the far plane. The
// points are returned in normalized view volume coordinates
// (projPoint is not added).
void
SbDPViewVolume::getPlaneRectangle(const double distance, SbVec3d & lowerleft,
                                  SbVec3d & lowerright,
                                  SbVec3d & upperleft,
                                  SbVec3d & upperright) const
{
  SbVec3d near_ur = this->ulf + (this->lrf-this->llf);

#if COIN_DEBUG
  if (this->llf == SbVec3d(0.0, 0.0, 0.0) ||
      this->lrf == SbVec3d(0.0, 0.0, 0.0) ||
      this->ulf == SbVec3d(0.0, 0.0, 0.0) ||
      near_ur == SbVec3d(0.0, 0.0, 0.0)) {
    SoDebugError::postWarning("SbDPViewVolume::getPlaneRectangle",
                              "Invalid frustum.");
    
  }
#endif // COIN_DEBUG

  if (this->type == PERSPECTIVE) {
    double depth = this->nearDist + distance;
    SbVec3d dir;
    dir = this->llf;
    (void) dir.normalize(); // safe to normalize here
    lowerleft = dir * depth / dir.dot(this->projDir);

    dir = this->lrf;
    dir.normalize(); // safe to normalize here
    lowerright = dir * depth / dir.dot(this->projDir);

    dir = this->ulf;
    (void) dir.normalize(); // safe to normalize here
    upperleft = dir * depth / dir.dot(this->projDir);
    
    dir = near_ur;
    (void) dir.normalize(); // safe to normalize here
    upperright = dir * depth / dir.dot(this->projDir);
  }
  else {
    lowerleft = this->llf + this->projDir * distance;
    lowerright = this->lrf + this->projDir * distance;
    upperleft = this->ulf + this->projDir * distance;
    upperright = near_ur + this->projDir * distance;
  }
}

/*!
  Copies all values of a single precision SbViewVolume \a vv 
  to the current double precision instance.
*/
void 
SbDPViewVolume::copyValues(SbViewVolume & vv)
{
  vv.type = static_cast<SbViewVolume::ProjectionType>(this->type);
  vv.projPoint = dp_to_sbvec3f(this->projPoint);
  vv.projDir = dp_to_sbvec3f(this->projDir);
  vv.nearDist = static_cast<float>(this->nearDist);
  vv.nearToFar = static_cast<float>(this->nearToFar);
  vv.llf = dp_to_sbvec3f(this->llf + this->projPoint);
  vv.lrf = dp_to_sbvec3f(this->lrf + this->projPoint);
  vv.ulf = dp_to_sbvec3f(this->ulf + this->projPoint);
}

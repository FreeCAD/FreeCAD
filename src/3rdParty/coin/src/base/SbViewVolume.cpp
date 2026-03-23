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
  \class SbViewVolume SbViewVolume.h Inventor/SbViewVolume.h
  \brief The SbViewVolume class is a viewing volume in 3D space.

  \ingroup coin_base

  This class contains the necessary information for storing a view
  volume.  It has methods for projection of primitives into the 3D
  volume from 2D points in the projection plane or vice versa, doing
  camera transforms, view volume transforms, etc.

  \sa SbViewportRegion
*/

// FIXME: I have a nagging feeling that it is bad design to use the
// *same* class entity for embedding the abstractions and the
// functionality of both perspective and orthographic view volumes.
//
// Should investigate, then possibly fixing this design flaw by
// splitting up into two different classes (inheriting a common
// abstract viewvolume-class?), while keeping this class around as a
// wrapper class for the new abstractions to be API compatible with
// client code using the original API. 20010824 mortene.

#include <Inventor/SbViewVolume.h>
#include <Inventor/SbRotation.h>
#include <Inventor/SbDPRotation.h>
#include <Inventor/SbLine.h>
#include <Inventor/SbDPLine.h>
#include <Inventor/SbDPMatrix.h>
#include <Inventor/SbVec3d.h>
#include <Inventor/SbVec2d.h>
#include <Inventor/SbMatrix.h>
#include <Inventor/SbPlane.h>
#include <Inventor/SbBox2f.h>
#include <Inventor/SbBox3f.h>
#include <Inventor/SbClip.h>

#if COIN_DEBUG
#include <Inventor/errors/SoDebugError.h>
#endif // COIN_DEBUG

#include <coindefs.h> // COIN_OBSOLETED()
#include <cassert>

/*!
  \enum SbViewVolume::ProjectionType

  An SbViewVolume instance can represent either an orthogonal projection
  volume or a perspective projection volume.

  \sa ortho(), perspective(), getProjectionType().
*/

/*!
  \var SbViewVolume::ProjectionType SbViewVolume::ORTHOGRAPHIC
  Orthographic projection.
*/

/*!
  \var SbViewVolume::ProjectionType SbViewVolume::PERSPECTIVE
  Perspective projection.
*/

/*!
  \var SbViewVolume::ProjectionType SbViewVolume::type
  \COININTERNAL
*/

/*!
  \var SbVec3f SbViewVolume::projPoint
  \COININTERNAL
*/

/*!
  \var SbVec3f SbViewVolume::projDir
  \COININTERNAL
*/

/*!
  \var float SbViewVolume::nearDist
  \COININTERNAL
*/

/*!
  \var float SbViewVolume::nearToFar
  \COININTERNAL
*/

/*!
  \var SbVec3f SbViewVolume::llf
  \COININTERNAL
*/

/*!
  \var SbVec3f SbViewVolume::lrf
  \COININTERNAL
*/

/*!
  \var SbVec3f SbViewVolume::ulf
  \COININTERNAL
*/


//
// some convenience function for converting between single precision
// and double precision classes.
//
static void 
copy_matrix(const SbDPMatrix & src, SbMatrix & dst)
{
  const double * s = src[0];
  float * d = dst[0];
  for (int i = 0; i < 16; i++) {
    d[i] = static_cast<float>(s[i]);
  }
}

static void 
copy_matrix(const SbMatrix & src, SbDPMatrix & dst)
{
  const float * s = src[0];
  double * d = dst[0];
  for (int i = 0; i < 16; i++) {
    d[i] = static_cast<double>(s[i]);
  }
}

static SbVec3f 
to_sbvec3f(const SbVec3d & v)
{
  return SbVec3f(static_cast<float>(v[0]), static_cast<float>(v[1]), static_cast<float>(v[2]));
}

static SbVec2f 
to_sbvec2f(const SbVec2d & v)
{
  return SbVec2f(static_cast<float>(v[0]), static_cast<float>(v[1]));
}

/*!
  Constructor. Note that the SbViewVolume instance will be uninitialized
  until you explicitly call \a ortho() or \a perspective().

  \sa ortho(), perspective().
 */
SbViewVolume::SbViewVolume(void)
{
}

/*!
  Destructor.
 */
SbViewVolume::~SbViewVolume(void)
{
}

/*!
  Returns the view volume's affine matrix and projection matrix.

  \sa getMatrix(), getCameraSpaceMatrix()
 */
void
SbViewVolume::getMatrices(SbMatrix& affine, SbMatrix& proj) const
{
  SbDPMatrix dpaffine, dpproj;
  this->dpvv.getMatrices(dpaffine, dpproj);
  copy_matrix(dpaffine, affine);
  copy_matrix(dpproj, proj);
}

/*!
  Returns the combined affine and projection matrix.

  \sa getMatrices(), getCameraSpaceMatrix()
 */
SbMatrix
SbViewVolume::getMatrix(void) const
{
  SbDPMatrix dpmatrix = this->dpvv.getMatrix();
  SbMatrix matrix;
  copy_matrix(dpmatrix, matrix);
  return matrix;
}

/*!
  Returns a matrix which will translate the view volume camera back to
  origo, and rotate the camera so it'll point along the negative Z-axis.

  Note that the matrix will \a not include the rotation necessary to
  make the camera up vector point along the positive Y-axis (i.e.
  camera roll is not accounted for).

  \sa getMatrices(), getMatrix()
 */
SbMatrix
SbViewVolume::getCameraSpaceMatrix(void) const
{
  SbDPMatrix m = this->dpvv.getCameraSpaceMatrix();
  SbMatrix ret;
  copy_matrix(m, ret);
  return ret;
}

/*!
  Project the given 2D point from the projection plane into a 3D line.

  \a pt coordinates should be normalized to be within [0, 1].
 */
void
SbViewVolume::projectPointToLine(const SbVec2f& pt, SbLine& line) const
{
  SbVec2d dppt;

  dppt[0] = pt[0];
  dppt[1] = pt[1];
  
  SbVec3d pt0, pt1;
  this->dpvv.projectPointToLine(dppt, pt0, pt1);
  line.setValue(to_sbvec3f(pt0), to_sbvec3f(pt1));
}

/*!
  Project the given 2D point from the projection plane into two points
  defining a 3D line. The first point, \a line0, will be the
  corresponding point for the projection on the near plane, while \a line1
  will be the line endpoint, lying in the far plane.
 */
void
SbViewVolume::projectPointToLine(const SbVec2f & pt,
                                 SbVec3f & line0, SbVec3f & line1) const
{
  SbVec2d dppt(pt[0], pt[1]);
  SbVec3d dpline0, dpline1;
  this->dpvv.projectPointToLine(dppt, dpline0, dpline1);
  line0 = to_sbvec3f(dpline0);
  line1 = to_sbvec3f(dpline1);
}

/*!
  Project the \a src point to a normalized set of screen coordinates in
  the projection plane and place the result in \a dst.

  It is safe to let \a src and \a dst be the same SbVec3f instance.
  
  The z-coordinate of \a dst is monotonically increasing for points
  closer to the far plane. Note however that this is not a linear
  relationship, the \a dst z-coordinate is calculated as follows:
  
  Orthogonal view:  DSTz = (-2 * SRCz - far - near) / (far - near),
  Perspective view:  DSTz = (-SRCz * (far - near) - 2*far*near) / (far - near)

  The returned coordinates (\a dst) are normalized to be in range [0, 1].
*/
void
SbViewVolume::projectToScreen(const SbVec3f& src, SbVec3f& dst) const
{
  SbVec3d dpsrc(src[0], src[1], src[2]);
  SbVec3d dpdst;
  this->dpvv.projectToScreen(dpsrc, dpdst);
  dst = to_sbvec3f(dpdst);
}

/*!
  Returns an SbPlane instance which has a normal vector in the opposite
  direction of which the camera is pointing. This means the
  plane will be parallel to the near and far clipping planes.

  \sa getSightPoint()
 */
SbPlane
SbViewVolume::getPlane(const float distFromEye) const
{
  return this->dpvv.getPlane(distFromEye);
}

/*!
  Returns the point on the center line-of-sight from the camera position
  with the given distance.

  \sa getPlane()
 */
SbVec3f
SbViewVolume::getSightPoint(const float distFromEye) const
{
  return to_sbvec3f(this->dpvv.getSightPoint(distFromEye));
}

/*!
  Return the 3D point which projects to \a normPoint and lies on the
  plane perpendicular to the camera direction and \a distFromEye
  distance away from the camera position.

  \a normPoint should be given in normalized coordinates, where the
  visible render canvas is covered by the range [0.0, 1.0].
 */
SbVec3f
SbViewVolume::getPlanePoint(const float distFromEye,
                            const SbVec2f & normPoint) const
{
  SbVec2d dpnormPoint(normPoint[0], normPoint[1]);
  return to_sbvec3f(this->dpvv.getPlanePoint(distFromEye, dpnormPoint));
}

/*!
  Returns a rotation that aligns an object so that its positive x-axis
  is to the right and its positive y-axis is up in the view volume.
  
  If rightangleonly is TRUE, it will create a rotation that aligns the
  x and y-axis with the closest orthogonal axes to right and up.
*/

SbRotation
SbViewVolume::getAlignRotation(SbBool rightangleonly) const
{
  SbDPRotation rot = this->dpvv.getAlignRotation(rightangleonly);

  const double * src = rot.getValue();
  float q[4];
  for (int i = 0; i < 4; i++){
    q[i] = static_cast<float>(src[i]);
  }
  return SbRotation(q);
}

/*!
  Given a sphere with center in \a worldCenter and an initial radius
  of 1.0, return the scale factor needed to make this sphere have a \a
  normRadius radius when projected onto the near clipping plane.
 */
float
SbViewVolume::getWorldToScreenScale(const SbVec3f& worldCenter,
                                    float normRadius) const
{
  SbVec3d dpworldCenter(worldCenter[0], worldCenter[1], worldCenter[2]);
  return static_cast<float>(this->dpvv.getWorldToScreenScale(dpworldCenter, normRadius));
}

/*!
  Projects the given box onto the projection plane and returns the
  normalized screen space it occupies.
 */
SbVec2f
SbViewVolume::projectBox(const SbBox3f& box) const
{
  return to_sbvec2f(this->dpvv.projectBox(box));
}

/*!
  Returns a narrowed version of the view volume which is within the
  given [0, 1] normalized coordinates. The coordinates are taken to be
  corner points of a normalized "view window" on the near clipping
  plane.  I.e.:

  \code
  SbViewVolume view;
  view.ortho(0, 100, 0, 100, 0.1, 1000);
  view = view.narrow(0.25, 0.5, 0.75, 1.0);
  \endcode

  ..will give a view volume with corner points <25, 75> and <50, 100>.

  \sa scale(), scaleWidth(), scaleHeight()
 */
SbViewVolume
SbViewVolume::narrow(float left, float bottom,
                     float right, float top) const
{
  SbDPViewVolume vv = this->dpvv.narrow(left, bottom, right, top);
  SbViewVolume ret;
  vv.copyValues(ret);
  ret.dpvv = vv;
  return ret;
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
SbViewVolume
SbViewVolume::narrow(const SbBox3f & box) const
{
  SbDPViewVolume vv = this->dpvv.narrow(box);
  SbViewVolume ret;
  vv.copyValues(ret);
  ret.dpvv = vv;
  return ret;
}

// FIXME: make an illustration for the following documentation of an
// orthographic view volume, annotated with the input arguments to the
// function. 20010824 mortene.
/*!
  Set up the view volume as a rectangular box for orthographic
  parallel projections.

  The line of sight will be along the negative Z-axis, through the
  center of the plane defined by the point

      [(right+left)/2, (top+bottom)/2, 0]

  \sa perspective().
*/
void
SbViewVolume::ortho(float left, float right,
                    float bottom, float top,
                    float nearval, float farval)
{
  this->dpvv.ortho(left, right, bottom, top, nearval, farval);
  this->dpvv.copyValues(*this);
}

// FIXME: make an illustration for the following documentation of a
// perspective view volume, annotated with the input arguments to the
// function. 20010824 mortene.
/*!
  Set up the view volume for perspective projections. The line of
  sight will be through origo along the negative Z-axis.

  \sa ortho().
*/
void
SbViewVolume::perspective(float fovy, float aspect,
                          float nearval, float farval)
{
  this->dpvv.perspective(fovy, aspect, nearval, farval);
  this->dpvv.copyValues(*this);
}

/*!
  Set up the frustum for perspective projection. This is an
  alternative to perspective() that lets you specify any kind of view
  volumes (e.g. off center volumes). It has the same arguments and
  functionality as the corresponding OpenGL glFrustum() function.

  \COIN_FUNCTION_EXTENSION

  \since Coin 2.0

  \sa perspective()
*/
void 
SbViewVolume::frustum(float left, float right,
                      float bottom, float top,
                      float nearval, float farval)
{
  this->dpvv.frustum(left, right, bottom, top, nearval, farval);
  this->dpvv.copyValues(*this);
}

/*!
  Rotate the direction which the camera is pointing in.

  \sa translateCamera().
 */
void
SbViewVolume::rotateCamera(const SbRotation & q)
{
  const float * quat = q.getValue();
  double dpquat[4];
  dpquat[0] = quat[0];
  dpquat[1] = quat[1];
  dpquat[2] = quat[2];
  dpquat[3] = quat[3];

  SbDPRotation dpq(dpquat);
  this->dpvv.rotateCamera(dpq);
  this->dpvv.copyValues(*this);
}

/*!
  Translate the camera position of the view volume.

  \sa rotateCamera().
 */
void
SbViewVolume::translateCamera(const SbVec3f & v)
{
  SbVec3d dpv(v[0], v[1], v[2]);
  this->dpvv.translateCamera(dpv);
  this->dpvv.copyValues(*this);
}

/*!
  Return the vector pointing from the center of the view volume towards
  the camera. This is just the vector pointing in the opposite direction
  of \a getProjectionDirection().

  \sa getProjectionDirection().
 */
SbVec3f
SbViewVolume::zVector(void) const
{
  return -this->projDir;
}

/*!
  Return a copy SbViewVolume with narrowed depth by supplying
  parameters for new near and far clipping planes.

  \a nearval and \a farval should be relative to the current clipping
  planes. A value of 1.0 is at the current near plane. A value of 0.0
  is at the current far plane.

  \sa zVector().
*/
SbViewVolume
SbViewVolume::zNarrow(float nearval, float farval) const
{
  SbDPViewVolume dpnarrowed = this->dpvv.zNarrow(nearval, farval);
  SbViewVolume narrowed;
  dpnarrowed.copyValues(narrowed);
  narrowed.dpvv = dpnarrowed;
  return narrowed;
}

/*!
  Scale width and height of viewing frustum by the given ratio around
  the projection plane center axis.

  \sa scaleWidth(), scaleHeight().
 */
void
SbViewVolume::scale(float factor)
{
  this->dpvv.scaleWidth(factor);
  this->dpvv.scaleHeight(factor);
}

/*!
  Scale width of viewing frustum by the given ratio around the vertical
  center axis in the projection plane.

  \sa scale(), scaleHeight().
 */
void
SbViewVolume::scaleWidth(float ratio)
{
  this->dpvv.scaleWidth(ratio);
  this->dpvv.copyValues(*this);
}

/*!
  Scale height of viewing frustum by the given ratio around the horizontal
  center axis in the projection plane.

  \sa scale(), scaleWidth().
 */
void
SbViewVolume::scaleHeight(float ratio)
{
  this->dpvv.scaleHeight(ratio);
  this->dpvv.copyValues(*this);  
}

/*!
  Return current view volume projection type, which can be
  either \a ORTHOGRAPHIC or \a PERSPECTIVE.

  \sa SbViewVolume::ProjectionType
 */
SbViewVolume::ProjectionType
SbViewVolume::getProjectionType(void) const
{
  return this->type;
}

/*!
  Returns the projection point, i.e. the camera position.
*/
const SbVec3f&
SbViewVolume::getProjectionPoint(void) const
{
  return this->projPoint;
}

/*!
  Returns the direction of projection, i.e. the direction the camera is
  pointing.

  \sa getNearDist().
 */
const SbVec3f&
SbViewVolume::getProjectionDirection(void) const
{
  return this->projDir;
}

/*!
  Returns distance from projection plane to near clipping plane.

  \sa getProjectionDirection().
 */
float
SbViewVolume::getNearDist(void) const
{
  return this->nearDist;
}

/*!
  Returns width of viewing frustum in the projection plane.

  \sa getHeight(), getDepth().
*/
float
SbViewVolume::getWidth(void) const
{
  return static_cast<float>(this->dpvv.getWidth());
}

/*!
  Returns height of viewing frustum in the projection plane.

  \sa getWidth(), getDepth().
*/
float
SbViewVolume::getHeight(void) const
{
  return static_cast<float>(this->dpvv.getHeight());
}

/*!
  Returns depth of viewing frustum, i.e. the distance from the near clipping
  plane to the far clipping plane.

  \sa getWidth(), getHeight().
 */
float
SbViewVolume::getDepth(void) const
{
  return this->nearToFar;
}

/*!
  Dump the state of this object to the \a fp file stream. Only works in
  debug version of library, method does nothing in an optimized build.
 */
void
SbViewVolume::print(FILE * fp) const
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
SbViewVolume::getViewVolumePlanes(SbPlane planes[6]) const
{
  this->dpvv.getViewVolumePlanes(planes);
}

/*!
  Transform the viewing volume by \a matrix.
 */
void
SbViewVolume::transform(const SbMatrix & matrix)
{
  SbDPMatrix dpmatrix;
  copy_matrix(matrix, dpmatrix);
  this->dpvv.transform(dpmatrix);
  this->dpvv.copyValues(*this);
}

/*!
  Returns the view up vector for this view volume. It's a vector which
  is perpendicular to the projection direction, and parallel and
  oriented in the same direction as the vector from the lower left
  corner to the upper left corner of the near plane.
*/
SbVec3f
SbViewVolume::getViewUp(void) const
{
  return to_sbvec3f(this->dpvv.getViewUp());
}


/*!
  Returns TRUE if \a p is inside the view volume.

  \since Coin 2.3
*/
SbBool 
SbViewVolume::intersect(const SbVec3f & p) const
{
  SbPlane planes[6];
  this->getViewVolumePlanes(planes);
  for (int i = 0; i < 6; i++) {
    if (!planes[i].isInHalfSpace(p)) return FALSE;
  }
  return TRUE;
}

/*!  
  Returns TRUE if the line segment \a p0, \a p1 may intersect
  volume. Be aware that it is not 100% certain that the line segment
  intersects the volume even if this function returns TRUE.
  
  \a closestpoint is set to the closest point on the line
  to the center ray of the view volume.
  
  \since Coin 2.3
*/
SbBool 
SbViewVolume::intersect(const SbVec3f & p0, const SbVec3f & p1,
                        SbVec3f & closestpoint) const
{
  SbVec3f dummy;
  SbLine centerray(this->getProjectionPoint(),
                   this->getProjectionPoint() + this->getProjectionDirection());
  SbLine line(p0, p1);

  (void) line.getClosestPoints(centerray, closestpoint, dummy);
  
  // bah, lame. FIXME: pederb, 2003-02-12
  SbBox3f bbox;
  bbox.extendBy(p0);
  bbox.extendBy(p1);
  return this->intersect(bbox);
}

/*!
  Returns TRUE if \a box may be inside the view volume.

  \since Coin 2.3
 */
SbBool 
SbViewVolume::intersect(const SbBox3f & box) const
{
  int i, j;
  SbVec3f bmin, bmax;
  bmin = box.getMin();
  bmax = box.getMax();
  SbVec3f pts[8];

  // create the 8 box corner points
  for (i = 0; i < 8; i++) {
    pts[i][0] = i & 1 ? bmin[0] : bmax[0];
    pts[i][1] = i & 2 ? bmin[1] : bmax[1];
    pts[i][2] = i & 4 ? bmin[2] : bmax[2];
  }

  SbPlane planes[6];
  this->getViewVolumePlanes(planes);

  for (i = 0; i < 6; i++) {
    for (j = 0; j < 8; j++) {
      if (planes[i].isInHalfSpace(pts[j])) break;
    }
    if (j == 8) return FALSE;
  }
  return TRUE;
}

namespace {
  void clip_face(SbClip & clipper, const SbVec3f & v0, const SbVec3f & v1,
                 const SbVec3f & v2, const SbVec3f & v3,
                 const SbPlane * planes, SbBox3f & isect)
  {
    int i;
    clipper.addVertex(v0);
    clipper.addVertex(v1);
    clipper.addVertex(v2);
    clipper.addVertex(v3);
    for (i = 0; i < 6; i++) {
      clipper.clip(planes[i]);
    }
    const int n = clipper.getNumVertices();
    for (i = 0; i < n; i++) {
      SbVec3f tmp;
      clipper.getVertex(i, tmp);
      isect.extendBy(tmp);
    }
    clipper.reset();
  }
};

/*!
  Calculates the bbox of the intersection between \a bbox and the view volume.
  
  \since Coin 4.0
*/

SbBox3f 
SbViewVolume::intersectionBox(const SbBox3f & box) const
{
  int i;
  SbVec3f vvpts[8];
  SbBox3f commonVolume;
  SbVec3f bmin = box.getMin();
  SbVec3f bmax = box.getMax();
  SbPlane planes[6];

  //*****************************************************************************
  // find the 8 view volume corners
  this->getPlaneRectangle(0.0f, vvpts[0], vvpts[1], vvpts[2], vvpts[3]);
  this->getPlaneRectangle(this->nearToFar, vvpts[4], vvpts[5], vvpts[6], vvpts[7]);

  //*****************************************************************************
  // all all view volume points inside the original bbox
  for (i = 0; i < 8; i++) {
    if (box.intersect(vvpts[i])) commonVolume.extendBy(vvpts[i]);
  }

  //*****************************************************************************
  // add all bbox corner points inside the view volume
  this->getViewVolumePlanes(planes);
  int inside = 0;
  for (i = 0; i < 8; i++) {
    SbVec3f pt((i&1)?bmin[0]:bmax[0],
               (i&2)?bmin[1]:bmax[1],
               (i&4)?bmin[2]:bmax[2]);
    int j;
    for (j = 0; j < 6; j++) {
      if (!planes[j].isInHalfSpace(pt)) break;
    }
    if (j == 6) {
      commonVolume.extendBy(pt);
      inside++;
    }
  }
  if (inside==8) return commonVolume;
  
  //*****************************************************************************
  // clip the view volume against the bbox and add intersection points
  // to commonVolume
  //
  SbClip clipper;
  // generate the 6 bbox planes, all pointing towards the center of the box
  for (i = 0; i < 6; i++) {
    int dim = i/2;
    SbVec3f n(0.0f, 0.0f, 0.0f);
    n[dim] = (i&1) ? 1.0f : -1.0f;
    planes[i] = SbPlane(n, ((i&1) ? bmin[dim] : -bmax[dim]));
  }

  // clip view volume polygons against the bbox planes
  clip_face(clipper, vvpts[0], vvpts[1], vvpts[3], vvpts[2], planes, commonVolume);
  clip_face(clipper, vvpts[1], vvpts[5], vvpts[7], vvpts[3], planes, commonVolume);
  clip_face(clipper, vvpts[5], vvpts[4], vvpts[6], vvpts[7], planes, commonVolume);
  clip_face(clipper, vvpts[4], vvpts[0], vvpts[2], vvpts[6], planes, commonVolume);
  clip_face(clipper, vvpts[4], vvpts[5], vvpts[1], vvpts[0], planes, commonVolume);
  clip_face(clipper, vvpts[2], vvpts[3], vvpts[7], vvpts[6], planes, commonVolume);

  return commonVolume;
}

/*!
  Returns the double precision version of this view volume.
*/
const SbDPViewVolume & 
  SbViewVolume::getDPViewVolume(void) const
{
  return this->dpvv;
}


/*!
  Returns TRUE if all eight corner points in \a bmin, \a bmax is
  outside \a p.
*/
SbBool 
SbViewVolume::outsideTest(const SbPlane & p,
                          const SbVec3f & bmin, 
                          const SbVec3f & bmax) const
{
  int i;
  SbVec3f pt;
  for (i = 0; i < 8; i++) {
    pt[0] = i & 1 ? bmin[0] : bmax[0];
    pt[1] = i & 2 ? bmin[1] : bmax[1];
    pt[2] = i & 4 ? bmin[2] : bmax[2];
    
    if (p.isInHalfSpace(pt)) return FALSE;
  }
  return TRUE;
}

//
// Returns the four points defining the view volume rectangle at the
// specified distance from the near plane, towards the far plane.
void
SbViewVolume::getPlaneRectangle(const float distance, SbVec3f & lowerleft,
                                SbVec3f & lowerright,
                                SbVec3f & upperleft,
                                SbVec3f & upperright) const
{
  SbVec3f near_ur = this->ulf + (this->lrf - this->llf);
  
#if COIN_DEBUG
  if (this->llf == SbVec3f(0.0, 0.0, 0.0) ||
      this->lrf == SbVec3f(0.0, 0.0, 0.0) ||
      this->ulf == SbVec3f(0.0, 0.0, 0.0) ||
      near_ur == SbVec3f(0.0, 0.0, 0.0)) {
    SoDebugError::postWarning("SbDPViewVolume::getPlaneRectangle",
                              "Invalid frustum.");

  }
#endif // COIN_DEBUG

  if (this->type == PERSPECTIVE) {
    SbVec3f dir;
    dir = this->llf - this->projPoint;
    (void) dir.normalize(); // safe to normalize here
    lowerleft = this->llf + dir * distance / dir.dot(this->projDir);

    dir = this->lrf - this->projPoint;
    dir.normalize(); // safe to normalize here
    lowerright = this->lrf + dir * distance / dir.dot(this->projDir);

    dir = this->ulf - this->projPoint;
    (void) dir.normalize(); // safe to normalize here
    upperleft = this->ulf + dir * distance / dir.dot(this->projDir);

    dir = near_ur - this->projPoint;
    (void) dir.normalize(); // safe to normalize here
    upperright = near_ur + dir * distance / dir.dot(this->projDir);
  }
  else {
    lowerleft = this->llf + this->projDir * distance;
    lowerright = this->lrf + this->projDir * distance;
    upperleft = this->ulf + this->projDir * distance;
    upperright = near_ur + this->projDir * distance;
  }
}

#ifdef COIN_TEST_SUITE

#include <Inventor/SbBox3f.h>
#include <cfloat>

BOOST_AUTO_TEST_CASE(intersect_ortho)
{
  SbViewVolume vv;
  vv.ortho(-0.5, 0.5, -0.5, 0.5, -1, 10);
  SbBox3f box(0, 0, 0, 1, 1, 1);

  SbBox3f isect = vv.intersectionBox(box);
  COIN_TESTCASE_CHECK_FLOAT(isect.getMin()[0], 0.0f);
  COIN_TESTCASE_CHECK_FLOAT(isect.getMin()[1], 0.0f);
  COIN_TESTCASE_CHECK_FLOAT(isect.getMin()[2], 0.0f);
  COIN_TESTCASE_CHECK_FLOAT(isect.getMax()[0], 0.5f);
  COIN_TESTCASE_CHECK_FLOAT(isect.getMax()[1], 0.5f);
  COIN_TESTCASE_CHECK_FLOAT(isect.getMax()[2], 1.0f);
}

BOOST_AUTO_TEST_CASE(intersect_bbox_inside_vv)
{
  SbViewVolume vv;
  vv.ortho(-0.5, 0.5, -0.5, 0.5, -1, 10);
  SbBox3f box(-0.25, -0.25, -0.25, 0.25, 0.25, 0.25);

  SbBox3f isect = vv.intersectionBox(box);
  COIN_TESTCASE_CHECK_FLOAT(isect.getMin()[0], -0.25);
  COIN_TESTCASE_CHECK_FLOAT(isect.getMin()[1], -0.25f);
  COIN_TESTCASE_CHECK_FLOAT(isect.getMin()[2], -0.25f);
  COIN_TESTCASE_CHECK_FLOAT(isect.getMax()[0], 0.25f);
  COIN_TESTCASE_CHECK_FLOAT(isect.getMax()[1], 0.25f);
  COIN_TESTCASE_CHECK_FLOAT(isect.getMax()[2], 0.25f);
}

BOOST_AUTO_TEST_CASE(intersect_vv_inside_bbox)
{
  SbViewVolume vv;
  vv.ortho(-0.5, 0.5, -0.5, 0.5, 0, 5);
  SbBox3f box(-10, -10, -10, 10, 10, 10);

  SbBox3f isect = vv.intersectionBox(box);
  COIN_TESTCASE_CHECK_FLOAT(isect.getMin()[0], -0.5f);
  COIN_TESTCASE_CHECK_FLOAT(isect.getMin()[1], -0.5f);
  COIN_TESTCASE_CHECK_FLOAT(isect.getMin()[2], -5.0f);
  COIN_TESTCASE_CHECK_FLOAT(isect.getMax()[0], 0.5f);
  COIN_TESTCASE_CHECK_FLOAT(isect.getMax()[1], 0.5f);
  COIN_TESTCASE_CHECK_FLOAT(isect.getMax()[2], 0.0f);
}

BOOST_AUTO_TEST_CASE(intersect_perspective)
{
  // FIXME: set up a better perspective vv which also tests left/right/top/bottom
  SbViewVolume vv;
  vv.perspective(0.78f, 1.0f, 4.25, 4.75);
  vv.translateCamera(SbVec3f(0.0f, 0.0f, 5.0f));

  SbBox3f box(0, 0, 0, 1, 1, 1);
  SbBox3f isect = vv.intersectionBox(box);

  COIN_TESTCASE_CHECK_FLOAT(isect.getMin()[0], 0.0);
  COIN_TESTCASE_CHECK_FLOAT(isect.getMin()[1], 0.0f);
  COIN_TESTCASE_CHECK_FLOAT(isect.getMin()[2], 0.25f);
  COIN_TESTCASE_CHECK_FLOAT(isect.getMax()[0], 1.0f);
  COIN_TESTCASE_CHECK_FLOAT(isect.getMax()[1], 1.0f);
  COIN_TESTCASE_CHECK_FLOAT(isect.getMax()[2], 0.75f);
}

#endif // COIN_TEST_SUITE

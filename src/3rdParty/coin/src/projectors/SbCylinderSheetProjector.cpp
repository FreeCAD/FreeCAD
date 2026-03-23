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
  \class SbCylinderSheetProjector SbCylinderSheetProjector.h Inventor/projectors/SbCylinderSheetProjector.h
  \brief The SbCylinderSheetProjector class projects 2D points to 3D points on a sheet covering a cylindrical shape.

  \ingroup coin_projectors
 */

// FIXME: we do not use a hyperbolic sheet, as we're supposed to do,
// for this class. Instead we use a straight plane. This should hardly
// be noticeable for the user, but for correctness, a hyperbolic sheet
// should of course be used. 20000308 mortene.

#include <Inventor/projectors/SbCylinderSheetProjector.h>
#include <cfloat> // FLT_EPSILON

#if COIN_DEBUG
#include <Inventor/errors/SoDebugError.h>
#endif // COIN_DEBUG

/*! \var SbCylinderSheetProjector::workingProjPoint
  Last projected point, in the working space coordinate system.
*/
/*! \var SbCylinderSheetProjector::planeDir
  Normal vector of the plane defining the orientation of the sheet.
*/
/*! \var SbCylinderSheetProjector::tolPlane
  The tolerance value specifying how much of the cylinder is "above"
  the sheet.
*/


/*!
  Constructor. Uses default cylinder definition, see
  SbCylinderProjector::SbCylinderProjector().

  \a orienttoeye decides whether or not the cylinder and sheet should
  always be oriented towards the viewer.
*/
SbCylinderSheetProjector::SbCylinderSheetProjector(const SbBool orienttoeye)
  : inherited(orienttoeye)
{
}

/*!
  Constructor with explicit definition of projection cylinder.
*/
SbCylinderSheetProjector::SbCylinderSheetProjector(const SbCylinder & cyl,
                                                   const SbBool orienttoeye)
  : inherited(cyl, orienttoeye)
{
}

// Documented in superclass.
SbProjector *
SbCylinderSheetProjector::copy(void) const
{
  return new SbCylinderSheetProjector(*this);
}

// Documented in superclass.
SbVec3f
SbCylinderSheetProjector::project(const SbVec2f & point)
{
  if (this->needSetup) this->setupPlane();

  SbLine projline = this->getWorkingLine(point);
  SbVec3f projpt;

  // FIXME: add code to intersect hyperbolic sheet (see code in
  // SbSphereSheetProjector).
  //
  // Here's a complete, standalone example that can be used while
  // testing projection. It projects a grid on top of the
  // SbCylinderSheetProjector and spits out an iv-file with an
  // SoPointSet that shows off how the sheet will look:
  //
  //
  // -----8<--- [snip] -----8<--- [snip] -----8<--- [snip] ---
  // #include <cstdio>
  // #include <Inventor/SbLinear.h>
  // #include <Inventor/projectors/SbCylinderSheetProjector.h>
  // #include <Inventor/SoDB.h>
  //
  // int
  // main(void)
  // {
  //   SoDB::init();
  //
  //   const float START = 0.0f;
  //   const float END = 1.0f;
  //   const float STEPS = 50.0f;
  //   const float STEPSIZE = ((END - START) / STEPS);
  //
  //   SbCylinderSheetProjector ssp;
  //
  //   SbViewVolume volume;
  //   volume.ortho(-1, 1, -1, 1, -1, 1);
  //   ssp.setViewVolume(volume);
  //
  //   (void)fprintf(stdout, "#Inventor V2.1 ascii\n\n"
  //                 "Separator {\n"
  //                 "  Coordinate3 {\n"
  //                 "    point [\n");
  //
  //   for (float i=START; i <= END; i += STEPSIZE) {
  //     for (float j=START; j <= END; j += STEPSIZE) {
  //       SbVec3f v = ssp.project(SbVec2f(j, i));
  //       (void)fprintf(stdout, "\t%f %f %f,\n", v[0], v[1], v[2]);
  //     }
  //   }
  //
  //   (void)fprintf(stdout, "      ]\n"
  //                 "    }\n"
  //                 "  DrawStyle { pointSize 2 }\n"
  //                 "  PointSet { }\n"
  //                 "}\n");
  //
  //   return 0;
  // }
  // -----8<--- [snip] -----8<--- [snip] -----8<--- [snip] ---

  SbBool tst = this->intersectCylinderFront(projline, projpt);
  if (!tst) {
    if (!this->tolPlane.intersect(projline, projpt)) {
#if COIN_DEBUG
      SoDebugError::postWarning("SbCylinderSheetProjector::project",
                                "working line is parallel to cylinder axis.");
#endif // COIN_DEBUG
      return SbVec3f(0.0f, 0.0f, 0.0f);
    }
  }
  this->lastPoint = projpt;
  this->workingProjPoint = projpt; // FIXME: investigate (pederb)
  return projpt;
}

// Documented in superclass.
SbRotation
SbCylinderSheetProjector::getRotation(const SbVec3f & point1,
                                      const SbVec3f & point2)
{
  const SbLine & axis = this->cylinder.getAxis();
  SbVec3f v1 = point1 - axis.getClosestPoint(point1);
  SbVec3f v2 = point2 - axis.getClosestPoint(point2);
  SbRotation rot(v1, v2); // rotate vector v1 into vector v2

  // FIXME: add rotation from sheet (pederb)

  SbVec3f dummy;
  float angle;
  rot.getValue(dummy, angle);

  if (dummy.dot(axis.getDirection()) > 0.0f)
    return SbRotation(axis.getDirection(), angle);
  return SbRotation(axis.getDirection(), -angle);
}

/*!
  Recalculates projection surface settings after changes to the
  parameters.
*/
void
SbCylinderSheetProjector::setupPlane(void)
{
  const SbLine & axis = this->cylinder.getAxis();
  SbVec3f refDir;
  if (this->orientToEye) {
    refDir = -this->viewVol.getProjectionDirection();
    this->worldToWorking.multDirMatrix(refDir, refDir);
  }
  else {
    refDir = SbVec3f(0.0f, 0.0f, 1.0f);
  }
  SbVec3f somePt = axis.getPosition() + refDir;
  SbVec3f ptOnAxis = axis.getClosestPoint(somePt);

  this->planeDir = somePt - ptOnAxis;

  if (this->planeDir.normalize() < FLT_EPSILON) {
    // the cylinder axis is parallel to the view direction. This is a
    // special case, and not really supported by the projector. Just
    // create a tilted plane to make it possible to rotate the
    // cylinder even for this case.
    this->planeDir = this->viewVol.getViewUp() + 
      this->viewVol.getProjectionDirection();
    this->worldToWorking.multDirMatrix(this->planeDir, this->planeDir);    
    (void) this->planeDir.normalize();
  }

  if (!this->intersectFront) this->planeDir = -this->planeDir;

  this->tolPlane = SbPlane(this->planeDir, axis.getPosition());
  this->needSetup = FALSE;
}

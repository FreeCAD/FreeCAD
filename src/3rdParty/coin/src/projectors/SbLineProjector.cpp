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
  \class SbLineProjector SbLineProjector.h Inventor/projectors/SbLineProjector.h
  \brief The SbLineProjector class projects 2D points to 3D points along a line.

  \ingroup coin_projectors

  The 3D projection of the 2D coordinates is for this projector class
  constrained to lie along a predefined line.

  Among other places, this is useful within the translation draggers,
  like for instance SoTranslate1Dragger, where we want to move
  "pieces" along one or more axes.
*/

#include <Inventor/projectors/SbLineProjector.h>
#include <Inventor/SbPlane.h>
#include <Inventor/SbVec2f.h>
#include <cassert>

/*!
  \var SbLineProjector::line

  The projection line. Projected 3D points will be constrained to be
  on this line.
*/
/*!
  \var SbLineProjector::lastPoint

  The last projected point.
*/


/*!
  Constructor. Initializes the projector instance to use a line from
  <0, 0, 0> to <0, 1, 0>.
 */
SbLineProjector::SbLineProjector(void)
  : line(SbVec3f(0.0f, 0.0f, 0.0f), SbVec3f(0.0f, 1.0f, 0.0f)),
    lastPoint(0.0f, 0.0f, 0.0f)
{
}

// Documented in superclass
SbBool 
SbLineProjector::tryProject(const SbVec2f & point, const float epsilon, SbVec3f & result)
{
  // first project the line into screen space to find the 2D point
  // closest to the projection line there. Then use that 2D point to
  // find the best projection point.
  SbLine wrldline;
  this->workingToWorld.multLineMatrix(this->line, wrldline);

  SbVec3f pt1 = wrldline.getPosition();
  SbVec3f pt2 = pt1 + wrldline.getDirection();
  
  this->viewVol.projectToScreen(pt1, pt1);
  this->viewVol.projectToScreen(pt2, pt2);

  // account for the view volume aspect ratio when creating the screen space line
  const float vvwidth  = (this->viewVol.getWidth()  == 0.0f) ? 1.0f : this->viewVol.getWidth();
  const float vvheight  = (this->viewVol.getHeight() == 0.0f) ? 1.0f : this->viewVol.getHeight();
  
  pt1 = SbVec3f(pt1[0]*vvwidth, pt1[1]*vvheight, 0.0f);
  pt2 = SbVec3f(pt2[0]*vvwidth, pt2[1]*vvheight, 0.0f);
  SbVec2f newpt = point;
  
  if (pt1 == pt2) newpt = SbVec2f(pt1[0], pt1[1]);
  else {
    SbVec3f vppoint(point[0] * vvwidth, point[1] * vvheight, 0.0f);
    SbLine scrline(pt1, pt2);
    SbVec3f dummy = scrline.getClosestPoint(vppoint);
    newpt = SbVec2f(dummy[0], dummy[1]);
  }
  // move back from screen space to the normalized position
  newpt = SbVec2f(newpt[0]/vvwidth, newpt[1]/vvheight);

  SbLine projline = this->getWorkingLine(newpt);
  SbVec3f projpt, dummy;

  // check how parallel the lines are
  SbBool nonparallel = TRUE;  
  if (epsilon > 0.0f) {
    const SbViewVolume & vv = this->getViewVolume();
    float dot = SbAbs(wrldline.getDirection().dot(vv.getProjectionDirection())); 
    nonparallel = SbAbs(1.0f - dot) > epsilon;
    // need to do some extra work to check angle for perspective projections
    if (!nonparallel && (vv.getProjectionType() == SbViewVolume::PERSPECTIVE)) {
      SbPlane nearplane = vv.getPlane(vv.getNearDist());
      SbVec3f nearpt;
      if (nearplane.intersect(wrldline, nearpt)) {
        SbVec3f dir = nearpt - vv.getProjectionPoint();
        (void)dir.normalize();
        dot = SbAbs(dir.dot(vv.getProjectionDirection())); 
        nonparallel = SbAbs(1.0f - dot) > epsilon;
      }
      else nonparallel = TRUE;
    }
  }
  
  if (nonparallel) {
    nonparallel = this->line.getClosestPoints(projline, projpt, dummy);
  }
  // if lines are parallel, we will never get an intersection, and
  // we set projection point to the middle of the view volume
  if (!nonparallel) {
    float depth = this->viewVol.getNearDist() +
      this->viewVol.getDepth() * 0.5f;
    SbPlane plane = this->viewVol.getPlane(depth);
    if (!plane.intersect(wrldline, projpt)) {
      assert(0 && "should never happen");
      projpt = SbVec3f(0.0f, 0.0f, 0.0f);
    }
    else this->worldToWorking.multVecMatrix(projpt, projpt);
  }
  else if (!this->verifyProjection(projpt)) {
    float depth = this->findVanishingDistance();
    SbPlane plane = this->viewVol.getPlane(depth);
    if (!plane.intersect(wrldline, projpt)) {
      assert(0 && "should never happen");
      projpt = SbVec3f(0.0f, 0.0f, 0.0f);
    }
    else {
      this->worldToWorking.multVecMatrix(projpt, projpt);
    }
  }

  result = projpt;
  if (nonparallel) {
    this->lastPoint = projpt;
  }
  return nonparallel;
}

// Documented in superclass.
SbVec3f
SbLineProjector::project(const SbVec2f & point)
{
  SbVec3f ret;
  (void) this->tryProject(point, 0.0f, ret);
  this->lastPoint = ret;
  return ret;
}

/*!
  Set a new projection line. 3D points will be mapped to be on this
  line.
 */
void
SbLineProjector::setLine(const SbLine & lineref)
{
  this->line = lineref;
}

/*!
  Returns the currently set projection line.
 */
const SbLine&
SbLineProjector::getLine(void) const
{
  return this->line;
}

/*!
  Calculates and returns a vector between the projected 3D position of
  \a viewpos1 and \a viewpos2.
*/
SbVec3f
SbLineProjector::getVector(const SbVec2f & viewpos1, const SbVec2f & viewpos2)
{
  SbVec3f mp1 = this->project(viewpos1);
  SbVec3f mp2 = this->project(viewpos2);
  this->lastPoint = mp2;
  return mp2 - mp1;
}

/*!
  Returns the 3D vector between the last projection and the one
  calculated for \a viewpos.
*/
SbVec3f
SbLineProjector::getVector(const SbVec2f & viewpos)
{
  SbVec3f lp = this->lastPoint; // lastPoint is updated in project()
  return (this->project(viewpos) - lp);
}

/*!
  Explicitly set position of initial projection, so we get correct
  values for later calls to getVector() etc.
*/
void
SbLineProjector::setStartPosition(const SbVec2f & viewpos)
{
  this->lastPoint = this->project(viewpos);
}

/*!
  Explicitly set position of initial projection, so we get correct
  values for later calls to getVector() etc.
*/
void
SbLineProjector::setStartPosition(const SbVec3f & point)
{
  this->lastPoint = point;
}

// Documented in superclass.
SbProjector *
SbLineProjector::copy(void) const
{
  return new SbLineProjector(*this);
}

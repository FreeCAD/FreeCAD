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
  \class SoGetBoundingBoxAction SoGetBoundingBoxAction.h Inventor/actions/SoGetBoundingBoxAction.h
  \brief The SoGetBoundingBoxAction class calculates bounding boxes for nodes and subgraphs.

  \ingroup coin_actions

  If this action is applied to a path or scene graph root, it will
  calculate the bounding box and the center point of the geometry
  contained within the scene.

  You don't have to apply an SoGetBoundingBoxAction to the \e root of
  a scene. When using the action, you will get the bounding box of the
  node you are applying it to and that node's subtree in the scene
  graph (if any).

  The calculated bounding box will be in the local coordinates of that
  subtree. If applying it to a scene graph root node, the calculated
  bounding box will be in global coordinates.

  The use of bounding boxes is ubiquitous within the Coin library. It
  is needed for the correct execution of and for performance
  enhancements during rendering, picking, caching, culling, etc.


  SoSeparator nodes are aggressively caching the results of bounding
  box calculations, so that they are really only recalculated
  whenever the scene graph rooted below any SoSeparator node has been
  modified. This means that applying this action to scene graphs, or
  parts of scene graphs, should be very quick on successive runs for
  "static" parts of the scene.

  Note that the algorithm used is not guaranteed to always give an
  exact bounding box: it combines bounding boxes in pairs and extends
  one of them to contain the other. Since the boxes need not be
  parallel to the principal axes the new box might not be a perfect
  fit for the box not extended (its coordinate system has been
  changed).

  Note also that what is returned from getBoundingBox() will be
  projected so as to be oriented along the principal axes, which can
  often cause it to become quite a lot larger than what it was before
  projection. For client code to obtain the best bounding box that
  Coin can calculate (and which will usually be exact), you need to
  use the getXfBoundingBox() method after having applied the
  SoGetBoundingBoxAction.

  \sa SoSeparator::boundingBoxCaching
*/

#include <Inventor/actions/SoGetBoundingBoxAction.h>

#include <Inventor/elements/SoBBoxModelMatrixElement.h>
#include <Inventor/elements/SoLocalBBoxMatrixElement.h>
#include <Inventor/elements/SoViewingMatrixElement.h>
#include <Inventor/elements/SoViewportRegionElement.h>
#include <Inventor/lists/SoEnabledElementsList.h>
#include <Inventor/misc/SoState.h>
#include <Inventor/nodes/SoNode.h>

#if COIN_DEBUG
#include <Inventor/errors/SoDebugError.h>
#endif // COIN_DEBUG

#include "actions/SoSubActionP.h"
#include "SbBasicP.h"

// FIXME: kristian investigated the assumed bug-cases listed below,
// and found that it is fundamentally impossible making a perfect fit
// around any scene graph geometry with the current strategy of
// combining SbXfBox3f instances. (To get a perfect fit, we'd need a
// "memory" of all boxes used for combining into larger ones.)
//
// Seems hard to fix without breaking or extending the API in
// non-trivial ways -- but it could perhaps be done (SoGroup-derived
// nodes would at least have to store bboxes and transforms from all
// their children, and another algorithm than the current simple
// "combine two and two" would have to be run over this list).
//
// The bug-reports stored below for reference, as they would be nice
// to use for testing if we ever decide to try to fix the defect(s)
// with the bbox accumulations.
//
// 20030116 mortene.
//
// ======================================================================
//
//  013 Bounding box calculation of the scene graph given below is
//      sub-optimal.
//
//      ----8<--- [snip] ---------8<--- [snip] ---------8<--- [snip] ---
//      #Inventor V2.1 ascii
//
//      Separator {
//         Separator {
//            Cube { }
//
//            BaseColor { rgb 1 0 0 }
//            Translation { translation +4 0 0 }
//            Separator {
//               Transform {
//                  translation 0 -0.5 0
//                  rotation 0 0 1 0.78
//                  scaleFactor 0.5 2 3
//                  scaleOrientation 1 0 0 0.78
//                  center 0.5 0.5 0.5
//               }
//               Cube { }
//            }
//         }
//
//         Translation { translation 0 +6 0 }
//         Separator {
//            Cube { }

//            BaseColor { rgb 1 0 0 }
//            Translation { translation +4 0 0 }
//            Separator {
//               Transform {
//                  translation 10 -0.5 0 ~
//                  rotation 0 0 1 0.78
//                  scaleFactor 0.5 2 3
//                  scaleOrientation 1 0 0 0.78
//                  center 0.5 0.5 0.5
//               }
//               Cube { }
//            }
//         }
//      }
//      ----8<--- [snip] ---------8<--- [snip] ---------8<--- [snip] ---
//
//      A good opening gambit for investigating the bug is using the
//      SoGuiExamples/engines/computexfbox example code to load the scene
//      and view the resulting bbox.
//
//      mortene 20020729.
//
// ======================================================================
//
//  022 Sub-optimal bounding box calculations.
//
//      The fairly simple scene graph below results in a rather sub-optimal
//      bounding box being calculated. For a good view of how it is, use
//      the SoGuiExamples/engines/computexfbbox example.
//
//      ----8<--- [snip] ---------8<--- [snip] ---------8<--- [snip] ---
//      #Inventor V2.1 ascii
//
//      Separator {
//         LightModel { model BASE_COLOR }
//
//         Cube { height 5 }
//
//         Separator {
//            Rotation { rotation 1 0 0  0.7854 }
//            Cube { }
//         }
//
//         Rotation { rotation 2 3 9  1.5708 }
//         Cube { height 4 }
//      }
//      ----8<--- [snip] ---------8<--- [snip] ---------8<--- [snip] ---
//
//      (Note that this scene graph does not show a _grave_ bbox error. I
//      prioritized getting it as small as possible while still
//      demonstrating that there *is* an error. I have the original
//      scene graph which I constructed this from, where the bbox is _way_
//      off.)
//
//      UPDATE 20020830 mortene: I used SGI Inventor to check both the
//      bounding box of the minimal case above and the larger scene where
//      it comes out fairly sub-optimal for us -- and the original SGI
//      Inventor doesn't make any tighter fit than we are. So there might
//      be something fundamental about the case above which makes it
//      impossible to have SbXfBox3f.extendBy(SbXfbox3f) come out with a
//      perfect fit?  Need to investigate.
//
//      20020826 mortene.
//
// ======================================================================


// *************************************************************************

/*!
  \enum SoGetBoundingBoxAction::ResetType
  \COININTERNAL
*/

class SoGetBoundingBoxActionP {
public:
};

SO_ACTION_SOURCE(SoGetBoundingBoxAction);


/*!
  \copydetails SoAction::initClass(void)
*/
void
SoGetBoundingBoxAction::initClass(void)
{
  SO_ACTION_INTERNAL_INIT_CLASS(SoGetBoundingBoxAction, SoAction);

  SO_ENABLE(SoGetBoundingBoxAction, SoViewportRegionElement);
}

/*!
  Constructor.

  It might seem unnecessary to have to pass in a viewport region
  argument to calculate bounding boxes, but there is a good reason for
  this: a few shape nodes needs to know the viewport region to
  calculate their bounding box -- these include SoText2 and SoImage,
  among others.

  What is particular about these shapes is that they are fundamentally
  2D shapes, but they are being rendered on the screen "surface" as if
  they were in a 3D scene. (This is possible because we can match each
  pixel's depth value against the 3D shapes in the scene.)

  To compute an accurate 3D bounding box of a shape rendered in 2D on
  the screen "surface", you need to "de-project" the screen space area
  it occupies to a 2D rectangle placed at some depth in the
  scene. This "de-projecting" operation needs to know about the
  dimensions of the viewport.

  Also, some 3D shapes like for instance SoNurbsSurface, get slightly
  distorted if there's an SoComplexity node in the scene graph with the
  SoComplexity::value field set to SCREEN_SPACE. Then it is also
  necessary to know the viewport region to find out how to accurately
  calculate the bounding box of those shapes.

  You would usually want to pass in a viewport region equal to the
  layout of the current render area canvas. If you have a viewer or
  So\@Gui\@RenderArea available, you can get hold of the viewport region
  data simply by doing

  \code
     const SbViewportRegion & vpreg = viewer->getViewportRegion();
  \endcode

  (If you don't have a viewer or render area available in your
  application at the point where you want to get the bounding box, it
  probably doesn't matter much what you set it to. The accuracy of the
  bounding box calculation might be slightly wrong versus the actual
  rendered appearance of the scene, but this is usually not
  noticeable.)
*/
SoGetBoundingBoxAction::SoGetBoundingBoxAction(const SbViewportRegion & vp)
  : center(0, 0, 0),
    vpregion(vp),
    resettype(SoGetBoundingBoxAction::ALL),
    resetpath(NULL),
    flags(SoGetBoundingBoxAction::RESET_BEFORE)
{
  SO_ACTION_CONSTRUCTOR(SoGetBoundingBoxAction);
}

/*!
  Destructor.
*/
SoGetBoundingBoxAction::~SoGetBoundingBoxAction()
{
}

// *************************************************************************

/*!
  Set a new viewport region with this method, if it has changed from
  the one passed in with the constructor.
*/
void
SoGetBoundingBoxAction::setViewportRegion(const SbViewportRegion & newregion)
{
  this->vpregion = newregion;
}

/*!
  Returns the viewport region used by the action instance.
*/
const SbViewportRegion &
SoGetBoundingBoxAction::getViewportRegion(void) const
{
  return this->vpregion;
}

/*!
  Returns the projected bounding box after (or during) traversal.
*/
SbBox3f
SoGetBoundingBoxAction::getBoundingBox(void) const
{
  return this->bbox.project();
}

/*!
  Returns the bounding box and transformation matrix to global
  coordinates. Use after (or during) traversal.
*/
SbXfBox3f &
SoGetBoundingBoxAction::getXfBoundingBox(void)
{
  return this->bbox;
}

/*!
  Returns center point of scene after the action has been applied.

  This might differ from the geometric center of the bounding box, as
  shape nodes may "weight" the center point according to various
  criteria (i.e. a faceset could for instance weight the center point
  according to the area within its bounding box where there are more
  polygons).
*/
const SbVec3f &
SoGetBoundingBoxAction::getCenter(void) const
{
  if (!this->isCenterSet()) {
    // Cast away constness and set.
    SoGetBoundingBoxAction * action = const_cast<SoGetBoundingBoxAction *>(this);
    action->center.setValue(0.0f, 0.0f, 0.0f);
  }
  // Center point should not be affected by the current
  // transformation.
  return this->center;
}

/*!
  Sets whether the returned bounding box should be calculated in the
  coordinate system of the camera space or not.
*/
void
SoGetBoundingBoxAction::setInCameraSpace(const SbBool on)
{
  if (on) this->flags |= SoGetBoundingBoxAction::CAMERA_SPACE;
  else this->flags &= ~SoGetBoundingBoxAction::CAMERA_SPACE;
}

/*!
  Returns whether the bounding box returned is to be in camera space.
*/
SbBool
SoGetBoundingBoxAction::isInCameraSpace(void) const
{
  return (this->flags & SoGetBoundingBoxAction::CAMERA_SPACE) != 0;
}

/*!
  Forces the computed bounding box to be reset and the transformation
  to be identity before or after the tail node of \a path, depending
  on the \a resetbefore argument.  \c NULL can be specified for the \a
  path argument to disable this behavior.

  \sa getResetPath(), isResetPath(), isResetBefore(), getWhatReset()
*/

void
SoGetBoundingBoxAction::setResetPath(const SoPath * path,
                                     const SbBool resetbefore,
                                     const ResetType what)
{
  this->resetpath = path;
  this->resettype = what;
  if (resetbefore) this->flags |= SoGetBoundingBoxAction::RESET_BEFORE;
  else this->flags &= ~SoGetBoundingBoxAction::RESET_BEFORE;
}

/*!
  Returns the reset path (or \c NULL).

  \sa setResetPath(), isResetPath(), isResetBefore(), getWhatReset()
*/
const SoPath *
SoGetBoundingBoxAction::getResetPath(void) const
{
  return this->resetpath;
}

/*!
  Returns whether a reset path is set or not.

  \sa setResetPath(), getResetPath(), isResetBefore(), getWhatReset()
*/
SbBool
SoGetBoundingBoxAction::isResetPath(void) const
{
  return this->resetpath != NULL;
}

/*!
  Returns whether the bounding box and transformation is reset before
  or after the tail node of the reset path.

  \sa setResetPath(), getResetPath(), isResetPath(), getWhatReset()
*/
SbBool
SoGetBoundingBoxAction::isResetBefore(void) const
{
  return (this->flags & SoGetBoundingBoxAction::RESET_BEFORE) != 0;
}

/*!
  Returns what type of reset has been specified for the reset path.

  \sa setResetPath(), getResetPath(), isResetPath(), isResetBefore()
*/
SoGetBoundingBoxAction::ResetType
SoGetBoundingBoxAction::getWhatReset(void) const
{
  return this->resettype;
}

/*!
  \COININTERNAL
  Called before node traversal of each node (from SoNode action method).
*/
void
SoGetBoundingBoxAction::checkResetBefore(void)
{
  if (this->resetpath && this->isResetBefore()) {
    const SoFullPath * curpath = reclassify_cast<const SoFullPath *>(this->getCurPath());
    const SoFullPath * theresetpath = reclassify_cast<const SoFullPath *>(this->resetpath);
    if ((curpath->getTail() == theresetpath->getTail()) &&
        curpath->containsPath(theresetpath)) {
      if (this->resettype & SoGetBoundingBoxAction::TRANSFORM) {
        SoBBoxModelMatrixElement::reset(this->getState(), curpath->getTail());
      }
      if (this->resettype & SoGetBoundingBoxAction::BBOX) {
        this->bbox.makeEmpty();
        this->bbox.setTransform(SbMatrix::identity());
        this->resetCenter();
      }
    }
  }
}

/*!
  \COININTERNAL
  Called after node traversal of each node (from SoNode action method).
*/
void
SoGetBoundingBoxAction::checkResetAfter(void)
{
  if (this->resetpath && !this->isResetBefore()) {
    const SoFullPath * curpath = reclassify_cast<const SoFullPath *>(this->getCurPath());
    const SoFullPath * theresetpath = reclassify_cast<const SoFullPath *>(this->resetpath);
    if ((curpath->getTail() == theresetpath->getTail()) &&
        curpath->containsPath(theresetpath)) {
      if (this->resettype & SoGetBoundingBoxAction::TRANSFORM) {
        SoBBoxModelMatrixElement::reset(this->getState(), curpath->getTail());
      }
      if (this->resettype & SoGetBoundingBoxAction::BBOX) {
        this->bbox.makeEmpty();
        this->bbox.setTransform(SbMatrix::identity());
        this->resetCenter();
      }
    }
  }
}

/*!
  Extend bounding box by the given \a box. Called from nodes during
  traversal.

  Should usually not be of interest to application programmers, unless
  you're extending Coin with your own shapenode extension classes.
*/
void
SoGetBoundingBoxAction::extendBy(const SbBox3f & box)
{
  if (box.isEmpty()) {
#if COIN_DEBUG
    SoDebugError::postWarning("SoGetBoundingBoxAction::extendBy", "empty box");
#endif // COIN_DEBUG
    return;
  }

  SbXfBox3f xfbox = box;
  SbMatrix transform = SoLocalBBoxMatrixElement::get(this->getState());
  if (this->isInCameraSpace()) {
    transform.multRight(SoViewingMatrixElement::get(this->getState()));
  }

  xfbox.transform(transform);
  this->bbox.extendBy(xfbox);
}

/*! \overload */
void
SoGetBoundingBoxAction::extendBy(const SbXfBox3f & box)
{
  if (box.isEmpty()) {
#if COIN_DEBUG
    SoDebugError::postWarning("SoGetBoundingBoxAction::extendBy", "empty box");
#endif // COIN_DEBUG
    return;
  }

  SbXfBox3f lbox = box;
  SbMatrix transform = SoLocalBBoxMatrixElement::get(this->state);
  if (this->isInCameraSpace()) {
    transform.multRight(SoViewingMatrixElement::get(this->state));
  }
  lbox.transform(transform);
  this->bbox.extendBy(lbox);
}

/*!
  \COININTERNAL
  Set a new center point during traversal.
*/
void
SoGetBoundingBoxAction::setCenter(const SbVec3f & centerarg,
                                  const SbBool transformcenter)
{
  assert(!this->isCenterSet());
  this->flags |= SoGetBoundingBoxAction::CENTER_SET;

  if (transformcenter) {
    SbMatrix lmat = SoLocalBBoxMatrixElement::get(this->state);
    if (this->isInCameraSpace()) {
      lmat.multRight(SoViewingMatrixElement::get(this->state));
    }
    lmat.multVecMatrix(centerarg, this->center);
  }
  else {
    this->center = centerarg;
  }

#if COIN_DEBUG && 0 // debug
  SoDebugError::postInfo("SoGetBoundingBoxAction::setCenter",
                         "center: <%f, %f, %f>, transformcenter: %s, "
                         "this->center: <%f, %f, %f>",
                         centerarg[0], centerarg[1], centerarg[2],
                         transformcenter ? "TRUE" : "FALSE",
                         this->center[0], this->center[1], this->center[2]);
#endif // debug
}

/*!
  \COININTERNAL
  Query about the center point during traversal.
*/
SbBool
SoGetBoundingBoxAction::isCenterSet(void) const
{
  return (this->flags & SoGetBoundingBoxAction::CENTER_SET) != 0;
}

/*!
  \COININTERNAL
  Reset the scene center point during traversal.
*/
void
SoGetBoundingBoxAction::resetCenter(void)
{
  this->flags &= ~SoGetBoundingBoxAction::CENTER_SET;
  this->center.setValue(0.0f, 0.0f, 0.0f);
}

// Documented in superclass. Overridden to reset center point and
// bounding box before traversal starts.
void
SoGetBoundingBoxAction::beginTraversal(SoNode * node)
{
  this->resetCenter();
  this->bbox.makeEmpty();

  SoViewportRegionElement::set(this->getState(), this->vpregion);
  inherited::beginTraversal(node);
}

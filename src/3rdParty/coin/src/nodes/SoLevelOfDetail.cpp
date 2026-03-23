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
  \class SoLevelOfDetail SoLevelOfDetail.h Inventor/nodes/SoLevelOfDetail.h
  \brief The SoLevelOfDetail class is used to choose a child based on projected size.

  \ingroup coin_nodes

  A level-of-detail mechanism is typically used by application
  programmers to assist the library in speeding up the rendering.

  The way a level-of-detail mechanism works is basically like this:

  Several versions of varying complexity of \e the \e same geometry /
  shape is provided by the application programmer in sorted order from
  "most complex" to "least complex" (where "complex" in this context
  should be taken to mean more or less detailed in the number of
  polygons / shapes used for rendering it).

  The runtime rendering system then, upon scene graph traversal, will
  on-the-fly calculate either the distance from the camera to the
  3D-model in question, or the number of pixels in the screen
  projection of the 3D-model. This value is then used to decide which
  version of the model to actually render: as the model is moved
  farther away from the camera, a less detailed version of the model
  is used. And vice versa, as the model moves closer to the camera,
  more and more detailed versions of it are rendered.

  This is under many different circumstances a very effective way to
  let the application programmer assist to \e profoundly optimize the
  rendering of her 3D-scene.

  There is of course a trade-off with the level-of-detail technique:
  more versions of the same 3D model means the scene graph will use up
  more application memory resources. Also, generating the set of less
  and less detailed versions of a 3D model from the original is often
  not a trivial task to do properly. The process is often assisted by
  software like what Kongsberg Oil & Gas Technologies offers in their
  <a href="http://www.rational-reducer.com>Rational Reducer</a> package.


  The SoLevelOfDetail node implements the "projected size" variety
  level-of-detail technique (as opposed to the "object distance"
  technique, as done by the SoLOD node).

  The node works by comparing the current projected size of the
  smallest rectangle covering the bounding box of its child geometry.
  

  Along with this set of models of the same shape, a specification of
  when to switch between them is also provided.

  Example scene graph layout:

  \code
  LevelOfDetail {
     screenArea [ 2000, 500, 50 ]

     DEF version-0 Separator {
       # most complex / detailed / heavy version of subgraph
     }
     DEF version-1 Separator {
       # less complex version of subgraph
     }
     DEF version-2 Separator {
       # even less complex version of subgraph
     }
     DEF version-3 Separator {
       # simplest / "lightest" version of subgraph
     }
  }
  \endcode

  The way the above sub-scene graph would work would be the following:
  if the rectangular area around the model's projected bounding box
  covers \e more than 2000 pixels (meaning it will be up pretty close
  to the camera), the most complex version of the model (\c version-0)
  would be traversed (and rendered, of course). If the projected area
  would be \e between 500 and 2000 pixels, the \c version-1 model
  would be used. Ditto if the projected area was between 50 and 500
  pixels, the \c version-2 version of the model would be
  used. Finally, if the projected bounding box area would be \e less
  than 50 square pixels, the presumably least detailed version of the
  model would be used.

  (A common "trick" is to let the last of the SoLevelOfDetail node's
  children be just an empty subgraph, so no geometry will be rendered
  at all if the model is sufficiently far away. This will of course
  have a positive effect on the total rendering time for the complete
  scene graph.)

  Note that the SoLevelOfDetail::screenArea vector will be influenced
  by preceding SoComplexity nodes in the following way: if
  SoComplexity::value is set from 0.0 up to 0.5, lower detail levels
  than normal will be selected for traversal. If SoComplexity::value
  is above 0.5, higher level details than normal will be used. An
  SoComplexity::value equal to 1.0 will cause the first child of
  SoLevelOfDetail to always be used.


  As mentioned above, there is one other level-of-detail node in the
  Coin library: SoLOD. The difference between that one and this is
  just that instead of projected bounding box area, SoLOD uses the
  distance between the camera and the object to find out when to
  switch between the different model versions.

  Using SoLOD is faster, since figuring out the projected bounding box
  area needs a certain amount of calculations. But using
  SoLevelOfDetail is often "better", in the sense that it's really a
  model's size and visibility in the viewport that determines whether
  we could switch to a less complex version without losing enough
  detail that it gives a noticeable visual degradation.

  <b>FILE FORMAT/DEFAULTS:</b>
  \code
    LevelOfDetail {
        screenArea 0
    }
  \endcode

  \sa SoLOD
*/

// *************************************************************************

#include <Inventor/nodes/SoLevelOfDetail.h>

#include <cstdlib>

#include <Inventor/actions/SoAudioRenderAction.h>
#include <Inventor/actions/SoCallbackAction.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/actions/SoGetBoundingBoxAction.h>
#include <Inventor/caches/SoBoundingBoxCache.h>
#include <Inventor/caches/SoBoundingBoxCache.h>
#include <Inventor/elements/SoCacheElement.h>
#include <Inventor/elements/SoComplexityElement.h>
#include <Inventor/elements/SoGLCacheContextElement.h>
#include <Inventor/elements/SoLocalBBoxMatrixElement.h>
#include <Inventor/elements/SoViewportRegionElement.h>
#include <Inventor/misc/SoChildList.h>
#include <Inventor/misc/SoState.h>
#include <Inventor/nodes/SoShape.h>
#include <Inventor/threads/SbStorage.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#ifdef COIN_THREADSAFE
#include <Inventor/threads/SbMutex.h>
#endif // COIN_THREADSAFE

#include "tidbitsp.h"
#include "nodes/SoSubNodeP.h"

// *************************************************************************

typedef struct {
  SoGetBoundingBoxAction * bboxaction;
} so_lod_static_data;

static void
so_lod_construct_data(void * closure)
{
  so_lod_static_data * data = (so_lod_static_data*) closure;
  data->bboxaction = NULL;
}

static void
so_lod_destruct_data(void * closure)
{
  so_lod_static_data * data = (so_lod_static_data*) closure;
  delete data->bboxaction;
}

static SbStorage * so_lod_storage = NULL;

// called from atexit
static void
so_lod_cleanup(void)
{
  delete so_lod_storage;
}

static SoGetBoundingBoxAction *
so_lod_get_bbox_action(void)
{
  so_lod_static_data * data = NULL;
  data = (so_lod_static_data*) so_lod_storage->get();
  
  if (data->bboxaction == NULL) {
    // The viewport region will be replaced every time the action is
    // used, so we can just feed it a dummy here.
    data->bboxaction = new SoGetBoundingBoxAction(SbViewportRegion());
  }
  return data->bboxaction;
}

// *************************************************************************

/*!
  \var SoMFFloat SoLevelOfDetail::screenArea

  The screen area limits for the children. See usage example in main
  class documentation of SoLevelOfDetail for an explanation of how
  this vector should be set up correctly.

  By default this vector just contains a single value 0.0f.
*/

// *************************************************************************


#ifndef DOXYGEN_SKIP_THIS

class SoLevelOfDetailP {
public:
  SoBoundingBoxCache * bboxcache;
#ifdef COIN_THREADSAFE
  // FIXME: a mutex for every instance seems a bit excessive,
  // especially since Microsoft Windows might have rather strict limits on the
  // total amount of mutex resources a process (or even a user) can
  // allocate. so consider making this a class-wide instance instead.
  // -mortene.
  SbMutex mutex;
#endif // COIN_THREADSAFE

  void lock(void) {
#ifdef COIN_THREADSAFE
    this->mutex.lock();
#endif // COIN_THREADSAFE 
 }
  void unlock(void) {
#ifdef COIN_THREADSAFE
    this->mutex.unlock();
#endif // COIN_THREADSAFE
  }
};

#endif // DOXYGEN_SKIP_THIS


SO_NODE_SOURCE(SoLevelOfDetail);

#define PRIVATE(obj) ((obj)->pimpl)

/*!
  Default constructor.
*/
SoLevelOfDetail::SoLevelOfDetail(void)
{
  this->commonConstructor();
}

/*!
  Constructor.

  The argument should be the approximate number of children which is
  expected to be inserted below this node. The number need not be
  exact, as it is only used as a hint for better memory resource
  allocation.
*/
SoLevelOfDetail::SoLevelOfDetail(int numchildren)
  : inherited(numchildren)
{
  this->commonConstructor();
}

// private
void
SoLevelOfDetail::commonConstructor(void)
{
  PRIVATE(this) = new SoLevelOfDetailP;
  PRIVATE(this)->bboxcache = NULL;

  SO_NODE_INTERNAL_CONSTRUCTOR(SoLevelOfDetail);

  SO_NODE_ADD_FIELD(screenArea, (0));
}

/*!
  Destructor.
*/
SoLevelOfDetail::~SoLevelOfDetail()
{
  if (PRIVATE(this)->bboxcache) PRIVATE(this)->bboxcache->unref();
  delete PRIVATE(this);
}

// Documented in superclass.
/*!
  \copybrief SoBase::initClass(void)
*/
void
SoLevelOfDetail::initClass(void)
{
  SO_NODE_INTERNAL_INIT_CLASS(SoLevelOfDetail, SO_FROM_INVENTOR_1);

  so_lod_storage = new SbStorage(sizeof(so_lod_static_data),
                                 so_lod_construct_data, so_lod_destruct_data);
  coin_atexit((coin_atexit_f*) so_lod_cleanup, CC_ATEXIT_NORMAL);
}

// Documented in superclass.
void
SoLevelOfDetail::doAction(SoAction *action)
{
  switch (action->getCurPathCode()) {
  case SoAction::IN_PATH:
    inherited::doAction(action); // normal path traversal
    return;
  case SoAction::OFF_PATH:
    return; // this is a separator node, return.
  case SoAction::BELOW_PATH:
  case SoAction::NO_PATH:
    break; // go on
  default:
    assert(0 && "unknown path code");
    return;
  }

  // for some strange reason, gcc (egcs-2.91.66) won't accept the code
  // below inside a case (yes, I did use brackets).
  // That's the reason for the strange switch/case above. pederb 19991116

  SoState * state = action->getState();
  int n = this->getNumChildren();
  if (n == 0) return;

  SbVec2s size;
  SbBox3f bbox;
  int i;
  int idx = -1;
  float projarea = 0.0f;

  SoComplexityTypeElement::Type complext = SoComplexityTypeElement::get(state);
  float complexity = SbClamp(SoComplexityElement::get(state), 0.0f, 1.0f);

  if (n == 1) { idx = 0; goto traverse; }
  if (complext == SoComplexityTypeElement::BOUNDING_BOX) { idx = n - 1; goto traverse; }
  if (complexity == 0.0f) { idx = n - 1; goto traverse; }
  if (complexity == 1.0f) { idx = 0; goto traverse; }
  if (this->screenArea.getNum() == 0) { idx = 0; goto traverse; }

  if (!PRIVATE(this)->bboxcache || !PRIVATE(this)->bboxcache->isValid(state)) {
    SoGetBoundingBoxAction * bboxAction = so_lod_get_bbox_action();

    bboxAction->setViewportRegion(SoViewportRegionElement::get(state));
    // need to apply on the current path, not on the node, since we
    // might need coordinates from the state. Also, we need to set the
    // reset path so that we get the local bounding box for the nodes
    // below this node.
    bboxAction->setResetPath(action->getCurPath());
    bboxAction->apply((SoPath*) action->getCurPath()); // find bbox of all children
    bbox = bboxAction->getBoundingBox();
  }
  else {
    bbox = PRIVATE(this)->bboxcache->getProjectedBox();
  }
  SoShape::getScreenSize(state, bbox, size);

  // The multiplication factor from the complexity setting is
  // complexity+0.5 because the documented behavior of SoLevelOfDetail
  // is to show lower detail levels than normal when
  // SoComplexity::value < 0.5, and to show higher detail levels when
  // SoComplexity::value > 0.5.
  projarea = float(size[0]) * float(size[1]) * (complexity + 0.5f);

  // In case there are too few or too many screenArea values.
  n = SbMin(n, this->screenArea.getNum());

  for (i = 0; i < n; i++) {
    if (projarea > this->screenArea[i]) { idx = i; goto traverse; }
  }

  // If we get here, projected area was lower than any of the
  // screenArea value, so the last child should be traversed.
  idx = this->getNumChildren() - 1;
  // (fall through to traverse:)

 traverse:
  this->getChildren()->traverse(action, idx);
  return;
}

// Documented in superclass.
void
SoLevelOfDetail::callback(SoCallbackAction *action)
{
  SoLevelOfDetail::doAction((SoAction*)action);
}

// Documented in superclass.
void
SoLevelOfDetail::GLRender(SoGLRenderAction *action)
{
  SoLevelOfDetail::doAction((SoAction*)action);
  // don't auto cache LevelOfDetail nodes.
  SoGLCacheContextElement::shouldAutoCache(action->getState(),
                                           SoGLCacheContextElement::DONT_AUTO_CACHE);
}

// Documented in superclass.
void
SoLevelOfDetail::rayPick(SoRayPickAction *action)
{
  SoLevelOfDetail::doAction((SoAction*)action);
}

// Documented in superclass.
void
SoLevelOfDetail::audioRender(SoAudioRenderAction * action)
{
  /* 
     FIXME: Implement proper support for audio rendering. The
     implementation will be similar to SoLOD, but will require
     enabling some more elements for SoAudioRenderAction, as well as
     rewriting this->doAction().

     The current implementation will render _all_ children instead of
     just one of them.

     2003-02-05 thammer.
   */
  // let SoGroup traverse the children
  inherited::audioRender(action);
}

void 
SoLevelOfDetail::getBoundingBox(SoGetBoundingBoxAction * action)
{
  SoState * state = action->getState();

  SbXfBox3f childrenbbox;
  SbBool childrencenterset;
  SbVec3f childrencenter;

  SbBool iscaching = TRUE;

  switch (action->getCurPathCode()) {
  case SoAction::OFF_PATH:
  case SoAction::IN_PATH:
    // can't cache if we're not traversing all children
    iscaching = FALSE;
    break;
    return; // no need to do any more work
  case SoAction::BELOW_PATH:
  case SoAction::NO_PATH:
    // check if this is a normal traversal
    if (action->isInCameraSpace()) iscaching = FALSE;
    break;
  default:
    iscaching = FALSE;
    assert(0 && "unknown path code");
    break;
  }

  SbBool validcache = PRIVATE(this)->bboxcache && PRIVATE(this)->bboxcache->isValid(state);

  if (iscaching && validcache) {
    SoCacheElement::addCacheDependency(state, PRIVATE(this)->bboxcache);
    childrenbbox = PRIVATE(this)->bboxcache->getBox();
    childrencenterset = PRIVATE(this)->bboxcache->isCenterSet();
    childrencenter = PRIVATE(this)->bboxcache->getCenter();
    if (PRIVATE(this)->bboxcache->hasLinesOrPoints()) {
      SoBoundingBoxCache::setHasLinesOrPoints(state);
    }
  }
  else {
    // used to restore the bounding box after we have traversed children
    SbXfBox3f abox = action->getXfBoundingBox();

    SbBool storedinvalid = FALSE;
    
    // always push since we update SoLocalBBoxMatrixElement
    state->push();

    if (iscaching) {
      storedinvalid = SoCacheElement::setInvalid(FALSE);

      // if we get here, we know bbox cache is not created or is invalid
      PRIVATE(this)->lock();
      if (PRIVATE(this)->bboxcache) PRIVATE(this)->bboxcache->unref();
      PRIVATE(this)->bboxcache = new SoBoundingBoxCache(state);
      PRIVATE(this)->bboxcache->ref();
      PRIVATE(this)->unlock();
      // set active cache to record cache dependencies
      SoCacheElement::set(state, PRIVATE(this)->bboxcache);
    }

    // the bounding box cache should be in the local coordinate system
    SoLocalBBoxMatrixElement::makeIdentity(state);
    action->getXfBoundingBox().makeEmpty();
    action->getXfBoundingBox().setTransform(SbMatrix::identity());

    inherited::getBoundingBox(action);

    childrenbbox = action->getXfBoundingBox();
    childrencenterset = action->isCenterSet();
    if (childrencenterset) childrencenter = action->getCenter();

    action->getXfBoundingBox() = abox; // reset action bbox

    if (iscaching) {
      PRIVATE(this)->bboxcache->set(childrenbbox, childrencenterset, childrencenter);
    }
    state->pop(); // we pushed before calculating the cache

    if (iscaching) {
      SoCacheElement::setInvalid(storedinvalid);
    }
  }
  
  if (!childrenbbox.isEmpty()) {
    action->extendBy(childrenbbox);
    if (childrencenterset) {
      // FIXME: shouldn't this assert() hold up? Investigate. 19990422 mortene.
#if 0 // disabled
      assert(!action->isCenterSet());
#else
      action->resetCenter();
#endif
      action->setCenter(childrencenter, TRUE);
    }
  }
}

// Doc from superclass.
void
SoLevelOfDetail::notify(SoNotList * nl)
{
  if (nl->getLastField() != &this->screenArea) {
    PRIVATE(this)->lock();
    if (PRIVATE(this)->bboxcache) PRIVATE(this)->bboxcache->invalidate();
    PRIVATE(this)->unlock();
  }
  inherited::notify(nl);
}

#undef PRIVATE

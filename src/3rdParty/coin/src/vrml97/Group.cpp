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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H

#ifdef HAVE_VRML97

/*!
  \class SoVRMLGroup SoVRMLGroup.h Inventor/VRMLnodes/SoVRMLGroup.h
  \brief The SoVRMLGroup class is used for grouping VRML nodes.

  \ingroup coin_VRMLnodes

  \WEB3DCOPYRIGHT

  \verbatim
  Group {
    eventIn      MFNode  addChildren
    eventIn      MFNode  removeChildren
    exposedField MFNode  children      []
    field        SFVec3f bboxCenter    0 0 0     # (-inf,inf)
    field        SFVec3f bboxSize      -1 -1 -1  # (0,inf) or -1,-1,-1
  }
  \endverbatim

  A Group node contains children nodes without introducing a new
  transformation.  It is equivalent to a Transform node containing an
  identity transform.  More details on the children, addChildren, and
  removeChildren fields and eventIns can be found in 4.6.5, Grouping
  and children nodes
  (<http://www.web3d.org/documents/specifications/14772/V2.0/part1/concepts.html#4.6.5>).

  The bboxCenter and bboxSize fields specify a bounding box that
  encloses the Group node's children. This is a hint that may be used
  for optimization purposes. The results are undefined if the
  specified bounding box is smaller than the actual bounding box of
  the children at any time.  A default bboxSize value, (-1, -1, -1),
  implies that the bounding box is not specified and, if needed, is
  calculated by the browser. A description of the bboxCenter and
  bboxSize fields is contained in 4.6.4, Bounding boxes
  (<http://www.web3d.org/documents/specifications/14772/V2.0/part1/concepts.html#4.6.4>).


*/

/*!
  SoSFEnum SoVRMLGroup::renderCaching
  Render caching policy. Default value is AUTO.
*/

/*!
  SoSFEnum SoVRMLGroup::boundingBoxCaching
  Bounding box cache policy. Default value is AUTO.
*/

/*!
  SoSFEnum SoVRMLGroup::renderCulling
  Render culling policy. Default value is AUTO.
*/

/*!
  SoSFEnum SoVRMLGroup::pickCulling
  Pick culling policy. Default value is AUTO.
*/

/*!
  SoSFVec3f SoVRMLGroup::bboxCenter
  Can be used to optimize bounding box calculations.
*/

/*!
  SoSFVec3f SoVRMLGroup::bboxSize
  Can be used to optimize bounding box calculations.
*/

#include <Inventor/VRMLnodes/SoVRMLGroup.h>

#include <Inventor/VRMLnodes/SoVRMLMacros.h>
#include <Inventor/actions/SoActions.h>
#include <Inventor/caches/SoBoundingBoxCache.h>
#include <Inventor/caches/SoGLCacheList.h>
#include <Inventor/elements/SoCacheElement.h>
#include <Inventor/elements/SoCullElement.h>
#include <Inventor/elements/SoLocalBBoxMatrixElement.h>
#include <Inventor/elements/SoSoundElement.h>
#include <Inventor/errors/SoDebugError.h>
#include <Inventor/misc/SoChildList.h>
#include <Inventor/misc/SoState.h>
#include <Inventor/threads/SbStorage.h>
#include <Inventor/system/gl.h>
#include <Inventor/C/tidbits.h> // coin_getenv()

#ifdef HAVE_THREADS
#include <Inventor/threads/SbMutex.h>
#endif // HAVE_THREADS

#include "rendering/SoGL.h"
#include "nodes/SoSubNodeP.h"
#include "glue/glp.h"
#include "profiler/SoNodeProfiling.h"

#include <cstdlib> // strtol(), rand()
#include <climits> // LONG_MIN, LONG_MAX

// *************************************************************************

// when doing threadsafe rendering, each thread needs its own
// glcachelist
typedef struct {
  SoGLCacheList * glcachelist;
} sovrmlgroup_storage;

static void
sovrmlgroup_storage_construct(void * data)
{
  sovrmlgroup_storage * ptr = (sovrmlgroup_storage*) data;
  ptr->glcachelist = NULL;
}

static void 
sovrmlgroup_storage_destruct(void * data)
{
  sovrmlgroup_storage * ptr = (sovrmlgroup_storage*) data;
  delete ptr->glcachelist;
}

// *************************************************************************

int SoVRMLGroup::numRenderCaches = 2;

class SoVRMLGroupP {
public:
  // lots of ifdefs here but it can't be helped...
  SoVRMLGroupP(void) {
    this->glcachestorage = 
      new SbStorage(sizeof(sovrmlgroup_storage),
                    sovrmlgroup_storage_construct,
                    sovrmlgroup_storage_destruct);
  }

  ~SoVRMLGroupP() {
    delete this->glcachestorage;
  }

  SoBoundingBoxCache * bboxcache;
  uint32_t bboxcache_usecount;
  uint32_t bboxcache_destroycount;

  SbStorage * glcachestorage;
  static void invalidate_gl_cache(void * tls, void *) {
    sovrmlgroup_storage * ptr = (sovrmlgroup_storage*) tls;
    if (ptr->glcachelist) {
      ptr->glcachelist->invalidateAll();
    }
  }

public:
  enum { YES, NO, MAYBE } hassoundchild;

  SoGLCacheList * getGLCacheList(const SbBool createifnull);

  void invalidateGLCaches(void) {
    glcachestorage->applyToAll(invalidate_gl_cache, NULL);
  }

#ifdef COIN_THREADSAFE
  SbMutex mutex;
  void lock(void) { this->mutex.lock(); }
  void unlock(void) { this->mutex.unlock(); }
#else // !COIN_THREADSAFE
  void lock(void) {  }
  void unlock(void) { }
#endif // !COIN_THREADSAFE
};

SoGLCacheList *
SoVRMLGroupP::getGLCacheList(const SbBool createifnull)
{
  sovrmlgroup_storage * ptr = 
    (sovrmlgroup_storage*) this->glcachestorage->get();
  if (createifnull && ptr->glcachelist == NULL) {
    ptr->glcachelist = new SoGLCacheList(SoVRMLGroup::getNumRenderCaches());
  }
  return ptr->glcachelist;
}

#define PRIVATE(obj) ((obj)->pimpl)

// *************************************************************************

SO_NODE_SOURCE(SoVRMLGroup);

// *************************************************************************

/*!
  \copydetails SoNode::initClass(void)
*/
void
SoVRMLGroup::initClass(void)
{
  SO_NODE_INTERNAL_INIT_CLASS(SoVRMLGroup, SO_VRML97_NODE_TYPE);

  SoType type = SoVRMLGroup::getClassTypeId();
  SoRayPickAction::addMethod(type, SoNode::rayPickS);
  SoVRMLGroup::numRenderCaches = 2;
}

/*!
  Constructor.
*/
SoVRMLGroup::SoVRMLGroup(void)
{
  this->commonConstructor();
}

/*!
  Constructor. \a numchildren is the expected number of children.
*/
SoVRMLGroup::SoVRMLGroup(int numchildren)
  : SoVRMLParent(numchildren)
{
  this->commonConstructor();
}

void
SoVRMLGroup::commonConstructor(void)
{
  PRIVATE(this) = new SoVRMLGroupP;
  PRIVATE(this)->bboxcache = NULL;
  PRIVATE(this)->bboxcache_usecount = 0;
  PRIVATE(this)->bboxcache_destroycount = 0;

  SO_VRMLNODE_INTERNAL_CONSTRUCTOR(SoVRMLGroup);

  SO_VRMLNODE_ADD_FIELD(bboxCenter, (0.0f, 0.0f, 0.0f));
  SO_VRMLNODE_ADD_FIELD(bboxSize, (-1.0f, -1.0f, -1.0f));

  SO_VRMLNODE_ADD_FIELD(renderCaching, (AUTO));
  SO_VRMLNODE_ADD_FIELD(boundingBoxCaching, (AUTO));
  SO_VRMLNODE_ADD_FIELD(renderCulling, (AUTO));
  SO_VRMLNODE_ADD_FIELD(pickCulling, (AUTO));

  SO_NODE_DEFINE_ENUM_VALUE(CacheEnabled, ON);
  SO_NODE_DEFINE_ENUM_VALUE(CacheEnabled, OFF);
  SO_NODE_DEFINE_ENUM_VALUE(CacheEnabled, AUTO);

  SO_NODE_SET_SF_ENUM_TYPE(renderCaching, CacheEnabled);
  SO_NODE_SET_SF_ENUM_TYPE(boundingBoxCaching, CacheEnabled);
  SO_NODE_SET_SF_ENUM_TYPE(renderCulling, CacheEnabled);
  SO_NODE_SET_SF_ENUM_TYPE(pickCulling, CacheEnabled);

  PRIVATE(this)->hassoundchild = SoVRMLGroupP::MAYBE;

  static long int maxcaches = -1;
  if (maxcaches == -1) {
    maxcaches = -2; // so we don't request the envvar later if it is not set
    const char * maxcachesstr = coin_getenv("IV_SEPARATOR_MAX_CACHES");
    if (maxcachesstr) {
      maxcaches = strtol(maxcachesstr, NULL, 10);
      if ((maxcaches == LONG_MIN) || (maxcaches == LONG_MAX) || (maxcaches < 0)) {
        SoDebugError::post("SoVRMLGroup::commonConstructor",
                           "Environment variable IV_SEPARATOR_MAX_CACHES "
                           "has invalid setting \"%s\"", maxcachesstr);
      }
      else {
        SoVRMLGroup::setNumRenderCaches(maxcaches);
      }
    }
  }
}

/*!
  Destructor.
*/
SoVRMLGroup::~SoVRMLGroup()
{
  if (PRIVATE(this)->bboxcache) PRIVATE(this)->bboxcache->unref();
  delete PRIVATE(this);
}

/*!
  Set the maximum number of render caches per group node.
*/
void
SoVRMLGroup::setNumRenderCaches(int num )
{
  SoVRMLGroup::numRenderCaches = num;
}

/*!
  Returns the maximum number of render caches per group node.
*/
int
SoVRMLGroup::getNumRenderCaches(void)
{
  return SoVRMLGroup::numRenderCaches;
}

// Doc in parent
void
SoVRMLGroup::doAction(SoAction * action)
{
  SoState * state = action->getState();
  state->push();
  inherited::doAction(action);
  state->pop();
}

// Doc in parent
void
SoVRMLGroup::callback(SoCallbackAction * action)
{
  SoState * state = action->getState();
  state->push();
  // culling planes should normally not be set, but can be set
  // manually by the application programmer to optimize callback
  // action traversal.
  if (!this->cullTest(state)) { inherited::callback(action); }
  state->pop();
}

// Doc in parent
void
SoVRMLGroup::GLRender(SoGLRenderAction * action )
{
  switch (action->getCurPathCode()) {
  case SoAction::NO_PATH:
  case SoAction::BELOW_PATH:
    this->GLRenderBelowPath(action);
    break;
  case SoAction::OFF_PATH:
    // do nothing. Separator will reset state.
    break;
  case SoAction::IN_PATH:
    this->GLRenderInPath(action);
    break;
  }
}

// Doc in parent
void
SoVRMLGroup::getBoundingBox(SoGetBoundingBoxAction * action)
{
  SoState * state = action->getState();

  SbXfBox3f childrenbbox;
  SbBool childrencenterset;
  SbVec3f childrencenter;

  // FIXME: AUTO is interpreted as ON for the boundingBoxCaching
  // field, but we should trigger some heuristics based on scene graph
  // "behavior" in the children subgraphs if the value is set to
  // AUTO. 19990513 mortene.
  SbBool iscaching = this->boundingBoxCaching.getValue() != OFF;

  switch (action->getCurPathCode()) {
  case SoAction::IN_PATH:
    // can't cache if we're not traversing all children
    iscaching = FALSE;
    break;
  case SoAction::OFF_PATH:
    return; // no need to do any more work
  case SoAction::BELOW_PATH:
  case SoAction::NO_PATH:
    // check if this is a normal traversal
    if (action->isInCameraSpace() || action->isResetPath()) iscaching = FALSE;
    break;
  default:
    iscaching = FALSE;
    assert(0 && "unknown path code");
    break;
  }

  SbBool validcache = iscaching && PRIVATE(this)->bboxcache && PRIVATE(this)->bboxcache->isValid(state);

  if (iscaching && validcache) {
    SoCacheElement::addCacheDependency(state, PRIVATE(this)->bboxcache);
    PRIVATE(this)->bboxcache_usecount++;
    childrenbbox = PRIVATE(this)->bboxcache->getBox();
    childrencenterset = PRIVATE(this)->bboxcache->isCenterSet();
    childrencenter = PRIVATE(this)->bboxcache->getCenter();
    if (PRIVATE(this)->bboxcache->hasLinesOrPoints()) {
      SoBoundingBoxCache::setHasLinesOrPoints(state);
    }
  }
  else {
    SbXfBox3f abox = action->getXfBoundingBox();
    SbBool storedinvalid = FALSE;

    // check if we should disable auto caching
    if (PRIVATE(this)->bboxcache_destroycount > 10 && this->boundingBoxCaching.getValue() == AUTO) {
      if (float(PRIVATE(this)->bboxcache_usecount) / float(PRIVATE(this)->bboxcache_destroycount) < 5.0f) { 
        iscaching = FALSE;
      }
    }
    if (iscaching) {
      storedinvalid = SoCacheElement::setInvalid(FALSE);
    }
    state->push();

    if (iscaching) {
      // if we get here, we know bbox cache is not created or is invalid
      PRIVATE(this)->lock();
      if (PRIVATE(this)->bboxcache) {
        PRIVATE(this)->bboxcache_destroycount++;
        PRIVATE(this)->bboxcache->unref();
      }
      PRIVATE(this)->bboxcache = new SoBoundingBoxCache(state);
      PRIVATE(this)->bboxcache->ref();
      PRIVATE(this)->unlock();
      // set active cache to record cache dependencies
      SoCacheElement::set(state, PRIVATE(this)->bboxcache);
    }

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
    state->pop();
    if (iscaching) SoCacheElement::setInvalid(storedinvalid);
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

// Doc in parent
void
SoVRMLGroup::getMatrix(SoGetMatrixAction * action)
{
  int numindices;
  const int * indices;
  if (action->getPathCode(numindices, indices) == SoAction::IN_PATH) {
    this->getChildren()->traverseInPath(action, numindices, indices);
  }
}

// compute object space ray and test for intersection
static SbBool
ray_intersect(SoRayPickAction * action, const SbBox3f & box)
{
  if (box.isEmpty()) return FALSE;
  action->setObjectSpace();
  return action->intersect(box, TRUE);
}

// Doc in parent
void
SoVRMLGroup::rayPick(SoRayPickAction * action)
{
  if (this->pickCulling.getValue() == OFF ||
      !PRIVATE(this)->bboxcache || !PRIVATE(this)->bboxcache->isValid(action->getState()) ||
      !action->hasWorldSpaceRay() ||
      ray_intersect(action, PRIVATE(this)->bboxcache->getProjectedBox())) {
    SoVRMLGroup::doAction(action);
  }
}

// Doc in parent
void
SoVRMLGroup::search(SoSearchAction * action)
{
  SoNode::search(action);
  if (action->isFound()) return;

  SoVRMLGroup::doAction(action);
}

// Doc in parent
void
SoVRMLGroup::write(SoWriteAction * action)
{
  this->boundingBoxCaching.setDefault(TRUE);
  this->renderCaching.setDefault(TRUE);
  inherited::write(action);
}

// Doc in parent
void
SoVRMLGroup::audioRender(SoAudioRenderAction * action)
{
  // Note: This function is similar to SoSeparator::audioRender()

  int numindices;
  const int * indices;
  SoState * state = action->getState();
  if (PRIVATE(this)->hassoundchild != SoVRMLGroupP::NO) {
    if (action->getPathCode(numindices, indices) != SoAction::IN_PATH) {
      action->getState()->push();
      SoSoundElement::setSceneGraphHasSoundNode(state, this, FALSE);
      inherited::doAction(action);
      PRIVATE(this)->hassoundchild = SoSoundElement::sceneGraphHasSoundNode(state) ? 
        SoVRMLGroupP::YES : SoVRMLGroupP::NO;
      action->getState()->pop();
    } else {
      SoVRMLGroup::doAction((SoAction*)action);
    }
  }
}

// Doc in parent
void
SoVRMLGroup::getPrimitiveCount(SoGetPrimitiveCountAction * action)
{
  SoState * state = action->getState();
  state->push();
  inherited::getPrimitiveCount(action);
  state->pop();
}

// Doc in parent
void
SoVRMLGroup::GLRenderBelowPath(SoGLRenderAction * action)
{
  SoState * state = action->getState();
  state->push();
  SbBool didcull = FALSE;
  SoGLCacheList * createcache = NULL;
  if ((this->renderCaching.getValue() != OFF) && 
      (SoVRMLGroup::getNumRenderCaches() > 0)) {
    if (!state->isCacheOpen()) {
      didcull = TRUE;
      if (this->cullTest(state)) {
        // we're outside the view frustum
        state->pop();
        return;
      }
    }
    
    PRIVATE(this)->lock();
    SoGLCacheList * glcachelist = PRIVATE(this)->getGLCacheList(TRUE);
    PRIVATE(this)->unlock();
    if (glcachelist->call(action)) {
#if GLCACHE_DEBUG && 1 // debug
      SoDebugError::postInfo("SoVRMLGroup::GLRenderBelowPath",
                             "Executing GL cache: %p", this);
#endif // debug
      state->pop();
      return;
    }
    if (!SoCacheElement::anyOpen(state)) {
#if GLCACHE_DEBUG // debug
      SoDebugError::postInfo("SoVRMLGroup::GLRenderBelowPath",
                             "Creating GL cache: %p", this);
#endif // debug
      createcache = glcachelist;
    }
  }

  if (createcache) createcache->open(action);
  
  SbBool outsidefrustum = (createcache || state->isCacheOpen() || didcull) ? 
    FALSE : this->cullTest(state);
  
  if (createcache || !outsidefrustum) {
    int n = this->getChildren()->getLength();
    SoNode ** childarray = (SoNode**) this->getChildren()->getArrayPtr();
    action->pushCurPath();
    for (int i = 0; i < n && !action->hasTerminated(); i++) {
      action->popPushCurPath(i, childarray[i]);
      if (action->abortNow()) {
        // only cache if we do a full traversal
        SoCacheElement::invalidate(state);
        break;
      }
      SoNodeProfiling profiling;
      profiling.preTraversal(action);
      childarray[i]->GLRenderBelowPath(action);
      profiling.postTraversal(action);

#if COIN_DEBUG
      // The GL error test is default disabled for this optimized
      // path.  If you get a GL error reporting an error in the
      // Separator node, enable this code by setting the environment
      // variable COIN_GLERROR_DEBUGGING to "1" to see exactly which
      // node caused the error.
      static SbBool chkglerr = sogl_glerror_debugging();
      if (chkglerr) {
        cc_string str;
        cc_string_construct(&str);
        const unsigned int errs = coin_catch_gl_errors(&str);
        if (errs > 0) {
          SoDebugError::post("SoVRMLGroup::GLRenderBelowPath",
                             "glGetError()s => '%s', nodetype: '%s'",
                             cc_string_get_text(&str),
                             (*this->getChildren())[i]->getTypeId().getName().getString());
        }
        cc_string_clean(&str);
      }
#endif // COIN_DEBUG
    }
    action->popCurPath();
  }
  state->pop();
  if (createcache) {
    createcache->close(action);
  }
}

// Doc in parent
void
SoVRMLGroup::GLRenderInPath(SoGLRenderAction * action)
{
  int numindices;
  const int * indices;
  
  SoAction::PathCode pathcode = action->getPathCode(numindices, indices);
  
  if (pathcode == SoAction::IN_PATH) {
    SoState * state = action->getState();
    SoNode ** childarray = (SoNode**) this->getChildren()->getArrayPtr();
    state->push();
    int childidx = 0;
    for (int i = 0; i < numindices; i++) {
      for (; childidx < indices[i] && !action->hasTerminated(); childidx++) {
        SoNode * offpath = childarray[childidx];
        if (offpath->affectsState()) {
          action->pushCurPath(childidx, offpath);
          if (!action->abortNow()) {
            SoNodeProfiling profiling;
            profiling.preTraversal(action);
            offpath->GLRenderOffPath(action);
            profiling.postTraversal(action);
          }
          else {
            SoCacheElement::invalidate(state);
          }
          action->popCurPath(pathcode);
        }
      }
      SoNode * inpath = childarray[childidx];
      action->pushCurPath(childidx, inpath);
      if (!action->abortNow()) {
        SoNodeProfiling profiling;
        profiling.preTraversal(action);
        inpath->GLRenderInPath(action);
        profiling.postTraversal(action);
      }
      else {
        SoCacheElement::invalidate(state);
      }
      action->popCurPath(pathcode);
      childidx++;
    }
    state->pop();
  }
  else {
    // we got to the end of the path
    assert(action->getCurPathCode() == SoAction::BELOW_PATH);
    this->GLRenderBelowPath(action);
  }
}

// Doc in parent
void
SoVRMLGroup::GLRenderOffPath(SoGLRenderAction * COIN_UNUSED_ARG(action))
{
  // do nothing
}

// Doc in parent
void
SoVRMLGroup::notify(SoNotList * list)
{
  inherited::notify(list);
  
  PRIVATE(this)->lock();
  if (PRIVATE(this)->bboxcache) PRIVATE(this)->bboxcache->invalidate();
  PRIVATE(this)->invalidateGLCaches();
  PRIVATE(this)->hassoundchild = SoVRMLGroupP::MAYBE;
  PRIVATE(this)->unlock();
}

/*!
  Returns TRUE if children can be culled.
*/
SbBool
SoVRMLGroup::cullTest(SoState * state)
{
  if (this->renderCulling.getValue() == SoVRMLGroup::OFF) return FALSE;
  if (SoCullElement::completelyInside(state)) return FALSE;
  
  SbBool outside = FALSE;
  if (PRIVATE(this)->bboxcache &&
      PRIVATE(this)->bboxcache->isValid(state)) {
    const SbBox3f & bbox = PRIVATE(this)->bboxcache->getProjectedBox();
    if (!bbox.isEmpty()) {
      outside = SoCullElement::cullBox(state, bbox);
    }
  }
  return outside;
}

//
// no-push culltest
//
SbBool
SoVRMLGroup::cullTestNoPush(SoState * state)
{
  if (this->renderCulling.getValue() == SoVRMLGroup::OFF) return FALSE;
  if (SoCullElement::completelyInside(state)) return FALSE;

  SbBool outside = FALSE;
  if (PRIVATE(this)->bboxcache &&
      PRIVATE(this)->bboxcache->isValid(state)) {
    const SbBox3f & bbox = PRIVATE(this)->bboxcache->getProjectedBox();
    if (!bbox.isEmpty()) {
      outside = SoCullElement::cullTest(state, bbox);
    }
  }
  return outside;
}

#undef PRIVATE

#endif // HAVE_VRML97

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
  \class SoGroup SoGroup.h Inventor/nodes/SoGroup.h
  \brief The SoGroup class is a node which managed other node instances.

  \ingroup coin_nodes

  The internal scene data structures in Coin are managed as directed
  graphs. The graphs are built by setting up a hierarchy through the
  use of group nodes (either of this type, or from subclasses like
  SoSeparator) which is then traversed when applying actions (like
  SoGLRenderAction) to it.

  SoGroup, SoSeparator, and other classes derived from SoGroup, are
  the "tools" the application programmer uses when making the layout
  of the scene graph.

  An often asked question about SoGroup nodes is: "Why is there no
  SoGroup::getParent() method?" The answer to this is that nodes in
  the scene graph can have multiple parents, so a simple getParent()
  method wouldn't work. If you have a node pointer (or other node
  identification) of a node that you want to remove from the scene
  graph, what you need to do is to use an SoSearchAction to find all
  paths down to the node, and then invoke SoGroup::removeChild() from
  all found parents of the node.

  The function would look something like this:

  \code
  void getParents(SoNode * node, SoNode * root, SoPathList & parents)
  {
    SoSearchAction sa;
    sa.setNode(node);
    sa.setInterest(SoSearchAction::ALL);
    sa.apply(root);
    parents = sa.getPaths();
  }
  \endcode

  Or if you \e know that your node of interest has only a single
  parent:

  \code
  SoGroup * getParent(SoNode * node, SoNode * root)
  {
    SoSearchAction sa;
    sa.setNode(node);
    sa.setInterest(SoSearchAction::FIRST);
    sa.apply(root);
    SoPath * p = sa.getPath();
    assert(p && "not found");
    if (p->getLength() < 2) { return NULL; } // no parent
    return (SoGroup *)p->getNodeFromTail(1);
  }
  \endcode


  An important note about a potential problem using SoGroup nodes
  which it is not common to stumble on, but which makes hard to find
  bugs when one does: you should not change the scene graph layout
  during any action traversal, as that is not allowed by the internal
  Coin code. I.e. do not use addChild(), removeChild(), insertChild()
  or replaceChild() from any callback that is triggered directly or
  indirectly from an action traversal. The most common way of getting
  hit by this error, would be something like the following
  (simplified) example:

  \code
  #include <Inventor/Qt/SoQt.h>
  #include <Inventor/Qt/viewers/SoQtExaminerViewer.h>

  #include <Inventor/nodes/SoEventCallback.h>
  #include <Inventor/nodes/SoSeparator.h>
  #include <Inventor/nodes/SoCone.h>
  #include <Inventor/manips/SoPointLightManip.h>
  #include <Inventor/events/SoMouseButtonEvent.h>

  SoPointLightManip * global_pointlightmanip;
  SoSeparator * global_root;

  // Remove point light when clicking right mouse button.
  static void
  mySelectionC(void * ud, SoEventCallback * n)
  {
    const SoMouseButtonEvent * mbe = (SoMouseButtonEvent*) n->getEvent();

    if ((mbe->getButton() == SoMouseButtonEvent::BUTTON2) &&
	(mbe->getState() == SoButtonEvent::DOWN)) {
      if (global_pointlightmanip) {
	global_root->removeChild(global_pointlightmanip);
	global_pointlightmanip = NULL;
      }
    }
  }

  int
  main(int argc, char ** argv)
  {
    QWidget * window = SoQt::init(argv[0]);

    global_root = new SoSeparator;
    global_root->ref();

    SoEventCallback * ecb = new SoEventCallback;
    ecb->addEventCallback(SoMouseButtonEvent::getClassTypeId(), mySelectionC, 0);
    global_root->addChild(ecb);

    global_root->addChild(new SoCone);

    global_pointlightmanip = new SoPointLightManip;
    global_root->addChild(global_pointlightmanip);

    SoQtExaminerViewer * viewer = new SoQtExaminerViewer(window);
    viewer->setSceneGraph(global_root);
    viewer->show();

    SoQt::show(window);
    SoQt::mainLoop();

    global_root->unref();
    delete viewer;

    return 0;
  }
  \endcode

  What happens in the above case is this: when clicking with the right
  mouse button, the SoQtExaminerViewer converts the Qt event to a Coin
  event, which is sent down the scene graph with an
  SoHandleEventAction. The action traversal reaches the "global_root"
  SoSeparator node, where it sees that it should further traverse 3
  child nodes (first the SoEventCallback, then the SoCone, then the
  SoPointLightManip). When it then traverses the SoEventCallback, the
  mySelectionC() callback will be invoked, which removes the last
  child. But the SoHandleEventAction will still continue its
  traversal as if the global_root node has 3 children -- and the code
  will crash.

  (This exact example would perhaps be straight-forward to handle
  internally in Coin, but there are other ways to change the scene
  graph layout that are very difficult to handle properly. So in
  general, changing layout during action traversal is not allowed.)

  What to do in these cases is to change the code inside the callback
  to not do any operations that immediately changes the layout of the
  scene graph, but to delay it for after the traversal is done. This
  can e.g. be done by using a Coin sensor.

  <b>FILE FORMAT/DEFAULTS:</b>
  \code
    Group {
    }
  \endcode
*/

// *************************************************************************

#include <Inventor/nodes/SoGroup.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include <cassert>

#include <Inventor/SoInput.h>
#include <Inventor/SoOutput.h>
#include <Inventor/misc/SoChildList.h>
#include <Inventor/actions/SoGetBoundingBoxAction.h>
#include <Inventor/actions/SoGetMatrixAction.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/actions/SoSearchAction.h>
#include <Inventor/actions/SoWriteAction.h>
#include <Inventor/actions/SoAudioRenderAction.h>
#include <Inventor/errors/SoReadError.h>
#include <Inventor/actions/SoCallbackAction.h>
#include <Inventor/elements/SoCacheElement.h>
#include <Inventor/errors/SoDebugError.h>
#include <Inventor/system/gl.h>

#include "nodes/SoSubNodeP.h"
#include "rendering/SoGL.h"
#include "glue/glp.h"
#include "io/SoWriterefCounter.h"

#include <Inventor/annex/Profiler/SoProfiler.h>
#include "profiler/SoNodeProfiling.h"

// *************************************************************************

/*!
  \var SoChildList * SoGroup::children
  List of managed child nodes.
*/

// *************************************************************************
// Note: just static data here, as there's no Cheshire Cat pattern (i.e.
// pimpl-ptr) implemented for SoNode. (The class should be as slim as
// possible.)

class SoGroupP {
public:
  typedef void GLRenderFunc(SoGroup *, SoNode *, SoGLRenderAction *);
  static GLRenderFunc * glrenderfunc;
  static void childGLRender(SoGroup * thisp, SoNode * child, SoGLRenderAction * action);
  static void childGLRenderProfiler(SoGroup * thisp, SoNode * child, SoGLRenderAction * action);
};

SoGroupP::GLRenderFunc * SoGroupP::glrenderfunc = NULL;

// *************************************************************************

SO_NODE_SOURCE(SoGroup);

// *************************************************************************

/*!
  Default constructor.
*/
SoGroup::SoGroup(void)
{
  this->pimpl = NULL; // just set to NULL for now
  SO_NODE_INTERNAL_CONSTRUCTOR(SoGroup);

  this->children = new SoChildList(this);
  this->setOperation();
}

/*!
  Constructor.

  The argument should be the approximate number of children which is
  expected to be inserted below this node. The number need not be
  exact, as it is only used as a hint for better memory resource
  allocation.
*/
SoGroup::SoGroup(int nchildren)
{
  SO_NODE_INTERNAL_CONSTRUCTOR(SoGroup);

  this->children = new SoChildList(this, nchildren);
  this->setOperation();
}

/*!
  Destructor.
*/
SoGroup::~SoGroup()
{
  delete this->children;
}

/*!
  Returns pointer to child node at \a index.

  Please note that this method is not virtual in the original SGI
  Inventor API.
*/
SoNode *
SoGroup::getChild(int index) const
{
  assert((index >= 0) && (index < this->getNumChildren()));

  return (SoNode*) this->getChildren()->getArrayPtr()[index];
}

/*!
  Returns number of child nodes managed by this group.

  Please note that this method is not virtual in the original SGI
  Inventor API.
*/
int
SoGroup::getNumChildren(void) const
{
  return this->getChildren()->getLength();
}

// Doc from superclass.
SbBool
SoGroup::readInstance(SoInput * in, unsigned short flags)
{
  SbBool readfields = TRUE;

  // Make sure we're compatible with binary format Inventor 1.0 and
  // 2.0 files.
  if (in->isBinary() && (in->getIVVersion() < 2.1f) &&
      this->getTypeId() == SoGroup::getClassTypeId()) {
    readfields = FALSE;
  }

  // Make sure we're compatible with binary format Inventor 1.0 files.
  if (in->isBinary() && (in->getIVVersion() < 2.0f) &&
      // For Inventor V1.0 files, no fields should be attempted read
      // from SoSeparator nodes when reading from binary format files,
      // or the input parsing will go wrong. I have just assumed this
      // goes for all SoGroup-derived nodes -- which may not be
      // correct. 20050706 mortene.
      this->isOfType(SoGroup::getClassTypeId())) {
    readfields = FALSE;
  }

  // This influences how SoFieldContainer::readInstance() handles
  // unknown field names inside the node: if it is a group, ignore, as
  // it can be the name of a node type.
  flags |= SoBase::IS_GROUP;

  // For nodes with fields inheriting SoGroup, the fields must come
  // before the children, according to the file format specification.
  if (readfields && !inherited::readInstance(in, flags)) return FALSE;

  return this->readChildren(in);
}

/*!
  Read all children of this node from \a in and attach them below this
  group in left-to-right order. Returns \c FALSE upon read error.
*/
SbBool
SoGroup::readChildren(SoInput * in)
{
  unsigned int numchildren = 0; // used by binary format import
  if (in->isBinary() && !in->read(numchildren)) {
    SoReadError::post(in, "Premature end of file");
    return FALSE;
  }

  for (unsigned int i=0; !in->isBinary() || (i < numchildren); i++) {
    SoBase * child;
    if (SoBase::read(in, child, SoNode::getClassTypeId())) {
      if (child == NULL) {
	if (in->eof()) {
	  SoReadError::post(in, "Premature end of file");
	  return FALSE;
	}
	else {
	  if (in->isBinary()) {
	    SoReadError::post(in, "Couldn't read valid identifier name");
	    return FALSE;
	  }

#if COIN_DEBUG && 0 // debug
	  char m;
	  if (in->read(m)) {
	    SoDebugError::postInfo("SoGroup::readChildren",
				   "next char: '%c'", m);
	  }
#endif // debug
	  // Completed reading of children for ASCII format import.
	  return TRUE;
	}
      }
      else {
	this->addChild((SoNode *)child);
      }
    }
    else {
      // SoReadError::post() is called within the SoBase::read()
      // frame upon error conditions, so don't duplicate with
      // another error message here.  mortene.
      return FALSE;
    }
  }

  // A successful import operation for binary format reading of child
  // nodes will exit here.
  return TRUE;
}

// Overridden from parent.
void
SoGroup::copyContents(const SoFieldContainer * from, SbBool copyconnections)
{
  this->removeAllChildren();

  inherited::copyContents(from, copyconnections);

  SoGroup * g = (SoGroup *)from;

  // Add children of "from" group node.
  for (int i=0 ; i < g->getNumChildren(); i++) {
    SoNode * cp = (SoNode *)
      SoFieldContainer::findCopy(g->getChild(i), copyconnections);
    this->addChild(cp);
  }
}

SoNotRec
SoGroup::createNotRec(void)
{
  SoNotRec rec(inherited::createNotRec());
  rec.setOperationType(operationType);
  rec.setGroupChild(changedChild);
  rec.setGroupPrevChild(changedPrevChild);
  rec.setIndex(changedIndex);
  return rec;
}

/*!
  \internal
*/
void
SoGroup::setOperation(const SoNotRec::OperationType opType,
		      const SoNode * nc,
		      const SoNode * pc,
		      const int ci)
{
  this->operationType = opType;
  this->changedChild = nc;
  this->changedPrevChild = pc;
  this->changedIndex = ci;
}

/*!
  Append a child \a node to the list of children nodes this group node
  is managing.

  Please note that this method is not virtual in the original SGI
  Inventor API.
*/
void
SoGroup::addChild(SoNode * node)
{
  assert(node != NULL);
  this->setOperation(SoNotRec::GROUP_ADDCHILD, node);
  this->getChildren()->append(node);
  this->setOperation();
}

/*!
  Insert a \a child node at position \a newchildindex.

  \a newchildindex must be <= this->getNumChildren()

  Please note that this method is not virtual in the original SGI
  Inventor API.
*/
void
SoGroup::insertChild(SoNode * child, int newchildindex)
{
#if COIN_DEBUG
  if (newchildindex < 0 || newchildindex > this->getNumChildren()) {
    SoDebugError::post("SoGroup::insertChild",
		       "idx %d is out of bounds (groupnode # children == %d)",
		       newchildindex, this->getNumChildren());
    return;
  }
#endif // COIN_DEBUG
  this->setOperation(SoNotRec::GROUP_INSERTCHILD, child, NULL, newchildindex);
  this->getChildren()->insert(child, newchildindex);
  this->setOperation();
}

/*!
  Remove node at \a childindex in our list of children.

  Please note that this method is not virtual in the original SGI
  Inventor API.
*/
void
SoGroup::removeChild(int childindex)
{
#if COIN_DEBUG
  if (childindex < 0 || childindex >= this->getNumChildren()) {
    SoDebugError::post("SoGroup::removeChild",
		       "idx %d is out of bounds (groupnode # children == %d)",
		       childindex, this->getNumChildren());
    return;
  }
#endif // COIN_DEBUG
  this->setOperation(SoNotRec::GROUP_REMOVECHILD,
		     this->getChild(childindex),
		     NULL, childindex);
  this->getChildren()->remove(childindex);
  this->setOperation();
}

/*!
  Returns index in our list of children for child \a node, or -1 if \a
  node is not a child of this group node.

  Please note that this method is not virtual in the original SGI
  Inventor API.
*/
int
SoGroup::findChild(const SoNode * node) const
{
  return this->getChildren()->find((SoNode *) node);
}

/*!
  \copybrief SoBase::initClass(void)
*/
void
SoGroup::initClass(void)
{
  SO_NODE_INTERNAL_INIT_CLASS(SoGroup, SO_FROM_INVENTOR_1);

  // for the built-in Coin profiler. set up the functionptr to use, so
  // we don't have any overhead when profiling is off:
  SoGroupP::glrenderfunc = SoGroupP::childGLRender;
  if (SoProfiler::isEnabled()) {
    SoGroupP::glrenderfunc = SoGroupP::childGLRenderProfiler;
  }
}

// *************************************************************************

// Doc from superclass.
void
SoGroup::doAction(SoAction * action)
{
  int numindices;
  const int * indices;
  if (action->getPathCode(numindices, indices) == SoAction::IN_PATH) {
    this->getChildren()->traverseInPath(action, numindices, indices);
  }
  else {
    this->getChildren()->traverse(action); // traverse all children
  }
}

// *************************************************************************

// Doc from superclass.
void
SoGroup::getBoundingBox(SoGetBoundingBoxAction * action)
{
  // Sanity check. This has caught bugs.
  assert(this->getNumChildren() == this->getChildren()->getLength());

  int numindices;
  const int * indices;
  int lastchildindex;

  if (action->getPathCode(numindices, indices) == SoAction::IN_PATH)
    lastchildindex = indices[numindices-1];
  else
    lastchildindex = this->getNumChildren() - 1;

  assert(lastchildindex < this->getNumChildren());

  // Initialize accumulation variables.
  SbVec3f acccenter(0.0f, 0.0f, 0.0f);
  int numcenters = 0;

  for (int i = 0; i <= lastchildindex; i++) {
    this->getChildren()->traverse(action, i);

    // If center point is set, accumulate.
    if (action->isCenterSet()) {
      acccenter += action->getCenter();
	numcenters++;
	action->resetCenter();
    }
  }

  if (numcenters != 0)
    action->setCenter(acccenter / float(numcenters), FALSE);
}

// *************************************************************************

void
SoGroupP::childGLRender(SoGroup * COIN_UNUSED_ARG(thisp), SoNode * child, SoGLRenderAction * action)
{
  child->GLRender(action);
}

// This function is called for each child to traverse, and
// action->getCurPath() is already updated at this point.
void
SoGroupP::childGLRenderProfiler(SoGroup * COIN_UNUSED_ARG(thisp), SoNode * child, SoGLRenderAction * action)
{
  SoNodeProfiling profiling;
  profiling.preTraversal(action);
  child->GLRender(action);
  profiling.postTraversal(action);
}

// Doc from superclass.
void
SoGroup::GLRender(SoGLRenderAction * action)
{
  int numindices;
  const int * indices;
  SoAction::PathCode pathcode = action->getPathCode(numindices, indices);

  SoNode ** childarray = (SoNode**) this->getChildren()->getArrayPtr();
  SoState * state = action->getState();

  if (pathcode == SoAction::IN_PATH) {
    int lastchild = indices[numindices - 1];
    for (int i = 0; i <= lastchild && !action->hasTerminated(); i++) {
      SoNode * child = childarray[i];

      action->pushCurPath(i, child);
      if (action->getCurPathCode() != SoAction::OFF_PATH ||
	  child->affectsState()) {
	if (!action->abortNow()) {
	  (*SoGroupP::glrenderfunc)(this, child, action);
	}
	else {
	  SoCacheElement::invalidate(state);
	}
      }
      action->popCurPath(pathcode);
    }
  }
  else {
    action->pushCurPath();
    int n = this->getChildren()->getLength();
    for (int i = 0; i < n && !action->hasTerminated(); i++) {
      action->popPushCurPath(i, childarray[i]);

      if (pathcode == SoAction::OFF_PATH && !childarray[i]->affectsState()) {
	continue;
      }

      if (action->abortNow()) {
	// only cache if we do a full traversal
	SoCacheElement::invalidate(state);
	break;
      }

      (*SoGroupP::glrenderfunc)(this, childarray[i], action);

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
	  SoDebugError::post("SoGroup::GLRender",
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
}

// *************************************************************************

// Doc from superclass.
void
SoGroup::callback(SoCallbackAction * action)
{
  SoGroup::doAction((SoAction *)action);
}

// Doc from superclass.
void
SoGroup::getMatrix(SoGetMatrixAction * action)
{
  switch (action->getCurPathCode()) {
  case SoAction::NO_PATH:
  case SoAction::BELOW_PATH:
    break;
  case SoAction::OFF_PATH:
  case SoAction::IN_PATH:
    SoGroup::doAction((SoAction *)action);
    break;
  }
}

// Doc from superclass.
void
SoGroup::pick(SoPickAction * action)
{
  SoGroup::doAction((SoAction *)action);
}

// Doc from superclass.
void
SoGroup::handleEvent(SoHandleEventAction * action)
{
  SoGroup::doAction((SoAction *)action);
}

// Doc from superclass
void
SoGroup::audioRender(SoAudioRenderAction * action)
{
  SoGroup::doAction(action);
}

// Doc from superclass.
void
SoGroup::addWriteReference(SoOutput * out, SbBool isfromfield)
{
  // SoGroup::write() used to count write references of children by calling
  // doAction() when ref was zero in the SoOutput::COUNT_REFS stage. However, 
  // also field connections may add write references without going through
  // SoGroup::write(), see SoField::countWriteRefs(). This resulted in wrong
  // write reference counts. Therefore addWriteReference() was overloaded to
  // correctly count the write references of children regardless from where
  // this is called.

  int ref = SoWriterefCounter::instance(out)->getWriteref(this);
  inherited::addWriteReference(out);

  // Traverse hierarchy only first time around
  if (ref == 0) {
    int n = this->getChildren()->getLength();
    for (int i = 0; i < n; i++) {
	  (*this->getChildren())[i]->addWriteReference(out);
    }
  }
}
	
// Doc from superclass.
void
SoGroup::write(SoWriteAction * action)
{
  SoOutput * out = action->getOutput();
  if (out->getStage() == SoOutput::COUNT_REFS) {
    this->addWriteReference(out);
  }
  else if (out->getStage() == SoOutput::WRITE) {
    if (this->writeHeader(out, TRUE, FALSE)) return;
    this->getFieldData()->write(out, this);
    if (out->isBinary()) out->write(this->getNumChildren());
    SoGroup::doAction((SoAction *)action);
    this->writeFooter(out);
  }
  else assert(0 && "unknown stage");
}

// Doc from superclass.
void
SoGroup::search(SoSearchAction * action)
{
  // Include this node in the search.
  inherited::search(action);
  if (action->isFound()) return;

  // If we're not the one being sought after, try child subgraphs.
  SoGroup::doAction((SoAction *)action);
}

/*!
  Returns list of children.
*/
SoChildList *
SoGroup::getChildren(void) const
{
  return ((SoGroup *)this)->children;
}

/*!
  Remove \a child from the set of children managed by this group node.
  Will decrease the reference count of \a child by 1.

  This is a convenience method. It will simply call findChild() with
  \a child as argument, and then call removeChild(int) if the child is
  found.

  Please note that this method is not virtual in the original SGI
  Inventor API.
*/
void
SoGroup::removeChild(SoNode * child)
{
  int idx = this->findChild(child);

  if (idx < 0) {
#if COIN_DEBUG
    SoDebugError::post("SoGroup::removeChild",
		       "tried to remove non-existent child %p (%s)",
		       child,
		       child ? child->getTypeId().getName().getString() : "");
#endif // COIN_DEBUG
    return;
  }

  this->removeChild(idx);
}

/*!
  Do not manage the children anymore. Will dereference all children by
  1 as they are removed.

  Please note that this method is not virtual in the original SGI
  Inventor API.
*/
void
SoGroup::removeAllChildren(void)
{
  this->setOperation(SoNotRec::GROUP_REMOVEALLCHILDREN);
  this->getChildren()->truncate(0);
  this->setOperation();
}

/*!
  Replace child at \a index with \a newChild.

  Dereferences the child previously at \a index, and increases the
  reference count of \a newChild by 1.

  \a index must be < this->getNumChildren()

  Please note that this method is not virtual in the original SGI
  Inventor API.
*/
void
SoGroup::replaceChild(int index, SoNode * newchild)
{
  // Note: its imperative that we use set() here, and not a
  // remove+insert pair of calls as that would puck up SoChildList
  // auditing from SoPath instances.
  this->setOperation(SoNotRec::GROUP_REPLACECHILD, newchild,
		     this->getChild(index), index);
  this->getChildren()->set(index, newchild);
  this->setOperation();
}

/*!
  Replace \a oldchild with \a newchild.

  Dereferences \a oldchild by 1, and increases the reference count of
  \a newchild by 1.

  This is a convenience method. It will simply call findChild() with
  \a oldchild as argument, and call replaceChild(int, SoNode*) if the
  child is found.

  Please note that this method is not virtual in the original SGI
  Inventor API.
*/
void
SoGroup::replaceChild(SoNode * oldchild, SoNode * newchild)
{
#if COIN_DEBUG && 0 // debug
  SoDebugError::postInfo("SoGroup::replaceChild",
			 "(%p) from %p (%s) to %p (%s)",
			 this,
			 oldchild,
			 oldchild->getTypeId().getName().getString(),
			 newchild,
			 newchild->getTypeId().getName().getString());
#endif // debug

  int idx = this->findChild(oldchild);

#if COIN_DEBUG
  if (idx < 0 || idx > this->getNumChildren()) {
    SoDebugError::post("SoGroup::replaceChild",
		       "(%p) Tried to remove non-existent child %p (%s)",
		       this,
		       oldchild,
		       oldchild->getTypeId().getName().getString());
    return;
  }
#endif // COIN_DEBUG

  this->replaceChild(idx, newchild);
}

// Doc from parent class.
void
SoGroup::getPrimitiveCount(SoGetPrimitiveCountAction * action)
{
  SoGroup::doAction((SoAction *)action);
}

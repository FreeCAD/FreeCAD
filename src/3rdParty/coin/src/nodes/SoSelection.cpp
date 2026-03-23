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
  \class SoSelection SoSelection.h Inventor/nodes/SoSelection.h
  \brief The SoSelection class manages a list of selected nodes.

  \ingroup coin_nodes

  Inserting an SoSelection node in your scene graph enables you to let
  the user "pick" with the left mouse button to select/deselect objects
  below the SoSelection node.

  Using an SoBoxHighlightRenderAction or an
  SoLineHighlightRenderAction to render scene graphs containing
  SoSelection nodes provides a convenient way of providing visual
  feedback about the selections to the application user.

  Beware that one common faulty assumption which is made about the
  node is that the scene will automatically be re-rendered whenever
  the user is picking objects. This is not the case, the application
  programmer must himself schedule a redraw. A straightforward way to
  accomplish this is to SoNode::touch() the SoSelection node in the
  selection / deselection callback.

  A "skeleton" for basic use of SoSelection nodes is given below:

  \code
  extern SoSeparator * make_scenegraph( void );
  static SoSelection * selection = NULL;

  // Callback function triggered for selection / deselection.
  void made_selection( void * userdata, SoPath * path )
  {
    (void)fprintf( stdout, "%sselected %s\n",
                   userdata == (void *)1L ? "" : "de",
                   path->getTail()->getTypeId().getName().getString() );

    selection->touch(); // to redraw
  }

  // *************************************************************************

  // Print a quick instructions notice on stdout.
  void show_instructions( void )
  {
    (void)fprintf( stdout, "\nThis example program demonstrates the use of the SoSelection node type.\n" );
    (void)fprintf( stdout, "\nQuick instructions:\n\n" );
    (void)fprintf( stdout, "  * pick with left mouse button\n" );
    (void)fprintf( stdout, "  * hold SHIFT to select multiple objects\n" );
    (void)fprintf( stdout, "  * hit ESC to toggle back and forth to view mode\n" );
    (void)fprintf( stdout, "\n" );
  }

  // *************************************************************************

  int main( int argc, char ** argv )
  {
    QWidget * window = SoQt::init( argv[0] );
    show_instructions();

    selection = new SoSelection;
    selection->policy = SoSelection::SHIFT;
    selection->ref();

    selection->addChild( make_scenegraph() );
    selection->addSelectionCallback( made_selection, (void *)1L );
    selection->addDeselectionCallback( made_selection, (void *)0L );

    SoQtExaminerViewer * examinerviewer = new SoQtExaminerViewer( window );
    examinerviewer->setSceneGraph( selection );
    examinerviewer->setGLRenderAction( new SoBoxHighlightRenderAction );
    examinerviewer->setViewing( FALSE );
    examinerviewer->show();

    SoQt::show( window );
    SoQt::mainLoop();

    delete examinerviewer;
    selection->unref();

    return 0;
  }
  \endcode

  This node is not initialized in SoDB::init(), since it is part of
  the interaction kit "add-on". Before using this node, you should
  therefore call SoInteraction::init(). If you're using one of the
  standard GUI-toolkits (SoXt / SoQt / SoWin) SoInteraction::init()
  will be called for you from the So[Xt|Qt|Win]\::init() method and you
  don't have to worry about it.

  With regard to using multiple SoSelection nodes at the same time in
  the same scene graph: this is possible, but it is not
  straightforward. The standard viewers provided by SoQt, SoWin, et
  al., will only snoop on one SoSelection node (part of the legacy
  API from SGI's InventorXt), so selection changes on the others
  don't trigger redraws. You don't necessarily see what's happening
  in other words.  You'll have to hook up manually and trigger redraws
  yourself.

  Also be aware that when having multiple SoSelection nodes in the
  scene graph active at the same time, the SoHandleEventAction
  traversals that you intend for selection-change on one SoSelection
  node will also affect all the other SoSelection nodes in the scene
  -- usually deselecting everything below them since you will be
  clicking outside the selectable objects.  You'll therefore also have
  to manually override that behaviour, if you want selection change on
  one SoSelection node to not affect the others.

  <b>FILE FORMAT/DEFAULTS:</b>
  \code
    Selection {
        renderCaching AUTO
        boundingBoxCaching AUTO
        renderCulling AUTO
        pickCulling AUTO
        policy SHIFT
    }
  \endcode
*/

// *************************************************************************

#include <Inventor/nodes/SoSelection.h>

#include <Inventor/actions/SoSearchAction.h>
#include <Inventor/actions/SoHandleEventAction.h>
#include <Inventor/lists/SoCallbackList.h>
#include <Inventor/SoPickedPoint.h>
#include <Inventor/events/SoMouseButtonEvent.h>

#include "tidbitsp.h"
#include "nodes/SoSubNodeP.h"

// *************************************************************************

// FIXME: this doesn't seem to end up in the Doxygen-generated
// documentation anywhere, with Doxygen version 1.2.18, at least.
// Find out why. There are also many other typedefs like this in Coin,
// which are not within the scope of a class declaration. 20040707 mortene.

/*!
  \typedef SoPath * SoSelectionPickCB(void * data, const SoPickedPoint * pick)

  Callback functions for the
  SoSelection::setPickFilterCallbacksetPassCallback() method need to
  be of this type.

  See documentation of that method for more information.
*/

// FIXME: document these:
//
// typedef void SoSelectionPathCB(void * data, SoPath * path);
// typedef void SoSelectionClassCB(void * data, SoSelection * sel);
//
// 20040707 mortene.

// *************************************************************************

/*!
  \enum SoSelection::Policy
  Enum for different pick policies.
*/
/*!
  \var SoSelection::Policy SoSelection::SINGLE

  Only one object can be selected at any time. When the user picks a
  new object, the previous selection will be unselected. If the user
  picks on nothing, the current selection will be unselected.

  Note that if a new selection matches one already present in the
  selection list, neither a deselect nor a select notification
  callback will be made about that selection path.
*/
/*!
  \var SoSelection::Policy SoSelection::TOGGLE
  Picking an object toggles its selection state.
*/
/*!
  \var SoSelection::Policy SoSelection::SHIFT
  Same as SINGLE, but when shift key is pressed the selection policy
  will be changed to TOGGLE.
*/
/*!
  \var SoSelection::Policy SoSelection::DISABLE
  Disables selection handling.
*/


/*!
  \var SoSFEnum SoSelection::policy
  Field for selection policy. Default value is SHIFT.
*/


/*!
  \var SoPathList SoSelection::selectionList
  \COININTERNAL
*/
/*!
  \var SoCallbackList * SoSelection::selCBList
  \COININTERNAL
*/
/*!
  \var SoCallbackList * SoSelection::deselCBList
  \COININTERNAL
*/
/*!
  \var SoCallbackList * SoSelection::startCBList
  \COININTERNAL
*/
/*!
  \var SoCallbackList * SoSelection::finishCBList
  \COININTERNAL
*/
/*!
  \var SoSelectionPickCB * SoSelection::pickCBFunc
  \COININTERNAL
*/
/*!
  \var void * SoSelection::pickCBData
  \COININTERNAL
*/
/*!
  \var SbBool SoSelection::callPickCBOnlyIfSelectable
  \COININTERNAL
*/
/*!
  \var SoCallbackList * SoSelection::changeCBList
  \COININTERNAL
*/
/*!
  \var SoPath * SoSelection::mouseDownPickPath
  \COININTERNAL
*/
/*!
  \var SbBool SoSelection::pickMatching
  \COININTERNAL
*/

// *************************************************************************

// Used to search for nodes. Just use one static action to avoid
// allocating a new action every time we need to search for a node.
static SoSearchAction * soselection_searchAction; 

static void
soselection_cleanup(void)
{
  delete soselection_searchAction;
  soselection_searchAction = NULL;
}

// *************************************************************************

SO_NODE_SOURCE(SoSelection);

// *************************************************************************

/*!
  Default constructor.
*/
SoSelection::SoSelection(void)
{
  this->init();
}

/*!
  Constructor.

  The argument should be the approximate number of children which is
  expected to be inserted below this node. The number need not be
  exact, as it is only used as a hint for better memory resource
  allocation.
*/
SoSelection::SoSelection(const int nChildren)
  : inherited(nChildren)
{
  this->init();
}

/*!
  Destructor.
*/
SoSelection::~SoSelection()
{
  delete this->selCBList;
  delete this->deselCBList;
  delete this->startCBList;
  delete this->finishCBList;
  delete this->changeCBList;
  if (this->mouseDownPickPath) this->mouseDownPickPath->unref();
}

/*!
  \copybrief SoBase::initClass(void)
*/
void
SoSelection::initClass(void)
{
  SO_NODE_INTERNAL_INIT_CLASS(SoSelection, SO_FROM_INVENTOR_1);
}



//
// common code for both constructors
//
void
SoSelection::init(void)
{
  SO_NODE_INTERNAL_CONSTRUCTOR(SoSelection);

  SO_NODE_ADD_FIELD(policy, (SoSelection::SHIFT));

  SO_NODE_DEFINE_ENUM_VALUE(Policy, SINGLE);
  SO_NODE_DEFINE_ENUM_VALUE(Policy, TOGGLE);
  SO_NODE_DEFINE_ENUM_VALUE(Policy, SHIFT);
  SO_NODE_DEFINE_ENUM_VALUE(Policy, DISABLE);
  SO_NODE_SET_SF_ENUM_TYPE(policy, Policy);

  this->selCBList = new SoCallbackList;
  this->deselCBList = new SoCallbackList;
  this->startCBList = new SoCallbackList;
  this->finishCBList = new SoCallbackList;
  this->changeCBList = new SoCallbackList;

  this->pickCBFunc = NULL;
  this->pickCBData = NULL;
  this->callPickCBOnlyIfSelectable = FALSE;

  this->mouseDownPickPath = NULL;
  this->pickMatching = TRUE;
}

/*!
  Adds \a path to the list of selected objects.
 */
void
SoSelection::select(const SoPath * path)
{
  SoPath * newpath = this->copyFromThis(path);
  // FIXME: memleak if path already stored in list. 20050107 mortene.
  if (newpath && this->findPath(newpath) < 0) {
    newpath->ref();
    this->addPath(newpath);
    newpath->unrefNoDelete();
  }
}

/*!
  Adds \a node to the list of selected objects. The scene graph
  below the selection node will be searched, and the path to
  \a node will be added if found.
 */
void
SoSelection::select(SoNode * node)
{
  SoPath * path = this->searchNode(node);
  if (path) {
    // don't ref() the path. searchNode() will ref it before returning
    if (this->findPath(path) < 0) this->addPath(path);
    path->unref();
  }
}

/*!
  Remove \a path from the list of selected objects.
*/
void
SoSelection::deselect(const SoPath * path)
{
  int idx = this->findPath(path);
  if (idx >= 0) this->removePath(idx);
}

/*!
  Remove objects \a which from the list of selected objects.
*/
void
SoSelection::deselect(const int which)
{
  this->removePath(which);
}

/*!
  Remove \a node from the list of selected objects. The scene graph
  below the selection node will be searched, and the path to
  \a node will be removed if found.
*/
void
SoSelection::deselect(SoNode * node)
{
  SoPath * path = this->searchNode(node);
  if (path) { 
    // don't ref() the path. searchNode() will ref it before returning
    this->deselect(path);
    path->unref();
  }
}

/*!
  If \a path is not already selected, add \a path to the list of selected
  objects. Otherwise remove \a path from the list of selected objects.
*/
void
SoSelection::toggle(const SoPath * path)
{
  int idx = this->findPath(path);
  if (idx >= 0) this->removePath(idx);
  else this->select(path); // call select instead of addPath to copy path before adding
}


/*!
  If \a node is not already selected, add \a path to the list of selected
  objects. Otherwise remove \a node from the list of selected objects.
*/
void
SoSelection::toggle(SoNode * node)
{
  SoPath * path = this->searchNode(node);
  if (path) {
    // don't ref() the path. searchNode() will ref it before returning
    this->toggle(path);
    path->unref();
  }
}

/*!
  Return \e TRUE if \a path is in the list of selected objects.
*/
SbBool
SoSelection::isSelected(const SoPath * path) const
{
  return this->findPath(path) >= 0;
}

/*!
  Return \e TRUE if the path to \a node is in the list of selected objects.
*/
SbBool
SoSelection::isSelected(SoNode * node) const
{
  SoPath * path = this->searchNode(node);
  SbBool ret = FALSE;
  if (path) {
    // don't ref() the path. searchNode() will ref it before returning
    ret = this->isSelected(path);
    path->unref();
  }
  return ret;
}

/*!
  Clears the selection list.
*/
void
SoSelection::deselectAll(void)
{
  while (this->getNumSelected())
    this->removePath(this->getNumSelected()-1);
}

/*!
  Returns the number of selected objects.
*/
int
SoSelection::getNumSelected(void) const
{
  return this->selectionList.getLength();
}

/*!
  Returns the list of selected objects.
*/
const SoPathList *
SoSelection::getList(void) const
{
  return &this->selectionList;
}

/*!
  Returns the \a index'th selected objects.
*/
SoPath *
SoSelection::getPath(const int index) const
{
  return this->selectionList[index];
}

/*!
  Operator for accessing selected objects.
*/
SoPath *
SoSelection::operator[](const int i) const
{
  return this->selectionList[i];
}

/*!
  Adds a callback which will be called every time an
  object is selected.

  \sa removeSelectionCallback()
*/
void
SoSelection::addSelectionCallback(SoSelectionPathCB * f, void * userData)
{
  this->selCBList->addCallback((SoCallbackListCB *)f, userData);
}

/*!
  Removes one of the selection callbacks.

  \sa addSelectionCallback()
*/
void
SoSelection::removeSelectionCallback(SoSelectionPathCB * f, void * userData)
{
  this->selCBList->removeCallback((SoCallbackListCB *)f, userData);
}

/*!
  Adds a callback which will be called every time an
  object is deselected.

  \sa removeDeselectionCallback()
*/
void
SoSelection::addDeselectionCallback(SoSelectionPathCB * f, void * userData)
{
  this->deselCBList->addCallback((SoCallbackListCB *)f, userData);
}

/*!
  Removes one of the deselection callbacks.

  \sa addDeselctionCallback()
*/
void
SoSelection::removeDeselectionCallback(SoSelectionPathCB * f, void * userData)
{
  this->deselCBList->removeCallback((SoCallbackListCB *)f, userData);
}

/*!
  Adds a callback which will be invoked when the user starts an interactive
  change to the list of selected objects.
  
  This callback is useful for storing the old selection list for undo/redo
  functionality.

  \sa addFinishCallback()
*/
void
SoSelection::addStartCallback(SoSelectionClassCB * f, void * userData)
{
  this->startCBList->addCallback((SoCallbackListCB *)f, userData);
}

/*!
  Removes \a f from the list of start callbacks.

  \sa addStartCallback()
*/
void
SoSelection::removeStartCallback(SoSelectionClassCB * f, void * userData)
{
  this->startCBList->removeCallback((SoCallbackListCB *)f, userData);
}

/*!
  Adds a callback which will be invoked when the user has finished
  an interactive change to the list of selected objects.


  \sa addStartCallback()
*/
void
SoSelection::addFinishCallback(SoSelectionClassCB * f, void * userData)
{
  this->finishCBList->addCallback((SoCallbackListCB *)f, userData);
}

/*!
  Removes \a f from the list of finish callbacks.
  
  \sa addFinishCallback()
*/
void
SoSelection::removeFinishCallback(SoSelectionClassCB * f, void * userData)
{
  this->finishCBList->removeCallback((SoCallbackListCB *)f, userData);
}

/*!
  Sets the pick filter callback. This callback will be called when a
  path is about to be added to or removed from the list of selected
  objects. The callback function should return a replacement path that
  should be used instead of the picked path. The returned path will
  be ref'ed, copied, and then unref'ed again by the SoSelection node.
  
  If no callback is set (the default), the picked path will be used
  for selecting/deselecting.

  Possible return values from the callback:

  <ul>
  <li> NULL: simulate that nothing was picked. This will clear the
    selection for the SINGLE policy. The handle event action will be
    halted.</li>
  <li> A path: the path will be selected/deselected. The handle event
    action will be halted. </li>
  <li> A path containing only the selection node: as NULL, but action
    will not be halted. </li>
  <li> An empty path or a path not containing the selection node: the
    pick will be ignored. </li>
  </ul>

  if \a callOnlyIfSelectable is \c TRUE, the callback will only be
  called if the selection node is in the picked path.  
*/
void
SoSelection::setPickFilterCallback(SoSelectionPickCB * f,
                                   void * userData,
                                   const SbBool callOnlyIfSelectable)
{
  this->pickCBFunc = f;
  this->pickCBData = userData;
  this->callPickCBOnlyIfSelectable = callOnlyIfSelectable;
}

/*!  
  When \a pickmatchflag is \c TRUE (the default), the mouse button
  release pick must match the mouse button press pick before object is
  selected/deselected.

  This flag should normally not be of interest to application
  programmers.
*/
void
SoSelection::setPickMatching(const SbBool pickmatchflag)
{
  this->pickMatching = pickmatchflag;
}

/*!
  Returns \e TRUE if pick matching is enabled.

  \sa setPickMatching()
*/
SbBool
SoSelection::isPickMatching(void) const
{
  return this->pickMatching;
}

/*!
  Returns \e TRUE if pick matching is enabled.

  \sa setPickMatching()
*/
SbBool
SoSelection::getPickMatching(void) const
{
  return this->pickMatching;
}

/*!
  \COININTERNAL.
  Used by render area to receive notification when the selection list changes.
*/
void
SoSelection::addChangeCallback(SoSelectionClassCB * f, void * userData)
{
  this->changeCBList->addCallback((SoCallbackListCB *)f, userData);
}

/*!
  \COININTERNAL
  Used by render area to receive notification when the selection list changes.
*/
void
SoSelection::removeChangeCallback(SoSelectionClassCB * f, void * userData)
{
  this->changeCBList->removeCallback((SoCallbackListCB *)f, userData);
}

/*!
  \COININTERNAL
*/
void
SoSelection::invokeSelectionPolicy(SoPath * path,
                                   SbBool shiftdown)
{
  if (this->policy.getValue() == SoSelection::DISABLE)
    return;

  SbBool toggle = this->policy.getValue() == SoSelection::TOGGLE ||
    (this->policy.getValue() == SoSelection::SHIFT && shiftdown);

  if (toggle)
    this->performToggleSelection(path);
  else
    this->performSingleSelection(path);
}

/*!
  \COININTERNAL
*/
void
SoSelection::performSingleSelection(SoPath * path)
{
  // Make a copy of the path from the selection node down, to use for
  // comparisons versus already selected paths.
  SoPath * cmppath = path ? this->copyFromThis(path) : NULL;
  if (cmppath) { cmppath->ref(); }

  const int nrsel = this->getNumSelected();
  SbBool alreadyselected = FALSE;

  // Remove all selected paths already present, *except* if one of
  // them matches the new selection path -- then we'll just keep it.
  for (int i = (nrsel - 1); i >= 0; i--) {
    SoPath * selp = this->getPath(i);

    // If selection happened on an already selected path, just keep it
    // in and don't trigger a "deselect + select" pair of
    // notifications.
    if (cmppath && (*selp == *cmppath)) { alreadyselected = TRUE; }
    else { this->removePath(i); }
  }

  // If path was not already selected, then add it to selection
  // list. (And since this is SINGLE mode selection, it will now be
  // the only selected path.)
  if (path && !alreadyselected) { this->select(path); }

  if (cmppath) { cmppath->unref(); }
}

/*!
  \COININTERNAL
*/
void
SoSelection::performToggleSelection(SoPath * path)
{
  if (path) {
    int idx = this->findPath(path);
    if (idx >= 0) {
      this->removePath(idx);
    }
    else if (path->findNode(this) >= 0) {
      this->select(path);
    }
  }
}

/*!
  \COININTERNAL
*/
SoPath *
SoSelection::copyFromThis(const SoPath * path) const
{
  SoPath * newpath = NULL;
  path->ref();
  int i = path->findNode(this);
  if (i >= 0) {
    newpath = path->copy(i);
  }
  path->unrefNoDelete();
  return newpath;
}

/*!
  \COININTERNAL
*/
void
SoSelection::addPath(SoPath * path)
{
  this->selectionList.append(path);
  this->selCBList->invokeCallbacks(path);
  this->changeCBList->invokeCallbacks(this);
}

/*!
  \COININTERNAL
*/
void
SoSelection::removePath(const int which)
{
  SoPath * path = this->selectionList[which];
  path->ref();
  this->selectionList.remove(which);
  this->deselCBList->invokeCallbacks(path);
  this->changeCBList->invokeCallbacks(this);
  path->unref();
}

/*!
  \COININTERNAL
*/
int
SoSelection::findPath(const SoPath * path) const
{
  int idx = -1;

  // make copy only if necessary
  if (path->getHead() != (SoNode *)this) {
    SoPath * newpath = this->copyFromThis(path);
    if (newpath) {
      newpath->ref();
      idx = this->selectionList.findPath(*newpath);
      newpath->unref();
    }
    else idx = -1;
  }
  else idx = this->selectionList.findPath(*path);
  return idx;
}

// Documented in superclass.
void
SoSelection::handleEvent(SoHandleEventAction * action)
{
  // Overridden to do selection picking.
  inherited::handleEvent(action);

  const SoEvent * event = action->getEvent();

  SbBool haltaction = FALSE;
  if (SO_MOUSE_PRESS_EVENT(event, BUTTON1)) {
    if (this->mouseDownPickPath) {
      this->mouseDownPickPath->unref();
      this->mouseDownPickPath = NULL;
    }
    const SoPickedPoint * pp = action->getPickedPoint();
    if (pp) {
      SoPath * selectionpath = pp->getPath();

      // call pick filter callback also for mouse down events
      if (this->pickCBFunc && (!this->callPickCBOnlyIfSelectable ||
                               selectionpath->findNode(this) >= 0)) {
        selectionpath = this->pickCBFunc(this->pickCBData, pp);
      }
      if (selectionpath) {
        this->mouseDownPickPath = selectionpath;
        this->mouseDownPickPath->ref();
        action->setHandled();
      }
    }
  }
  else if (SO_MOUSE_RELEASE_EVENT(event, BUTTON1)) {
    SbBool ignorepick = FALSE;
    // call pick filter callback (called from getSelectionPath()) even
    // if the event was handled by a child node.
    SoPath * selpath = this->getSelectionPath(action, ignorepick, haltaction);
    if (action->isHandled()) {
      // if the event was handled by a child node we should not invoke
      // the selection policy
      if (selpath) {
        selpath->ref();
        selpath->unref();
      }
    }
    else {
      if (haltaction) action->setHandled();
      
      if (!ignorepick) {
        if (selpath) selpath->ref();
        this->startCBList->invokeCallbacks(this);
        this->invokeSelectionPolicy(selpath, event->wasShiftDown());
        this->finishCBList->invokeCallbacks(this);
        if (selpath) selpath->unref();
      }
    }
    if (this->mouseDownPickPath) {
      this->mouseDownPickPath->unref();
      this->mouseDownPickPath = NULL;
    }
  }
}


// Uses a static search action to find path to node from this. If the
// node is found, the returned path will be ref'ed. It's the caller's
// responsibility to unref the returned path when != NULL.
SoPath *
SoSelection::searchNode(SoNode * node) const
{
  if (soselection_searchAction == NULL) {
    soselection_searchAction = new SoSearchAction;
    soselection_searchAction->setInterest(SoSearchAction::FIRST);
    coin_atexit((coin_atexit_f*) soselection_cleanup, CC_ATEXIT_NORMAL);
  }
  soselection_searchAction->setNode(node);
  soselection_searchAction->apply((SoNode *)this);
  SoPath * path = soselection_searchAction->getPath();
  if (path) path->ref();
  // reset action before returning 
  soselection_searchAction->reset();
  return path;
}

//
// Returns the pick selection path. Considers pick filter callback.
//
SoPath *
SoSelection::getSelectionPath(SoHandleEventAction * action, SbBool & ignorepick,
                              SbBool & haltaction)
{
  //
  // handled like described in the man-pages for SoSelection
  //

  haltaction = FALSE;
  ignorepick = FALSE;
  if (this->pickMatching && this->mouseDownPickPath == NULL) {
    return NULL;
  }
  const SoPickedPoint * pp = action->getPickedPoint();
  SoPath * selectionpath = NULL;
  if (pp) {
    selectionpath = pp->getPath();
    // if there's no pickCBFunc we can just test against
    // mouseDownPickPath and (possibly) return here.
    if (this->pickMatching && !this->pickCBFunc) {
      if (*(this->mouseDownPickPath) != *selectionpath) {
        ignorepick = TRUE;
        return NULL;
      }
    }
    // if we have a pickCBFunc we have to get the pick filter path
    // before comparing the mouse press and mouse release paths
    if (this->pickCBFunc && (!this->callPickCBOnlyIfSelectable ||
                             selectionpath->findNode(this) >= 0)) {
      selectionpath = this->pickCBFunc(this->pickCBData, pp);

      // From the SoSelection man-pages:
      // Possible return values from pickCBFunc:
      // 1) NULL - behave as if nothing was picked, halt action
      // 2) path through the selection node - select/deselect path
      // 3) path containing only the selection node - as 1, but do not halt action 
      // 4) path not through the selection node - ignore event
      if (selectionpath) {
        if (selectionpath->getLength() == 1 &&
            selectionpath->getNode(0) == this) {
          selectionpath->ref();
          selectionpath->unref();
          selectionpath = NULL;
        }
        else if (selectionpath->findNode(this) >= 0) {
          if (*(this->mouseDownPickPath) == *selectionpath) {
            // pick matched
            haltaction = TRUE;
          }
          else {
            // mouse release didn't match mouse down
            selectionpath->ref();
            selectionpath->unref();
            selectionpath = NULL;
            ignorepick = TRUE;
          }
        }
        else { // path with this not in the path (most probably an empty path)
          selectionpath->ref();
          selectionpath->unref();
          selectionpath = NULL;
          ignorepick = TRUE;
        }
      }
      else { // pickCBFunc returned NULL
        haltaction = TRUE;
      }
    }
    else { // no pickCBFunc or not a valid path
      haltaction = FALSE;
    }
  }
  else if (this->mouseDownPickPath) {
    ignorepick = TRUE;
  }
  return selectionpath;
}

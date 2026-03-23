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
#include "config.h"
#endif // HAVE_CONFIG_H

#ifdef HAVE_NODEKITS

/*!
  \class SoNodeKitListPart SoNodeKitListPart.h Inventor/nodekits/SoNodeKitListPart.h
  \brief The SoNodeKitListPart class is a container node.

  \ingroup coin_nodekits

  This node is basically just a container node with these differences versus
  the other group nodes:

  1. It is possible to make a list of which node class types are
  allowed as children.

  2. You can freely select which kind of group node behaviour you want
  this container to have. Default is to act like an SoGroup node, but
  by using SoNodeKitListPart::setContainerType(), you can change the
  behaviour to be like a separator, a switch or whatever else you have
  of node types inheriting SoGroup.

  It might be necessary to get the container node, e.g. to change
  SoSwitch::whichChild if the container node is a SoSwitch.
  The container node is located in the first position of its children,
  i.e. (*getChildren())[0].
    Note that getNumChildren() returns 0, while getChildren()->getLength()
  returns 1 after initialization. The former amount returns its
  internal group nodes, while the latter returns all the nodes containing
  the container node.

  \sa SoGroup, SoSeparator, SoSwitch
 */

#include <Inventor/nodekits/SoNodeKitListPart.h>

#include <cassert>

#include <Inventor/misc/SoChildList.h>
#include <Inventor/nodes/SoGroup.h>
#include <Inventor/actions/SoSearchAction.h>
#include <Inventor/actions/SoGetBoundingBoxAction.h>
#include <Inventor/actions/SoGetMatrixAction.h>
#include <Inventor/actions/SoAudioRenderAction.h>

#if COIN_DEBUG
#include <Inventor/errors/SoDebugError.h>
#endif // COIN_DEBUG

#include "tidbitsp.h"
#include "nodes/SoSubNodeP.h"

static SoTypeList * sonodekitlistpart_deflist = NULL;

static void sonodekitlistpart_atexit_cleanup(void)
{
  delete sonodekitlistpart_deflist;
  sonodekitlistpart_deflist = NULL;
}

/*!
  \var SoChildList * SoNodeKitListPart::children
  \COININTERNAL
*/

SO_NODE_SOURCE(SoNodeKitListPart);


/*!
  Constructor.
*/
SoNodeKitListPart::SoNodeKitListPart(void)
{
  SO_NODE_INTERNAL_CONSTRUCTOR(SoNodeKitListPart);

  SO_NODE_ADD_FIELD(containerTypeName,(SoGroup::getClassTypeId().getName().getString()));
  SO_NODE_ADD_FIELD(childTypeNames,(SoNode::getClassTypeId().getName().getString()));
  SO_NODE_ADD_FIELD(containerNode,(NULL));

  this->containerNode.setValue(new SoGroup);
  // to avoid notification problems (container node is in the scene
  // graph and notification will be propagated throughout the scene
  // graph). If notification is enabled for this field, the trigger
  // path will be incorrect since it will stop at this node.
  this->containerNode.enableNotify(FALSE);
  this->containerNode.setDefault(TRUE);

  this->typelistlocked = FALSE;
  this->children = new SoChildList(this);
  this->children->append(this->containerNode.getValue());
}

/*!
  Destructor.
*/
SoNodeKitListPart::~SoNodeKitListPart()
{
  delete this->children;
  this->containerNode.setValue(NULL);
}

/*!
  Does initialization common for all objects of the
  SoNodeKitListPart class. This includes setting up the
  type system, among other things.
*/
void
SoNodeKitListPart::initClass(void)
{
  SO_NODE_INTERNAL_INIT_CLASS(SoNodeKitListPart, SO_FROM_INVENTOR_1);
  SoType type = SoNodeKitListPart::getClassTypeId();
  SoAudioRenderAction::addMethod(type,
                                 SoAudioRenderAction::callDoAction);
}

/*!
  Return type id for the list container. Default is SoGroup.
*/
SoType
SoNodeKitListPart::getContainerType(void) const
{
  return this->containerNode.getValue()->getTypeId();
}

/*!
  Change the behaviour of this container node. \a newContainerType
  must be derived from SoGroup.
*/
void
SoNodeKitListPart::setContainerType(SoType newContainerType)
{
  if (this->typelistlocked) {
#if COIN_DEBUG
    SoDebugError::post("SoNodeKitListPart::setContainerType",
                       "type list has been locked");
#endif // COIN_DEBUG
    return;
  }

#if COIN_DEBUG
  if (!newContainerType.isDerivedFrom(SoGroup::getClassTypeId())) {
    SoDebugError::postInfo("SoNodeKitListPart::setContainerType",
                           "'%s' is not a group node class type",
                           newContainerType.getName().getString());
    return;
  }
#endif // debug

  SoGroup * newroot = (SoGroup *) newContainerType.createInstance();
  SoChildList * kids = this->containerNode.getValue()->getChildren();
  for (int i=0; i < kids->getLength(); i++) {
    newroot->addChild((*kids)[i]);
  }

  this->containerNode.setValue(newroot);
  this->children->remove(0);
  this->children->append(newroot);

  const SbName nameoftype = newContainerType.getName();
  if (nameoftype != this->containerTypeName.getValue()) {
    this->containerTypeName.setValue(nameoftype);
  }
}

/*!
  Return a list of all types which are allowed as children of this node.
  Default is to allow all node types.
*/
const SoTypeList &
SoNodeKitListPart::getChildTypes(void) const
{
  if (this->allowedtypes.getLength()) return this->allowedtypes;

  // Dynamically allocated to avoid problems on systems which doesn't
  // handle static constructors.
  if (!sonodekitlistpart_deflist) {
    sonodekitlistpart_deflist = new SoTypeList;
    sonodekitlistpart_deflist->append(SoNode::getClassTypeId());
    coin_atexit((coin_atexit_f*)sonodekitlistpart_atexit_cleanup, CC_ATEXIT_NORMAL);
  }
  return *sonodekitlistpart_deflist;
}

/*!
  Add one more node class type which should be allowed to be used in the
  node list.

  Note: the default single SoNode type in the list will be removed
  upon the first call to this method.
*/
void
SoNodeKitListPart::addChildType(SoType typeToAdd)
{
  if (this->typelistlocked) {
#if COIN_DEBUG
    SoDebugError::post("SoNodeKitListPart::addChildType",
                       "type list has been locked");
#endif // COIN_DEBUG
    return;
  }

  for (int i=0; i < this->allowedtypes.getLength(); i++) {
    if (this->allowedtypes[i] == typeToAdd) {
#if COIN_DEBUG
      SoDebugError::post("SoNodeKitListPart::addChildType",
                         "type '%s' already present in the list",
                         typeToAdd.getName().getString());
#endif // COIN_DEBUG
      return;
    }
  }

  this->childTypeNames.set1Value(this->allowedtypes.getLength(),
                                 typeToAdd.getName());
  this->allowedtypes.append(typeToAdd);
}

/*!
  Return \c TRUE if nodes of the \a typeToCheck class type can be added
  to the list.
*/
SbBool
SoNodeKitListPart::isTypePermitted(SoType typeToCheck) const
{
  int numtypes = this->allowedtypes.getLength();

  if ((numtypes == 0) && typeToCheck.isDerivedFrom(SoNode::getClassTypeId()))
    return TRUE;

  for (int i=0; i < numtypes; i++) {
    if (typeToCheck.isDerivedFrom(this->allowedtypes[i])) return TRUE;
  }
  return FALSE;
}

/*!
  Return \c TRUE if \a child has a class type which is permitted to be
  in the list.
*/
SbBool
SoNodeKitListPart::isChildPermitted(const SoNode * child) const
{
  return this->isTypePermitted(child->getTypeId());
}

/*!
  Calls set() on the container node with \a fieldDataString.
*/
void
SoNodeKitListPart::containerSet(const char * fieldDataString)
{
  this->containerNode.getValue()->set(fieldDataString);
}

/*!
  Lock type list so no more node class types can be added by using
  addChildType(), and locks container type so it no longer can be
  changed by setContainerType().
*/
void
SoNodeKitListPart::lockTypes(void)
{
  this->typelistlocked = TRUE;
}

/*!
  Returns \c TRUE if the list of allowable node types and the container
  type have both been locked.
*/
SbBool
SoNodeKitListPart::isTypeLocked(void) const
{
  return this->typelistlocked;
}

/*!
  Adds \a child to the internal list of children, if it is of a
  type permitted to be added.
*/
void
SoNodeKitListPart::addChild(SoNode * child)
{
  if (!this->isChildPermitted(child)) {
#if COIN_DEBUG
    SoDebugError::post("SoNodeKitListPart::addChild",
                       "can't add child of type '%s'",
                       child->getTypeId().getName().getString());
#endif // COIN_DEBUG
    return;
  }

  ((SoGroup *) this->containerNode.getValue())->addChild(child);
}

/*!
  Inserts \a child in the internal list of children at position
  \c childIndex, if it is of a type permitted to be added.
*/
void
SoNodeKitListPart::insertChild(SoNode * child, int childIndex)
{
  if (!this->isChildPermitted(child)) {
#if COIN_DEBUG
    SoDebugError::post("SoNodeKitListPart::insertChild",
                       "can't insert child of type '%s'",
                       child->getTypeId().getName().getString());
#endif // COIN_DEBUG
    return;
  }

  ((SoGroup *) this->containerNode.getValue())->insertChild(child, childIndex);
}

/*!
  Return child node at position \a index.
*/
SoNode *
SoNodeKitListPart::getChild(int index) const
{
  return ((SoGroup *) this->containerNode.getValue())->getChild(index);
}

/*!
  Return position index of \a child in list of children.
*/
int
SoNodeKitListPart::findChild(SoNode * child) const
{
  return ((SoGroup *) this->containerNode.getValue())->findChild(child);
}

/*!
  Return number of children.
*/
int
SoNodeKitListPart::getNumChildren(void) const
{
  return ((SoGroup *) this->containerNode.getValue())->getNumChildren();
}

/*!
  \copydoc SoGroup::removeChild(int index)
*/
void
SoNodeKitListPart::removeChild(int index)
{
  ((SoGroup *) this->containerNode.getValue())->removeChild(index);
}

/*!
  \copydoc SoGroup::removeChild(SoNode * child)
*/
void
SoNodeKitListPart::removeChild(SoNode * child)
{
  ((SoGroup *) this->containerNode.getValue())->removeChild(child);
}

/*!
  Replace child at \a index with \a newChild, if \a newChild is of a
  permitted type.
*/
void
SoNodeKitListPart::replaceChild(int index, SoNode * newChild)
{
  if (!this->isChildPermitted(newChild)) {
#if COIN_DEBUG
    SoDebugError::post("SoNodeKitListPart::replaceChild",
                       "can't replace with child of type '%s'",
                       newChild->getTypeId().getName().getString());
#endif // COIN_DEBUG
    return;
  }

  ((SoGroup *) this->containerNode.getValue())->replaceChild(index, newChild);
}

/*!
  Replace \a oldChild with \a newChild, if \a newChild is of a
  permitted type.
*/
void
SoNodeKitListPart::replaceChild(SoNode * oldChild, SoNode * newChild)
{
  if (!this->isChildPermitted(newChild)) {
#if COIN_DEBUG
    SoDebugError::post("SoNodeKitListPart::replaceChild",
                       "can't replace with child of type '%s'",
                       newChild->getTypeId().getName().getString());
#endif // COIN_DEBUG
    return;
  }

  ((SoGroup *) this->containerNode.getValue())->replaceChild(oldChild, newChild);
}

// Documented in superclass.
SbBool
SoNodeKitListPart::affectsState(void) const
{
  // Just "forwards" the call to the same method at the container
  // node.
  return this->containerNode.getValue()->affectsState();
}

/*!
  This just "forwards" the call to the same method at the container node.
*/
void
SoNodeKitListPart::doAction(SoAction * action)
{
  int numindices;
  const int * indices;
  if (action->getPathCode(numindices, indices) == SoAction::IN_PATH) {
    children->traverseInPath(action, numindices, indices);
  }
  else {
    this->children->traverse(action);
  }
}

/*!
  This just "forwards" the call to the same method at the container node.
*/
void
SoNodeKitListPart::callback(SoCallbackAction * action)
{
  SoNodeKitListPart::doAction((SoAction*)action);
}

/*!
  This just "forwards" the call to the same method at the container node.
*/
void
SoNodeKitListPart::GLRender(SoGLRenderAction * action)
{
  SoNodeKitListPart::doAction((SoAction*)action);
}

/*!
  This just "forwards" the call to the same method at the container node.
*/
void
SoNodeKitListPart::getBoundingBox(SoGetBoundingBoxAction * action)
{
  int numindices;
  const int * indices;
  int last = action->getPathCode(numindices, indices) == SoAction::IN_PATH ?
    indices[numindices-1] : this->children->getLength() - 1;

  SbVec3f acccenter(0.0f, 0.0f, 0.0f);
  int numacc = 0;

  for (int i = 0; i <= last; i++) {
    children->traverse(action, i, i);
    if (action->isCenterSet()) {
      acccenter += action->getCenter();
      numacc++;
      action->resetCenter();
    }
  }
  if (numacc) action->setCenter(acccenter / float(numacc), FALSE);
}

/*!
  This just "forwards" the call to the same method at the container node.
*/
void
SoNodeKitListPart::getMatrix(SoGetMatrixAction * action)
{
  int numindices;
  const int * indices;

  switch (action->getPathCode(numindices, indices)) {
  case SoAction::IN_PATH:
    this->children->traverseInPath(action, numindices, indices);
    break;
  case SoAction::OFF_PATH:
    this->children->traverse(action);
    break;
  case SoAction::NO_PATH:
  case SoAction::BELOW_PATH:
    break;
  default:
    assert(0 && "unknown path code");
  }
}

/*!
  This just "forwards" the call to the same method at the container node.
*/
void
SoNodeKitListPart::handleEvent(SoHandleEventAction * action)
{
  SoNodeKitListPart::doAction((SoAction*)action);
}

/*!
  This just "forwards" the call to the same method at the container node.
*/
void
SoNodeKitListPart::pick(SoPickAction * action)
{
  SoNodeKitListPart::doAction((SoAction*)action);
}

/*!
  This just "forwards" the call to the same method at the container node.
*/
void
SoNodeKitListPart::search(SoSearchAction * action)
{
  SoNode::search(action);
  if (!action->isFound())
    SoNodeKitListPart::doAction(action);
}

/*!
  This just "forwards" the call to the same method at the container node.
*/
void
SoNodeKitListPart::getPrimitiveCount(SoGetPrimitiveCountAction * action)
{
  SoNodeKitListPart::doAction((SoAction*)action);
}

/*!
  This just "forwards" the call to the same method at the container node.
*/
SoChildList *
SoNodeKitListPart::getChildren(void) const
{
  return this->children;
}

/*!
  Return the SoGroup container which is the parent of all the children
  which have been added.
*/
SoGroup *
SoNodeKitListPart::getContainerNode(void)
{
  return (SoGroup *) this->containerNode.getValue();
}

// Documented in superclass. Overrides this method to set up internal
// data according to what is contained within the imported field
// values.
SbBool
SoNodeKitListPart::readInstance(SoInput * in, unsigned short flags)
{
  if (inherited::readInstance(in, flags)) {
    this->syncInternalData();
    return TRUE;
  }
  return FALSE;
}

// Documented in superclass. Overrides this method to set up internal
// data according to what is contained within the copied field values.
void
SoNodeKitListPart::copyContents(const SoFieldContainer * fromFC,
                                SbBool copyConnections)
{
  inherited::copyContents(fromFC, copyConnections);
  this->syncInternalData();
}

/*!
  Synchronize internal data with the contents of the fields.
 */
void
SoNodeKitListPart::syncInternalData(void)
{
  // Set up type list.
  this->allowedtypes.truncate(0);
  for (int i=0; i < this->childTypeNames.getNum(); i++) {
    this->allowedtypes.append(SoType::fromName(this->childTypeNames[i]));
  }

  // Set up container node, if necessary.
  if (this->containerNode.getValue() == NULL) {
    SoType containerType = SoType::fromName(this->containerTypeName.getValue());
    this->containerNode.setValue((SoNode*)containerType.createInstance());
    this->containerNode.setDefault(TRUE);
  }

  if (this->children->getLength() == 0) {
    this->children->append(this->containerNode.getValue());
  }
  else if ((*this->children)[0] != this->containerNode.getValue()) {
    this->children->remove(0);
    this->children->append(this->containerNode.getValue());
  }
}

// TRUE if default child can be created
SbBool
SoNodeKitListPart::canCreateDefaultChild(void) const
{
  return this->getDefaultChildType() != SoType::badType();
}

// create, add and return a new default child. Should not be called if default
// child cannot be created.
SoNode *
SoNodeKitListPart::createAndAddDefaultChild(void)
{
  SoType type = this->getDefaultChildType();
  assert(type != SoType::badType());
  SoNode * newnode = (SoNode*) type.createInstance();
  this->addChild(newnode);
  return newnode;
}

// return the first non abstract child type, or SoType::badType if none exists
SoType
SoNodeKitListPart::getDefaultChildType(void) const
{
  const SoTypeList & typelist = this->getChildTypes();
  int n = typelist.getLength();
  for (int i = 0; i < n; i++) {
    if (typelist[i].canCreateInstance()) return typelist[i];
  }
  return SoType::badType();
}

#endif // HAVE_NODEKITS

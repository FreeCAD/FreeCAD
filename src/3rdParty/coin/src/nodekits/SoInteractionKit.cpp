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
  \class SoInteractionKit SoInteractionKit.h Inventor/nodekits/SoInteractionKit.h
  \brief The SoInteractionKit class is a base class for draggers.

  \ingroup coin_nodekits

  This nodekit class makes it possible to set surrogate paths for
  parts. Instead of creating new geometry for the dragger, it is
  possible to specify an existing path in your scene to be used for
  interaction. All picks on this path will be handled by the dragger.

  The SoInteractionKit is primarily an internal class used as a
  superclass for the dragger classes, and it is unlikely that it
  should be of interest to application programmers, unless you have
  very special needs in your application.

  \NODEKIT_PRE_DIAGRAM

  \verbatim
  CLASS SoInteractionKit
  -->"this"
        "callbackList"
  -->   "topSeparator"
  -->      "geomSeparator"
  \endverbatim

  \NODEKIT_POST_DIAGRAM


  \NODEKIT_PRE_TABLE

  \verbatim
  CLASS SoInteractionKit
  PVT   "this",  SoInteractionKit  --- 
        "callbackList",  SoNodeKitListPart [ SoCallback, SoEventCallback ] 
  PVT   "topSeparator",  SoSeparator  --- 
  PVT   "geomSeparator",  SoSeparator  --- 
  \endverbatim

  \NODEKIT_POST_TABLE
*/

#include <Inventor/nodekits/SoInteractionKit.h>

#include <cstdlib>

#include <Inventor/C/tidbits.h>
#include <Inventor/SoDB.h>
#include <Inventor/SoInput.h>
#include <Inventor/actions/SoSearchAction.h>
#include <Inventor/errors/SoDebugError.h>
#include <Inventor/errors/SoReadError.h>
#include <Inventor/lists/SbList.h>
#include <Inventor/lists/SoPathList.h>
#include <Inventor/misc/SoChildList.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoSwitch.h>
#include <Inventor/nodes/SoText2.h>
#include <Inventor/sensors/SoFieldSensor.h>

#include "tidbitsp.h"
#include "coindefs.h" // COIN_OBSOLETED()
#include "nodekits/SoSubKitP.h"

/*!
  \enum SoInteractionKit::CacheEnabled

  Enumeration of valid values for the cache control fields
  SoInteractionKit::renderCaching,
  SoInteractionKit::boundingBoxCaching,
  SoInteractionKit::renderCulling and SoInteractionKit::pickCulling.

  The same values with the same semantics are present in this enum as
  for SoSeparator::CacheEnabled, so see that documentation.
*/

/*!
  \var SoSFEnum SoInteractionKit::renderCaching

  Controls the value of the SoSeparator::renderCaching field in the
  SoInteractionKit catalog's topSeparator instance.

  See documentation of SoSeparator::renderCaching.
*/
/*!
  \var SoSFEnum SoInteractionKit::boundingBoxCaching

  Controls the value of the SoSeparator::boundingBoxCaching field in
  the SoInteractionKit catalog's topSeparator instance.

  See documentation of SoSeparator::boundingBoxCaching.
*/
/*!
  \var SoSFEnum SoInteractionKit::renderCulling

  Controls the value of the SoSeparator::renderCulling field in the
  SoInteractionKit catalog's topSeparator instance.

  See documentation of SoSeparator::renderCulling.
*/
/*!
  \var SoSFEnum SoInteractionKit::pickCulling

  Controls the value of the SoSeparator::pickCulling field in the
  SoInteractionKit catalog's topSeparator instance.

  See documentation of SoSeparator::pickCulling.
*/

/*!
  \var SoFieldSensor * SoInteractionKit::fieldSensor
  Obsoleted in Coin.
*/
/*!
  \var SoFieldSensor * SoInteractionKit::oldTopSep
  Obsoleted in Coin.
*/

#ifndef DOXYGEN_SKIP_THIS

class SoInteractionKitP {
public:
  SoInteractionKitP(SoInteractionKit * kit) : kit(kit) { }

  SoInteractionKit * kit;
  SoFieldSensor * fieldsensor;
  SoSeparator * connectedseparator;

  void connectFields(const SbBool onoff);
  void attachSensor(const SbBool onoff);

  static void sensorCB(void *, SoSensor *);

  SoPathList surrogatepathlist;
  SbList <SbName> surrogatenamelist;

  void addSurrogatePath(SoPath * path, const SbName & name);
  void removeSurrogatePath(const SbName & partname);
  void removeSurrogatePath(const int idx);
  int findSurrogateIndex(const SbName & partname) const;
  int findSurrogateInPath(const SoPath * path);
};

#endif // DOXYGEN_SKIP_THIS


static SbList <SoNode*> * defaultdraggerparts = NULL;


//
// atexit callback used to unref() each draggerdefaults file
//
static void
defaultdraggerparts_cleanup(void)
{
  int n = defaultdraggerparts->getLength();
  for (int i = 0; i < n; i++) {
    (*defaultdraggerparts)[i]->unref();
  }
}

//
// atexit callback used to delete static data for this class
//
static void
interactionkit_cleanup(void)
{
  delete defaultdraggerparts;
  defaultdraggerparts = NULL;
}

#define PRIVATE(obj) ((obj)->pimpl)

SO_KIT_SOURCE(SoInteractionKit);


/*!
  Constructor.
*/
SoInteractionKit::SoInteractionKit(void)
{
  PRIVATE(this) = new SoInteractionKitP(this);
  SO_KIT_INTERNAL_CONSTRUCTOR(SoInteractionKit);

  SO_KIT_ADD_FIELD(renderCaching, (SoInteractionKit::AUTO));
  SO_KIT_ADD_FIELD(boundingBoxCaching, (SoInteractionKit::AUTO));
  SO_KIT_ADD_FIELD(renderCulling, (SoInteractionKit::AUTO));
  SO_KIT_ADD_FIELD(pickCulling, (SoInteractionKit::AUTO));

  SO_KIT_DEFINE_ENUM_VALUE(CacheEnabled, ON);
  SO_KIT_DEFINE_ENUM_VALUE(CacheEnabled, OFF);
  SO_KIT_DEFINE_ENUM_VALUE(CacheEnabled, AUTO);

  SO_KIT_SET_SF_ENUM_TYPE(renderCaching, CacheEnabled);
  SO_KIT_SET_SF_ENUM_TYPE(boundingBoxCaching, CacheEnabled);
  SO_KIT_SET_SF_ENUM_TYPE(renderCulling, CacheEnabled);
  SO_KIT_SET_SF_ENUM_TYPE(pickCulling, CacheEnabled);


  // Note: we must use "" instead of , , to humour MS VisualC++ 6.

  SO_KIT_ADD_CATALOG_ENTRY(topSeparator, SoSeparator, TRUE, this, "", FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(geomSeparator, SoSeparator, TRUE, topSeparator, "", FALSE);

  SO_KIT_INIT_INSTANCE();

  PRIVATE(this)->connectedseparator = NULL;
  PRIVATE(this)->fieldsensor = new SoFieldSensor(SoInteractionKit::fieldSensorCB, PRIVATE(this));
  PRIVATE(this)->fieldsensor->setPriority(0);

  SoInteractionKit::setUpConnections(TRUE, TRUE);
}

/*!
  Destructor.
*/
SoInteractionKit::~SoInteractionKit()
{
  PRIVATE(this)->connectFields(FALSE);
  delete PRIVATE(this)->fieldsensor;
  delete PRIVATE(this);
}

/*!
  \copybrief SoNode::initClass(void)
*/
void
SoInteractionKit::initClass(void)
{
  defaultdraggerparts = new SbList <SoNode*>;
  coin_atexit((coin_atexit_f *)defaultdraggerparts_cleanup, CC_ATEXIT_DRAGGERDEFAULTS);
  coin_atexit((coin_atexit_f *)interactionkit_cleanup, CC_ATEXIT_NORMAL);

  SO_KIT_INTERNAL_INIT_CLASS(SoInteractionKit, SO_FROM_INVENTOR_1);
}

/*!
  Sets a part in the kit as a surrogate path. The \a partname part is
  set to \c NULL, and the surrogate path is remembered. Following
  picks on the surrogate path will be regarded as a pick on \a
  partname.
*/
SbBool
SoInteractionKit::setPartAsPath(const SbName & partname,
                                SoPath * path)
{
  return this->setAnySurrogatePath(partname, path, TRUE, TRUE);
}

/*!
  Sets the value of \a partname to \a node, and sets the part's field
  to default (i.e. node will not be written on scene graph export).

  If \a onlyifdefault is \c TRUE, \a partname is only set if it is
  already in the default state.

  The reason for this method is to make it possible for dragger
  subclasses to avoid having their default parts written out on
  export.
*/
SbBool
SoInteractionKit::setPartAsDefault(const SbName & partname,
                                   SoNode * node,
                                   SbBool onlyifdefault)
{
  return this->setAnyPartAsDefault(partname, node, FALSE, onlyifdefault);
}

/*!
  Find node in the global dictionary, and set as default.

  \sa setPartAsDefault()
*/
SbBool
SoInteractionKit::setPartAsDefault(const SbName & partname,
                                   const SbName & nodename,
                                   SbBool onlyifdefault)
{
  return this->setAnyPartAsDefault(partname, nodename, FALSE, onlyifdefault);
}


/*!
  Checks if \a path is contained within any of the surrogate paths
  in any interaction kits from this node down. Returns information
  about the owner and the surrogate path if found, and \a fillargs is
  \e TRUE. The returned path (\a pathToOwner) is not ref'ed, It's the
  callers responsibility to ref and unref this path.
*/
SbBool
SoInteractionKit::isPathSurrogateInMySubgraph(const SoPath * path,
                                              SoPath *& pathToOwner,
                                              SbName  & surrogatename,
                                              SoPath *& surrogatepath,
                                              SbBool fillargs)
{
  int idx = PRIVATE(this)->findSurrogateInPath(path);
  if (idx >= 0) {
    if (fillargs) {
      pathToOwner = new SoPath(this); // a very short path
      surrogatename = PRIVATE(this)->surrogatenamelist[idx];
      surrogatepath = PRIVATE(this)->surrogatepathlist[idx];
    }
    return TRUE;
  }
  else {
    SoSearchAction sa;
    sa.setType(SoInteractionKit::getClassTypeId());
    sa.setFind(SoSearchAction::ALL);
    sa.setSearchingAll(TRUE);
    sa.apply(this);
    SoPathList & pathlist = sa.getPaths();
    for (int i = 0; i < pathlist.getLength(); i++) {
      SoInteractionKit * kit = (SoInteractionKit *)pathlist[i]->getTail();
      assert(kit->isOfType(SoInteractionKit::getClassTypeId()));
      int idx = kit->pimpl->findSurrogateInPath(path);
      if (idx >= 0) {
        if (fillargs) {
          pathToOwner = pathlist[i]->copy();
          surrogatename = kit->pimpl->surrogatenamelist[idx];
          surrogatepath = kit->pimpl->surrogatepathlist[idx];
        }
        return TRUE;
      }
    }
  }
  return FALSE;
}

/*!
  \overload
*/
SbBool
SoInteractionKit::isPathSurrogateInMySubgraph(const SoPath * path)
{
  SoPath * dummypath, * dummypath2;
  SbName dummyname;

  return this->isPathSurrogateInMySubgraph(path, dummypath,
                                           dummyname, dummypath2, FALSE);
}

/*!
  Convenience method that sets the switch value for a switch node.
  Checks if node != 0, and only sets the switch value if value
  has changed.
*/
void
SoInteractionKit::setSwitchValue(SoNode * node, const int newVal)
{
  if (node == NULL) return;
  assert(node->isOfType(SoSwitch::getClassTypeId()));
  SoSwitch * mySwitch = (SoSwitch *)node;
  if (mySwitch->whichChild.getValue() != newVal) {
    mySwitch->whichChild = newVal;
  }
}

// Doc in superclass. Overridden to copy the surrogate lists.
void
SoInteractionKit::copyContents(const SoFieldContainer * fromFC,
                               SbBool copyConnections)
{
  int i;
  inherited::copyContents(fromFC, copyConnections);

  assert(fromFC->isOfType(SoInteractionKit::getClassTypeId()));
  SoInteractionKit * kit = (SoInteractionKit *) fromFC;

  PRIVATE(this)->surrogatenamelist.truncate(0);
  PRIVATE(this)->surrogatepathlist.truncate(0);

  const int n = kit->pimpl->surrogatenamelist.getLength();
  for (i = 0; i < n; i++) {
    PRIVATE(this)->surrogatenamelist.append(kit->pimpl->surrogatenamelist[i]);
    PRIVATE(this)->surrogatepathlist.append(kit->pimpl->surrogatepathlist[i]);
  }
}

// Doc in superclass. Overridden to check topSeperator and fields
// after reading.
SbBool
SoInteractionKit::readInstance(SoInput * in, unsigned short flags)
{
  SbBool ret = inherited::readInstance(in, flags); // will handle fields
  if (ret) {
    // remove surrogate paths where part != NULL and not an empty
    // group or separator
    int n = PRIVATE(this)->surrogatenamelist.getLength();
    for (int i = 0; i < n; i++) {
      SbName name = PRIVATE(this)->surrogatenamelist[i];
      SoNode * node = this->getAnyPart(name, FALSE, FALSE, FALSE);
      if (node && ((node->getTypeId() != SoGroup::getClassTypeId() &&
                    node->getTypeId() != SoSeparator::getClassTypeId()) ||
                   node->getChildren()->getLength())) {
        n--; // don't forget this!
        PRIVATE(this)->surrogatenamelist.remove(i);
        PRIVATE(this)->surrogatepathlist.remove(i);
      }
    }
  }
  return ret;
}

/*!
  Reads default parts for a dragger.

  This method is called from dragger constructors to set up a
  dragger's nodekit catalog of interaction and feedback geometry.

  \a fileName is the user-changeable resource file in the Inventor
  file format, while \a defaultBuffer and \a defBufSize can point to
  the statically compiled default parts.

  The environment variable \c SO_DRAGGER_DIR must be set to a valid
  directory prefix for \a fileName, or no resource file will be loaded
  (and \a defaultBuffer will be used instead).

  If both a \a fileName and a \a defaultBuffer are provided, the file
  will be attempted found and loaded first, if that fails, the
  geometry will be attempted read from the buffer.
*/
void
SoInteractionKit::readDefaultParts(const char * fileName,
                                   const char defaultBuffer[],
                                   int defBufSize)
{
  // FIXME: it'd be great if this code could be changed so that the
  // dragger parts file (if any) would just replace the parts from the
  // default buffer. Then it would be possible to let the diskfile
  // contain a subset of the full set of geometries for the dragger. I
  // seem to remember that SGI Inventor works this way. 20020322 mortene.

  SoInput input;
  SoNode * root = NULL;

  const char * draggerdir = coin_getenv("SO_DRAGGER_DIR");

  if (fileName && draggerdir) {
    SbString fullname = draggerdir;
    const char dirsep = '/';

    if (fullname.getLength() && fullname[fullname.getLength()-1] != dirsep) {
      fullname += dirsep;
    }
    fullname += fileName;
    if (input.openFile(fullname.getString(), TRUE)) {
      root = (SoNode *)SoDB::readAll(&input);
    }
    else if (COIN_DEBUG) {
      SoDebugError::post("SoInteractionKit::readDefaultParts",
                         "Could not find file '%s' for the dragger "
                         "default parts.",
                         fullname.getString());
    }
  }

  if (!root && defaultBuffer) {
    input.setBuffer(defaultBuffer, defBufSize);
    root = (SoNode *)SoDB::readAll(&input);
  }

  if (root) {
    root->ref(); // this node is unref'ed at exit

    // FIXME: the nodes are later picked up by SoNode::getByName(),
    // which means this is a rather lousy and error-prone technique
    // with the potential for namespace clashes. Should *at* *least*
    // append a prefix "coininternal_draggerdefaultpart_" or something
    // to all nodes. See also the related FIXME in
    // setAnyPartAsDefault(SbName,SbName). 20020322 mortene.
    defaultdraggerparts->append(root);
  }
  else {
    SoDebugError::post("SoInteractionKit::readDefaultParts",
                       "Dragger default parts not available.");
  }
}

/*!
  Protected version of setPartAsDefault(), to make it possible to set
  non-leaf and private parts (if \a anypart is \c TRUE).

  \sa setPartAsDefault()
*/
SbBool
SoInteractionKit::setAnyPartAsDefault(const SbName & partname,
                                      SoNode * node,
                                      SbBool COIN_UNUSED_ARG(anypart),
                                      SbBool onlyifdefault)
{
  SoBaseKit * kit = this;
  int partNum;
  SbBool isList;
  int listIdx;
  if (SoBaseKit::findPart(SbString(partname.getString()), kit, partNum,
                          isList, listIdx, TRUE)) {
    SoSFNode * field = kit->getCatalogInstances()[partNum];
    // FIXME: default check not working properly. pederb, 2000-01-21
    if (1 || (!onlyifdefault || field->isDefault())) {
      kit->setPart(partNum, node);
      field->setDefault(TRUE);
    }
    else {
      if (COIN_DEBUG) {
        SoDebugError::postInfo("SoInteractionKit::setAnyPartAsDefault",
                               "no permission to set");
      }
    }
  }
  else if (COIN_DEBUG) {
    SoDebugError::postInfo("SoInteractionKit::setAnyPartAsDefault",
                           "part %s not found", partname.getString());
  }

  // FIXME: this method is _always_ returning FALSE, which seems
  // completely bogus. 20020322 mortene.
  return FALSE;
}

/*!
  Protected version of setPartAsDefault(), to make it possible to set
  non-leaf and private parts (if anypart is \c TRUE).

  \sa setPartAsDefault()
*/
SbBool
SoInteractionKit::setAnyPartAsDefault(const SbName & partname,
                                      const SbName & nodename,
                                      SbBool anypart,
                                      SbBool onlyifdefault)
{
  // FIXME: this is lame and error-prone -- default dragger-parts are
  // actually just stored outside any scene graph, and then picked up
  // like this. We should at least prefix the node names with an
  // internal namespace prefix. See also the related FIXME in
  // readDefaultParts(). 20020322 mortene.
  SoNode * node = (SoNode *)
    SoBase::getNamedBase(nodename, SoNode::getClassTypeId());

  if (node) {
    return this->setAnyPartAsDefault(partname, node, anypart, onlyifdefault);
  }
  else if (COIN_DEBUG && 1) { // debug
    SoDebugError::postInfo("SoInteractionKit::setAnyPartAsDefault",
                           "nodename %s not found", nodename.getString());

    // FIXME: temporary code, pederb 2000-01-21
    node = new SoText2();
    ((SoText2 *)node)->string = "Default dragger part not found";
    return this->setAnyPartAsDefault(partname, node, anypart, onlyifdefault);
  }
  return FALSE;
}

// FIXME: the API doc on setAnySurrogatePath() below stinks. Surrogate
// parts is such a useful mechanism that it deserves proper
// documentation. We should explain what it's good for, the details of
// setting up a surrogate part, and add in a small usage example (i.e.
// source code). 20021008 mortene.

/*!
  Protected version of setPartAsPath(), to make it possible to set
  non-leaf and private parts.

  ("The nice thing about C++ is that only your friends can handle your
  private parts.")

  \sa setPartAsPath()
*/
SbBool
SoInteractionKit::setAnySurrogatePath(const SbName & partname,
                                      SoPath * path,
                                      SbBool leafcheck,
                                      SbBool publiccheck)
{
  SoBaseKit * kit = this;
  int partNum;
  SbBool isList;
  int listIdx;
  if (SoBaseKit::findPart(SbString(partname.getString()), kit, partNum,
                          isList, listIdx, TRUE)) {
    assert(kit->isOfType(SoInteractionKit::getClassTypeId()));
    const SoNodekitCatalog * catalog = kit->getNodekitCatalog();
    if (leafcheck && !catalog->isLeaf(partNum)) {
      if (COIN_DEBUG) {
        SoDebugError::postInfo("SoInteractionKit::setAnySurrogatePath",
                               "part %s is not leaf", partname.getString());
      }
      return FALSE;
    }
    if (publiccheck && !catalog->isPublic(partNum)) {
      if (COIN_DEBUG) {
        SoDebugError::postInfo("SoInteractionKit::setAnySurrogatePath",
                               "part %s is not public", partname.getString());
      }
      return FALSE;
    }
    int parentIdx = catalog->getParentPartNumber(partNum);
    SoNode * parent = kit->getCatalogInstances()[parentIdx]->getValue();
    if (parent->isOfType(SoSwitch::getClassTypeId())) {
      SoNode * node = kit->getCatalogInstances()[partNum]->getValue();
      SoType type = node->getTypeId();
      if (type == SoGroup::getClassTypeId() ||
          type == SoSeparator::getClassTypeId()) {
        // replace with empty group to keep switch numbering
        kit->setPart(partNum, (SoNode *)type.createInstance());
      }
      else { // set to NULL and update switch numbering
        SoSwitch * sw = (SoSwitch *)parent;
        int whichChild = sw->whichChild.getValue();
        int partIdx = sw->findChild(node);
        if (partIdx == whichChild) {
          sw->whichChild.setValue(SO_SWITCH_NONE);
        }
        else if (partIdx < whichChild) {
          sw->whichChild.setValue(whichChild-1);
        }
        kit->setPart(partNum, NULL);
      }
    }
    else {
      // set part to NULL
      kit->setPart(partNum, NULL);
    }
    // add the path
    ((SoInteractionKit *)kit)->pimpl->addSurrogatePath(path, catalog->getName(partNum));
    return TRUE;
  }
  else if (COIN_DEBUG) {
    SoDebugError::postInfo("SoInteractionKit::setAnyPartAsDefault",
                           "part %s not found", partname.getString());
  }
  return FALSE;
}

// doc in super
SbBool
SoInteractionKit::setUpConnections(SbBool onoff, SbBool doitalways)
{
  if (onoff == this->connectionsSetUp && !doitalways)
    return onoff;

  if (onoff) {
    (void)inherited::setUpConnections(onoff, FALSE);
    PRIVATE(this)->connectFields(TRUE);
    PRIVATE(this)->attachSensor(TRUE);
  }
  else {
    PRIVATE(this)->attachSensor(FALSE);
    PRIVATE(this)->connectFields(FALSE);
    (void)inherited::setUpConnections(onoff, FALSE);
  }
  return !(this->connectionsSetUp = onoff);
}

// Doc in superclass.
SbBool
SoInteractionKit::setPart(const int partNum, SoNode * node)
{
  // Overridden to detect when part changes value. If a substitute path
  // for that part exists, it must be cleared.

  PRIVATE(this)->removeSurrogatePath(this->getNodekitCatalog()->getName(partNum));
  return inherited::setPart(partNum, node);
}

// test if ok to set default and then do it if test succeeds
static void
test_set_default(SoSFEnum * field, int value)
{
  if (!(field->isConnected() && field->isConnectionEnabled()) &&
      field->getValue() == value) field->setDefault(TRUE);
}

// doc in super
void
SoInteractionKit::setDefaultOnNonWritingFields(void)
{
  this->topSeparator.setDefault(TRUE);
  this->geomSeparator.setDefault(TRUE);

  test_set_default(&this->renderCaching, SoInteractionKit::AUTO);
  test_set_default(&this->boundingBoxCaching, SoInteractionKit::AUTO);
  test_set_default(&this->pickCulling, SoInteractionKit::AUTO);
  test_set_default(&this->renderCulling, SoInteractionKit::AUTO);

  const SoNodekitCatalog * catalog = this->getNodekitCatalog();

  for (int i = 1; i < this->getCatalogInstances().getLength(); i++) {
    if (!catalog->isLeaf(i)) {
      SoNode * node = this->getCatalogInstances()[i]->getValue();
      if (node && node->getTypeId() == SoSwitch::getClassTypeId()) {
        this->getCatalogInstances()[i]->setDefault(TRUE);
      }
    }
  }

  inherited::setDefaultOnNonWritingFields();
}

// doc in super
SbBool
SoInteractionKit::setPart(const SbName & partname, SoNode * from)
{
  // Overridden only to fool some incredibly stupid behaviour in the
  // gcc 2.95.2 compiler, who couldn't figure out I wanted to call
  // this function in SoBaseKit, but instead insisted that I tried to
  // call SoInteractionKit::setPart(int, SoNode *). Cheeessssh.
  // <pederb>.
  return inherited::setPart(partname, from);
}

/*! \COININTERNAL */
void
SoInteractionKit::fieldSensorCB(void * d, SoSensor * s)
{
  SoInteractionKitP::sensorCB(d, s);
}

/*!
  Obsoleted in Coin.
*/
void
SoInteractionKit::connectSeparatorFields(SoSeparator * COIN_UNUSED_ARG(dest), SbBool onOff)
{
  COIN_OBSOLETED();
  SoDebugError::postWarning("SoInteractionKit::connectSeparatorFields",
                            "SoSeparator* input argument ignored, "
                            "using topSeparator");
  PRIVATE(this)->connectFields(onOff);
}

#undef PRIVATE

/*** methods for SoInteractionKitP are below *****************************/

#ifndef DOXYGEN_SKIP_THIS

//
// checks if partname is in surrogate list. Returns index, -1 if not found.
//
int
SoInteractionKitP::findSurrogateIndex(const SbName & partname) const
{
  int i, n = this->surrogatenamelist.getLength();
  for (i = 0; i < n; i++) {
    if (this->surrogatenamelist[i] == partname) return i;
  }
  return -1;
}

//
// removes surrogate path with name 'partname'
//
void
SoInteractionKitP::removeSurrogatePath(const SbName & partname)
{
  int idx = this->findSurrogateIndex(partname);
  if (idx >= 0) this->removeSurrogatePath(idx);
}

//
// removes a specified surrogate path
//
void
SoInteractionKitP::removeSurrogatePath(const int idx)
{
  assert(idx >= 0 && idx < this->surrogatenamelist.getLength());
  this->surrogatenamelist.remove(idx);
  this->surrogatepathlist.remove(idx);
}

//
// return index of surrogate path that is contained within path,
// or -1 if none found.
//
int
SoInteractionKitP::findSurrogateInPath(const SoPath * path)
{
  int n = this->surrogatepathlist.getLength();
  for (int i = 0; i < n; i++) {
    if (path->containsPath(this->surrogatepathlist[i])) return i;
  }
  return -1;
}

//
// adds or replaces a surrogate path
//
void
SoInteractionKitP::addSurrogatePath(SoPath * path, const SbName & name)
{
  int idx = this->findSurrogateIndex(name);
  if (idx >= 0) {
    this->surrogatepathlist.remove(idx);
    this->surrogatenamelist.remove(idx);
  }
  this->surrogatepathlist.append(path);
  this->surrogatenamelist.append(name);
}


//
// connect fields in topSeparator to the fields in this node.
//
void
SoInteractionKitP::connectFields(const SbBool onoff)
{
  if (this->connectedseparator) { // always disconnect
    this->connectedseparator->renderCaching.disconnect();
    this->connectedseparator->boundingBoxCaching.disconnect();
    this->connectedseparator->renderCulling.disconnect();
    this->connectedseparator->pickCulling.disconnect();
    this->connectedseparator->unref();
    this->connectedseparator = NULL;
  }
  if (onoff) {
    SoSeparator * sep = (SoSeparator*) this->kit->topSeparator.getValue();
    if (sep) {
      this->connectedseparator = sep;
      this->connectedseparator->ref(); // ref to make sure pointer is legal
      sep->renderCaching.connectFrom(&this->kit->renderCaching);
      sep->boundingBoxCaching.connectFrom(&this->kit->boundingBoxCaching);
      sep->renderCulling.connectFrom(&this->kit->renderCulling);
      sep->pickCulling.connectFrom(&this->kit->pickCulling);
    }
  }
}

//
// attach sensor to topSeparator if onoff, detach otherwise
//
void
SoInteractionKitP::attachSensor(const SbBool onoff)
{
  if (onoff) {
    if (this->fieldsensor->getAttachedField() != &this->kit->topSeparator) {
      this->fieldsensor->attach(&this->kit->topSeparator);
    }
  }
  else {
    if (this->fieldsensor->getAttachedField()) this->fieldsensor->detach();
  }
}

//
// callback from field sensor connected to topSeparator
//
void
SoInteractionKitP::sensorCB(void * data, SoSensor *)
{
  SoInteractionKitP * thisp = (SoInteractionKitP*) data;
  if (thisp->connectedseparator != thisp->kit->topSeparator.getValue()) {
    thisp->connectFields(TRUE);
  }
}

#endif // DOXYGEN_SKIP_THIS
#endif // HAVE_NODEKITS

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
  \class SoWWWInline SoWWWInline.h Inventor/nodes/SoWWWInline.h
  \brief The SoWWWInline class is a node used to include data from an URL.

  \ingroup coin_nodes

  If the URL is not a local file, the application is responsible for
  supplying a callback to a function which will fetch the data of the
  URL.

  As long as no data have been imported, the scene graph representation
  of the node will be that of a bounding box enclosing the geometry we
  expect to fetch from the URL.  The application is naturally also
  responsible for specifying the expected dimensions of the geometry.

  If FetchURLCallBack isn't set, the alternateRep will be rendered
  instead.

  <b>FILE FORMAT/DEFAULTS:</b>
  \code
    WWWInline {
        name "<Undefined file>"
        bboxCenter 0 0 0
        bboxSize 0 0 0
        alternateRep NULL
    }
  \endcode

  \since SGI Inventor 2.1
  \since Coin 1.0
*/

// *************************************************************************

// FIXME: as far as I can tell, SoWWWInline does not automatically
// trigger a (re-)load when the "SoWWWInline::name" field
// changes. Shouldn't it? Test what SGI/TGS Inventor does and mimic
// its behaviour. 20020522 mortene.

// FIXME: setting up the alternateRep field doesn't seem to work as
// expected (or at all, actually). This simple scene graph shown in an
// examiner viewer will not display the alternateRep for Coin, while
// SGI Inventor ivview will correctly show the Cone:
// ----8<-------------- [snip] -------8<-------------- [snip] -------------
// #Inventor V2.1 ascii
//
// WWWInline { alternateRep Group { BaseColor { rgb .2 .4 .6 } Cone { } } }
// ----8<-------------- [snip] -------8<-------------- [snip] -------------
// 20050315 mortene.

// *************************************************************************

#include <Inventor/nodes/SoWWWInline.h>

#include <cstddef>
#include <cstdlib>
#include <cstdio>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */

#include <Inventor/SbColor.h>
#include <Inventor/actions/SoGetBoundingBoxAction.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/actions/SoSearchAction.h>
#include <Inventor/elements/SoGLMultiTextureEnabledElement.h>
#include <Inventor/elements/SoLazyElement.h>
#include <Inventor/bundles/SoMaterialBundle.h>
#include <Inventor/misc/SoChildList.h>
#include <Inventor/SbColor.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/SoInput.h>
#include <Inventor/SoDB.h>
#include <Inventor/errors/SoReadError.h>
#include <Inventor/errors/SoDebugError.h>
#include <Inventor/threads/SbStorage.h>
#include <Inventor/system/gl.h>

#include "tidbitsp.h"
#include "coindefs.h" // COIN_OBSOLETED()
#include "nodes/SoSubNodeP.h"

// *************************************************************************

/*!
  \enum SoWWWInline::BboxVisibility
  Used to enumerate bounding box rendering strategies.
*/
/*!
  \var SoWWWInline::BboxVisibility SoWWWInline::NEVER
  Never render bounding box.
*/
/*!
  \var SoWWWInline::BboxVisibility SoWWWInline::UNTIL_LOADED
  Render bounding box until children are loaded.
*/
/*!
  \var SoWWWInline::BboxVisibility SoWWWInline::ALWAYS
  Always render bounding box, event when children are loaded.
*/

/*!
  \var SoSFString SoWWWInline::name
  Name of file/URL where children should be read from.
*/

/*!
  \var SoSFVec3f SoWWWInline::bboxCenter
  Center of bounding box.
*/
/*!
  \var SoSFVec3f SoWWWInline::bboxSize
  Size of bounding box.
*/
/*!
  \var SoSFNode SoWWWInline::alternateRep
  Alternate representation. Used when children can't be read from name.
*/

// *************************************************************************

// static members
SoWWWInlineFetchURLCB * SoWWWInline::fetchurlcb = NULL;
void * SoWWWInline::fetchurlcbdata = NULL;
SbColor * SoWWWInline::bboxcolor = NULL;
static SbStorage * wwwinline_colorpacker_storage = NULL;
SoWWWInline::BboxVisibility SoWWWInline::bboxvisibility = SoWWWInline::UNTIL_LOADED;
SbBool SoWWWInline::readassofile = FALSE;

static void alloc_colorpacker(void * ptr)
{
  SoColorPacker ** cptr = (SoColorPacker**) ptr;
  *cptr = new SoColorPacker;
}

static void free_colorpacker(void * ptr)
{
  SoColorPacker ** cptr = (SoColorPacker**) ptr;
  delete *cptr;
}

void
SoWWWInline::cleanup(void)
{
  delete SoWWWInline::bboxcolor;
  SoWWWInline::bboxcolor = NULL;
  delete wwwinline_colorpacker_storage;
  wwwinline_colorpacker_storage = NULL;

  SoWWWInline::fetchurlcb = NULL;
  SoWWWInline::fetchurlcbdata = NULL;
  SoWWWInline::bboxvisibility = SoWWWInline::UNTIL_LOADED;
  SoWWWInline::readassofile = FALSE;
}

// *************************************************************************

class SoWWWInlineP {
 public:
  SoWWWInlineP(SoWWWInline * ownerptr) {
    this->owner = ownerptr;
  }
  SoWWWInline * owner;
  SoChildList * children;
  SbBool readNamedFile();
  SbBool readChildren();
  SbString fullname;
  SbBool didrequest;

  static const char UNDEFINED_FILE[];
};

const char SoWWWInlineP::UNDEFINED_FILE[] = "<Undefined file>";

#define PRIVATE(obj) ((obj)->pimpl)

// *************************************************************************

SO_NODE_SOURCE(SoWWWInline);

// *************************************************************************

/*!
  Constructor.
*/
SoWWWInline::SoWWWInline()
{
  SO_NODE_INTERNAL_CONSTRUCTOR(SoWWWInline);

  PRIVATE(this) = new SoWWWInlineP(this);
  PRIVATE(this)->children = new SoChildList(this);
  PRIVATE(this)->didrequest = FALSE;

  SO_NODE_ADD_FIELD(name, (SoWWWInlineP::UNDEFINED_FILE));
  SO_NODE_ADD_FIELD(bboxCenter, (0.0f, 0.0f, 0.0f));
  SO_NODE_ADD_FIELD(bboxSize, (0.0f, 0.0f, 0.0f));
  SO_NODE_ADD_FIELD(alternateRep, (NULL));

  // Instantiated dynamically to avoid problems on platforms with
  // systemloaders that hate static constructors in C++ libraries.
  if (SoWWWInline::bboxcolor == NULL) {
    SoWWWInline::bboxcolor = new SbColor(0.8f, 0.8f, 0.8f);
    wwwinline_colorpacker_storage = new SbStorage(sizeof(void*), alloc_colorpacker, free_colorpacker);
    coin_atexit((coin_atexit_f *)SoWWWInline::cleanup, CC_ATEXIT_NORMAL);
  }
}

/*!
  Destructor.
*/
SoWWWInline::~SoWWWInline()
{
  delete PRIVATE(this)->children;
  delete PRIVATE(this);
}

// doc in super
/*!
  \copybrief SoBase::initClass(void)
*/
void
SoWWWInline::initClass(void)
{
  SO_NODE_INTERNAL_INIT_CLASS(SoWWWInline, SO_FROM_INVENTOR_2_1|SoNode::VRML1);
}

/*!
  If the SoWWWInline::name field specifies a relative URL, use this
  method to name the complete URL.
*/
void
SoWWWInline::setFullURLName(const SbString & url)
{
  PRIVATE(this)->fullname = url;
}

/*!
  If a full URL has been set with the SoWWWInline::setFullURLName()
  method, return it.  If not, returns the value of the
  SoWWWInline::name field.
*/
const SbString &
SoWWWInline::getFullURLName(void)
{
  return PRIVATE(this)->fullname.getLength() ? PRIVATE(this)->fullname : this->name.getValue();
}

/*!
  Returns a subgraph with a deep copy of the children of this node.
*/
SoGroup *
SoWWWInline::copyChildren(void) const
{
  SoChildList * children = this->getChildren();

  if (children->getLength() == 0) return NULL;
  assert(children->getLength() == 1);
  SoNode * rootcopy = (*children)[0]->copy();
  assert(rootcopy->isOfType(SoGroup::getClassTypeId()));
  return (SoGroup *) rootcopy;
}

/*!
  Start requesting URL data. This might trigger a callback to
  the callback set in SoWWWInline::setFetchURLCallBack().
*/
void
SoWWWInline::requestURLData(void)
{
  if (!PRIVATE(this)->didrequest) {
    PRIVATE(this)->didrequest = TRUE;
    (void) PRIVATE(this)->readChildren();
  }
}

/*!
  Returns \c TRUE if SoWWWInline::requestURLData() has been called
  without being canceled by SoWWWInline::cancelURLData().
*/
SbBool
SoWWWInline::isURLDataRequested(void) const
{
  return PRIVATE(this)->didrequest;
}

/*!
  Return \c TRUE if the current child data have been read from file/URL
  and set using setChildData().
*/
SbBool
SoWWWInline::isURLDataHere(void) const
{
  SoChildList * children = this->getChildren();
  if (children->getLength() == 0 ||
      (*children)[0] == this->alternateRep.getValue()) return FALSE;
  return FALSE;
}

/*!
  Can be used to signal that URL loading has been canceled.  You
  should use this method if you intend to request URL data more than
  once.
*/
void
SoWWWInline::cancelURLDataRequest(void)
{
  PRIVATE(this)->didrequest = FALSE;
}

/*!
  Manually set up the subgraph for this node. This should be used
  by the application to set the data that was read from the file/URL.
*/
void
SoWWWInline::setChildData(SoNode * urldata)
{
  PRIVATE(this)->children->truncate(0);
  PRIVATE(this)->children->append(urldata);
}

/*!
  Returns the child data for this node. This can be data read from a
  file, from an URL, from the contents of SoWWWInline::alternateRep or
  data that was manually set with SoWWWInline::setChildData().
*/
SoNode *
SoWWWInline::getChildData(void) const
{
  if (PRIVATE(this)->children->getLength()) { return (*PRIVATE(this)->children)[0]; }
  return NULL;
}

/*!
  Sets the URL fetch callback. This will be used in
  SoWWWInline::readInstance() or when the user calls
  SoWWWInline::requestURLData().
// FIXME: Shouldn't called on readInstance(), only when we need to
// render the node (or calculate the bbox if we don't have one). kintel 20060203.
*/
void
SoWWWInline::setFetchURLCallBack(SoWWWInlineFetchURLCB * f,
                                 void * userdata)
{
  SoWWWInline::fetchurlcb = f;
  SoWWWInline::fetchurlcbdata = userdata;
}

/*!
  Sets the bounding box visibility strategy.
  The default is UNTIL_LOADED.
*/
void
SoWWWInline::setBoundingBoxVisibility(BboxVisibility b)
{
  SoWWWInline::bboxvisibility = b;
}

/*!
  Returns the bounding box visibility.
*/
SoWWWInline::BboxVisibility
SoWWWInline::getBoundingBoxVisibility(void)
{
  return SoWWWInline::bboxvisibility;
}

/*!
  Sets the bounding box color.
*/
void
SoWWWInline::setBoundingBoxColor(SbColor & c)
{
  *SoWWWInline::bboxcolor = c;
}

/*!
  Returns the bounding box color.
*/
const SbColor &
SoWWWInline::getBoundingBoxColor(void)
{
  return *SoWWWInline::bboxcolor;
}

/*!
  Sets whether children should be read from a local file, in the same
  manner as SoFile children are read.

  If this is set to \c TRUE, the URL must point to a file on the local
  file system, as can be accessed by the standard C library fopen()
  call.
*/
void
SoWWWInline::setReadAsSoFile(SbBool onoff)
{
  SoWWWInline::readassofile = onoff;
}

/*!
  Returns if children should be read from local files.

  \sa setReadAsSoFile()
*/
SbBool
SoWWWInline::getReadAsSoFile(void)
{
  return SoWWWInline::readassofile;
}


// Documented in superclass.  Overridden to render children and/or
// bounding box.
void
SoWWWInline::GLRender(SoGLRenderAction * action)
{
  if (this->getChildData()) {
    SoWWWInline::doAction(action);
    if (SoWWWInline::bboxvisibility == UNTIL_LOADED) return;
  }
  if (SoWWWInline::bboxvisibility == NEVER) return;

  SoState * state = action->getState();
  state->push();

  SoColorPacker ** cptr = (SoColorPacker**) wwwinline_colorpacker_storage->get();

  SoLazyElement::setDiffuse(state, this, 1, SoWWWInline::bboxcolor, *cptr);

  // disable lighting
  SoLazyElement::setLightModel(state, SoLazyElement::BASE_COLOR);
  // disable texture mapping
  SoGLMultiTextureEnabledElement::disableAll(state);

  SoMaterialBundle mb(action);
  mb.sendFirst(); // set current color

  float cx, cy, cz;
  this->bboxCenter.getValue().getValue(cx, cy, cz);

  float x0, y0, z0;
  this->bboxSize.getValue().getValue(x0, y0, z0);
  x0 = -x0/2.0f + cx;
  y0 = -y0/2.0f + cy;
  z0 = -z0/2.0f + cz;
  float x1, y1, z1;
  this->bboxSize.getValue().getValue(x1, y1, z1);
  x1 = x1/2.0f + cx;
  y1 = y1/2.0f + cy;
  z1 = z1/2.0f + cz;

  glBegin(GL_LINE_LOOP);
  glVertex3f(x0, y0, z0);
  glVertex3f(x1, y0, z0);
  glVertex3f(x1, y1, z0);
  glVertex3f(x0, y1, z0);
  glEnd();
  glBegin(GL_LINE_LOOP);
  glVertex3f(x0, y0, z1);
  glVertex3f(x1, y0, z1);
  glVertex3f(x1, y1, z1);
  glVertex3f(x0, y1, z1);
  glEnd();

  glBegin(GL_LINES);
  glVertex3f(x0, y0, z0);
  glVertex3f(x0, y0, z1);
  glVertex3f(x0, y1, z0);
  glVertex3f(x0, y1, z1);
  glVertex3f(x1, y0, z0);
  glVertex3f(x1, y0, z1);
  glVertex3f(x1, y1, z0);
  glVertex3f(x1, y1, z1);
  glEnd();

  state->pop(); // restore state
}

// doc in super
void
SoWWWInline::getBoundingBox(SoGetBoundingBoxAction * action)
{
  if (this->getChildren()->getLength()) {
    int numindices;
    const int * indices;
    int lastchildindex;
    
    if (action->getPathCode(numindices, indices) == SoAction::IN_PATH)
      lastchildindex = indices[numindices-1];
    else
      lastchildindex = this->getChildren()->getLength() - 1;
    
    assert(lastchildindex < this->getChildren()->getLength());
    
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
  else {
    SbVec3f halfsize = bboxSize.getValue()/2.0f;
    SbVec3f center = bboxCenter.getValue();
    
    action->extendBy(SbBox3f(-halfsize[0] + center[0],
                             -halfsize[1] + center[1],
                             -halfsize[2] + center[2],
                             halfsize[0] + center[0],
                             halfsize[1] + center[1],
                             halfsize[2] + center[2]));
    
    assert(! action->isCenterSet());
    action->setCenter(center, TRUE);
  }
}

/*!
  Returns the child list with the child data for this node.
*/
SoChildList *
SoWWWInline::getChildren(void) const
{
  return PRIVATE(this)->children;
}

// doc in super
void
SoWWWInline::doAction(SoAction * action)
{
  if (this->getChildren()->getLength()) {
    int numindices;
    const int * indices;
    if (action->getPathCode(numindices, indices) == SoAction::IN_PATH) {
      this->getChildren()->traverseInPath(action, numindices, indices);
    }
    else {
      this->getChildren()->traverse((SoAction *)action);
    }
  }
}

/*!
  This method should probably have been private in Open Inventor API. It is
  obsoleted in Coin. Let us know if you need it.
*/
void
SoWWWInline::doActionOnKidsOrBox(SoAction * COIN_UNUSED_ARG(action))
{
  COIN_OBSOLETED();
}

// doc in super
void
SoWWWInline::callback(SoCallbackAction * action)
{
  SoWWWInline::doAction((SoAction *)action);
}

// doc in super
void
SoWWWInline::getMatrix(SoGetMatrixAction * action)
{
  SoWWWInline::doAction((SoAction *)action);
}

// doc in super
void
SoWWWInline::handleEvent(SoHandleEventAction * action)
{
  SoWWWInline::doAction((SoAction *)action);
}

// doc in super
void
SoWWWInline::search(SoSearchAction * action)
{
  SoNode::search(action);
  if (!action->isFound()) {
    SoWWWInline::doAction(action);
  }
}

// doc in super
void
SoWWWInline::pick(SoPickAction * action)
{
  SoWWWInline::doAction((SoAction *)action);
}

// doc in super
void
SoWWWInline::getPrimitiveCount(SoGetPrimitiveCountAction * action)
{
  SoWWWInline::doAction((SoAction *)action);
}

// doc in super
void
SoWWWInline::audioRender(SoAudioRenderAction * action)
{
  SoWWWInline::doAction((SoAction *)action);
}

/*!
  Convenience method that extends the current bounding box to
  include the box specified by \a center and \a size.
*/
void
SoWWWInline::addBoundingBoxChild(SbVec3f center, SbVec3f size)
{
  SbVec3f orgsize = this->bboxSize.getValue();
  SbVec3f orgcenter = this->bboxCenter.getValue();

  orgsize *= 0.5f;
  SbBox3f bbox(orgcenter-orgsize, orgcenter+orgsize);

  size *= 0.5f;
  SbBox3f newbox(center-size, center+size);

  bbox.extendBy(newbox);
  this->bboxCenter = bbox.getCenter();
  bbox.getSize(size[0], size[1], size[2]);
  this->bboxSize = size;
}

// Documented in superclass. Overridden to fetch/read child data.
SbBool
SoWWWInline::readInstance(SoInput * in, unsigned short flags)
{
  SbBool ret = inherited::readInstance(in, flags);
  if (ret) {
    ret = PRIVATE(this)->readChildren();
  }
  return ret;
}

// Documented in superclass. Overridden to copy children.
void
SoWWWInline::copyContents(const SoFieldContainer * fromfc,
                          SbBool copyconnections)
{
  this->getChildren()->truncate(0);
  inherited::copyContents(fromfc, copyconnections);

  SoWWWInline * inlinenode = (SoWWWInline *) fromfc;

  if (inlinenode->getChildren()->getLength() == 0) return;

  assert(inlinenode->getChildren()->getLength() == 1);

  SoNode * cp = (SoNode *)
    SoFieldContainer::findCopy((*(inlinenode->getChildren()))[0], copyconnections);
  this->getChildren()->append(cp);
}

#undef PRIVATE

// *************************************************************************

// Read the file named in the name field.
SbBool
SoWWWInlineP::readNamedFile()
{
  // If we can't find file, ignore it. Note that this does not match
  // the way Inventor works, which will make the whole read process
  // exit with a failure code.
  SoInput in;

  SbString name = this->owner->getFullURLName();
  if (!in.openFile(name.getString())) return TRUE;

  SoSeparator * node = SoDB::readAll(&in);

  // Popping the file off the stack again is done implicit in SoInput
  // upon hitting EOF (unless the read fails, see below).

  if (node) {
    this->children->truncate(0);
    this->children->append((SoNode *)node);
  }
  else {
    // Note that we handle this differently than Inventor, which lets
    // the whole import fail.
    SoReadError::post(&in, "Unable to read subfile: \"%s\"",
                      name.getString());
  }

  return TRUE;
}

/*!
  Read children, either using the URL callback or by reading from
  local file directly.

       fetchURLCB is NULL:   Use alternaterep. NB! Always uses alternate rep 
                             in this case.
  else name not set:         Do nothing
  else readassofile is TRUE: Assume name points to a local file and load
                             automatically without using fetchURLCB.

*/
SbBool
SoWWWInlineP::readChildren()
{
  if (!SoWWWInline::fetchurlcb) {
    if (this->owner->alternateRep.getValue()) {
      this->owner->setChildData(this->owner->alternateRep.getValue());
    }
  }
  else if (this->owner->name.getValue() != SoWWWInlineP::UNDEFINED_FILE) {
    if (SoWWWInline::readassofile) {
      return this->readNamedFile();
    }
    else {
      SoWWWInline::fetchurlcb(this->owner->getFullURLName(),
                              SoWWWInline::fetchurlcbdata,
                              this->owner);
    }
  }
  return TRUE;
}

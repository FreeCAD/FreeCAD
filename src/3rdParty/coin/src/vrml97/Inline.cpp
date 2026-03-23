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
  \class SoVRMLInline SoVRMLInline.h Inventor/VRMLnodes/SoVRMLInline.h
  \brief The SoVRMLInline class is used to insert VRML files into a scene.

  \ingroup coin_VRMLnodes

  \WEB3DCOPYRIGHT
  
  \verbatim
  Inline {
    exposedField MFString url        []
    field        SFVec3f  bboxCenter 0 0 0     # (-,)
    field        SFVec3f  bboxSize   -1 -1 -1  # (0,) or -1,-1,-1
  }
  \endverbatim

  The Inline node is a grouping node that reads its children data from
  a location in the World Wide Web. Exactly when its children are read
  and displayed is not defined (e.g. reading the children may be
  delayed until the Inline node's bounding box is visible to the
  viewer). The url field specifies the URL containing the children. An
  Inline node with an empty URL does nothing.  

  Each specified URL shall refer to a valid VRML file that contains a
  list of children nodes, prototypes, and routes at the top level as
  described in 4.6.5, Grouping and children nodes.  

  The results are undefined if the URL refers to a file that is not
  VRML or if the VRML file contains non-children nodes at the top
  level.  

  If multiple URLs are specified, the browser may display a URL of a
  lower preference VRML file while it is obtaining, or if it is unable
  to obtain, the higher preference VRML file. Details on the url field
  and preference order can be found in 4.5, VRML and the World Wide
  Web
  (<http://www.web3d.org/documents/specifications/14772/V2.0/part1/concepts.html#4.5>).  

  The results are undefined if the contents of the URL change after it
  has been loaded.  

  The bboxCenter and bboxSize fields specify a bounding box that
  encloses the Inline node's children. This is a hint that may be used
  for optimization purposes. The results are undefined if the
  specified bounding box is smaller than the actual bounding box of
  the children at any time. A default bboxSize value, (-1, -1, -1),
  implies that the bounding box is not specified and if needed shall
  be calculated by the browser. A description of the bboxCenter and
  bboxSize fields is in 4.6.4, Bounding boxes
  (<http://www.web3d.org/documents/specifications/14772/V2.0/part1/concepts.html#4.6.4>).  

*/

/*!
  SoSFVec3f SoVRMLInline::bboxCenter
  Center of bounding box.
*/

/*!
  SoSFVec3f SoVRMLInline::bboxSize
  Size of bounding box.
*/

/*!
  SoMFString SoVRMLInline::url
  The VRML file URL.
*/

/*!
  enum SoVRMLInline::BboxVisibility
  Used to enumerate bounding box visibility settings.
*/

/*!
  \var SoVRMLInline::BboxVisibility SoVRMLInline::NEVER
  Never display bounding box.
*/

/*!
  \var SoVRMLInline::BboxVisibility SoVRMLInline::UNTIL_LOADED
  Display bounding box until file is loaded.
*/

/*!
  \var SoVRMLInline::BboxVisibility SoVRMLInline::ALWAYS
  Always display bounding box.
*/

#include <Inventor/VRMLnodes/SoVRMLInline.h>
#include "coindefs.h"

#include <cstdlib>

#include <Inventor/VRMLnodes/SoVRMLMacros.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/SbColor.h>
#include <Inventor/SoInput.h>
#include <Inventor/SoDB.h>
#include <Inventor/SbBox3f.h>
#include <Inventor/errors/SoReadError.h>
#include <Inventor/actions/SoActions.h>
#include <Inventor/actions/SoSearchAction.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/actions/SoGetBoundingBoxAction.h>
#include <Inventor/misc/SoChildList.h>
#include <Inventor/sensors/SoFieldSensor.h>
#include <Inventor/elements/SoGLLazyElement.h>
#include <Inventor/elements/SoGLMultiTextureEnabledElement.h>
#include <Inventor/system/gl.h>

#include "nodes/SoSubNodeP.h"
#include "tidbitsp.h"

class SoVRMLInlineP {
public:
  SbString fullurlname;
  SbBool isrequested;
  SoChildList * children;
  SoFieldSensor * urlsensor;
};

static SoVRMLInline::BboxVisibility
sovrmlinline_bboxvisibility = SoVRMLInline::UNTIL_LOADED;
static SoVRMLInlineFetchURLCB * sovrmlinline_fetchurlcb = NULL;
static void * sovrmlinline_fetchurlcbclosure;

static SbColor * sovrmlinline_bboxcolor = NULL;
static SbBool sovrmlinline_readassofile = TRUE;

static void
sovrmlinline_cleanup(void)
{
  delete sovrmlinline_bboxcolor;
  sovrmlinline_bboxcolor = NULL;
  sovrmlinline_bboxvisibility = SoVRMLInline::UNTIL_LOADED;
  sovrmlinline_fetchurlcb = NULL;  
  sovrmlinline_readassofile = TRUE;
}

SO_NODE_SOURCE(SoVRMLInline);

/*!
  \copydetails SoNode::initClass(void)
*/
void
SoVRMLInline::initClass(void)
{
  SO_NODE_INTERNAL_INIT_CLASS(SoVRMLInline, SO_VRML97_NODE_TYPE);
  sovrmlinline_bboxcolor = new SbColor(0.8f, 0.8f, 0.8f);
  coin_atexit((coin_atexit_f*) sovrmlinline_cleanup, CC_ATEXIT_NORMAL);
  SoAudioRenderAction::addMethod(SoVRMLInline::getClassTypeId(),
                                 SoAudioRenderAction::callDoAction);
}

#define PRIVATE(obj) ((obj)->pimpl)

/*!
  Constructor
*/
SoVRMLInline::SoVRMLInline(void)
{
  PRIVATE(this) = new SoVRMLInlineP;
  PRIVATE(this)->isrequested = FALSE;
  PRIVATE(this)->children = new SoChildList(this);

  SO_VRMLNODE_INTERNAL_CONSTRUCTOR(SoVRMLInline);

  SO_VRMLNODE_ADD_FIELD(bboxCenter, (0.0f, 0.0f, 0.0f));
  SO_VRMLNODE_ADD_FIELD(bboxSize, (-1.0f, -1.0f, -1.0f));
  SO_VRMLNODE_ADD_EMPTY_EXPOSED_MFIELD(url);

  PRIVATE(this)->urlsensor = new SoFieldSensor(SoVRMLInline::urlFieldModified, this);
  PRIVATE(this)->urlsensor->setPriority(0); // immediate sensor
  PRIVATE(this)->urlsensor->attach(& this->url);
}

/*!
  Destructor.
*/
SoVRMLInline::~SoVRMLInline()
{
  delete PRIVATE(this)->urlsensor;
  delete PRIVATE(this)->children;
  delete PRIVATE(this);
}

/*!
  Sets the full (non-relative) URL name.
*/
void
SoVRMLInline::setFullURLName(const SbString & urlref)
{
  PRIVATE(this)->fullurlname = urlref;
}

/*!
  Returns the full URL name.
*/
const SbString &
SoVRMLInline::getFullURLName(void)
{
  return PRIVATE(this)->fullurlname;
}

/*!
  Returns a copy of the children.
*/
SoGroup *
SoVRMLInline::copyChildren(void) const
{
  if (PRIVATE(this)->children->getLength() == 0) return NULL;
  assert(PRIVATE(this)->children->getLength() == 1);
  SoNode * rootcopy = (*(PRIVATE(this)->children))[0]->copy();
  assert(rootcopy->isOfType(SoGroup::getClassTypeId()));
  return (SoGroup *)rootcopy;
}

// Doc in parent
SoChildList *
SoVRMLInline::getChildren(void) const
{
  return PRIVATE(this)->children;
}

/*!
  Request URL data.
*/
void
SoVRMLInline::requestURLData(void)
{
  PRIVATE(this)->isrequested = TRUE;
  if (sovrmlinline_fetchurlcb) {
    sovrmlinline_fetchurlcb(PRIVATE(this)->fullurlname,
                            sovrmlinline_fetchurlcbclosure,
                            this);
  }
}

/*!
  Returns TRUE if the URL data have been requested.
*/
SbBool
SoVRMLInline::isURLDataRequested(void) const
{
  return PRIVATE(this)->isrequested;
}

/*!
  Returns TRUE if the data have been loaded.
*/
SbBool
SoVRMLInline::isURLDataHere(void) const
{
  return this->getChildData() != NULL;
}

/*!
  Cancel the URL data request.
*/
void
SoVRMLInline::cancelURLDataRequest(void)
{
  PRIVATE(this)->isrequested = FALSE;
}

/*!
  Sets the child data. Can be used by the URL fetch callback.
*/
void
SoVRMLInline::setChildData(SoNode * urldata)
{
  PRIVATE(this)->isrequested = FALSE;
  PRIVATE(this)->children->truncate(0);
  if (urldata) {
    PRIVATE(this)->children->append(urldata);
  }
}

/*!
  Returns the child data (the scene loaded from the URL).
*/
SoNode *
SoVRMLInline::getChildData(void) const
{
  if (PRIVATE(this)->children->getLength()) {
    return (*PRIVATE(this)->children)[0];
  }
  return NULL;
}

/*!
  Sets the callback used to handle URL loading.
*/
void
SoVRMLInline::setFetchURLCallBack(SoVRMLInlineFetchURLCB * f,
                                  void * closure)
{
  sovrmlinline_fetchurlcb = f;
  sovrmlinline_fetchurlcbclosure = closure;
}

/*!
  Sets the bounding box visibility strategy.
*/
void
SoVRMLInline::setBoundingBoxVisibility(BboxVisibility b)
{
  sovrmlinline_bboxvisibility = b;
}

/*!
  Returns the bounding box visibility strategy.
*/
SoVRMLInline::BboxVisibility
SoVRMLInline::getBoundingBoxVisibility(void)
{
  return sovrmlinline_bboxvisibility;
}

/*!
  Sets the color of the bounding box.
*/
void
SoVRMLInline::setBoundingBoxColor(SbColor & color)
{
  sovrmlinline_bboxcolor->setValue(color[0], color[1], color[2]);
}

/*!
  Returns the color of the bounding box.
*/
SbColor &
SoVRMLInline::getBoundingBoxColor(void)
{
  return *sovrmlinline_bboxcolor;
}

/*!
  Sets whether Inline nodes should be treated as a normal Inventor SoFile node.
*/
void
SoVRMLInline::setReadAsSoFile(SbBool enable)
{
  sovrmlinline_readassofile = enable;
}

/*!
  Returns whether Inline nodes are read as SoFile nodes.
*/
SbBool
SoVRMLInline::getReadAsSoFile(void)
{
  return sovrmlinline_readassofile;
}

// Doc in parent
void
SoVRMLInline::doAction(SoAction * action)
{
  int numindices;
  const int * indices;
  if (action->getPathCode(numindices, indices) == SoAction::IN_PATH) {
    PRIVATE(this)->children->traverseInPath(action, numindices, indices);
  }
  else {
    PRIVATE(this)->children->traverse(action);
  }
}

// Doc in parent
void
SoVRMLInline::callback(SoCallbackAction * action)
{
  SoVRMLInline::doAction((SoAction*)action);
}

// Doc in parent
void
SoVRMLInline::GLRender(SoGLRenderAction * action)
{
  BboxVisibility vis = sovrmlinline_bboxvisibility;
  SbVec3f size = this->bboxSize.getValue();
  SoNode * child = this->getChildData();
  if ((size[0] >= 0.0f && size[1] >= 0.0f && size[2] >= 0.0f) &&
      ((vis == ALWAYS) || 
       (vis == UNTIL_LOADED && child == NULL))) {
    SoState * state = action->getState();
    state->push();

    SoGLMultiTextureEnabledElement::disableAll(state);
    
    uint32_t packedcolor = sovrmlinline_bboxcolor->getPackedValue();
    SoGLLazyElement::sendLightModel(state, SoLazyElement::BASE_COLOR);
    SoGLLazyElement::sendPackedDiffuse(state, packedcolor);
    
    SbVec3f center = this->bboxCenter.getValue();
    SbVec3f minv = center - size*0.5f;
    SbVec3f maxv = center + size*0.5f;
    
    SbVec3f p[8];
    for (int i = 0; i < 8; i++) {
      p[i][0] = i & 1 ? minv[0] : maxv[0];
      p[i][1] = i & 2 ? minv[1] : maxv[1];
      p[i][2] = i & 4 ? minv[2] : maxv[2];
    }

    glBegin(GL_LINE_LOOP);
    glVertex3fv(p[0].getValue());
    glVertex3fv(p[1].getValue());
    glVertex3fv(p[3].getValue());
    glVertex3fv(p[2].getValue());
    glEnd();

    glBegin(GL_LINE_LOOP);
    glVertex3fv(p[4].getValue());
    glVertex3fv(p[5].getValue());
    glVertex3fv(p[7].getValue());
    glVertex3fv(p[6].getValue());
    glEnd();

    glBegin(GL_LINES);

    glVertex3fv(p[0].getValue());
    glVertex3fv(p[4].getValue());

    glVertex3fv(p[2].getValue());
    glVertex3fv(p[6].getValue());

    glVertex3fv(p[3].getValue());
    glVertex3fv(p[7].getValue());

    glVertex3fv(p[1].getValue());
    glVertex3fv(p[5].getValue());

    glEnd();
    state->pop();
  }
  SoVRMLInline::doAction(action);
}

// Doc in parent
void
SoVRMLInline::getBoundingBox(SoGetBoundingBoxAction * action)
{
  SbVec3f size = this->bboxSize.getValue();
  if (size[0] > 0.0f || size[1] > 0.0f || size[2] > 0) {
    size[0] = SbMax(size[0], 0.0f);
    size[1] = SbMax(size[1], 0.0f);
    size[2] = SbMax(size[2], 0.0f);
    SbVec3f center = this->bboxCenter.getValue();
    size *= 0.5f;
    SbBox3f box(center[0]-size[0],
                center[1]-size[1],
                center[2]-size[2],
                center[0]+size[0],
                center[1]+size[1],
                center[2]+size[2]);
    if (!box.isEmpty()) {
      action->extendBy(box);
      action->setCenter(center, TRUE);
    }
  }
  else {
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
}

// Doc in parent
void
SoVRMLInline::getMatrix(SoGetMatrixAction * action)
{
  SoVRMLInline::doAction((SoAction*)action);
}

// Doc in parent
void
SoVRMLInline::handleEvent(SoHandleEventAction * action)
{
  SoVRMLInline::doAction((SoAction*)action);
}

// Doc in parent
void
SoVRMLInline::search(SoSearchAction * action)
{
  SoNode::search(action);
  if (action->isFound()) return;

  SoVRMLInline::doAction(action);
}

// Doc in parent
void
SoVRMLInline::pick(SoPickAction * action)
{
  SoVRMLInline::doAction((SoAction*)action);
}

// Doc in parent
void
SoVRMLInline::getPrimitiveCount(SoGetPrimitiveCountAction * action)
{
  SoVRMLInline::doAction((SoAction*)action);
}

void
SoVRMLInline::addBoundingBoxChild(SbVec3f COIN_UNUSED_ARG(center),
                                  SbVec3f COIN_UNUSED_ARG(size))
{
  // FIXME: implement
}

// Doc in parent
SbBool
SoVRMLInline::readInstance(SoInput * in,
                           unsigned short flags)
{
  SbBool ret = TRUE;

  PRIVATE(this)->urlsensor->detach();
  if (sovrmlinline_readassofile) {
    PRIVATE(this)->fullurlname.makeEmpty();
    ret = inherited::readInstance(in, flags);
    ret = ret && this->readLocalFile(in);
  }
  else {
    ret = inherited::readInstance(in, flags);
    if (ret) this->requestURLData();
  }
  PRIVATE(this)->urlsensor->attach(&this->url);

  return ret; 
}

// Doc in parent
void
SoVRMLInline::copyContents(const SoFieldContainer * from,
                           SbBool copyconnections)
{
  PRIVATE(this)->children->truncate(0);
  inherited::copyContents(from, copyconnections);

  SoVRMLInline * inlinenode = (SoVRMLInline *)from;
  PRIVATE(this)->fullurlname = inlinenode->pimpl->fullurlname;
  // the request will go to the original node, not this one.
  PRIVATE(this)->isrequested = FALSE;

  if (inlinenode->pimpl->children->getLength() == 0) return;

  assert(inlinenode->pimpl->children->getLength() == 1);

  SoNode * cp = (SoNode *)
    SoFieldContainer::findCopy((*(inlinenode->pimpl->children))[0],
                               copyconnections);
  PRIVATE(this)->children->append(cp);
}

/*!
  Read the (local) file named in the SoVRMLInline::url field.
*/
SbBool
SoVRMLInline::readLocalFile(SoInput * in)
{
  if (this->url.getNum() == 0) {
    return TRUE;
  }

  SbString filename = this->url[0];

  // If we can't find file, ignore it. Note that this does not match
  // the way Inventor works, which will make the whole read process
  // exit with a failure code.
  if (!in->pushFile(filename.getString())) return TRUE;

  PRIVATE(this)->fullurlname = in->getCurFileName();

  SoSeparator * node = SoDB::readAll(in);

  if (node) {
    PRIVATE(this)->children->truncate(0);
    PRIVATE(this)->children->append((SoNode *)node);
  }
  else {
    if (in->getCurFileName() == PRIVATE(this)->fullurlname) {
      // Take care of popping the file off the stack. This is a bit
      // "hack-ish", but its done this way instead of loosening the
      // protection of SoInput::popFile().
      char dummy;
      while (!in->eof() && in->get(dummy)) {}
      assert(in->eof());
      
      // Make sure the stack is really popped on EOF. Popping happens
      // when attempting to read when the current file in the stack is
      // at EOF.
      SbBool gotchar = in->get(dummy);
      if (gotchar) in->putBack(dummy);
    }

    // Note that we handle this differently than Inventor, which lets
    // the whole import fail.
    SoReadError::post(in, "Unable to read Inline file: \"%s\"",
                      filename.getString());
  }

  return TRUE;
}

// Callback for the field sensor.
void
SoVRMLInline::urlFieldModified(void * userdata, SoSensor * COIN_UNUSED_ARG(sensor))
{
  SoVRMLInline * thisp = (SoVRMLInline *)userdata;
  SoInput in;
  thisp->pimpl->fullurlname.makeEmpty();
  if (sovrmlinline_readassofile) {
    (void)thisp->readLocalFile(&in);
  }
  else {
    thisp->requestURLData();
  }
}

#undef PRIVATE

#endif // HAVE_VRML97

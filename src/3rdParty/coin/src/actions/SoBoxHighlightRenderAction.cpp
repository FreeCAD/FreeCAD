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
  \class SoBoxHighlightRenderAction SoBoxHighlightRenderAction.h Inventor/actions/SoBoxHighlightRenderAction.h
  \brief The SoBoxHighlightRenderAction class renders the scene with highlighted boxes around selections.

  \ingroup coin_actions

  This action performs the same tasks as its parent class,
  SoGLRenderAction, with the added ability to render highlighted
  bounding boxes around geometry in selected nodes. This is a simple
  but convenient way of giving feedback to the user upon interaction
  with the scene graph.

  To have the highlighting actually happen (and to be able to
  automatically "select" nodes by picking with the mouse cursor), you
  need to use SoSelection nodes in place of group nodes.

  \sa SoLineHighlightRenderAction, SoSelection
*/

#include <Inventor/actions/SoBoxHighlightRenderAction.h>
#include <Inventor/actions/SoSearchAction.h>
#include <Inventor/actions/SoGetBoundingBoxAction.h>
#include <Inventor/nodes/SoSelection.h>
#include <Inventor/nodes/SoCube.h>
#include <Inventor/nodes/SoCamera.h>
#include <Inventor/nodes/SoMatrixTransform.h>
#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/nodes/SoComplexity.h>
#include <Inventor/nodes/SoLightModel.h>
#include <Inventor/nodes/SoBaseColor.h>
#include <cassert>

#include "actions/SoSubActionP.h"
#include "SbBasicP.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

/*!
  \var SoBoxHighlightRenderAction::hlVisible

  Boolean which decides whether or not the highlights for selected
  nodes should be visible.
 */

#ifndef DOXYGEN_SKIP_THIS

class SoBoxHighlightRenderActionP {
public:
  SoBoxHighlightRenderActionP(void) : master(NULL) { }

  SoBoxHighlightRenderAction * master;
  SoSearchAction * searchaction;
  SoSearchAction * camerasearch;
  SoGetBoundingBoxAction * bboxaction;
  SoBaseColor * basecolor;
  SoTempPath * postprocpath;
  SoSeparator * bboxseparator;
  SoMatrixTransform * bboxtransform;
  SoCube * bboxcube;
  SoDrawStyle * drawstyle;

  void initBoxGraph();
  void drawHighlightBox(const SoPath * path);
};

#define PRIVATE(obj) ((obj)->pimpl)
#define PUBLIC(obj) ((obj)->master)

// used to initialize the internal storage class with variables
void
SoBoxHighlightRenderActionP::initBoxGraph()
{
  this->bboxseparator = new SoSeparator;
  this->bboxseparator->ref();
  this->bboxseparator->renderCaching = SoSeparator::OFF;
  this->bboxseparator->boundingBoxCaching = SoSeparator::OFF;

  this->bboxtransform = new SoMatrixTransform;
  this->bboxcube = new SoCube;

  this->drawstyle = new SoDrawStyle;
  this->drawstyle->style = SoDrawStyleElement::LINES;
  this->basecolor = new SoBaseColor;

  SoLightModel * lightmodel = new SoLightModel;
  lightmodel->model = SoLazyElement::BASE_COLOR;

  SoComplexity * complexity = new SoComplexity;
  complexity->textureQuality = 0.0f;
  complexity->type = SoComplexityTypeElement::BOUNDING_BOX;

  this->bboxseparator->addChild(this->drawstyle);
  this->bboxseparator->addChild(this->basecolor);

  this->bboxseparator->addChild(lightmodel);
  this->bboxseparator->addChild(complexity);

  this->bboxseparator->addChild(this->bboxtransform);
  this->bboxseparator->addChild(this->bboxcube);
}


// used to render shape and non-shape nodes (usually SoGroup or SoSeparator).
void
SoBoxHighlightRenderActionP::drawHighlightBox(const SoPath * path)
{
  if (this->camerasearch == NULL) {
    this->camerasearch = new SoSearchAction;
  }

  // find camera used to render node
  this->camerasearch->setFind(SoSearchAction::TYPE);
  this->camerasearch->setInterest(SoSearchAction::FIRST); // find first camera to break out asap
  this->camerasearch->setType(SoCamera::getClassTypeId());
  this->camerasearch->apply(const_cast<SoPath*>(path));

  if (this->camerasearch->getPath()) {
    this->bboxseparator->insertChild(this->camerasearch->getPath()->getTail(), 0);
  }
  this->camerasearch->reset();

  if (this->bboxaction == NULL) {
    this->bboxaction = new SoGetBoundingBoxAction(SbViewportRegion(100, 100));
  }
  this->bboxaction->setViewportRegion(PUBLIC(this)->getViewportRegion());
  this->bboxaction->apply(const_cast<SoPath*>(path));

  SbXfBox3f & box = this->bboxaction->getXfBoundingBox();

  if (!box.isEmpty()) {
    // set cube size
    float x, y, z;
    box.getSize(x, y, z);
    this->bboxcube->width  = x;
    this->bboxcube->height  = y;
    this->bboxcube->depth = z;

    SbMatrix transform = box.getTransform();

    // get center (in the local bbox coordinate system)
    SbVec3f center = box.SbBox3f::getCenter();

    // if center != (0,0,0), move the cube
    if (center != SbVec3f(0.0f, 0.0f, 0.0f)) {
      SbMatrix t;
      t.setTranslate(center);
      transform.multLeft(t);
    }
    this->bboxtransform->matrix = transform;

    PUBLIC(this)->SoGLRenderAction::apply(this->bboxseparator);
  }
  // remove camera
  this->bboxseparator->removeChild(0);
}

#endif // DOXYGEN_SKIP_THIS

SO_ACTION_SOURCE(SoBoxHighlightRenderAction);

/*!
  \copydetails SoAction::initClass(void)
*/
void
SoBoxHighlightRenderAction::initClass(void)
{
  SO_ACTION_INTERNAL_INIT_CLASS(SoBoxHighlightRenderAction, SoGLRenderAction);
}


/*!
  Default constructor. Note: passes a default SbViewportRegion to the
  parent constructor.
 */
SoBoxHighlightRenderAction::SoBoxHighlightRenderAction(void)
  : inherited(SbViewportRegion())
{
  this->init();
}

/*!
  Constructor, taking an explicit \a viewportregion to render.
*/
SoBoxHighlightRenderAction::SoBoxHighlightRenderAction(const SbViewportRegion & viewportregion)
  : inherited(viewportregion)
{
  this->init();
}

//
// private. called by both constructors
//
void
SoBoxHighlightRenderAction::init(void)
{
  SO_ACTION_CONSTRUCTOR(SoBoxHighlightRenderAction);

  PRIVATE(this)->master = this;

  // Initialize local variables
  PRIVATE(this)->initBoxGraph();

  this->hlVisible = TRUE;

  PRIVATE(this)->basecolor->rgb.setValue(1.0f, 0.0f, 0.0f);
  PRIVATE(this)->drawstyle->linePattern = 0xffff;
  PRIVATE(this)->drawstyle->lineWidth = 3.0f;
  PRIVATE(this)->searchaction = NULL;
  PRIVATE(this)->camerasearch = NULL;
  PRIVATE(this)->bboxaction = NULL;

  // SoBase-derived objects should be dynamically allocated.
  PRIVATE(this)->postprocpath = new SoTempPath(32);
  PRIVATE(this)->postprocpath->ref();
}


/*!
  Destructor.
*/
SoBoxHighlightRenderAction::~SoBoxHighlightRenderAction(void)
{
  PRIVATE(this)->postprocpath->unref();
  PRIVATE(this)->bboxseparator->unref();

  delete PRIVATE(this)->searchaction;
  delete PRIVATE(this)->camerasearch;
  delete PRIVATE(this)->bboxaction;
}

// Documented in superclass. Overridden to add highlighting after the
// "ordinary" rendering.
void
SoBoxHighlightRenderAction::apply(SoNode * node)
{
  SoGLRenderAction::apply(node);
  if (this->hlVisible) {
    if (PRIVATE(this)->searchaction == NULL) {
      PRIVATE(this)->searchaction = new SoSearchAction;
    }
    const SbBool searchall = FALSE;
    PRIVATE(this)->searchaction->setType(SoSelection::getClassTypeId());
    PRIVATE(this)->searchaction->setInterest(searchall ? SoSearchAction::ALL : SoSearchAction::FIRST);
    PRIVATE(this)->searchaction->apply(node);

    if (searchall) {
      const SoPathList & pathlist = PRIVATE(this)->searchaction->getPaths();
      if (pathlist.getLength() > 0) {
        int i;
        for (i = 0; i < pathlist.getLength(); i++) {
          SoFullPath * path = static_cast<SoFullPath *>(pathlist[i]);
          assert(path);
          SoSelection * selection = static_cast<SoSelection *>(path->getTail());
          if (selection->getNumSelected() > 0)
            this->drawBoxes(path, selection->getList());
        }
      }
    }
    else {
      SoFullPath * path =
        static_cast<SoFullPath *>(PRIVATE(this)->searchaction->getPath());
      if (path) {
        SoSelection * selection = static_cast<SoSelection *>(path->getTail());
        if (selection->getNumSelected()) {
          this->drawBoxes(path, selection->getList());
        }
      }
    }
    PRIVATE(this)->searchaction->reset();
  }
}

// Documented in superclass. This method will just call the
// SoGLRenderAction::apply() method (so no highlighting will be done).
//
// It has been overridden to avoid confusing the compiler, which
// typically want to see either all or none of the apply() methods
// overridden.
void
SoBoxHighlightRenderAction::apply(SoPath * path)
{
  SoGLRenderAction::apply(path);
}

// Documented in superclass.  This method will just call the
// SoGLRenderAction::apply() method (so no highlighting will be done).
//
// It has been overridden to avoid confusing the compiler, which
// typically want to see either all or none of the apply() methods
// overridden.
void
SoBoxHighlightRenderAction::apply(const SoPathList & pathlist,
                                  SbBool obeysrules)
{
  SoGLRenderAction::apply(pathlist, obeysrules);
}

/*!
  Sets if highlighted boxes should be \a visible when
  rendering. Defaults to \c TRUE.
*/
void
SoBoxHighlightRenderAction::setVisible(const SbBool visible)
{
  this->hlVisible = visible;
}

/*!
  Returns \c TRUE if highlighted boxes are visible otherwise \c FALSE.
*/
SbBool
SoBoxHighlightRenderAction::isVisible(void) const
{
  return this->hlVisible;
}

/*!
  Sets the \a color for the highlighted boxes. Defaults to completely
  red.
*/
void
SoBoxHighlightRenderAction::setColor(const SbColor & color)
{
  PRIVATE(this)->basecolor->rgb = color;
}

/*!
  Returns the rendering color of the highlighted boxes.
*/
const SbColor &
SoBoxHighlightRenderAction::getColor(void)
{
  return PRIVATE(this)->basecolor->rgb[0];
}

/*!
  Sets the line \a pattern used for the highlighted boxes. Defaults to
  \c 0xffff (i.e. drawn with no stipples).
*/
void
SoBoxHighlightRenderAction::setLinePattern(unsigned short pattern)
{
  PRIVATE(this)->drawstyle->linePattern = pattern;
}

/*!
  Returns the line pattern used when drawing boxes.
*/
unsigned short
SoBoxHighlightRenderAction::getLinePattern(void) const
{
  return PRIVATE(this)->drawstyle->linePattern.getValue();
}

/*!
  Sets the line \a width used when drawing boxes, in screen pixels (as
  for all OpenGL rendering). Defaults to 3.
*/
void
SoBoxHighlightRenderAction::setLineWidth(const float width)
{
  PRIVATE(this)->drawstyle->lineWidth = width;
}

/*!
  Returns the line width used when drawing highlight boxes.
*/
float
SoBoxHighlightRenderAction::getLineWidth(void) const
{
  return PRIVATE(this)->drawstyle->lineWidth.getValue();
}

void
SoBoxHighlightRenderAction::drawBoxes(SoPath * pathtothis, const SoPathList * pathlist)
{
  int i;
  int thispos = reclassify_cast<SoFullPath *>(pathtothis)->getLength()-1;
  assert(thispos >= 0);
  PRIVATE(this)->postprocpath->setHead(pathtothis->getHead()); // reset

  for (i = 1; i < thispos; i++) {
    PRIVATE(this)->postprocpath->append(pathtothis->getIndex(i));
  }

  // we need to disable accumulation buffer antialiasing while
  // rendering selected objects
  int oldnumpasses = this->getNumPasses();
  this->setNumPasses(1);

  SoState * thestate = this->getState();
  thestate->push();

  for (i = 0; i < pathlist->getLength(); i++) {
    SoFullPath * path = reclassify_cast<SoFullPath *>((*pathlist)[i]);
    PRIVATE(this)->postprocpath->append(path->getHead());
    for (int j = 1; j < path->getLength(); j++) {
      PRIVATE(this)->postprocpath->append(path->getIndex(j));
    }

    // Previously SoGLRenderAction was used to draw the bounding boxes
    // of shapes in selection paths, by overriding renderstyle state
    // elements to lines drawstyle and simply doing:
    //
    //   SoGLRenderAction::apply(PRIVATE(this)->postprocpath); // Bug
    //
    // This could have the unwanted side effect of rendering
    // non-selected shapes, as they could be part of the path (due to
    // being placed below SoGroup nodes (instead of SoSeparator
    // nodes)) up to the selected shape.
    //
    //
    // A better approach turned out to be to soup up and draw only the
    // bounding boxes of the selected shapes:
    PRIVATE(this)->drawHighlightBox(PRIVATE(this)->postprocpath);

    // Remove temporary path from path buffer
    PRIVATE(this)->postprocpath->truncate(thispos);
  }

  this->setNumPasses(oldnumpasses);
  thestate->pop();
}


#undef PRIVATE
#undef PUBLIC

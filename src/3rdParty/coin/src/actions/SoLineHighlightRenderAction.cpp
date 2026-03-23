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
  \class SoLineHighlightRenderAction SoLineHighlightRenderAction.h Inventor/actions/SoLineHighlightRenderAction.h
  \brief The SoLineHighlightRenderAction class renders selections with line highlighting.

  \ingroup coin_actions

  See the documentation of SoBoxHighlightRenderAction.

  The only difference between SoBoxHighlightRenderAction and this
  action is that this action renders highlights by superposing a
  wireframe image onto each shape instead of the bounding box when
  drawing the highlight.

  Note: an important limitation for this rendering action is that the
  superimposition of a wireframe is only guaranteed to work correctly
  for filled shapes, due to a limitation in OpenGL's glPolygonOffset()
  functionality. For more information, see the class documentation of
  the SoPolygonOffset node.

  \sa SoBoxHighlightRenderAction, SoSelection
*/

// *************************************************************************

#include <Inventor/actions/SoLineHighlightRenderAction.h>

#include <cassert>

#include <Inventor/SbName.h>
#include <Inventor/actions/SoSearchAction.h>
#include <Inventor/actions/SoSubAction.h>
#include <Inventor/elements/SoDrawStyleElement.h>
#include <Inventor/elements/SoLazyElement.h>
#include <Inventor/elements/SoLinePatternElement.h>
#include <Inventor/elements/SoLineWidthElement.h>
#include <Inventor/elements/SoOverrideElement.h>
#include <Inventor/elements/SoPolygonOffsetElement.h>
#include <Inventor/elements/SoTextureOverrideElement.h>
#include <Inventor/elements/SoTextureQualityElement.h>
#include <Inventor/elements/SoMaterialBindingElement.h>
#include <Inventor/elements/SoNormalElement.h>
#include <Inventor/lists/SoEnabledElementsList.h>
#include <Inventor/lists/SoPathList.h>
#include <Inventor/misc/SoState.h>
#include <Inventor/nodes/SoSelection.h>
#include <Inventor/threads/SbStorage.h>

#include "SbBasicP.h"
#include "actions/SoSubActionP.h"

// *************************************************************************

/*!
  \var SoLineHighlightRenderAction::hlVisible

  Boolean which decides whether or not the highlights for selected
  nodes should be visible.
 */

// *************************************************************************

#define PRIVATE(obj) ((obj)->pimpl)
#define PUBLIC(obj) ((obj)->owner)

extern "C" {
static void alloc_colorpacker(void * data);
static void free_colorpacker(void * data);
}

class SoLineHighlightRenderActionP {
public:
  SoLineHighlightRenderActionP(void)
    : colorpacker_storage(sizeof(void*), alloc_colorpacker, free_colorpacker),
      owner(NULL)
  {
    this->color = SbColor(1.0f, 0.0f, 0.0f);
    this->linepattern = 0xffff;
    this->linewidth = 3.0f;
    this->searchaction = NULL;

    // SoBase-derived objects should be dynamically allocated.
    this->postprocpath = new SoTempPath(32);
    this->postprocpath->ref();
  }

  ~SoLineHighlightRenderActionP() {
    this->postprocpath->unref();
    delete this->searchaction;
  }

  void drawBoxes(SoPath * pathtothis, const SoPathList * pathlist);

  SoSearchAction * searchaction;
  SbColor color;
  uint16_t linepattern;
  float linewidth;
  SoTempPath * postprocpath;
  SbStorage colorpacker_storage;

  SoLineHighlightRenderAction * owner;

};

void alloc_colorpacker(void * data) {
  SoColorPacker ** cptr = static_cast<SoColorPacker ** >(data);
  *cptr = new SoColorPacker;
}

void free_colorpacker(void * data) {
  SoColorPacker ** cptr = static_cast<SoColorPacker ** >(data);
  delete *cptr;
}

// *************************************************************************

SO_ACTION_SOURCE(SoLineHighlightRenderAction);

// *************************************************************************

/*!
  \copydetails SoAction::initClass(void)
*/
void
SoLineHighlightRenderAction::initClass(void)
{
  SO_ACTION_INTERNAL_INIT_CLASS(SoLineHighlightRenderAction, SoGLRenderAction);
}


/*!
  Default constructor. Note: passes a default SbViewportRegion to the
  parent constructor.
 */
SoLineHighlightRenderAction::SoLineHighlightRenderAction(void)
  : inherited(SbViewportRegion())
{
  PRIVATE(this)->owner = this;
  // need to set hlVisible here, and not in the pimpl constructor, since
  // "owner" is not initialized when the pimpl constructor is called
  this->hlVisible = TRUE;

  SO_ACTION_CONSTRUCTOR(SoLineHighlightRenderAction);
}

/*!
  Constructor, taking an explicit \a viewportregion to render.
*/
SoLineHighlightRenderAction::SoLineHighlightRenderAction(const SbViewportRegion & viewportregion)
  : inherited(viewportregion)
{
  PRIVATE(this)->owner = this;
  // need to set hlVisible here, and not in the pimpl constructor, since
  // "owner" is not initialized when the pimpl constructor is called
  this->hlVisible = TRUE;
  SO_ACTION_CONSTRUCTOR(SoLineHighlightRenderAction);
}

/*!
  The destructor.
*/
SoLineHighlightRenderAction::~SoLineHighlightRenderAction()
{
}

// Documented in superclass. Overridden to add highlighting after the
// "ordinary" rendering.
void
SoLineHighlightRenderAction::apply(SoNode * node)
{
  SoGLRenderAction::apply(node);
  
  if (this->hlVisible) {
    if (PRIVATE(this)->searchaction == NULL) {
      PRIVATE(this)->searchaction = new SoSearchAction;
    }
    // Coin, and SGI Inventor, only supports one Selection node in a
    // graph, so just search for the first one to avoid that the whole
    // scene graph is searched
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
            PRIVATE(this)->drawBoxes(path, selection->getList());
        }
      }
    }
    else {
      SoFullPath * path =
        static_cast<SoFullPath *>(PRIVATE(this)->searchaction->getPath());
      if (path) {
        SoSelection * selection = static_cast<SoSelection *>(path->getTail());
        assert(selection->getTypeId().isDerivedFrom(SoSelection::getClassTypeId()));
        if (selection->getNumSelected() > 0) {
          PRIVATE(this)->drawBoxes(path, selection->getList());
        }
      }
    }
    // reset action to clear path
    PRIVATE(this)->searchaction->reset();
  }
}

// Documented in superclass.  This method will just call the
// SoGLRenderAction::apply() method (so no highlighting will be done).
//
// It has been overridden to avoid confusing the compiler, which
// typically want to see either all or none of the apply() methods
// overridden.
void
SoLineHighlightRenderAction::apply(SoPath * path)
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
SoLineHighlightRenderAction::apply(const SoPathList & pathlist,
                                   SbBool obeysrules)
{
  SoGLRenderAction::apply(pathlist, obeysrules);
}

/*!
  Sets if highlight wireframes should be \a visible when
  rendering. Defaults to \c TRUE.
*/
void
SoLineHighlightRenderAction::setVisible(const SbBool visible)
{
  this->hlVisible = visible;
}

/*!
  Return if selection wireframes should be visible.
*/
SbBool
SoLineHighlightRenderAction::isVisible(void) const
{
  return this->hlVisible;
}

/*!
  Sets the \a color of the wireframes. Defaults to red.
*/
void
SoLineHighlightRenderAction::setColor(const SbColor & color)
{
  PRIVATE(this)->color = color;
}

/*!
  Returns color of selection wireframes.
*/
const SbColor &
SoLineHighlightRenderAction::getColor(void)
{
  return PRIVATE(this)->color;
}

/*!
  Sets the line \a pattern used when drawing wireframes. Defaults to
  \c 0xffff (i.e. full, unstippled lines).
*/
void
SoLineHighlightRenderAction::setLinePattern(uint16_t pattern)
{
  PRIVATE(this)->linepattern = pattern;
}

/*!
  Returns line pattern used when drawing wireframe.
*/
uint16_t
SoLineHighlightRenderAction::getLinePattern(void) const
{
  return PRIVATE(this)->linepattern;
}

/*!
  Sets the line \a width used when drawing wireframe. Defaults to 3
  (measured in screen pixels).
*/
void
SoLineHighlightRenderAction::setLineWidth(const float width)
{
  PRIVATE(this)->linewidth = width;
}

/*!
  Returns the line width used when drawing wireframe.
*/
float
SoLineHighlightRenderAction::getLineWidth(void) const
{
  return PRIVATE(this)->linewidth;
}

void
SoLineHighlightRenderActionP::drawBoxes(SoPath * pathtothis,
                                        const SoPathList * pathlist)
{
  int i;
  int thispos = reclassify_cast<SoFullPath *>(pathtothis)->getLength()-1;
  assert(thispos >= 0);
  this->postprocpath->setHead(pathtothis->getHead()); // reset

  for (i = 1; i < thispos; i++) {
    this->postprocpath->append(pathtothis->getIndex(i));
  }

  SoState * state = PUBLIC(this)->getState();
  state->push();

  // we need to disable accumulation buffer antialiasing while
  // rendering selected objects
  int oldnumpasses = PUBLIC(this)->getNumPasses();
  PUBLIC(this)->setNumPasses(1);

  SoLazyElement::setLightModel(state, SoLazyElement::BASE_COLOR);
  
  SoColorPacker ** cptr = static_cast<SoColorPacker **>(this->colorpacker_storage.get());

  SoLazyElement::setDiffuse(state, pathtothis->getHead(), 1, &this->color, *cptr);
  // FIXME: we should check this versus the actual max line width
  // supported by the underlying OpenGL context. 20050610 mortene.
  SoLineWidthElement::set(state, this->linewidth);
  SoLinePatternElement::set(state, this->linepattern);
  SoTextureQualityElement::set(state, 0.0f);
  SoDrawStyleElement::set(state, SoDrawStyleElement::LINES);
  SoPolygonOffsetElement::set(state, NULL, -1.0f, -1.0f, SoPolygonOffsetElement::LINES, TRUE);
  SoMaterialBindingElement::set(state, NULL, SoMaterialBindingElement::OVERALL); 
  SoNormalElement::set(state, NULL, 0, NULL, FALSE);
 
  SoOverrideElement::setNormalVectorOverride(state, NULL, TRUE);
  SoOverrideElement::setMaterialBindingOverride(state, NULL, TRUE);
  SoOverrideElement::setLightModelOverride(state, NULL, TRUE);
  SoOverrideElement::setDiffuseColorOverride(state, NULL, TRUE);
  SoOverrideElement::setLineWidthOverride(state, NULL, TRUE);
  SoOverrideElement::setLinePatternOverride(state, NULL, TRUE);
  SoOverrideElement::setDrawStyleOverride(state, NULL, TRUE);
  SoOverrideElement::setPolygonOffsetOverride(state, NULL, TRUE);
  SoTextureOverrideElement::setQualityOverride(state, TRUE);

  for (i = 0; i < pathlist->getLength(); i++) {
    SoFullPath * path = reclassify_cast<SoFullPath *>((*pathlist)[i]);

    this->postprocpath->append(path->getHead());
    for (int j = 1; j < path->getLength(); j++) {
      this->postprocpath->append(path->getIndex(j));
    }

    PUBLIC(this)->SoGLRenderAction::apply(this->postprocpath);
    this->postprocpath->truncate(thispos);
  }

  PUBLIC(this)->setNumPasses(oldnumpasses);
  state->pop();
}

#undef PUBLIC
#undef PRIVATE

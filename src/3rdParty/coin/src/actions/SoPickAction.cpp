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
  \class SoPickAction SoPickAction.h Inventor/actions/SoPickAction.h
  \brief The SoPickAction class is the base class for picking actions.

  \ingroup coin_actions

  The basis for all interaction features that Coin provides for the
  application programmer is the pick actions. Draggers, manipulators,
  SoSelection nodes, etc all make use of the functionality provided by
  the pick actions for selecting and manipulating scene geometry in
  various ways.

  This class is not supposed to be used directly by the application
  programmer, as it is more a placeholder for the common interface of
  picking operations. It does not contain any actual code for doing
  scene graph picks.

  Coin provides a fully functional picking action for the application
  programmer to use through the SoRayPickAction class (which inherits
  the SoPickAction class).

  \sa SoSelection
*/

#include <Inventor/actions/SoPickAction.h>

#include <Inventor/elements/SoDecimationPercentageElement.h>
#include <Inventor/elements/SoDecimationTypeElement.h>
#include <Inventor/elements/SoLazyElement.h>
#include <Inventor/elements/SoCacheElement.h>
#include <Inventor/elements/SoViewportRegionElement.h>
#include <Inventor/lists/SoEnabledElementsList.h>
#include <Inventor/misc/SoState.h>
#include <Inventor/nodes/SoNode.h>

#include "actions/SoSubActionP.h"

/*!
  \var SbViewportRegion SoPickAction::vpRegion
  The viewport region used by pick actions.
*/

class SoPickActionP {
public:
};

SO_ACTION_SOURCE(SoPickAction);


/*!
  \copydetails SoAction::initClass(void)
*/
void
SoPickAction::initClass(void)
{
  SO_ACTION_INTERNAL_INIT_CLASS(SoPickAction, SoAction);

  SO_ENABLE(SoPickAction, SoViewportRegionElement);
  SO_ENABLE(SoPickAction, SoDecimationTypeElement);
  SO_ENABLE(SoPickAction, SoDecimationPercentageElement);
  SO_ENABLE(SoPickAction, SoLazyElement);
  SO_ENABLE(SoPickAction, SoCacheElement);
}


/*!
  Constructor.
*/
SoPickAction::SoPickAction(const SbViewportRegion & viewportregion)
  : vpRegion(viewportregion), cullingenabled(TRUE)
{
  SO_ACTION_CONSTRUCTOR(SoPickAction);
}

/*!
  Destructor.
*/
SoPickAction::~SoPickAction(void)
{
}

/*!
  Set a new viewport region to replace the one passed in with the
  constructor.
*/
void
SoPickAction::setViewportRegion(const SbViewportRegion & newregion)
{
  this->vpRegion = newregion;
}

/*!
  Returns the viewport region used by the action.
 */
const SbViewportRegion &
SoPickAction::getViewportRegion(void) const
{
  return this->vpRegion;
}

// Documented in superclass. Overrides parent traversal to set up the
// state element for the viewport region.
void
SoPickAction::beginTraversal(SoNode * node)
{
  this->getState()->push();
  SoViewportRegionElement::set(this->getState(), this->vpRegion);
  inherited::beginTraversal(node);
  this->getState()->pop();
}

/*!
  Don't calculate bounding boxes and try to do culling when picking.

  This can provide a speed-up in cases where the default setting of \c
  on would be inefficient.
*/
void
SoPickAction::enableCulling(const SbBool flag)
{
  this->cullingenabled = flag;
}

/*!
  Returns the current state of the culling flag.
*/
SbBool
SoPickAction::isCullingEnabled(void) const
{
  return this->cullingenabled;
}

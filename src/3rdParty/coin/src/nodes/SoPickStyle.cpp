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
  \class SoPickStyle SoPickStyle.h Inventor/nodes/SoPickStyle.h
  \brief The SoPickStyle class is a node for setting up how to do picking.

  \ingroup coin_nodes

  By default, all geometry in a scene is available for picking. Upon
  writing applications with interaction possibilities, this is often
  \e not what you want. To exclude parts of the scene graph from pick
  actions, use the SoPickStyle::UNPICKABLE.

  You can also optimize pick operations by using the
  SoPickStyle::BOUNDING_BOX pick style.

  <b>FILE FORMAT/DEFAULTS:</b>
  \code
    PickStyle {
        style SHAPE
    }
  \endcode

  \sa SoRayPickAction
*/

// *************************************************************************

#include <Inventor/actions/SoPickAction.h>

#include <Inventor/elements/SoOverrideElement.h>
#include <Inventor/actions/SoCallbackAction.h>

#include "nodes/SoSubNodeP.h"

// *************************************************************************

/*!
  \enum SoPickStyle::Style

  Enumeration of the available picking strategies.
*/
/*!
  \var SoPickStyle::Style SoPickStyle::SHAPE

  Do "exact" picks, finding the correct intersection point(s), etc.
*/
/*!
  \var SoPickStyle::Style SoPickStyle::BOUNDING_BOX

  Only compare pick intersection with the bounding boxes of
  shapes. This is usually much faster than SoPickStyle::SHAPE.
*/
/*!
  \var SoPickStyle::Style SoPickStyle::UNPICKABLE

  The geometry following this node in the scene will not be available
  for picking.
*/
/*!
  \var SoPickStyle::Style SoPickStyle::SHAPE_ON_TOP

  Do exact picks, like SHAPE, but sort the shape to the front of the
  list of picked points so it appears to be the frontmost item. This
  does not affect the SoPickedPoint pick coordinate though, just the
  sorting by depth done on the picked point list.

  \since Coin 3.0
*/
/*!
  \var SoPickStyle::Style SoPickStyle::BOUNDING_BOX_ON_TOP

  Do picks against the enclosing bounding box of the object, like
  BOUNDING_BOX, but sort the shape to the front of the list of picked
  points so it appears to be the frontmost item. This does not affect
  the SoPickedPoint pick coordinate though, just the sorting by depth
  done on the picked point list.

  \since Coin 3.0
*/
/*!
  \var SoPickStyle::Style SoPickStyle::SHAPE_FRONTFACES

  Do exact picks, like SHAPE, but cull all the backface intersections
  from the list. Note that this logic is relative to the ray, not the
  view, in case the ray and the view direction are not aligned.

  The behaviour of this setting will also depend on the SoShapeStyle
  vertexOrdering and shapeType setting, which are used to flip
  front faces to become back faces and vice versa and to turn backface
  culling on and off.

  \since Coin 3.0
*/

/*!
  \var SoSFEnum SoPickStyle::style

  Which strategy to use for the picking actions for subsequent shapes
  in the scene graph. Default value is SoPickStyle::SHAPE.
*/

// *************************************************************************

SO_NODE_SOURCE(SoPickStyle);

/*!
  Constructor.
*/
SoPickStyle::SoPickStyle(void)
{
  SO_NODE_INTERNAL_CONSTRUCTOR(SoPickStyle);

  SO_NODE_ADD_FIELD(style, (SoPickStyle::SHAPE));

  SO_NODE_DEFINE_ENUM_VALUE(Style, SHAPE);
  SO_NODE_DEFINE_ENUM_VALUE(Style, BOUNDING_BOX);
  SO_NODE_DEFINE_ENUM_VALUE(Style, UNPICKABLE);
  SO_NODE_DEFINE_ENUM_VALUE(Style, SHAPE_ON_TOP);
  SO_NODE_DEFINE_ENUM_VALUE(Style, BOUNDING_BOX_ON_TOP);
  SO_NODE_DEFINE_ENUM_VALUE(Style, SHAPE_FRONTFACES);
  SO_NODE_SET_SF_ENUM_TYPE(style, Style);
}

/*!
  Destructor.
*/
SoPickStyle::~SoPickStyle()
{
}

// doc in super
/*!
  \copybrief SoBase::initClass(void)
*/
void
SoPickStyle::initClass(void)
{
  SO_NODE_INTERNAL_INIT_CLASS(SoPickStyle, SO_FROM_INVENTOR_2_0);

  SO_ENABLE(SoCallbackAction, SoPickStyleElement);
  SO_ENABLE(SoPickAction, SoPickStyleElement);
}

void
SoPickStyle::doAction(SoAction * action)
{
  if (!this->style.isIgnored()
      && !SoOverrideElement::getPickStyleOverride(action->getState())) {
    SoPickStyleElement::set(action->getState(), this,
                            (int32_t) this->style.getValue());
    if (this->isOverride()) {
      SoOverrideElement::setPickStyleOverride(action->getState(), this, TRUE);
    }
  }
}

void
SoPickStyle::callback(SoCallbackAction * action)
{
  SoPickStyle::doAction(action);
}

void
SoPickStyle::pick(SoPickAction * action)
{
  SoPickStyle::doAction(action);
}

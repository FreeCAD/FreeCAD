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
  \class SoProfileCoordinate3 SoProfileCoordinate3.h Inventor/nodes/SoProfileCoordinate3.h
  \brief The SoProfileCoordinate3 class is a node specifying a set of 3D coordinates for profiles.

  \ingroup coin_nodes

  Use nodes of this type to provide coordinates to profiles.

  The third element of the coordinate vector is used for
  normalization. A node of this type where all the normalization
  values are equal to 1.0 is the equivalent of setting up an
  SoProfileCoordinate2 node.

  Note that an SoProfileCoordinate3 node will \e replace the profile
  coordinates already present in the state (if any).

  <b>FILE FORMAT/DEFAULTS:</b>
  \code
    ProfileCoordinate3 {
        point 0 0 1
    }
  \endcode

  \sa SoProfile, SoProfileCoordinate2
*/

// *************************************************************************

#include <Inventor/nodes/SoProfileCoordinate3.h>

#include <Inventor/actions/SoCallbackAction.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/actions/SoGetBoundingBoxAction.h>
#include <Inventor/actions/SoGetPrimitiveCountAction.h>
#include <Inventor/actions/SoPickAction.h>
#include <Inventor/elements/SoProfileCoordinateElement.h>

#include "nodes/SoSubNodeP.h"

// *************************************************************************

/*!
  \var SoMFVec3f SoProfileCoordinate3::point

  Pool of coordinate points for the traversal state.
*/

// *************************************************************************

SO_NODE_SOURCE(SoProfileCoordinate3);

/*!
  Constructor.
*/
SoProfileCoordinate3::SoProfileCoordinate3(void)
{
  SO_NODE_INTERNAL_CONSTRUCTOR(SoProfileCoordinate3);

  SO_NODE_ADD_FIELD(point, (0.0f, 0.0f, 1.0f));
}

/*!
  Destructor.
*/
SoProfileCoordinate3::~SoProfileCoordinate3()
{
}

// doc in super
/*!
  \copybrief SoBase::initClass(void)
*/
void
SoProfileCoordinate3::initClass(void)
{
  SO_NODE_INTERNAL_INIT_CLASS(SoProfileCoordinate3, SO_FROM_INVENTOR_1);

  SO_ENABLE(SoGLRenderAction, SoProfileCoordinateElement);
  SO_ENABLE(SoPickAction, SoProfileCoordinateElement);
  SO_ENABLE(SoCallbackAction, SoProfileCoordinateElement);
  SO_ENABLE(SoGetBoundingBoxAction, SoProfileCoordinateElement);
  SO_ENABLE(SoGetPrimitiveCountAction, SoProfileCoordinateElement);
}

void
SoProfileCoordinate3::getBoundingBox(SoGetBoundingBoxAction * action)
{
  SoProfileCoordinate3::doAction(action);
}

void
SoProfileCoordinate3::doAction(SoAction * action)
{
  SoProfileCoordinateElement::set3(action->getState(), this,
                                   this->point.getNum(),
                                   this->point.getValues(0));
}

void
SoProfileCoordinate3::GLRender(SoGLRenderAction * action)
{
  SoProfileCoordinate3::doAction(action);
}

void
SoProfileCoordinate3::callback(SoCallbackAction * action)
{
  SoProfileCoordinate3::doAction(action);
}

void
SoProfileCoordinate3::pick(SoPickAction * action)
{
  SoProfileCoordinate3::doAction(action);
}

void
SoProfileCoordinate3::getPrimitiveCount(SoGetPrimitiveCountAction * action)
{
  SoProfileCoordinate3::doAction(action);
}

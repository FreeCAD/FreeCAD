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
  \class SoProfile SoProfile.h Inventor/nodes/SoProfile.h
  \brief The SoProfile class is the abstract superclass for profile definitions.

  \ingroup coin_nodes

  Node subclasses of SoProfile specify profiles for extruded 3D text
  and NURBS surface data.
*/

#include <Inventor/nodes/SoProfile.h>

#include <Inventor/actions/SoCallbackAction.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/actions/SoGetBoundingBoxAction.h>
#include <Inventor/actions/SoGetPrimitiveCountAction.h>
#include <Inventor/actions/SoPickAction.h>
#include <Inventor/elements/SoProfileCoordinateElement.h>
#include <Inventor/elements/SoProfileElement.h>

#include "nodes/SoSubNodeP.h"

/*!
  \enum SoProfile::Profile

  Enumeration of various choices of how to link together multiple
  profiles.
*/
/*!
  \var SoProfile::Profile SoProfile::START_FIRST
  Replace the current profile state set with this profile alone.
*/
/*!
  \var SoProfile::Profile SoProfile::START_NEW
  Append this profile to the state as a new profile.
*/
/*!
  \var SoProfile::Profile SoProfile::ADD_TO_CURRENT
  Append indices of this node to the last profile.
*/


/*!
  \var SoMFInt32 SoProfile::index

  Profile coordinate indices.

  These must match what is available from previous
  SoProfileCoordinate2 and SoProfileCoordinate3 nodes in the
  traversal.
*/
/*!
  \var SoSFEnum SoProfile::linkage

  How the indices of this profile node should be combined with the
  current profile index set of the traversal state.

  Default value is SoProfile::START_FIRST.
*/


/*!
  \fn void SoProfile::getTrimCurve(SoState * state, int32_t & numpoints, float *& points, int & floatspervec, int32_t & numknots, float *& knotvector)
  Return \a points and \a knotvector of the \a state.
*/

/*!
  \fn void SoProfile::getVertices(SoState * state, int32_t & numvertices, SbVec2f *& vertices)
  Return vertex set of \a state.
*/


// *************************************************************************

SO_NODE_ABSTRACT_SOURCE(SoProfile);

/*!
  Constructor.
*/
SoProfile::SoProfile(void)
{
  SO_NODE_INTERNAL_CONSTRUCTOR(SoProfile);

  SO_NODE_ADD_FIELD(index, (0));
  SO_NODE_ADD_FIELD(linkage, (START_FIRST));
  SO_NODE_DEFINE_ENUM_VALUE(Profile, START_FIRST);
  SO_NODE_DEFINE_ENUM_VALUE(Profile, START_NEW);
  SO_NODE_DEFINE_ENUM_VALUE(Profile, ADD_TO_CURRENT);
  SO_NODE_SET_SF_ENUM_TYPE(linkage, Profile);
}

/*!
  Destructor.
*/
SoProfile::~SoProfile()
{
}

/*!
  \copybrief SoBase::initClass(void)
*/
void
SoProfile::initClass(void)
{
  SO_NODE_INTERNAL_INIT_ABSTRACT_CLASS(SoProfile, SO_FROM_INVENTOR_1);

  SO_ENABLE(SoCallbackAction, SoProfileCoordinateElement);
  SO_ENABLE(SoCallbackAction, SoProfileCoordinateElement);
  SO_ENABLE(SoCallbackAction, SoProfileElement);
  SO_ENABLE(SoGLRenderAction, SoProfileCoordinateElement);
  SO_ENABLE(SoGLRenderAction, SoProfileElement);
  SO_ENABLE(SoGetBoundingBoxAction, SoProfileCoordinateElement);
  SO_ENABLE(SoGetBoundingBoxAction, SoProfileElement);
  SO_ENABLE(SoGetPrimitiveCountAction, SoProfileElement);
  SO_ENABLE(SoPickAction, SoProfileCoordinateElement);
  SO_ENABLE(SoPickAction, SoProfileElement);
}

// Doc from superclass.
void
SoProfile::getBoundingBox(SoGetBoundingBoxAction * action)
{
  SoProfile::doAction(action);
}

// Doc from superclass.
void
SoProfile::doAction(SoAction * action)
{
  SoProfileElement::add(action->getState(), this);
}

// Doc from superclass.
void
SoProfile::callback(SoCallbackAction * action)
{
  SoProfile::doAction(action);
}

// Doc from superclass.
void
SoProfile::GLRender(SoGLRenderAction * action)
{
  SoProfile::doAction(action);
}

// Doc from superclass.
void
SoProfile::pick(SoPickAction * action)
{
  SoProfile::doAction(action);
}

// Doc from superclass.
void
SoProfile::getPrimitiveCount(SoGetPrimitiveCountAction * action)
{
  SoProfile::doAction(action);
}

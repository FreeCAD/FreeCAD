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
  \class SoBumpMapCoordinate SoBumpMapCoordinate.h Inventor/nodes/SoBumpMapCoordinate.h
  \brief The SoBumpMapCoordinate class is a node for providing bump map coordinates to shape nodes.

  \ingroup coin_nodes

  When encountering nodes of this type during traversal, the
  coordinates it contains will be put on the state stack for later use
  by shape nodes. The bump map coordinates can be used to specify
  explicit coordinates for a bump map. The SoBumpMap node is used to
  specify a bump map for the shape nodes.

  Note that an SoBumpMapCoordinate node will \e replace the bump map
  coordinates already present in the state (if any).

  Also note that since the indexed shape nodes have no
  bumpMapCoordIndex field, the textureCoordIndex field will be used
  for selecting bump map coordinate indices. You can set the bump map
  coordinate binding using the SoTextureCoordinateBinding node. Bump
  map coordinates must therefore have the same binding as the texture
  coordinates for texture unit 0.

  If you supply no bump map coordinates for a shape, the texture
  coordinates for texture unit 0 will be used.

  <b>FILE FORMAT/DEFAULTS:</b>
  \code
    BumpMapCoordinate {
        point [  ]
    }
  \endcode

  \since Coin 2.2
*/

#include <Inventor/nodes/SoBumpMapCoordinate.h>

#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/actions/SoCallbackAction.h>
#include <Inventor/actions/SoRayPickAction.h>
#include <Inventor/elements/SoBumpMapCoordinateElement.h>

#include "nodes/SoSubNodeP.h"

/*!
  \var SoMFVec3f SoBumpMapCoordinate::point
  Set of 2D points. Contains no points by default.
*/


// *************************************************************************

SO_NODE_SOURCE(SoBumpMapCoordinate);

/*!
  Constructor.
*/
SoBumpMapCoordinate::SoBumpMapCoordinate(void)
{
  SO_NODE_INTERNAL_CONSTRUCTOR(SoBumpMapCoordinate);

  SO_NODE_ADD_FIELD(point, (0.0f, 0.0f));
  this->point.setNum(0);
  this->point.setDefault(TRUE);
}

/*!
  Destructor.
*/
SoBumpMapCoordinate::~SoBumpMapCoordinate()
{
}

/*!
  \copybrief SoBase::initClass(void)
*/
void
SoBumpMapCoordinate::initClass(void)
{
  SO_NODE_INTERNAL_INIT_CLASS(SoBumpMapCoordinate, SO_FROM_COIN_2_2);

  SO_ENABLE(SoGLRenderAction, SoBumpMapCoordinateElement);
  SO_ENABLE(SoCallbackAction, SoBumpMapCoordinateElement);
  SO_ENABLE(SoPickAction, SoBumpMapCoordinateElement);
}

// Doc from superclass.
void
SoBumpMapCoordinate::doAction(SoAction * action)
{
  SoBumpMapCoordinateElement::set(action->getState(), this,
                                  point.getNum(), point.getValues(0));
}

// Doc from superclass.
void
SoBumpMapCoordinate::GLRender(SoGLRenderAction * action)
{
  SoBumpMapCoordinateElement::set(action->getState(), this,
                                  point.getNum(), point.getValues(0));
}

// Doc from superclass.
void
SoBumpMapCoordinate::callback(SoCallbackAction * action)
{
  SoBumpMapCoordinate::doAction((SoAction*)action);
}

// Doc from superclass.
void
SoBumpMapCoordinate::pick(SoPickAction * action)
{
  SoBumpMapCoordinate::doAction((SoAction*) action);
}

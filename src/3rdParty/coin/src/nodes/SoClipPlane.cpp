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
  \class SoClipPlane SoClipPlane.h Inventor/nodes/SoClipPlane.h
  \brief The SoClipPlane class is a node type for specifying clipping planes.

  \ingroup coin_nodes

  A scene graph \e without any SoClipPlane nodes uses six clipping
  planes to define the viewing frustum: top, bottom, left, right, near
  and far. If you want extra clipping planes for "slicing" the visible
  geometry, you can do that by using nodes of this type.  Geometry on
  the back side of the clipping plane is clipped away.

  Note that OpenGL implementations have a fixed maximum number of
  clipping planes available. To find out what this number is, you can
  use the following code:

  \code
      #include <Inventor/elements/SoGLClipPlaneElement.h>
      // ...[snip]...
      int maxplanes = SoGLClipPlaneElement::getMaxGLPlanes();
  \endcode

  Below is a simple, basic scene graph usage example of
  SoClipPlane. It connects an SoClipPlane to an SoCenterballDragger,
  for end-user control over the orientation and position of the
  clipping plane:

  \verbatim
  #Inventor V2.1 ascii
  
  Separator {
     Separator {
        Translation { translation -6 0 0 }
        DEF cbdragger CenterballDragger { }
     }
  
     TransformSeparator {
        Transform {
           rotation 0 0 1 0 = USE cbdragger . rotation
           translation 0 0 0 = USE cbdragger . center
        }
        ClipPlane { }
     }
  
     Sphere { }
  }
  \endverbatim

  Note that SoClipPlane is a state-changing appearance node, and as
  such, it will only assert its effects under the current SoSeparator
  node (as the SoSeparator pops the state stack when traversal returns
  above it), as can be witnessed by loading this simple example file
  into a Coin viewer:

  \verbatim
  #Inventor V2.1 ascii
  
  Separator {
     ClipPlane { }
     Cube { }
  }
  
  Separator {
     Translation { translation -3 0 0 }
     Cube { }
  }
  \endverbatim


  <b>FILE FORMAT/DEFAULTS:</b>
  \code
    ClipPlane {
        plane 1 0 0  0
        on TRUE
    }
  \endcode
*/

// *************************************************************************

#include <Inventor/nodes/SoClipPlane.h>

#include <Inventor/actions/SoCallbackAction.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/actions/SoPickAction.h>
#include <Inventor/elements/SoGLClipPlaneElement.h>
#include <Inventor/elements/SoCullElement.h>
#include <Inventor/elements/SoModelMatrixElement.h>

#include "nodes/SoSubNodeP.h"

// *************************************************************************

/*!
  \var SoSFPlane SoClipPlane::plane

  Definition of clipping plane.  Geometry on the back side of the
  clipping plane is clipped away.

  The default clipping plane has its normal pointing in the <1,0,0>
  direction, and intersects origo. (I.e., everything along the
  negative X-axis is clipped.)
*/
/*!
  \var SoSFBool SoClipPlane::on
  Whether clipping plane should be on or off. Defaults to \c TRUE.
*/


// *************************************************************************

SO_NODE_SOURCE(SoClipPlane);

/*!
  Constructor.
*/
SoClipPlane::SoClipPlane(void)
{
  SO_NODE_INTERNAL_CONSTRUCTOR(SoClipPlane);

  SO_NODE_ADD_FIELD(plane, (SbPlane(SbVec3f(1.0f, 0.0f, 0.0f), 0.0f)));
  SO_NODE_ADD_FIELD(on, (TRUE));
}

/*!
  Destructor.
*/
SoClipPlane::~SoClipPlane()
{
}

/*!
  \copybrief SoBase::initClass(void)
*/
void
SoClipPlane::initClass(void)
{
  SO_NODE_INTERNAL_INIT_CLASS(SoClipPlane, SO_FROM_INVENTOR_1);

  SO_ENABLE(SoGLRenderAction, SoGLClipPlaneElement);
  SO_ENABLE(SoPickAction, SoClipPlaneElement);
  SO_ENABLE(SoCallbackAction, SoClipPlaneElement);
}

// Doc from superclass.
void
SoClipPlane::doAction(SoAction * action)
{
  if (this->on.isIgnored() || this->on.getValue()) {
    SoClipPlaneElement::add(action->getState(), this, plane.getValue());
  }
}

// Doc from superclass.
void
SoClipPlane::GLRender(SoGLRenderAction * action)
{
  SoClipPlane::doAction(action);
  if (this->on.isIgnored() || this->on.getValue()) {
    SbPlane p(SbVec3f(1.0f, 0.0f, 0.0f), 0.0f);
    if (!this->plane.isIgnored()) p = this->plane.getValue();
    p.transform(SoModelMatrixElement::get(action->getState()));
    SoCullElement::addPlane(action->getState(), p);
  }
}

// Doc from superclass.
void
SoClipPlane::callback(SoCallbackAction * action)
{
  SoClipPlane::doAction(action);
}

// Doc from superclass.
void
SoClipPlane::pick(SoPickAction * action)
{
  SoClipPlane::doAction(action);
}

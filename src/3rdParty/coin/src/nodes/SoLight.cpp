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
  \class SoLight SoLight.h Inventor/nodes/SoLight.h
  \brief The SoLight class is the base class for light emitting nodes.

  \ingroup coin_nodes

  This node type is abstract and does not in itself provide any light
  sources to the scene, you need to use one of its subclasses.

  There are a few important things to know about light sources in
  Coin. First of all, the more light sources you have in your scene,
  the slower the rendering will be. The impact on rendering speed is
  highly dependent on the graphics hardware and/or rendering subsystem
  software implementation (i.e. how optimized an OpenGL you or your
  users are running), but it could be severe.

  Another issue which is important to know is that OpenGL rendering
  engines usually have a fixed maximum number of available light
  sources which can be present in the state at the same time. If you
  reach the maximum number, further light sources will simply be
  ignored. The maximum number of light sources for OpenGL rendering
  can be found by using:

  \code
      #include <Inventor/elements/SoGLLightIdElement.h>
      #include <Inventor/nodes/SoSubNodeP.h>
      // ...[snip]...
      int nrlights = SoGLLightIdElement::getMaxGLSources();
  \endcode

  If you are clever with how you use light sources, you can get away
  with using a lot more lights in a scene graph than the max available
  from the rendering system. This is because light sources are stacked
  on the traversal state, just like other appearance data. So if you
  put light sources under SoSeparator nodes, they will be popped off
  and "forgotten" for the remaining geometry of the scene graph after
  the subgraph below an SoSeparator has been traversed.
*/

#include <Inventor/nodes/SoLight.h>

#include <Inventor/actions/SoCallbackAction.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/elements/SoGLLightIdElement.h>
#include <Inventor/elements/SoLightAttenuationElement.h>
#include <Inventor/elements/SoLightElement.h>
#include <Inventor/elements/SoModelMatrixElement.h>
#include <Inventor/elements/SoViewingMatrixElement.h>

#include "nodes/SoSubNodeP.h"

/*!
  \var SoSFBool SoLight::on

  Whether light source should be on or off. The on-flag defaults to \c
  TRUE.
*/
/*!
  \var SoSFFloat SoLight::intensity

  Intensity of light source. This decides how much the light source
  should affect the colors etc of the scene geometry. Valid range is
  0.0 (none) to 1.0 (maximum). Default value is 1.0.
*/
/*!
  \var SoSFColor SoLight::color

  Color of light source. Default is an all-white light source.
*/


SO_NODE_ABSTRACT_SOURCE(SoLight);

/*!
  Constructor.
*/
SoLight::SoLight(void)
{
  SO_NODE_INTERNAL_CONSTRUCTOR(SoLight);

  SO_NODE_ADD_FIELD(on, (TRUE));
  SO_NODE_ADD_FIELD(intensity, (1.0f));
  SO_NODE_ADD_FIELD(color, (SbColor(1.0f, 1.0f, 1.0f)));
}

/*!
  Destructor.
*/
SoLight::~SoLight()
{
}

/*!
  \copybrief SoBase::initClass(void)
*/
void
SoLight::initClass(void)
{
  SO_NODE_INTERNAL_INIT_ABSTRACT_CLASS(SoLight, SO_FROM_INVENTOR_1);

  SO_ENABLE(SoGLRenderAction, SoLightAttenuationElement);
  SO_ENABLE(SoGLRenderAction, SoGLLightIdElement);
  SO_ENABLE(SoGLRenderAction, SoLightElement);

  SO_ENABLE(SoCallbackAction, SoLightAttenuationElement);
  SO_ENABLE(SoCallbackAction, SoLightElement);
}

// Doc from superclass.
void
SoLight::callback(SoCallbackAction * action)
{
  SoState * state = action->getState();
  SoLightElement::add(state, this, (SoModelMatrixElement::get(state) * 
                                    SoViewingMatrixElement::get(state)));
}

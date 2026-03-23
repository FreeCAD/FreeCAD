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
  \class SoShadowDirectionalLight SoShadowDirectionalLight.h Inventor/annex/FXViz/nodes/SoShadowDirectionalLight.h
  \brief The SoShadowDirectionalLight class is a node for setting up a directional light which casts shadows.

  Directional lights usually affect everything, but since it is not
  always feasible to use one shadow map for the entire scene
  graph. This node has some extra features to work around this.
  
  It calculates the intersection between the current view volume and
  either the scene bounding box, or the bounding box provided in this
  node.  The shadows are only calculated for this volume. This means
  that you'll get more detailed shadows as you zoom in on items in the
  scene graph.

  In addition, you can set the maximum distance from the camera which
  will be shaded with shadows. Think of this a new far plane for the
  camera which only affects shadows.

  As with SoShadowSpotLight, it's possible to optimize further by
  setting your own shadow caster scene graph in the shadowMapScene
  field.

  The example scene graph below demonstrates how you can use this node
  to create shadows on a large number of objects, and still get decent
  precision when zooming in. To further reduce the volume covered by
  the shadow map, you can set \a maxShadowDistance to some number > 0.
  This is the distance from the camera where shadows will be visible.

  \code

  DirectionalLight {
     direction 0 0 -1
     intensity 0.2
  }

  ShadowGroup {
    quality 1 # to get per pixel lighting
    precision 1

    ShadowDirectionalLight {
      direction 1 1 -1
      intensity 0.8
      # enable this to reduce the shadow view distance
      # maxShadowDistance 200
    }

    # 900 cubes spaced out over a fairly large area
    Array {
      origin CENTER
      numElements1 30
      numElements2 30
      numElements3 1
      separation1 20 0 0
      separation2 0 20 0
      separation3 0 0 0

      Material { diffuseColor 1 0 0 specularColor 1 1 1 }
      Cube { width 4 height 4 depth 4 }
    }

    # a chess board
    Coordinate3 { point [ -400 -400 -3, 400 -400 -3, 400 400 -3, -400 400 -3 ] }
    Material { specularColor 1 1 1 shininess 0.9 }
    Complexity { textureQuality 0.1 }
    Texture2 { image 2 2 3 0xffffff 0x225588 0x225588 0xffffff }
    Texture2Transform { scaleFactor 20 20 }
    FaceSet { numVertices 4 }
  }
  \endcode

  \since Coin 3.0
*/

/*!
  \var SoSFNode SoShadowDirectionalLight::shadowMapScene

  The shadow map scene graph. If this is NULL (the default), the node
  will behave as a normal SoDirectionalLight node.

*/

/*!
  \var SoSFFloat SoShadowDirectionalLight::maxShadowDistance

  The maximum distance (from the camera) that we'll see shadows from this light source.
*/

/*!
  \var SoSFVec3f SoShadowDirectionalLight::bboxCenter

  Can be used to specify the volume that should be used for
  calculating the resulting shadow volume.
*/

/*!
  \var SoSFVec3f SoShadowDirectionalLight::bboxSize

  Can be used to specify the volume that should be used for
  calculating the resulting shadow volume.
*/

// *************************************************************************

#include <Inventor/annex/FXViz/nodes/SoShadowDirectionalLight.h>

#include <cstdio>
#include <Inventor/actions/SoGLRenderAction.h>

#include "nodes/SoSubNodeP.h"

// *************************************************************************


SO_NODE_SOURCE(SoShadowDirectionalLight);

/*!
  Constructor.
*/
SoShadowDirectionalLight::SoShadowDirectionalLight(void)
{
  SO_NODE_INTERNAL_CONSTRUCTOR(SoShadowDirectionalLight);
  SO_NODE_ADD_FIELD(shadowMapScene, (NULL));
  SO_NODE_ADD_FIELD(maxShadowDistance, (-1.0f));
  SO_NODE_ADD_FIELD(bboxCenter, (0.0f, 0.0f, 0.0f));
  SO_NODE_ADD_FIELD(bboxSize, (-1.0f, -1.0f, -1.0f));
}

/*!
  Destructor.
*/
SoShadowDirectionalLight::~SoShadowDirectionalLight()
{
}

/*!
  \copydetails SoNode::initClass(void)
*/
void
SoShadowDirectionalLight::initClass(void)
{
  SO_NODE_INTERNAL_INIT_CLASS(SoShadowDirectionalLight, SO_FROM_COIN_4_0);
}

// Doc from superclass.
void
SoShadowDirectionalLight::GLRender(SoGLRenderAction * action)
{
  inherited::GLRender(action);
}

#ifdef COIN_TEST_SUITE

BOOST_AUTO_TEST_CASE(initialized)
{
  SoShadowDirectionalLight * node = new SoShadowDirectionalLight;
  assert(node);
  node->ref();
  BOOST_CHECK_MESSAGE(node->getTypeId() != SoType::badType(),
                      "missing class initialization");
  node->unref();
}

#endif // COIN_TEST_SUITE

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
  \class SoTextureScalePolicy SoTextureScalePolicy.h Inventor/nodes/SoTextureScalePolicy.h
  \brief The SoTextureScalePolicy class is a node for controlling the texture scale policy.

  \ingroup coin_nodes

  If a texture map is of size != 2^n, it must be scaled before OpenGL
  can handle it.  This node enables you to control how/if textures are
  scaled before they are sent to OpenGL.

  Also, if a texture map is bigger than the maximum OpenGL texture
  size (implementation and context dependent), it will be scaled down
  to the maximum size. You can avoid this by setting the texture
  policy to SoTextureScalePolicy::FRACTURE, in which case the texture
  will be split into several small subtextures before the geometry
  using the texture is rendered.

  Setting SoTextureScalePolicy::policy to
  SoTextureScalePolicy::FRACTURE will also cause the internal texture
  handling unit in Coin to automatically downsample the individual
  subtextures to not use more graphics card memory than necessary to
  cover the current screen size of the texture.

  These two aspects of SoTextureScalePolicy::FRACTURE rendering
  together, subtexture fracturing and automatic downsampling, makes it
  possible to have textures with almost unlimited size. The only real
  limit is the amount of memory on the system, since the entire
  texture must fit into CPU memory.


  The SoTextureScalePolicy::FRACTURE policy is also very handy for
  using the Coin libraries built-in handling of non-power-of-2
  textures. This will then be done completely transparent to the
  application programmer, for maximum convenience. Below is a very
  simple example which demonstrates how to use it. The texture has
  dimensions 3x3, but no scaling (and thereby interpolation) will have
  to be done when SoTextureScalePolicy::FRACTURE is specified:

  \verbatim
  #Inventor V2.1 ascii
  
  Separator {
     TextureScalePolicy { policy FRACTURE }
     Complexity { textureQuality 0.01 }  # don't generate smoothed mipmaps
     Texture2 { 
        image 3 3 4  # dimensions 3x3, RGBA (4-component) image
        0xff0000ff 0x00ff00ff 0x0000ffff  # red, green, blue
        0xffff00ff 0xff00ffff 0x00ffffff  # yellow, magenta, cyan
        0x222222ff 0x777777ff 0xccccccff  # dark, medium and light gray
     }
     Cube { }
  }
  \endverbatim

  Be aware that the triangle throughput is much slower when using the
  FRACTURE texture mode, since all triangles need to be clipped (using
  the CPU) against subtextures. It's therefore usually not a good idea
  to use the FRACTURE mode on large triangle meshes.

  \COIN_CLASS_EXTENSION

  <b>FILE FORMAT/DEFAULTS:</b>
  \code
    TextureScalePolicy {
        policy USE_TEXTURE_QUALITY
        quality 0.5
    }
  \endcode

  \since Coin 2.0
*/

// *************************************************************************

#include <Inventor/nodes/SoTextureScalePolicy.h>

#include <Inventor/actions/SoGLRenderAction.h>

#include "nodes/SoSubNodeP.h"
#include "elements/SoTextureScaleQualityElement.h"
#include "elements/SoTextureScalePolicyElement.h"

/*!
  \enum SoTextureScalePolicy::Policy

  Enumerates the available policy settings.
*/

/*!
  \var SoTextureScalePolicy::Policy SoTextureScalePolicy::USE_TEXTURE_QUALITY

  Uses the texture quality to decide whether to scale up or down.
*/

/*!
  \var SoTextureScalePolicy::Policy SoTextureScalePolicy::SCALE_DOWN

  Always scales down.
*/

/*!
  \var SoTextureScalePolicy::Policy SoTextureScalePolicy::SCALE_UP

  Always scales up.
*/

/*!
  \var SoTextureScalePolicy::Policy SoTextureScalePolicy::FRACTURE

  Splits the texture into several subtextures, and clips the geometry
  into each subtexture. Also automatically downsamples the subtextures
  to not use more graphics card memory than necessary versus the
  current screen size of the texture.

  These two features makes it possible to have textures with almost
  unlimited size. The only real limit is the amount of memory on the
  system, since the entire texture must fit into CPU memory.

  Be aware that the rendering is quite slow with this mode if the
  texture(s) will be mapped onto lots of polygon primitives.
*/

/*!
  \var SoSFEnum SoTextureScalePolicy::policy

  The policy setting. Default value is USE_TEXTURE_QUALITY.

  USE_TEXTURE_QUALITY means that SoComplexity::textureQuality will be
  used to decide if the texture should be scaled up or down.
  SoComplexity::textureQuality >= 0.7 means scale up, while < 0.7
  means scale down. Textures smaller than 256 pixels are never scaled
  down since you lose too much information.
*/

/*!
  \var SoSFFloat SoTextureScalePolicy::quality
  
  The texture scale/resize quality. Default value is 0.5.

  This field can be used to force Coin to use a lower quality (but
  much faster) image resize function.  Currently, if you set this
  field to a value < 0.5, a low quality resize function will be used,
  otherwise a high quality (but slow) function will be used.
*/

// *************************************************************************

SO_NODE_SOURCE(SoTextureScalePolicy);

/*!
  Constructor.
*/
SoTextureScalePolicy::SoTextureScalePolicy(void)
{
  SO_NODE_INTERNAL_CONSTRUCTOR(SoTextureScalePolicy);
  SO_NODE_ADD_FIELD(policy, (SoTextureScalePolicy::USE_TEXTURE_QUALITY));
  SO_NODE_ADD_FIELD(quality, (0.5f));
  
  SO_NODE_DEFINE_ENUM_VALUE(Policy, USE_TEXTURE_QUALITY);
  SO_NODE_DEFINE_ENUM_VALUE(Policy, SCALE_DOWN);
  SO_NODE_DEFINE_ENUM_VALUE(Policy, SCALE_UP);
  SO_NODE_DEFINE_ENUM_VALUE(Policy, FRACTURE);
  SO_NODE_SET_SF_ENUM_TYPE(policy, Policy);
}

/*!
  Destructor.
*/
SoTextureScalePolicy::~SoTextureScalePolicy()
{
}

/*!
  \copybrief SoBase::initClass(void)
*/
void
SoTextureScalePolicy::initClass(void)
{
  SO_NODE_INTERNAL_INIT_CLASS(SoTextureScalePolicy, SO_FROM_COIN_2_0);
  SO_ENABLE(SoGLRenderAction, SoTextureScalePolicyElement);
  SO_ENABLE(SoGLRenderAction, SoTextureScaleQualityElement);
}

static SoTextureScalePolicyElement::Policy
convert_policy(const SoTextureScalePolicy::Policy policy)
{
  switch (policy) {
  default:
    assert(0 && "unknown policy");
  case SoTextureScalePolicy::USE_TEXTURE_QUALITY:
    return SoTextureScalePolicyElement::USE_TEXTURE_QUALITY;
    break;
  case SoTextureScalePolicy::SCALE_DOWN:
    return SoTextureScalePolicyElement::SCALE_DOWN;
    break;
  case SoTextureScalePolicy::SCALE_UP:
    return SoTextureScalePolicyElement::SCALE_UP;
    break;
  case SoTextureScalePolicy::FRACTURE:
    return SoTextureScalePolicyElement::FRACTURE;
    break;
  }
  // needed for gcc 4.0.1 (Mac OS X)
  return SoTextureScalePolicyElement::USE_TEXTURE_QUALITY;
}

// Doc from superclass.
void
SoTextureScalePolicy::GLRender(SoGLRenderAction * action)
{
  if (!this->policy.isIgnored()) {
    SoTextureScalePolicyElement::set(action->getState(), this, 
                                     convert_policy((Policy)this->policy.getValue()));
  }
  if (!this->quality.isIgnored()) {
    SoTextureScaleQualityElement::set(action->getState(), this,
                                      this->quality.getValue());
  }
}

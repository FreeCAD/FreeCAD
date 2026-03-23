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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H

#ifdef HAVE_VRML97

/*!
  \class SoVRMLTexture SoVRMLTexture.h Inventor/VRMLnodes/SoVRMLTexture.h
  \brief The SoVRMLTexture class is a superclass for VRML texture nodes.
*/

/*!
  \var SoSFBool SoVRMLTexture::repeatS
  TRUE if texture should be repeated in the S direction. Default is TRUE.
*/

/*!
  \var SoSFBool SoVRMLTexture::repeatT
  TRUE if texture should be repeated in the T direction. Default is TRUE.
*/

#include <Inventor/VRMLnodes/SoVRMLTexture.h>
#include "coindefs.h"

#include <Inventor/VRMLnodes/SoVRMLMacros.h>

#include "nodes/SoSubNodeP.h"

SO_NODE_ABSTRACT_SOURCE(SoVRMLTexture);

/*!
  \copydetails SoNode::initClass(void)
*/
void
SoVRMLTexture::initClass(void)
{
  SO_NODE_INTERNAL_INIT_ABSTRACT_CLASS(SoVRMLTexture, SO_VRML97_NODE_TYPE);
}

/*!
  Constructor.
*/
SoVRMLTexture::SoVRMLTexture(void)
{
  SO_VRMLNODE_INTERNAL_CONSTRUCTOR(SoVRMLTexture);

  SO_VRMLNODE_ADD_FIELD(repeatS, (TRUE));
  SO_VRMLNODE_ADD_FIELD(repeatT, (TRUE));
}

/*!
  Destructor.
*/
SoVRMLTexture::~SoVRMLTexture()
{
}

// Doc in parent
void
SoVRMLTexture::GLRender(SoGLRenderAction * COIN_UNUSED_ARG(action))
{
}

#endif // HAVE_VRML97

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
  \class SoMaterialBundle SoMaterialBundle.h Inventor/bundles/SoMaterialBundle.h
  \brief The SoMaterialBundle class simplifies material handling.

  \ingroup coin_bundles

  Every shape node should create (on the stack) an instance of this
  class and call sendFirst() before sending anything to GL. During
  rendering, send() should be used to send material values to GL.
*/

#include <Inventor/bundles/SoMaterialBundle.h>
#include <Inventor/elements/SoGLLazyElement.h>
#include <Inventor/misc/SoState.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H

#include <Inventor/system/gl.h>

#include "SbBasicP.h"

#include "glue/glp.h"
#include "rendering/SoGL.h"

#define FLAG_COLORONLY  0x01
#define FLAG_NVIDIA_BUG 0x02

/*!
  Constructor with \a action being the action applied to the
  geometry node.
*/
SoMaterialBundle::SoMaterialBundle(SoAction *action)
  : SoBundle(action)
{
  this->firsttime = TRUE; // other members will be set in setUpElements

  // HACK warning: The colorindex data member is used as a
  // bitmask. Needed to store some extra flags in this class. pederb,
  // 2004-09-02
  this->coloronly = 0;
  
  if (SoLazyElement::getLightModel(this->state) == SoLazyElement::BASE_COLOR) 
    this->coloronly |= FLAG_COLORONLY;

  const cc_glglue * glue = sogl_glue_instance(this->state);
  if (glue->nvidia_color_per_face_bug) {
    this->coloronly |= FLAG_NVIDIA_BUG;
  }
}

/*!
  Destructor
*/
SoMaterialBundle::~SoMaterialBundle()
{
}

/*!
  Currently not in use. It is only provided for OIV compliance.
*/
void
SoMaterialBundle::setUpMultiple(void)
{
  this->setupElements(FALSE);
}

/*!
  Sends the initial material values to GL. Must be done once in all
  geometry nodes before the rendering begins.
*/
void
SoMaterialBundle::sendFirst(void)
{
  this->setupElements(FALSE);
}

/*!
  Sends material values with index \a index to GL. Will test
  whether the current index equals \a index before sending.

  \a betweenbeginend should be \c TRUE if your program is
  between a glBegin() and glEnd() (it is illegal to change the
  polygon stipple between a glBegin() and glEnd()).
*/
void
SoMaterialBundle::send(const int index, const SbBool betweenbeginend)
{
  if (this->firsttime) this->setupElements(betweenbeginend);
  //if (index != this->currindex || (this->coloronly & FLAG_NVIDIA_BUG)) {
  // Bug fix: Force setting the color for all indices. ATI cards do not
  // use the previous color if it is reported as equal, but sets
  // it to black. - jostein 20/09/2010
    this->lazyelem->sendDiffuseByIndex(index);    
    this->currindex = index;
  //}
}

/*!
  Will send the material to GL even though \a index equals the current
  index.

  Provided for compatibility with the SGI Open Inventor v2.1 API.
*/
void
SoMaterialBundle::forceSend(const int index)
{
  if (this->firsttime) this->setupElements(FALSE);
  this->reallySend(index);
  this->currindex = index;
}

/*!
  Returns \c TRUE if the current light model is BASE_COLOR.
*/
SbBool
SoMaterialBundle::isColorOnly(void) const
{
  return (this->coloronly & FLAG_COLORONLY) != 0;
}

//
// private method. Will send needed material values to GL.
//
void
SoMaterialBundle::reallySend(const int index)
{
  this->lazyelem->sendDiffuseByIndex(index);
}

//
// private method. Stores info and element pointers.
//
void
SoMaterialBundle::setupElements(const SbBool isbetweenbeginend)
{
  this->lazyelem = static_cast<const SoGLLazyElement *>(SoLazyElement::getInstance(this->state));
  this->currindex = 0;
  
  if (isbetweenbeginend || (this->coloronly & FLAG_COLORONLY)) {
    this->lazyelem->send(this->state, SoLazyElement::DIFFUSE_ONLY_MASK); 
  }
  else {
    this->lazyelem->send(this->state, SoLazyElement::ALL_MASK); 
  }
  this->firsttime = FALSE;
}

#undef FLAG_COLORONLY
#undef FLAG_NVIDIA_BUG

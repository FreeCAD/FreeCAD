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
  \class SoPackedColorV20 SoPackedColorV20.h
  \brief The SoPackedColorV20 class is a node is for Inventor V2.0 support only.

  \ingroup coin_nodes

  \sa SoPackedColor
*/

#include "upgraders/SoPackedColorV20.h"

#include <Inventor/nodes/SoPackedColor.h>

#include "nodes/SoSubNodeP.h"

/*!
  \var SoMFUInt32 SoPackedColorV20::rgba

  Set of packed 32-bit RGBA vectors.

  The most significant 8 bits specify the transparency value, where
  0x00 means completely transparent, and 0xff completely opaque.

  The least significant 24 bits specify 8 bits each for the blue,
  green and red components.
*/

// *************************************************************************

SO_NODE_SOURCE(SoPackedColorV20);

/*!
  Constructor.
*/
SoPackedColorV20::SoPackedColorV20()
{
  SO_NODE_INTERNAL_CONSTRUCTOR(SoPackedColorV20);

  SO_NODE_ADD_FIELD(rgba, (0xffcccccc));
}

/*!
  Destructor.
*/
SoPackedColorV20::~SoPackedColorV20()
{
}

// Doc from superclass.
void
SoPackedColorV20::initClass(void)
{
  SO_NODE_INTERNAL_INIT_CLASS(SoPackedColorV20, SoNode::INVENTOR_2_0|SoNode::INVENTOR_1);
}

SoPackedColor * 
SoPackedColorV20::createUpgrade(void) const
{
  SoPackedColor * pp = new SoPackedColor;
  pp->ref();

  const int n = this->rgba.getNum();
  const uint32_t * src = this->rgba.getValues(0);
  pp->orderedRGBA.setNum(n);
  uint32_t * dst = pp->orderedRGBA.startEditing();
  for (int i = 0; i < n; i++) {
    uint32_t val = src[i];
    dst[i] = (val<<24)|((val<<8)&0xff0000)|((val>>8)&0xff00)|(val>>24);
  }
  pp->orderedRGBA.finishEditing();

  pp->unrefNoDelete();
  return pp;
}

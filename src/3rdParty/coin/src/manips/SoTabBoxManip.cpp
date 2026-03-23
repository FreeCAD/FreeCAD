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
#include "config.h"
#endif // HAVE_CONFIG_H

#ifdef HAVE_MANIPULATORS

/*!
  \class SoTabBoxManip SoTabBoxManip.h Inventor/manips/SoTabBoxManip.h
  \brief The SoTabBoxManip class wraps an SoTabBoxDragger.

  \ingroup coin_manips

  The SoTabBoxManip provides a convenient mechanism for the
  application programmer for setting up an SoTabBoxDragger in the
  scene connected to the relevant fields of an SoTransform node.

  <center>
  \image html tabbox.png "Example of TabBox Manipulator"
  </center>

  The interaction from the end-user with the manipulator will then
  automatically influence the transformation matrix for the geometry
  following it in the scene graph.
*/

#include <Inventor/manips/SoTabBoxManip.h>

#include <Inventor/nodes/SoSurroundScale.h>
#include <Inventor/draggers/SoTabBoxDragger.h>

#include "nodes/SoSubNodeP.h"

class SoTabBoxManipP {
public:
};

SO_NODE_SOURCE(SoTabBoxManip);

/*!
  \copybrief SoNode::initClass(void)
*/
void
SoTabBoxManip::initClass(void)
{
  SO_NODE_INTERNAL_INIT_CLASS(SoTabBoxManip, SO_FROM_INVENTOR_1);
}

/*!
  Constructor sets us up with an SoTabBoxDragger for manipulating a
  transformation.
*/
SoTabBoxManip::SoTabBoxManip(void)
{
  SO_NODE_INTERNAL_CONSTRUCTOR(SoTabBoxManip);

  SoTabBoxDragger *dragger = new SoTabBoxDragger;
  this->setDragger(dragger);

  SoSurroundScale * ss =
    (SoSurroundScale *)dragger->getPart("surroundScale", TRUE);
  ss->numNodesUpToContainer = 4;
  ss->numNodesUpToReset = 3;
}


/*!
  Protected destructor. (SoHandleBoxManip is automatically destructed
  when its reference count goes to 0.)
 */
SoTabBoxManip::~SoTabBoxManip()
{
}

#endif // HAVE_MANIPULATORS

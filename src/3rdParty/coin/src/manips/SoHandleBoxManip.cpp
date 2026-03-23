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
  \class SoHandleBoxManip SoHandleBoxManip.h Inventor/manips/SoHandleBoxManip.h
  \brief The SoHandleBoxManip class wraps an SoHandleBoxDragger for manipulating a transformation.

  \ingroup coin_manips

  A manipulator is used by replacing the node you want to edit in the
  graph with the manipulator. Draggers are used to manipulate the
  node. When manipulation is finished, the node is put back into the
  graph, replacing the manipulator.

  <center>
  \image html handlebox.png "Example of HandleBox Manipulator"
  </center>

  \sa SoHandleBoxDragger, SoDragger
*/
// FIXME: more class documentation? 20010909 mortene.

#include <Inventor/manips/SoHandleBoxManip.h>

#include <Inventor/nodes/SoSurroundScale.h>
#include <Inventor/draggers/SoHandleBoxDragger.h>

#include "nodes/SoSubNodeP.h"

class SoHandleBoxManipP {
public:
};

SO_NODE_SOURCE(SoHandleBoxManip);

/*!
  \copybrief SoNode::initClass(void)
*/
void
SoHandleBoxManip::initClass(void)
{
  SO_NODE_INTERNAL_INIT_CLASS(SoHandleBoxManip, SO_FROM_INVENTOR_1);
}

/*!
  Constructor sets us up with an SoHandleBoxDragger for manipulating a
  transformation.
 */
SoHandleBoxManip::SoHandleBoxManip(void)
{
  SO_NODE_INTERNAL_CONSTRUCTOR(SoHandleBoxManip);

  SoHandleBoxDragger * dragger = new SoHandleBoxDragger;
  this->setDragger(dragger);

  SoSurroundScale * ss =
    (SoSurroundScale *)dragger->getPart("surroundScale", TRUE);
  // FIXME: be robust here in case user supplied a faulty graph for
  // the dragger? Or is that taken care of by the import code in the
  // dragger? 20010909 mortene.

  ss->numNodesUpToContainer = 4;
  ss->numNodesUpToReset = 3;
}

/*!
  Protected destructor. (SoHandleBoxManip is automatically destructed
  when its reference count goes to 0.)
*/
SoHandleBoxManip::~SoHandleBoxManip()
{
}

#endif // HAVE_MANIPULATORS

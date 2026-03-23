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
  \class SoJackManip SoJackManip.h Inventor/manips/SoJackManip.h
  \brief The SoJackManip wraps an SoJackDragger for convenience.

  \ingroup coin_manips

  The manipulator class takes care of wrapping up the SoJackDragger in
  a simple and convenient API for the application programmer, making
  it automatically surround the geometry it influences and taking care
  of the book-keeping routines for its interaction with the relevant
  fields of an SoTransformation node.

  <center>
  \image html jack.png "Example of Jack Manipulator"
  </center>
*/

#include <Inventor/manips/SoJackManip.h>

#include <Inventor/nodes/SoSurroundScale.h>
#include <Inventor/draggers/SoJackDragger.h>

#include "nodes/SoSubNodeP.h"

class SoJackManipP {
public:
};

SO_NODE_SOURCE(SoJackManip);


/*!
  \copybrief SoNode::initClass(void)
*/
void
SoJackManip::initClass(void)
{
  SO_NODE_INTERNAL_INIT_CLASS(SoJackManip, SO_FROM_INVENTOR_1);
}

/*!
  Default constructor. Allocates an SoJackDragger and an
  SoSurroundScale node to surround the geometry within our part of the
  scene graph.
*/
SoJackManip::SoJackManip(void)
{
  SO_NODE_INTERNAL_CONSTRUCTOR(SoJackManip);

  SoJackDragger *dragger = new SoJackDragger;
  this->setDragger(dragger);

  SoSurroundScale *ss = (SoSurroundScale*) dragger->getPart("surroundScale", TRUE);
  assert(ss);
  ss->numNodesUpToContainer = 4;
  ss->numNodesUpToReset = 3;
}


/*!
  Destructor.
*/
SoJackManip::~SoJackManip()
{
}

#endif // HAVE_MANIPULATORS

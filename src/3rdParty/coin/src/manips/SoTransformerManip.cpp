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
  \class SoTransformerManip SoTransformerManip.h Inventor/manips/SoTransformerManip.h
  \brief The SoTransformerManip wraps an SoTransformerDragger for convenience.

  \ingroup coin_manips

  The manipulator class takes care of wrapping up the
  SoTransformerDragger in a simple and convenient API for the
  application programmer, making it automatically surround the
  geometry it influences and taking care of the book-keeping routines
  for its interaction with the relevant fields of an SoTransformation
  node.

  <center>
  \image html transformer.png "Example of Transformer Manipulator"
  </center>
*/

#include <Inventor/manips/SoTransformerManip.h>

#include <Inventor/nodes/SoSurroundScale.h>
#include <Inventor/draggers/SoTransformerDragger.h>

#if COIN_DEBUG
#include <Inventor/errors/SoDebugError.h>
#endif // COIN_DEBUG

#include "nodes/SoSubNodeP.h"

class SoTransformerManipP {
public:
};

SO_NODE_SOURCE(SoTransformerManip);


/*!
  \copybrief SoNode::initClass(void)
*/
void
SoTransformerManip::initClass(void)
{
  SO_NODE_INTERNAL_INIT_CLASS(SoTransformerManip, SO_FROM_INVENTOR_1);
}

/*!
  Default constructor. Allocates an SoTransformerDragger and an
  SoSurroundScale node to surround the geometry within our part of the
  scene graph.
*/
SoTransformerManip::SoTransformerManip(void)
{
  SO_NODE_INTERNAL_CONSTRUCTOR(SoTransformerManip);

  SoTransformerDragger *dragger = new SoTransformerDragger;
  this->setDragger(dragger);

  SoSurroundScale *ss = (SoSurroundScale*) dragger->getPart("surroundScale", TRUE);
  ss->numNodesUpToContainer = 4;
  ss->numNodesUpToReset = 3;
}

/*!
  Destructor.
*/
SoTransformerManip::~SoTransformerManip()
{
}

/*!
  Convenience function to use the
  SoTransformerDragger::isLocateHighlighting() method of the embedded
  dragger. See documentation of that method.
*/
SbBool
SoTransformerManip::isLocateHighlighting(void)
{
  SoDragger *dragger = this->getDragger();
  if (dragger && dragger->isOfType(SoTransformerDragger::getClassTypeId())) {
    return ((SoTransformerDragger*)dragger)->isLocateHighlighting();
  }
#if COIN_DEBUG
  SoDebugError::postWarning("SoTransformerManip::isLocateHighlighting",
                            "Not a valid dragger in manipulator");
#endif // debug
  return FALSE;
}

/*!
  Convenience function to use the
  SoTransformerDragger::setLocateHighlighting() method of the embedded
  dragger. See documentation of that method.
*/
void
SoTransformerManip::setLocateHighlighting(SbBool onoff)
{
  SoDragger *dragger = this->getDragger();
  if (dragger && dragger->isOfType(SoTransformerDragger::getClassTypeId())) {
    ((SoTransformerDragger*)dragger)->setLocateHighlighting(onoff);
  }
  else {
#if COIN_DEBUG
    SoDebugError::postWarning("SoTransformerManip::setLocateHighlighting",
                              "Not a valid dragger in manipulator");
#endif // debug
  }
}

/*!
  Convenience function to use the SoTransformerDragger::unsquishKnobs()
  method of the embedded dragger. See documentation of that method.
*/
void
SoTransformerManip::unsquishKnobs(void)
{
  SoDragger *dragger = this->getDragger();
  if (dragger && dragger->isOfType(SoTransformerDragger::getClassTypeId())) {
    ((SoTransformerDragger*)dragger)->unsquishKnobs();
  }
  else {
#if COIN_DEBUG
    SoDebugError::postWarning("SoTransformerManip::setLocateHighlighting",
                              "Not a valid dragger in manipulator");
#endif // debug
  }
}

#endif // HAVE_MANIPULATORS

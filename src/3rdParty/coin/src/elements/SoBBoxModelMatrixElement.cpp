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
  \class SoBBoxModelMatrixElement Inventor/elements/SoBBoxModelMatrixElement.h
  \brief The SoBBoxModelMatrixElement class keeps track of the current model
  matrix during a scene graph traversal.  It is used by amongst others the
  SoGetBoundingBoxAction class.

  \ingroup coin_elements
*/

#include <Inventor/elements/SoBBoxModelMatrixElement.h>
#include <Inventor/elements/SoLocalBBoxMatrixElement.h>

SO_ELEMENT_SOURCE(SoBBoxModelMatrixElement);

/*!
  \copydetails SoElement::initClass(void)
*/

void
SoBBoxModelMatrixElement::initClass(void)
{
  SO_ELEMENT_INIT_CLASS(SoBBoxModelMatrixElement, inherited);
}

/*!
  Destructor.
*/

SoBBoxModelMatrixElement::~SoBBoxModelMatrixElement()
{
}

//! FIXME: write doc.

void
SoBBoxModelMatrixElement::init(SoState * stateptr)
{
  inherited::init(stateptr);
  this->state = stateptr;
}

//! FIXME: write doc.
#include "SbBasicP.h"
void
SoBBoxModelMatrixElement::push(SoState * stateptr)
{
  inherited::push(stateptr);

  const SoBBoxModelMatrixElement * const prev =
    coin_assert_cast<const SoBBoxModelMatrixElement *>(this->getNextInStack());
  this->state = prev->state;
}

/*!
  This method is for the SoGetBoundingBoxAction class so it can reset the
  current model matrix and all local matrices to identity.
*/

void
SoBBoxModelMatrixElement::reset(SoState * const state,
                                SoNode * const node)
{
  SoModelMatrixElement::makeIdentity(state, node);
  SoLocalBBoxMatrixElement::resetAll(state);
}

/*!
  This method keeps two matrices up-to-date as opposed to the method it
  replaces.
*/

void
SoBBoxModelMatrixElement::pushMatrix(SoState * const state,
                                     SbMatrix &matrix,
                                     SbMatrix &localmatrix)
{
  matrix = SoModelMatrixElement::pushMatrix(state);
  localmatrix = SoLocalBBoxMatrixElement::pushMatrix(state);
}

/*!
  This method keeps two matrices up-to-date as opposed to the method it
  replaces.
*/

void
SoBBoxModelMatrixElement::popMatrix(SoState * const state,
                                    const SbMatrix & matrix,
                                    const SbMatrix & localmatrix)
{
  SoModelMatrixElement::popMatrix(state,  matrix);
  SoLocalBBoxMatrixElement::popMatrix(state, localmatrix);
}

//! FIXME: write doc.

void
SoBBoxModelMatrixElement::makeEltIdentity()
{
  inherited::makeEltIdentity();
  SoLocalBBoxMatrixElement::makeIdentity(state);
}

//! FIXME: write doc.

void
SoBBoxModelMatrixElement::setElt(const SbMatrix &matrix)
{
  inherited::setElt(matrix);
  SoLocalBBoxMatrixElement::set(state, matrix);
}

//! FIXME: write doc.

void
SoBBoxModelMatrixElement::multElt(const SbMatrix & matrix)
{
  inherited::multElt(matrix);
  SoLocalBBoxMatrixElement::mult(this->state, matrix);
}

//! FIXME: write doc.

void
SoBBoxModelMatrixElement::translateEltBy(const SbVec3f & translation)
{
  inherited::translateEltBy(translation);
  SoLocalBBoxMatrixElement::translateBy(this->state, translation);
}

//! FIXME: write doc.

void
SoBBoxModelMatrixElement::rotateEltBy(const SbRotation &rotation)
{
  inherited::rotateEltBy(rotation);
  SoLocalBBoxMatrixElement::rotateBy(this->state, rotation);
}

//! FIXME: write doc.

void
SoBBoxModelMatrixElement::scaleEltBy(const SbVec3f &scaleFactor)
{
  inherited::scaleEltBy(scaleFactor);
  SoLocalBBoxMatrixElement::scaleBy(this->state, scaleFactor);
}

/*!
  This method is for debug use only.
*/

SbMatrix
SoBBoxModelMatrixElement::pushMatrixElt()
{
  return inherited::pushMatrixElt();
}

/*!
  This method is for debug use only.
*/

void
SoBBoxModelMatrixElement::popMatrixElt(const SbMatrix & m)
{
  inherited::popMatrixElt(m);
}

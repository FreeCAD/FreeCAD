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
  \class SoViewingMatrixElement Inventor/elements/SoViewingMatrixElement.h
  \brief The SoViewingMatrixElement class stores the world-to-camera transformation.

  \ingroup coin_elements

  \sa SoModelMatrixElement
*/

#include <Inventor/elements/SoViewingMatrixElement.h>

#include "SbBasicP.h"

#include <cassert>

/*!
  \fn SoViewingMatrixElement::viewingMatrix

  FIXME: write doc.
*/

SO_ELEMENT_SOURCE(SoViewingMatrixElement);

/*!
  \copydetails SoElement::initClass(void)
*/

void
SoViewingMatrixElement::initClass(void)
{
  SO_ELEMENT_INIT_CLASS(SoViewingMatrixElement, inherited);
}

/*!
  Destructor.
*/

SoViewingMatrixElement::~SoViewingMatrixElement(void)
{
}

//! FIXME: write doc.

void
SoViewingMatrixElement::set(SoState * const state,
                            SoNode * const node,
                            const SbMatrix & viewingMatrix)
{
  SoViewingMatrixElement * element = coin_safe_cast<SoViewingMatrixElement *>
    (
     SoReplacedElement::getElement(state, classStackIndex, node)
     );
  if (element) {
    element->setElt(viewingMatrix);
  }
}

//! FIXME: write doc.

const SbMatrix &
SoViewingMatrixElement::get(SoState * const state)
{
  const SoViewingMatrixElement * element =
    coin_assert_cast<const SoViewingMatrixElement *>
    (
     SoElement::getConstElement(state, classStackIndex)
     );
  return element->viewingMatrix;
}

/*!
  Initializes the element to its default value. The default
  value is the identity matrix.
*/

void
SoViewingMatrixElement::init(SoState * state)
{
  inherited::init(state);
  viewingMatrix.makeIdentity();
#if 0 // debug
  SoDebugError::postInfo("SoViewingMatrixElement::init",
                         "matrix set to identity");
#endif // debug
}

//! FIXME: write doc.

void
SoViewingMatrixElement::setElt(const SbMatrix &matrix)
{
  this->viewingMatrix = matrix;
#if 0 // debug
  SoDebugError::postInfo("SoViewingMatrixElement::setElt",
                         "viewingmatrix:");
  matrix.print(stdout);
#endif // debug
}

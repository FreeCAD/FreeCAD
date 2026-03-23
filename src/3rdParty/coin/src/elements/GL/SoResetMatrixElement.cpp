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

#include "coindefs.h"
#include "elements/GL/SoResetMatrixElement.h"

SO_ELEMENT_SOURCE(SoResetMatrixElement);

/*!
  \copydetails SoElement::initClass(void)
*/

void
SoResetMatrixElement::initClass(void)
{
  SO_ELEMENT_INIT_CLASS(SoResetMatrixElement, inherited);
}

/*!
  Destructor.
*/
SoResetMatrixElement::~SoResetMatrixElement(void)
{
}

// doc in parent
void
SoResetMatrixElement::init(SoState * COIN_UNUSED_ARG(stateptr))
{
  this->matrix = SbMatrix::identity();
}

void
SoResetMatrixElement::set(SoState * stateptr, const SbMatrix & matrix)
{
  SoResetMatrixElement * elem = (SoResetMatrixElement*)
    SoElement::getElement(stateptr, SoResetMatrixElement::classStackIndex);
  elem->setElt(matrix);
}

const SbMatrix  &
SoResetMatrixElement::get(SoState * state)
{
  const SoResetMatrixElement * elem = (const SoResetMatrixElement*)
    SoElement::getConstElement(state, SoResetMatrixElement::classStackIndex);
  return elem->matrix;
}

void
SoResetMatrixElement::setElt(const SbMatrix & matrix_in)
{
  this->matrix = matrix_in;
}

SbBool
SoResetMatrixElement::matches(const SoElement * element) const
{
  const SoResetMatrixElement * other = (const SoResetMatrixElement*) element;
  return this->matrix == other->matrix;
}

SoElement *
SoResetMatrixElement::copyMatchInfo(void) const
{
  SoResetMatrixElement * element =
    (SoResetMatrixElement *)(getTypeId().createInstance());
  element->matrix = this->matrix;
  return element;
}

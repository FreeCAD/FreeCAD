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
  \class SoMaterialBindingElement Inventor/elements/SoMaterialBindingElement.h
  \brief The SoMaterialBindingElement class is yet to be documented.

  \ingroup coin_elements

  FIXME: write doc.
*/

#include <Inventor/elements/SoMaterialBindingElement.h>


#include <cassert>

/*!
  \fn SoMaterialBindingElement::Binding

  FIXME: write doc.
*/

SO_ELEMENT_SOURCE(SoMaterialBindingElement);

/*!
  \copydetails SoElement::initClass(void)
*/

void
SoMaterialBindingElement::initClass(void)
{
  SO_ELEMENT_INIT_CLASS(SoMaterialBindingElement, inherited);
}

/*!
  Destructor.
*/

SoMaterialBindingElement::~SoMaterialBindingElement(void)
{
}

//! FIXME: write doc.

void
SoMaterialBindingElement::set(SoState * const state,
                              SoNode * const node,
                              const Binding binding)
{
  assert(static_cast<int>(binding) >= static_cast<int>(OVERALL) &&
         static_cast<int>(binding) <= static_cast<int>(PER_VERTEX_INDEXED)
        );
  SoInt32Element::set(classStackIndex, state, node, binding);
}

/*!
  Initializes the element to its default value. The default
  value for materialBinding is SoMaterialBindingElement::DEFAULT.
*/

void
SoMaterialBindingElement::init(SoState * state)
{
  inherited::init(state);
  this->data = getDefault();
}

//! FIXME: write doc.

//$ EXPORT INLINE
void
SoMaterialBindingElement::set(SoState * const state, const Binding binding)
{
  set(state, NULL, binding);
}

//! FIXME: write doc.

//$ EXPORT INLINE
SoMaterialBindingElement::Binding
SoMaterialBindingElement::get(SoState * const state)
{
  return static_cast<Binding>(SoInt32Element::get(classStackIndex, state));
}

//! FIXME: write doc.

//$ EXPORT INLINE
SoMaterialBindingElement::Binding
SoMaterialBindingElement::getDefault()
{
  return DEFAULT;
}

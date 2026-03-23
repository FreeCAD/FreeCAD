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
  \class SoVertexAttributeBindingElement Inventor/elements/SoVertexAttributeBindingElement.h
  \brief The SoVertexAttributeBindingElement class is yet to be documented.

  \ingroup coin_elements

  FIXME: write doc.
*/

#include <Inventor/elements/SoVertexAttributeBindingElement.h>


#include <cassert>

/*!
  \fn SoVertexAttributeBindingElement::Binding

  FIXME: write doc.
*/

SO_ELEMENT_SOURCE(SoVertexAttributeBindingElement);

/*!
  \copydetails SoElement::initClass(void)
*/

void
SoVertexAttributeBindingElement::initClass(void)
{
  SO_ELEMENT_INIT_CLASS(SoVertexAttributeBindingElement, inherited);
}

/*!
  Destructor.
*/

SoVertexAttributeBindingElement::~SoVertexAttributeBindingElement(void)
{
}

//! FIXME: write doc.

void
SoVertexAttributeBindingElement::set(SoState * const state,
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
  value for the VertexAttributeBinding is SoVertexAttributeBindingElement::DEFAULT.
*/

void
SoVertexAttributeBindingElement::init(SoState * state)
{
  inherited::init(state);
  this->data = getDefault();
}

//! FIXME: write doc.

//$ EXPORT INLINE
void
SoVertexAttributeBindingElement::set(SoState * const state, const Binding binding)
{
  set(state, NULL, binding);
}

//! FIXME: write doc.

//$ EXPORT INLINE
SoVertexAttributeBindingElement::Binding
SoVertexAttributeBindingElement::get(SoState * const state)
{
  return static_cast<Binding>(SoInt32Element::get(classStackIndex, state));
}

//! FIXME: write doc.

//$ EXPORT INLINE
SoVertexAttributeBindingElement::Binding
SoVertexAttributeBindingElement::getDefault()
{
  return DEFAULT;
}

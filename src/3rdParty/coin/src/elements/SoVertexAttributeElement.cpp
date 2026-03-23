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
  \class SoVertexAttributeElement Inventor/elements/SoVertexAttributeElement.h
  \brief The SoVertexAttributeElement class is yet to be documented.

  \ingroup coin_elements

  FIXME: write doc.
*/

#include <Inventor/elements/SoVertexAttributeElement.h>

#include "SbBasicP.h"
#include "elements/SoVertexAttributeData.h"
#include "misc/SbHash.h"

#include <Inventor/elements/SoGLCacheContextElement.h>
#include <Inventor/fields/SoMFFloat.h>
#include <Inventor/errors/SoDebugError.h>
#include <Inventor/C/glue/gl.h>

class SoVertexAttributeElementP {
public:
  typedef SbHash<const char *, SoVertexAttributeData *> AttribDict;
  AttribDict attribdict;
};

#define PRIVATE(obj) ((obj)->pimpl)

SO_ELEMENT_SOURCE(SoVertexAttributeElement);

/*!
  \copydetails SoElement::initClass(void)
*/

void
SoVertexAttributeElement::initClass(void)
{
  SO_ELEMENT_INIT_CLASS(SoVertexAttributeElement, inherited);
}

// Doc from superclass

void
SoVertexAttributeElement::init(SoState * state)
{
  inherited::init(state);
  this->clearNodeIds();
}

/*!
  Destructor.
*/

SoVertexAttributeElement::~SoVertexAttributeElement()
{

}

/*!
  Overridden to copy vertex attributes and node ids.
*/
void
SoVertexAttributeElement::push(SoState * state)
{
  inherited::push(state);

  const SoVertexAttributeElement * prev =
    coin_assert_cast<SoVertexAttributeElement *>(this->getNextInStack());

  PRIVATE(this)->attribdict = PRIVATE(prev)->attribdict;
  this->copyNodeIds(prev);
}

void
SoVertexAttributeElement::add(SoState * const state,
                              SoVertexAttributeData * attribdata)
{
  SoVertexAttributeElement * thisp =
    static_cast<SoVertexAttributeElement *>(SoElement::getElement(state, classStackIndex));

  thisp->addElt(attribdata);
  thisp->addNodeId(attribdata->nodeid);
}

void
SoVertexAttributeElement::addElt(SoVertexAttributeData * attribdata)
{
  PRIVATE(this)->attribdict.put(attribdata->name.getString(), attribdata);
}

const SoVertexAttributeElement *
SoVertexAttributeElement::getInstance(SoState * const state)
{
  return coin_assert_cast<const SoVertexAttributeElement *>
    (getConstElement(state, classStackIndex));
}


unsigned int
SoVertexAttributeElement::getNumAttributes(void) const
{
  return PRIVATE(this)->attribdict.getNumElements();
}

void
SoVertexAttributeElement::applyToAttributes(AttributeApplyFunc * func, void * closure) const
{
  for(
      SoVertexAttributeElementP::AttribDict::const_iterator iter =
       PRIVATE(this)->attribdict.const_begin();
      iter!=PRIVATE(this)->attribdict.const_end();
      ++iter
      ) {
    func(iter->key,iter->obj,closure);
  }
}

#undef PRIVATE

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
  \class SoCreaseAngleElement Inventor/elements/SoCreaseAngleElement.h
  \brief The SoCreaseAngleElement class stores the crease angle during a scene graph traversal.

  \ingroup coin_elements
*/

#include <Inventor/elements/SoCreaseAngleElement.h>


#include <cassert>

SO_ELEMENT_SOURCE(SoCreaseAngleElement);

/*!
  \fn static SoType SoCreaseAngleElement::getClassTypeId(void)

  This static method returns the class type.
*/

/*!
  \fn static int SoCreaseAngleElement::getClassStackIndex(void)

  This static method returns the state stack index for the class.
*/

/*!
  This static method initializes static data for the SoCreaseAngleElement class.
*/

void
SoCreaseAngleElement::initClass(void)
{
  SO_ELEMENT_INIT_CLASS(SoCreaseAngleElement, inherited);
}

/*!
  Destructor.
*/
SoCreaseAngleElement::~SoCreaseAngleElement(void)
{
}

// doc from parent
void
SoCreaseAngleElement::init(SoState * state)
{
  inherited::init(state);

  // a negative value means that the element contains the default
  // value, which is 0.0 for Inventor 2.1 and 0.5 for VRML1.
  this->data = -1.0f;
}

/*!
  Sets the value of this element.
*/
void
SoCreaseAngleElement::set(SoState * const state, SoNode * const node,
                          const float complexity)
{
  SoFloatElement::set(classStackIndex, state, node, complexity);
}


/*!
  \overload
*/
void
SoCreaseAngleElement::set(SoState * const state, const float complexity)
{
  SoCreaseAngleElement::set(state, NULL, complexity);
}

/*!
  Returns the element value. This method can be used if you know
  that the node that is going to use the crease angle is an Inventor
  node.
*/
float
SoCreaseAngleElement::get(SoState * const state)
{
  float val = SoFloatElement::get(classStackIndex, state);
  return val < 0.0f ? SoCreaseAngleElement::getDefault() : val;
}

/*!
  Returns the element value. \a isvrml1 should be TRUE if the node
  requesting the value is a VRML1 node.

  This method is an extension versus the Open Inventor API.

  \sa SoNode::getNodeType()
*/
float
SoCreaseAngleElement::get(SoState * const state, const SbBool isvrml1)
{
  float val = SoFloatElement::get(classStackIndex, state);
  return val < 0.0f ? SoCreaseAngleElement::getDefault(isvrml1) : val;
}

/*!
  Returns the default value for Inventor scene graphs (0.0).
*/
float
SoCreaseAngleElement::getDefault(void)
{
  return 0.0f;
}

/*!
  Returns the default value for this element. \a isvrml1 should
  be TRUE if the node requesting the value is a VRML1 node.

  This method is an extension versus the Open Inventor API.

  \sa SoNode::getNodeType()
*/
float
SoCreaseAngleElement::getDefault(const SbBool isvrml1)
{
  return isvrml1 ? 0.5f : 0.0f;
}

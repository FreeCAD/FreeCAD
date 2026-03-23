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
  \class SoNormalBinding SoNormalBinding.h Inventor/nodes/SoNormalBinding.h
  \brief The SoNormalBinding class is a node for specifying normal vector bindings.

  \ingroup coin_nodes

  Use nodes of this type to specify how to map normal vectors from
  SoNormal nodes in the scene graph to shape nodes.

  <b>FILE FORMAT/DEFAULTS:</b>
  \code
    NormalBinding {
        value PER_VERTEX_INDEXED
    }
  \endcode

*/

// *************************************************************************

#include <Inventor/actions/SoCallbackAction.h>

#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/actions/SoGetPrimitiveCountAction.h>
#include <Inventor/actions/SoPickAction.h>
#include <Inventor/elements/SoNormalBindingElement.h>
#include <Inventor/elements/SoOverrideElement.h>

#include "nodes/SoSubNodeP.h"

// *************************************************************************

/*!
  \enum SoNormalBinding::Binding

  Enumeration of available types of mappings. See documentation of
  SoMaterialBinding node for explanation of the different values.

  Note that SoNormalBinding::DEFAULT and SoNormalBinding::NONE have
  been obsoleted and are both mapped to
  SoNormalBinding::PER_VERTEX_INDEXED.
*/
/*!
  \var SoNormalBinding::Binding SoNormalBinding::PER_PART

  Get a new normal from the pool of normal values for each \e part
  of the shape, where the definition of a \e part depends on the shape
  type.

  Normals are fetched from index 0 and onwards, incrementing the
  index into the normal pool by 1 for each new part in the order
  defined by the particular shape type.

  Important portability note: it was previously allowed with the SGI
  Inventor library to supply too few colors versus the number of parts
  of the subsequent shapes in the scene graph. Coloring would then
  cycle through the available colors.

  Since SGI Open Inventor v2.1, this was disallowed, though -- enough
  normals for all parts must be supplied, or behavior will be
  unspecified (likely to cause a crash, in fact).  Note that the
  &laquo;Inventor Mentor&raquo; book contains a description of the \e old, \e
  invalid behavior in Chapter 5, section "Binding Nodes" (middle of
  page 128 in the 10th printing).
*/
/*!
  \var SoNormalBinding::Binding SoNormalBinding::PER_PART_INDEXED

  Get a new normal from the pool of normal values for each \e part
  of the shape, where the definition of a \e part depends on the shape
  type.

  Normals are fetched by the index value settings of the shape.
*/
/*!
  \var SoNormalBinding::Binding SoNormalBinding::PER_FACE

  Get a new normal from the pool of normal values for each polygon
  \e face of the shape.

  Normals are fetched from index 0 and onwards, incrementing the
  index into the normal pool by 1 for each new face of the shape
  node.

  Note that individual faces after tessellation of complex shapes
  (like SoCylinder nodes) are \e not considered to be separate
  entities, and so this binding will be treated in the same manner as
  SoNormalBinding::PER_PART for those.
*/
/*!
  \var SoNormalBinding::Binding SoNormalBinding::PER_FACE_INDEXED

  Get a new normal from the pool of normal values for each polygon
  \e face of the shape.

  Normals are fetched by the index value settings of the shape.

  Note that individual faces after tessellation of complex shapes
  (like SoCylinder nodes) are \e not considered to be separate
  entities, and so this binding will be treated in the same manner as
  SoNormalBinding::PER_PART_INDEXED for those.
*/
/*!
  \var SoNormalBinding::Binding SoNormalBinding::PER_VERTEX

  Get a new normal from the pool of normal values for each
  polygon, line or point \e vertex of the shape.

  Normals are fetched from index 0 and onwards, incrementing the
  index into the normal pool by 1 for each new vertex of the shape
  node.
*/
/*!
  \var SoNormalBinding::Binding SoNormalBinding::PER_VERTEX_INDEXED

  Get a new normal from the pool of normal values for each
  polygon, line or point \e vertex of the shape.

  Normals are fetched by the index value settings of the shape.
*/
/*!
  \var SoNormalBinding::Binding SoNormalBinding::DEFAULT
  Obsolete. Don't use this value.
*/
/*!
  \var SoNormalBinding::Binding SoNormalBinding::NONE
  Obsolete. Don't use this value.
*/

/*!
  \var SoSFEnum SoNormalBinding::value

  The normal binding to use for subsequent shape nodes in the scene
  graph.
*/

// *************************************************************************

SO_NODE_SOURCE(SoNormalBinding);

/*!
  Constructor.
*/
SoNormalBinding::SoNormalBinding(void)
{
  SO_NODE_INTERNAL_CONSTRUCTOR(SoNormalBinding);

  SO_NODE_ADD_FIELD(value, (DEFAULT));

  SO_NODE_DEFINE_ENUM_VALUE(Binding, OVERALL);
  SO_NODE_DEFINE_ENUM_VALUE(Binding, PER_PART);
  SO_NODE_DEFINE_ENUM_VALUE(Binding, PER_PART_INDEXED);
  SO_NODE_DEFINE_ENUM_VALUE(Binding, PER_FACE);
  SO_NODE_DEFINE_ENUM_VALUE(Binding, PER_FACE_INDEXED);
  SO_NODE_DEFINE_ENUM_VALUE(Binding, PER_VERTEX);
  SO_NODE_DEFINE_ENUM_VALUE(Binding, PER_VERTEX_INDEXED);
  SO_NODE_DEFINE_ENUM_VALUE(Binding, DEFAULT);
  SO_NODE_DEFINE_ENUM_VALUE(Binding, NONE);

  SO_NODE_SET_SF_ENUM_TYPE(value, Binding);
}

/*!
  Destructor.
*/
SoNormalBinding::~SoNormalBinding()
{
}

/*!
  \copybrief SoBase::initClass(void)
*/
void
SoNormalBinding::initClass(void)
{
  SO_NODE_INTERNAL_INIT_CLASS(SoNormalBinding, SO_FROM_INVENTOR_1|SoNode::VRML1);

  SO_ENABLE(SoGLRenderAction, SoNormalBindingElement);
  SO_ENABLE(SoPickAction, SoNormalBindingElement);
  SO_ENABLE(SoCallbackAction, SoNormalBindingElement);
  SO_ENABLE(SoGetPrimitiveCountAction, SoNormalBindingElement);
}

// Doc from superclass.
void
SoNormalBinding::GLRender(SoGLRenderAction * action)
{
  SoNormalBinding::doAction(action);
}

// Doc from superclass.
void
SoNormalBinding::doAction(SoAction * action)
{
  SoState * state = action->getState();
  if (!this->value.isIgnored() && !SoOverrideElement::getNormalBindingOverride(state)) {
    SoNormalBindingElement::set(state, this,
                                (SoNormalBindingElement::Binding)
                                this->value.getValue());
    if (this->isOverride()) {
      SoOverrideElement::setNormalBindingOverride(state, this, TRUE);
    }
  }
}

// Doc from superclass.
void
SoNormalBinding::callback(SoCallbackAction * action)
{
  SoNormalBinding::doAction(action);
}

// Doc from superclass.
void
SoNormalBinding::pick(SoPickAction * action)
{
  SoNormalBinding::doAction(action);
}

// Doc from superclass.
void
SoNormalBinding::getPrimitiveCount(SoGetPrimitiveCountAction * action)
{
  SoNormalBinding::doAction(action);
}

SbBool
SoNormalBinding::readInstance(SoInput * in, unsigned short flags)
{
  SbBool ret = inherited::readInstance(in, flags);

  if (this->value.getValue() < 2) {
    // old OIV files might use obsolete bindings.  Change to
    // PER_VERTEX_INDEXED
    this->value = PER_VERTEX_INDEXED;
  }

  return ret;
}

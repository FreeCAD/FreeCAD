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
  \class SoTranslation SoTranslation.h Inventor/nodes/SoTranslation.h
  \brief The SoTranslation class is a node type for specifying geometry translations.

  \ingroup coin_nodes

  For simply translating some geometry in a scene graph, you can use
  this node type.

  <b>FILE FORMAT/DEFAULTS:</b>
  \code
    Translation {
        translation 0 0 0
    }
  \endcode
*/

// *************************************************************************

#include <Inventor/nodes/SoTranslation.h>

#include <Inventor/actions/SoGetMatrixAction.h>
#include <Inventor/actions/SoGetBoundingBoxAction.h>
#include <Inventor/elements/SoModelMatrixElement.h>

#include "nodes/SoSubNodeP.h"

// *************************************************************************

/*!
  \var SoSFVec3f SoTranslation::translation
  Set the X, Y and Z translation values. Defaults to <0, 0, 0>.
*/

// *************************************************************************

SO_NODE_SOURCE(SoTranslation);

/*!
  Constructor.
*/
SoTranslation::SoTranslation(void)
{
  SO_NODE_INTERNAL_CONSTRUCTOR(SoTranslation);

  SO_NODE_ADD_FIELD(translation, (0.0f, 0.0f, 0.0f));
}

/*!
  Destructor.
*/
SoTranslation::~SoTranslation()
{
}

// Doc in superclass.
/*!
  \copybrief SoBase::initClass(void)
*/
void
SoTranslation::initClass(void)
{
  SO_NODE_INTERNAL_INIT_CLASS(SoTranslation, SO_FROM_INVENTOR_1|SoNode::VRML1);
}

// Doc in superclass.
void
SoTranslation::doAction(SoAction * action)
{
  if (this->translation.getValue() != SbVec3f(0.0f, 0.0f, 0.0f)) {
    SoModelMatrixElement::translateBy(action->getState(), this,
                                      this->translation.getValue());
  }
}

// Doc in superclass.
void
SoTranslation::GLRender(SoGLRenderAction * action)
{
  SoTranslation::doAction((SoAction *)action);
}

// Doc in superclass.
void
SoTranslation::getBoundingBox(SoGetBoundingBoxAction * action)
{
  SoTranslation::doAction(action);
}

// Doc in superclass.
void
SoTranslation::callback(SoCallbackAction * action)
{
  SoTranslation::doAction((SoAction *)action);
}

// Doc in superclass.
void
SoTranslation::getMatrix(SoGetMatrixAction * action)
{
  SbVec3f v = this->translation.getValue();
  SbMatrix m;
  m.setTranslate(v);
  action->getMatrix().multLeft(m);
  m.setTranslate(-v);
  action->getInverse().multRight(m);
}

// Doc in superclass.
void
SoTranslation::pick(SoPickAction * action)
{
  SoTranslation::doAction((SoAction *)action);
}

// Doc in superclass. Overrides the traversal method in this class for
// the SoGetPrimitiveCountAction because the number of primitives can
// be different depending on scene location (and thereby distance to
// camera) if there are e.g. SoLOD nodes in the scene.
void
SoTranslation::getPrimitiveCount(SoGetPrimitiveCountAction * action)
{
  SoTranslation::doAction((SoAction *)action);
}

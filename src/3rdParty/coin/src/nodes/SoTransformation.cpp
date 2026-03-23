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
  \class SoTransformation SoTransformation.h Inventor/nodes/SoTransformation.h
  \brief The SoTransformation class is the abstract base class for transformation nodes.

  \ingroup coin_nodes

  To position and orient geometry within the 3D world space, various
  node types for transformations are used. These nodes all inherit the
  SoTransformation base class.

  Note that transformations will be accumulated through the scene
  graph, unless used under SoSeparator nodes, where the transformation
  matrix stack will be pushed and popped. Here is a short example
  demonstrating this principle:

  \verbatim
#Inventor V2.1 ascii

Separator {
   BaseColor { rgb 1 0 0 }
   Cone { }

   Translation { translation 3 0 0 }

   Separator {
      Rotation { rotation 1 0 0  1.57 }
      BaseColor { rgb 0 1 0 }
      Cone { }
   } # SoSeparator restores transformation matrix here

   Translation { translation 3 0 0 }

   Rotation { rotation 1 0 0  1.57 }
   BaseColor { rgb 0 0 1 }
   Cone { }

   Translation { translation 3 0 0 }

   # Last SoRotation was not within SoSeparator, so this next
   # SoRotation will accumulate with the previous.
   Rotation { rotation 1 0 0  1.57 }
   BaseColor { rgb 1 1 0 }
   Cone { }
}
  \endverbatim

  Which results in the following scene:

  <center>
  \image html transformaccum.png "Rendering of Example Scenegraph"
  </center>
*/


#include <Inventor/actions/SoCallbackAction.h>

#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/actions/SoGetBoundingBoxAction.h>
#include <Inventor/actions/SoGetPrimitiveCountAction.h>
#include <Inventor/actions/SoPickAction.h>
#include <Inventor/elements/SoBBoxModelMatrixElement.h>
#include <Inventor/elements/SoGLModelMatrixElement.h>
#include <Inventor/elements/SoLocalBBoxMatrixElement.h>

#include "nodes/SoSubNodeP.h"

SO_NODE_ABSTRACT_SOURCE(SoTransformation);

/*!
  Constructor.
*/
SoTransformation::SoTransformation(void)
{
  SO_NODE_INTERNAL_CONSTRUCTOR(SoTransformation);
}

/*!
  Destructor.
*/
SoTransformation::~SoTransformation()
{
}

/*!
  \copybrief SoBase::initClass(void)
*/
void
SoTransformation::initClass(void)
{
  SO_NODE_INTERNAL_INIT_ABSTRACT_CLASS(SoTransformation, SO_FROM_INVENTOR_1);

  SO_ENABLE(SoGetBoundingBoxAction, SoModelMatrixElement);
  SO_ENABLE(SoGetBoundingBoxAction, SoBBoxModelMatrixElement);
  SO_ENABLE(SoGetBoundingBoxAction, SoLocalBBoxMatrixElement);

  SO_ENABLE(SoGLRenderAction, SoGLModelMatrixElement);

  SO_ENABLE(SoPickAction, SoModelMatrixElement);

  SO_ENABLE(SoCallbackAction, SoModelMatrixElement);

  SO_ENABLE(SoGetPrimitiveCountAction, SoModelMatrixElement);
}

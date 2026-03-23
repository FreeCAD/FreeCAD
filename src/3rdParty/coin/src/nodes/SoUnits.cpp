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
  \class SoUnits SoUnits.h Inventor/nodes/SoUnits.h
  \brief The SoUnits class is a node for setting unit types.

  \ingroup coin_nodes

  Even though Coin doesn't care what units you are using in your scene
  graph \e per \e se, there's an advantage to using SoUnits nodes: you
  have a way to split your scene graph into different "conceptual"
  parts.

  When encountering SoUnit nodes, the traversal actions methods makes
  sure the following geometry is scaled accordingly.

  <b>FILE FORMAT/DEFAULTS:</b>
  \code
    Units {
        units METERS
    }
  \endcode
*/

// *************************************************************************

#include <Inventor/nodes/SoUnits.h>

#include <Inventor/actions/SoGetBoundingBoxAction.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/actions/SoPickAction.h>
#include <Inventor/actions/SoCallbackAction.h>
#include <Inventor/actions/SoGetMatrixAction.h>
#include <Inventor/actions/SoGetPrimitiveCountAction.h>

#include <Inventor/elements/SoModelMatrixElement.h>
#include <Inventor/elements/SoUnitsElement.h>

#include "nodes/SoSubNodeP.h"

// *************************************************************************

/*!
  \enum SoUnits::Units

  Enumerates the available unit settings.
*/


/*!
  \var SoSFEnum SoUnits::units

  The units which will be used for nodes following this node in the
  traversal (until the next SoUnit node, if any). Default value of the
  field is SoUnits::METERS.
*/


// *************************************************************************

static const float factors[] = {
  1.0f, // METERS
  0.01f, // CENTIMETERS
  0.001f, // MILLIMETERS
  0.000001f, // MICROMETERS
  0.000001f, // MICRONS
  0.000000001f, // NANOMETERS
  0.0000000001f, // ANGSTROMS
  1000.0f, // KILOMETERS
  0.3048f, // FEET
  0.0254f, // INCHES
  3.52777737e-4f, // POINTS
  0.9144f, // YARDS
  1609.3f, // MILES
  1852.0f, // NAUTICAL
};

// *************************************************************************

SO_NODE_SOURCE(SoUnits);

/*!
  Constructor.
*/
SoUnits::SoUnits(void)
{
  SO_NODE_INTERNAL_CONSTRUCTOR(SoUnits);

  SO_NODE_ADD_FIELD(units, (SoUnits::METERS));

  SO_NODE_DEFINE_ENUM_VALUE(Units, METERS);
  SO_NODE_DEFINE_ENUM_VALUE(Units, CENTIMETERS);
  SO_NODE_DEFINE_ENUM_VALUE(Units, MILLIMETERS);
  SO_NODE_DEFINE_ENUM_VALUE(Units, MICROMETERS);
  SO_NODE_DEFINE_ENUM_VALUE(Units, MICRONS);
  SO_NODE_DEFINE_ENUM_VALUE(Units, NANOMETERS);
  SO_NODE_DEFINE_ENUM_VALUE(Units, ANGSTROMS);
  SO_NODE_DEFINE_ENUM_VALUE(Units, KILOMETERS);
  SO_NODE_DEFINE_ENUM_VALUE(Units, FEET);
  SO_NODE_DEFINE_ENUM_VALUE(Units, INCHES);
  SO_NODE_DEFINE_ENUM_VALUE(Units, POINTS);
  SO_NODE_DEFINE_ENUM_VALUE(Units, YARDS);
  SO_NODE_DEFINE_ENUM_VALUE(Units, MILES);
  SO_NODE_DEFINE_ENUM_VALUE(Units, NAUTICAL_MILES);
  SO_NODE_SET_SF_ENUM_TYPE(units, Units);
}

/*!
  Destructor.
*/
SoUnits::~SoUnits()
{
}

/*!
  \copybrief SoBase::initClass(void)
*/
void
SoUnits::initClass(void)
{
  SO_NODE_INTERNAL_INIT_CLASS(SoUnits, SO_FROM_INVENTOR_1);

  SO_ENABLE(SoGetBoundingBoxAction, SoUnitsElement);
  SO_ENABLE(SoGetMatrixAction, SoUnitsElement);
  SO_ENABLE(SoGLRenderAction, SoUnitsElement);
  SO_ENABLE(SoPickAction, SoUnitsElement);
  SO_ENABLE(SoCallbackAction, SoUnitsElement);
  SO_ENABLE(SoGetPrimitiveCountAction, SoUnitsElement);
}

// Doc from superclass.
void
SoUnits::getBoundingBox(SoGetBoundingBoxAction * action)
{
  SoUnits::doAction((SoAction*)action);
}

// Doc from superclass.
void
SoUnits::GLRender(SoGLRenderAction * action)
{
  SoUnits::doAction((SoAction*)action);
}

// Doc from superclass.
void
SoUnits::doAction(SoAction * action)
{
  if (this->units.isIgnored()) return;
  SoState * state = action->getState();

  SoUnitsElement::Units currentunit = SoUnitsElement::get(state);

  if (currentunit != (SoUnitsElement::Units)units.getValue()) {
    SoUnitsElement::set(state,
                        (SoUnitsElement::Units)units.getValue());

    float scale = factors[units.getValue()] / factors[currentunit];
    SoModelMatrixElement::scaleBy(state, this,
                                  SbVec3f(scale, scale, scale));
  }
}

// Doc from superclass.
void
SoUnits::callback(SoCallbackAction * action)
{
  SoUnits::doAction((SoAction *)action);
}

// Doc from superclass.
void
SoUnits::getMatrix(SoGetMatrixAction * action)
{
  if (this->units.isIgnored()) return;

  SoState * state = action->getState();
  SoUnitsElement::Units currentunit = SoUnitsElement::get(state);
  if (currentunit != (SoUnitsElement::Units) this->units.getValue()) {
    SoUnitsElement::set(state,
                        (SoUnitsElement::Units)units.getValue());

    float scale = factors[(int)this->units.getValue()] / factors[(int) currentunit];
    float inv = 1.0f / scale;
    
    SbMatrix m;
    m.setScale(SbVec3f(scale, scale, scale));
    action->getMatrix().multLeft(m);
    m.setScale(SbVec3f(inv, inv, inv));
    action->getInverse().multRight(m);
  }
}

// Doc from superclass.
void
SoUnits::pick(SoPickAction * action)
{
  SoUnits::doAction((SoAction *)action);
}

// Doc from superclass.
void
SoUnits::getPrimitiveCount(SoGetPrimitiveCountAction * action)
{
  SoUnits::doAction((SoAction *)action);
}

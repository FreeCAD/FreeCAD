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
  \class SoSField SoSField.h Inventor/fields/SoSField.h
  \brief The SoSField class is the base class for fields which contains
  only a single value.

  \ingroup coin_fields

  All field types which should always contain only a single member
  value inherits this class. SoSField is an abstract class.

  You use methods setValue() and getValue() to store or fetch the
  value of single-value fields. Example:

  \code
    SoSpotLight * lightnode = new SoSpotLight;
    lightnode->on.setValue(TRUE); // The "on" field of SoSpotLight is
                                  // a single value field of type SoSFBool.
    ...
    ...
    // Change lightswitch.
    if (lightnode->on.getValue() == FALSE)
      lightnode->on = TRUE; // We can use operator = instead of setValue().
    else
      lightnode->on = FALSE;
    ...
  \endcode

  When nodes, engines or other types of field containers are written
  to file, their single-value fields are written to file in this
  format:

  \code
    containerclass {
      fieldname value
      fieldname value
      ...
    }
  \endcode

  ..like this, for instance, a SpotLight node from a scene
  graph which will be default \e off when read back from file:

  \code
    SpotLight {
      on FALSE
    }
  \endcode

  \sa SoMField
*/


// FIXME: document the SO_SFIELD_HEADER, SO_SFIELD_SOURCE and
// SO_SFIELD_INIT_CLASS macros. Also write a note (with link to an
// online bookstore?) about the Inventor Toolmaker, if the application
// programmer should need to extend Coin with new single-value
// fields. 20010913 mortene.

// *************************************************************************

#include <Inventor/fields/SoSField.h>

#include <cassert>

#include <Inventor/fields/SoSubField.h>
#include <Inventor/errors/SoDebugError.h>

#include "tidbitsp.h"

// *************************************************************************

SoType SoSField::classTypeId STATIC_SOTYPE_INIT;

// *************************************************************************

/*!
  \copydetails SoField::getClassTypeId(void)
*/
SoType
SoSField::getClassTypeId(void)
{
  return SoSField::classTypeId;
}

/*!
  The SoSField constructor is protected, as this is an abstract
  class.
*/
SoSField::SoSField(void)
{
}

/*!
  The SoSField destructor is empty, and is only defined so we could
  make it virtual.
*/
SoSField::~SoSField()
{
}

/*!
  \copydetails SoField::initClass(void)
*/
void
SoSField::initClass(void)
{
  PRIVATE_FIELD_INIT_CLASS(SoSField, "SField", inherited, NULL);
}

void
SoSField::atexit_cleanup(void)
{
  SoType::removeType(SoSField::classTypeId.getName());
  SoSField::classTypeId STATIC_SOTYPE_INIT;
}

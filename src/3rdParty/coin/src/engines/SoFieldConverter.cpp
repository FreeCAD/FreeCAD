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
  \class SoFieldConverter SoFieldConverter.h Inventor/engines/SoFieldConverter.h
  \brief The SoFieldConverter class is the abstract base class for field converters.

  \ingroup coin_engines

  When fields of different types are attempted connected, the Coin
  library tries to find a field converter class which can be inserted
  between them, acting as a filter converting values from the master
  field to values matching the type of the slave field.

  If a value type conversion is possible (like between an SoSFFloat
  field and an SoSFInt32 field, for instance, where we could do a
  simple typecast for converting the value of one to the other), an
  SoFieldConverter derived object is inserted.

  This class is the abstract base superclass which all such field
  converters needs to inherit.

  Coin comes with one built-in field converter class which takes care
  of all possible field-to-field conversions we know about. This means
  applications programmers should seldom need to care about this
  class, it will probably only be useful if you expand the Coin
  library with your own field type classes. (Doing so is considered
  advanced use of the library.)

  \sa SoDB::addConverter()
 */

#include <Inventor/engines/SoFieldConverter.h>

#include <cassert>

#include <Inventor/fields/SoField.h>
#include <Inventor/engines/SoOutputData.h>
#include <Inventor/lists/SoTypeList.h>
#include <Inventor/lists/SoEngineOutputList.h>
#if COIN_DEBUG
#include <Inventor/errors/SoDebugError.h>
#endif // COIN_DEBUG

#include "engines/SoConvertAll.h"
#include "engines/SoSubEngineP.h"
#include "coindefs.h" // COIN_OBSOLETED

/*!
  \fn SoField * SoFieldConverter::getInput(SoType type)

  Returns input field for the converter engine. Must be overridden in
  non-abstract converter engine classes.
*/
/*!
  \fn SoEngineOutput * SoFieldConverter::getOutput(SoType type)

  Returns output for the converter engine. Must be overridden in
  non-abstract converter engine classes.
*/


SO_ENGINE_ABSTRACT_SOURCE(SoFieldConverter);


/*!
  Default constructor.
*/
SoFieldConverter::SoFieldConverter(void)
{
#if COIN_DEBUG && 0 // debug
  SoDebugError::postInfo("SoFieldConverter::SoFieldConverter", "%p", this);
#endif // debug
}

/*!
  Default destructor.
*/
SoFieldConverter::~SoFieldConverter()
{
}

/*!
  \copybrief SoBase::initClass(void)
*/
void
SoFieldConverter::initClass(void)
{
  SO_ENGINE_INTERNAL_INIT_ABSTRACT_CLASS(SoFieldConverter);
  SoFieldConverter::initClasses();
}

/*!
  Initializes all field conversions. Automatically called from
  SoFieldConverter::initClass() upon initialization of Coin.
*/
void
SoFieldConverter::initClasses(void)
{
  SoConvertAll::initClass();
}

/*!
  This method is obsoleted in Coin. It should probably have been
  private in OIV.
*/
SoField *
SoFieldConverter::getConnectedInput(void)
{
  COIN_OBSOLETED();
  return NULL;
}

/*!
  Returns fields which are connected as slaves of the engine output.
*/
int
SoFieldConverter::getForwardConnections(SoFieldList & l) const
{
  SoEngineOutputList outputlist;
  int n = 0;
  (void) this->getOutputs(outputlist);

  for (int i = 0; i < outputlist.getLength(); i++) {
    n += outputlist[i]->getForwardConnections(l);
  }
  return n;
}

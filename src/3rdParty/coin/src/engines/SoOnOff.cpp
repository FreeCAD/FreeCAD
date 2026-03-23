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
  \class SoOnOff SoOnOff.h Inventor/engines/SoOnOff.h
  \brief The SoOnOff engine is a simple on/off switch.

  \ingroup coin_engines
*/

#include <Inventor/engines/SoOnOff.h>
#include <Inventor/lists/SoEngineOutputList.h>
#include <Inventor/fields/SoSFBool.h>

#include "engines/SoSubEngineP.h"

/*!
  \var SoSFTrigger SoOnOff::on

  An input signal on this trigger makes the SoOnOff::isOn output
  become \c TRUE.
*/
/*!
  \var SoSFTrigger SoOnOff::off

  An input signal on this trigger makes the SoOnOff::isOn output
  become \c FALSE.
*/
/*!
  \var SoSFTrigger SoOnOff::toggle

  An input signal on this trigger toggles the \c TRUE or \c FALSE
  value of the SoOnOff::isOn output.
*/

/*!
  \var SoEngineOutput SoOnOff::isOn
  (SoSFBool) Whether the engine is "on" or not.
*/
/*!
  \var SoEngineOutput SoOnOff::isOff
  (SoSFBool) Always the opposite value of SoOnOff::isOn.
*/

SO_ENGINE_SOURCE(SoOnOff);

/*!
  Default constructor.
*/
SoOnOff::SoOnOff(void)
{
  SO_ENGINE_INTERNAL_CONSTRUCTOR(SoOnOff);

  SO_ENGINE_ADD_INPUT(on, ());
  SO_ENGINE_ADD_INPUT(off, ());
  SO_ENGINE_ADD_INPUT(toggle, ());

  SO_ENGINE_ADD_OUTPUT(isOn, SoSFBool);
  SO_ENGINE_ADD_OUTPUT(isOff, SoSFBool);

  this->state = FALSE;
}

/*!
  \copybrief SoBase::initClass(void)
*/
void
SoOnOff::initClass(void)
{
  SO_ENGINE_INTERNAL_INIT_CLASS(SoOnOff);
}

/*!
  Destructor is protected because engines are reference counted.
*/
SoOnOff::~SoOnOff()
{
}

// Documented in superclass.
void
SoOnOff::evaluate(void)
{
  SO_ENGINE_OUTPUT(isOn, SoSFBool, setValue(this->state));
  SO_ENGINE_OUTPUT(isOff, SoSFBool, setValue(!this->state));
}

// Documented in superclass.
void
SoOnOff::inputChanged(SoField * which)
{
  if (which == &on) this->state = TRUE;
  else if (which == &off) this->state = FALSE;
  if (which == &toggle) this->state = this->state ? FALSE : TRUE;
}

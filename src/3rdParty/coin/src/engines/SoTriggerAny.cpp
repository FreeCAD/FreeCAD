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
  \class SoTriggerAny SoTriggerAny.h Inventor/engines/SoTriggerAny.h
  \brief The SoTriggerAny class is a fan-in engine for triggers.

  \ingroup coin_engines

  When any one of the input triggers are "pulsed", any field connected
  as a slave to the engine output will be notified.
*/

#include <Inventor/engines/SoTriggerAny.h>
#include <Inventor/lists/SoEngineOutputList.h>

#include "engines/SoSubEngineP.h"

/*!
  \var SoSFTrigger SoTriggerAny::input0
  Input trigger.
*/
/*!
  \var SoSFTrigger SoTriggerAny::input1
  Input trigger.
*/
/*!
  \var SoSFTrigger SoTriggerAny::input2
  Input trigger.
*/
/*!
  \var SoSFTrigger SoTriggerAny::input3
  Input trigger.
*/
/*!
  \var SoSFTrigger SoTriggerAny::input4
  Input trigger.
*/
/*!
  \var SoSFTrigger SoTriggerAny::input5
  Input trigger.
*/
/*!
  \var SoSFTrigger SoTriggerAny::input6
  Input trigger.
*/
/*!
  \var SoSFTrigger SoTriggerAny::input7
  Input trigger.
*/
/*!
  \var SoSFTrigger SoTriggerAny::input8
  Input trigger.
*/
/*!
  \var SoSFTrigger SoTriggerAny::input9
  Input trigger.
*/

/*!
  \var SoEngineOutput SoTriggerAny::output

  (SoSFTrigger) Connect to the output with the field(s) you want
  notified upon any input trigger "pulses".
*/


SO_ENGINE_SOURCE(SoTriggerAny);

/*!
  \copybrief SoBase::initClass(void)
*/
void
SoTriggerAny::initClass(void)
{
  SO_ENGINE_INTERNAL_INIT_CLASS(SoTriggerAny);
}

/*!
  Default constructor.
*/
SoTriggerAny::SoTriggerAny(void)
{
  SO_ENGINE_INTERNAL_CONSTRUCTOR(SoTriggerAny);

  SO_ENGINE_ADD_INPUT(input0, ());
  SO_ENGINE_ADD_INPUT(input1, ());
  SO_ENGINE_ADD_INPUT(input2, ());
  SO_ENGINE_ADD_INPUT(input3, ());
  SO_ENGINE_ADD_INPUT(input4, ());
  SO_ENGINE_ADD_INPUT(input5, ());
  SO_ENGINE_ADD_INPUT(input6, ());
  SO_ENGINE_ADD_INPUT(input7, ());
  SO_ENGINE_ADD_INPUT(input8, ());
  SO_ENGINE_ADD_INPUT(input9, ());

  SO_ENGINE_ADD_OUTPUT(output, SoSFTrigger);
}

/*!
  Destructor is protected because explicit destruction of engines is
  not allowed.
*/
SoTriggerAny::~SoTriggerAny()
{
}

// Documented in superclass.
void
SoTriggerAny::evaluate(void)
{
  SO_ENGINE_OUTPUT(output, SoSFTrigger, setValue());
}

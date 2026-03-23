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
  \class SoInterpolate SoInterpolate.h Inventor/engines/SoInterpolate.h
  \brief The SoInterpolate class is the base class for all interpolator engines.

  \ingroup coin_engines

  Interpolators are used to linearly interpolate between two values.

  In Coin, we've chosen to implement all interpolators in separate
  files. If you want to be OIV compatible when programming, you should
  include the SoInterpolate.h, and not the interpolator file(s) you
  need.
*/

#include <Inventor/engines/SoInterpolate.h>
#include <Inventor/lists/SoEngineOutputList.h>

#if COIN_DEBUG
#include <Inventor/errors/SoDebugError.h>
#endif // COIN_DEBUG

#include "engines/SoSubEngineP.h"

/*!
  \var SoSFFloat SoInterpolate::alpha

  The value which says how much we've should interpolate from first
  value to second value. A value equal to 0 will give an output equal
  to the first value, alpha equal to 1 gives the second value, any
  value in between gives a "weighted" interpolation between the two
  values.
*/
/*!
  \var SoEngineOutput SoInterpolate::output

  Interpolated values from the input fields. The type of the output
  will of course be the same as the type of the input fields of each
  non-abstract subclass inheriting SoInterpolate.
*/

SO_ENGINE_ABSTRACT_SOURCE(SoInterpolate);


/*!
  Default constructor.
*/
SoInterpolate::SoInterpolate(void)
{
  // Don't use standard SO_ENGINE_CONSTRUCTOR.

  // Catch attempts to use an engine class which has not been
  // initialized.
  assert(SoInterpolate::classTypeId != SoType::badType());
  // Initialize a inputdata container for the class only once.
  if (!SoInterpolate::inputdata) {
    // FIXME: is this really necessary for SoInterpolate? 20000505 mortene.
    SoInterpolate::inputdata =
      new SoFieldData(SoInterpolate::parentinputdata ?
                      *SoInterpolate::parentinputdata : NULL);
    SoInterpolate::outputdata =
      new SoEngineOutputData(SoInterpolate::parentoutputdata ?
                             *SoInterpolate::parentoutputdata : NULL);
  }

  SO_ENGINE_ADD_INPUT(alpha, (0.0f));
}

/*!
  \copybrief SoBase::initClass(void)
*/
void
SoInterpolate::initClass(void)
{
  SO_ENGINE_INTERNAL_INIT_ABSTRACT_CLASS(SoInterpolate);
}

/*!
  This method is provided only for API compatibility, and does nothing
  in Coin.
*/
void 
SoInterpolate::initClasses(void)
{
}

/*!
  Destructor.
*/
SoInterpolate::~SoInterpolate()
{
#if COIN_DEBUG && 0 // debug
  SoDebugError::postInfo("SoInterpolate::~SoInterpolate", "%p", this);
#endif // debug
  delete this->inputdata; this->inputdata = NULL;
  delete this->outputdata; this->outputdata = NULL;
}

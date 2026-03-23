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
  \class SoShuttle SoShuttle.h Inventor/nodes/SoShuttle.h
  \brief The SoShuttle class is used to oscillate between two translations.

  \ingroup coin_nodes

  A smooth transition between translation0 and translation1 is created
  using a cosine function. In the beginning of the cycle, translation0
  is used. Halfway through the cycle, the resulting translation equals
  translation1, and at the end of the cycle, we're at translation0
  again.

  <b>FILE FORMAT/DEFAULTS:</b>
  \code
    Shuttle {
        translation 0 0 0
        translation0 0 0 0
        translation1 0 0 0
        speed 1
        on TRUE
    }
  \endcode
*/

// *************************************************************************

#include <Inventor/nodes/SoShuttle.h>

#include <Inventor/SoOutput.h>
#include <Inventor/actions/SoWriteAction.h>
#include <Inventor/engines/SoCalculator.h>
#include <Inventor/engines/SoElapsedTime.h>
#include <Inventor/engines/SoInterpolateVec3f.h>

#include "nodes/SoSubNodeP.h"

// *************************************************************************

/*!
  \var SoSFVec3f SoShuttle::translation0

  Translation at the start and end of the cycle. Default value is (0,
  0, 0).
*/
/*!
  \var SoSFVec3f SoShuttle::translation1

  Translation at the middle of the cycle. Default value is (0, 0, 0).
*/
/*!
  \var SoSFFloat SoShuttle::speed
  Number of cycles per second. Default value is 1.
*/
/*!
  \var SoSFBool SoShuttle::on
  Toggles animation on or off. Defaults to \c TRUE.
*/

// *************************************************************************

SO_NODE_SOURCE(SoShuttle);

// *************************************************************************

/*!
  Constructor.
*/
SoShuttle::SoShuttle(void)
{
  SO_NODE_INTERNAL_CONSTRUCTOR(SoShuttle);

  SO_NODE_ADD_FIELD(translation0, (SbVec3f(0.0f, 0.0f, 0.0f)));
  SO_NODE_ADD_FIELD(translation1, (SbVec3f(0.0f, 0.0f, 0.0f)));
  SO_NODE_ADD_FIELD(speed, (1.0f));
  SO_NODE_ADD_FIELD(on, (TRUE));

  this->interpolator = new SoInterpolateVec3f;
  this->interpolator->ref();
  this->calculator = new SoCalculator;
  this->calculator->ref();
  this->timer = new SoElapsedTime;
  this->timer->ref();

  this->calculator->expression = "oa = (1.0 - cos(a*b*2*M_PI)) * 0.5";
  this->calculator->a.connectFrom(&this->timer->timeOut);
  this->timer->on.connectFrom(&this->on);
  this->calculator->b.connectFrom(&this->speed);
  this->interpolator->input0.connectFrom(&this->translation0);
  this->interpolator->input1.connectFrom(&this->translation1);
  this->interpolator->alpha.connectFrom(&this->calculator->oa);

  this->translation.connectFrom(&this->interpolator->output, TRUE);
}

/*!
  Destructor.
*/
SoShuttle::~SoShuttle()
{
  this->interpolator->unref();
  this->calculator->unref();
  this->timer->unref();
}

/*!
  \copybrief SoBase::initClass(void)
*/
void
SoShuttle::initClass(void)
{
  SO_NODE_INTERNAL_INIT_CLASS(SoShuttle, SO_FROM_INVENTOR_1);
}

// Documented in superclass.
void
SoShuttle::write(SoWriteAction * action)
{
  // Overridden to not write out internal engine connections.

  SoOutput * out = action->getOutput();

  // Decouple connections to/from internal engines to avoid them being
  // written. (Only done at first pass.)
  if (out->getStage() == SoOutput::COUNT_REFS)
    this->deconnectInternalEngines();

  inherited::write(action);

  // Re-enable all connections to/from internal engine. (Only done at
  // last pass.)
  if (out->getStage() == SoOutput::WRITE)
    this->reconnectInternalEngines();
}

// FIXME: I _think_ we made a mistake when overriding SoNode::copy()
// and making it virtual. See FIXME-comment above
// SoBlinker::copy(). 20011220 mortene.

// Overridden to decouple and reconnect engine around copy operation.
SoNode *
SoShuttle::copy(SbBool copyconnections) const
{
  // Decouple connections to/from internal engines to avoid them being
  // copied.
  ((SoShuttle *)this)->deconnectInternalEngines();

  SoShuttle * cp = (SoShuttle *)inherited::copy(copyconnections);

  // Re-enable all connections to/from internal engines.
  ((SoShuttle *)this)->reconnectInternalEngines();

  return cp;
}

// Remove connections to and from internal engines.
void
SoShuttle::deconnectInternalEngines(void)
{
  // Do this first, to avoid field being set due to subsequent engine
  // input value change.
  this->translation.disconnect(&this->interpolator->output);

  this->timer->on.disconnect(&this->on);
  this->timer->on = FALSE;
  this->calculator->b.disconnect(&this->speed);
  this->interpolator->input0.disconnect(&this->translation0);
  this->interpolator->input1.disconnect(&this->translation1);
}


// Re-enable all connections to/from internal engines.
void
SoShuttle::reconnectInternalEngines(void)
{
  this->timer->on.connectFrom(&this->on);
  this->calculator->b.connectFrom(&this->speed);
  this->interpolator->input0.connectFrom(&this->translation0);
  this->interpolator->input1.connectFrom(&this->translation1);

  this->translation.connectFrom(&this->interpolator->output, TRUE);
}

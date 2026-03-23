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
  \class SoBlinker SoBlinker.h Inventor/nodes/SoBlinker.h
  \brief The SoBlinker class is a cycling switch node.

  \ingroup coin_nodes

  This switch node cycles its children SoBlinker::speed number of
  times per second. If the node has only one child, it will be cycled
  on and off. Cycling can be turned off using the SoBlinker::on field,
  and the node then behaves like a normal SoSwitch node.

  <b>FILE FORMAT/DEFAULTS:</b>
  \code
    Blinker {
        whichChild -1
        speed 1
        on TRUE
    }
  \endcode
*/

// *************************************************************************

#include <Inventor/nodes/SoBlinker.h>
#include "coindefs.h"

#include <Inventor/actions/SoWriteAction.h>
#include <Inventor/sensors/SoFieldSensor.h>
#include <Inventor/engines/SoTimeCounter.h>
#include <Inventor/engines/SoCalculator.h>
#include <Inventor/misc/SoChildList.h>
#include <Inventor/SoOutput.h>
#include <Inventor/errors/SoDebugError.h>
#include <Inventor/sensors/SoOneShotSensor.h>

#include "nodes/SoSubNodeP.h"

// *************************************************************************

/*!
  \var SoSFFloat SoBlinker::speed
  Number of cycles per second.
*/
/*!
  \var SoSFBool SoBlinker::on
  Controls whether cycling is on or off.
*/


// *************************************************************************

SO_NODE_SOURCE(SoBlinker);

// *************************************************************************

#define PRIVATE(_obj_) (_obj_)->pimpl

class SoBlinkerP {
public:
  SoBlinkerP(SoBlinker * master) : master(master) { }

  static void whichChildCB(void * closure, SoSensor * COIN_UNUSED_ARG(sensor)) {
    SoBlinkerP * thisp = (SoBlinkerP*) closure;
    thisp->counter->reset.setValue(thisp->whichvalue);

    // if sensor/blinker isn't enabled, we need to manually set the whichChild field
    if (!thisp->counter->on.getValue()) {
      SbBool old = thisp->master->whichChild.enableNotify(FALSE);
      thisp->master->whichChild = thisp->whichvalue;
      thisp->master->whichChild.enableNotify(old);
    }
  }
  SoBlinker * master;
  int whichvalue;
  SoTimeCounter * counter;
  SoCalculator * calculator;
  SoOneShotSensor * whichChildSensor;
};

// *************************************************************************

/*!
  Constructor.
*/
SoBlinker::SoBlinker(void)
{
  PRIVATE(this) = new SoBlinkerP(this);

  PRIVATE(this)->calculator = new SoCalculator;
  PRIVATE(this)->calculator->ref();
  PRIVATE(this)->calculator->a.connectFrom(&this->on);
  PRIVATE(this)->calculator->b.connectFrom(&this->speed);
  PRIVATE(this)->calculator->expression = "oa = ((b > 0) && (a != 0)) ? 1.0 : 0.0;";
  
  PRIVATE(this)->counter = new SoTimeCounter;
  PRIVATE(this)->counter->ref();
  PRIVATE(this)->counter->min = SO_SWITCH_NONE;
  PRIVATE(this)->counter->max = SO_SWITCH_NONE;
  PRIVATE(this)->counter->frequency.connectFrom(&this->speed);
  PRIVATE(this)->counter->on.connectFrom(&PRIVATE(this)->calculator->oa);
  PRIVATE(this)->whichChildSensor = 
    new SoOneShotSensor(SoBlinkerP::whichChildCB, PRIVATE(this));
  PRIVATE(this)->whichChildSensor->setPriority(1);
  PRIVATE(this)->whichvalue = SO_SWITCH_NONE;


  SO_NODE_INTERNAL_CONSTRUCTOR(SoBlinker);

  SO_NODE_ADD_FIELD(speed, (1));
  SO_NODE_ADD_FIELD(on, (TRUE));
  
  this->whichChild.connectFrom(&PRIVATE(this)->counter->output, TRUE);
}

/*!
  Destructor.
*/
SoBlinker::~SoBlinker()
{
  delete PRIVATE(this)->whichChildSensor;
  PRIVATE(this)->counter->unref();
  PRIVATE(this)->calculator->unref();
  delete PRIVATE(this);
}

/*!
  \copybrief SoBase::initClass(void)
*/
void
SoBlinker::initClass(void)
{
  SO_NODE_INTERNAL_INIT_CLASS(SoBlinker, SO_FROM_INVENTOR_1);
}

// Documented in superclass. Overridden to calculate bounding box of all
// children.
void
SoBlinker::getBoundingBox(SoGetBoundingBoxAction * action)
{
  this->whichChild.getValue();
  SoGroup::getBoundingBox(action);
}

// Documented in superclass. Overridden to not write internal engine
// connections.
void
SoBlinker::write(SoWriteAction * action)
{
  SoOutput * out = action->getOutput();

  // Decouple connections to/from internal engine to avoid it being
  // written. (Only done at first pass.)
  if (out->getStage() == SoOutput::COUNT_REFS)
    this->deconnectInternalEngine();

  inherited::write(action);

  // Re-enable all connections to/from internal engine. (Only done at
  // last pass.)
  if (out->getStage() == SoOutput::WRITE)
    this->reconnectInternalEngine();
}

// Documented in superclass. Overridden to detect "external" changes
// (i.e. not caused by the internal timer engine).
void
SoBlinker::notify(SoNotList * nl)
{
  // See if the whichChild field was "manually" set.
  if (nl->getFirstRec()->getBase() == this &&
      nl->getLastField() == &this->whichChild) {
    // delay whichChild reset with the one shot sensor (to enable
    // children to be added before the reset is actually done)

    // disable connection while reading whichChild to get the actual value set
    SbBool old = this->whichChild.isConnectionEnabled();
    this->whichChild.enableConnection(FALSE);
    PRIVATE(this)->whichvalue = this->whichChild.getValue();
    this->whichChild.enableConnection(old);
    PRIVATE(this)->whichChildSensor->schedule();
  }

  // Check if a child was added or removed.
  int lastchildidx = this->getNumChildren() - 1;

  if (PRIVATE(this)->counter->max.getValue() != lastchildidx) {
    // Wrap to avoid recursive invocation.
    PRIVATE(this)->counter->enableNotify(FALSE);

    // Note that if we have one child, the counting should go from -1
    // to 0 (so the child is toggled on and off).
    PRIVATE(this)->counter->min.setValue(lastchildidx > 0 ? 0 : SO_SWITCH_NONE);
    PRIVATE(this)->counter->max.setValue(lastchildidx >= 0 ? lastchildidx : SO_SWITCH_NONE);

    // To avoid SoSwitch getting an out-of-range whichChild value, in
    // case whichChild was at the end.
    if (lastchildidx < this->whichChild.getValue()) {
      PRIVATE(this)->counter->reset.setValue(lastchildidx);
      this->whichChild.setDirty(TRUE); // Force evaluate() on the field.
    }
    PRIVATE(this)->counter->enableNotify(TRUE);
  }
  
  inherited::notify(nl);
}

// FIXME: I _think_ we made a mistake when overriding SoNode::copy()
// and making it virtual. The special handling we need below could
// just as well have been done in an overridden copyContents() method,
// which is the recommended mechanism for application programmers. But
// now I think we'll have to support the "virtual-ity" of
// SoNode::copy(), even though it's confusing to have 2 virtual
// copy-methods (both copy() and copyContents()). 20011220 mortene.

// Overridden to decouple and reconnect engine around copy operation.
SoNode *
SoBlinker::copy(SbBool copyconnections) const
{
  // Decouple connections to/from internal engine to avoid it being
  // copied.
  ((SoBlinker *)this)->deconnectInternalEngine();

  SoBlinker * cp = (SoBlinker *)inherited::copy(copyconnections);

  // Re-enable all connections to/from internal engine.
  ((SoBlinker *)this)->reconnectInternalEngine();

  // Need to set this explicitly after reconnect, as the internal
  // engine for the copy initially contains incorrect values. The
  // resulting notification on the copy also sets up correct min and
  // max values for the engine.
  cp->whichChild.setValue(this->whichChild.getValue());

  return cp;
}

// Remove connections to and from internal engine.
void
SoBlinker::deconnectInternalEngine(void)
{
  // Do this first, to avoid field being set due to subsequent engine
  // input value change.
  this->whichChild.disconnect(&PRIVATE(this)->counter->output);

  PRIVATE(this)->counter->on.disconnect(&this->on);
  PRIVATE(this)->counter->on = FALSE;
  PRIVATE(this)->counter->frequency.disconnect(&this->speed);
}


// Reset connections to and from internal engine.
void
SoBlinker::reconnectInternalEngine(void)
{
  PRIVATE(this)->counter->frequency.connectFrom(&this->speed);
  PRIVATE(this)->counter->on.connectFrom(&this->on);

  this->whichChild.connectFrom(&PRIVATE(this)->counter->output, TRUE);
}

#undef PRIVATE

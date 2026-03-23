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
  \class SoPathSensor SoPathSensor.h Inventor/sensors/SoPathSensor.h
  \brief The SoPathSensor class detects changes to paths.

  \ingroup coin_sensors

  If you need to know when a path changes (i.e. nodes in the path have
  been removed, or new nodes are added), use this sensor to get a
  notification.

  You can also use this sensor to detect when some node in the path
  is changed.

  An SoPathSensor can also act for delete-callback purposes alone and
  does not need a regular notification-based callback.  The delete
  callback will be invoked for when the SoPath instance is deleted,
  not for anything you would be monitoring in a path.
*/

/*!
  \enum SoPathSensor::TriggerFilter
  
  Trigger filter, which decides if the sensor should trigger on 
  path changes, changes on nodes in the path, or both.
*/

/*!
  \var SoPathSensor::TriggerFilter SoPathSensor::PATH

  Trigger on path changes only.
*/

/*!
  \var SoPathSensor::TriggerFilter SoPathSensor::NODES

  Trigger on node changes only. This can be nodes in the path, or
  nodes affecting the nodes in the path (nodes that updates the state
  and are left of the node in the path).
*/

/*!
  \var SoPathSensor::TriggerFilter SoPathSensor::PATH_AND_NODES

  Trigger on both path changes and node changes.
*/

#define PRIVATE(p) (p)->pimpl

#include <Inventor/sensors/SoPathSensor.h>
#include <Inventor/SoFullPath.h>
#include <Inventor/nodes/SoNode.h>

class SoPathSensorP {
public:
  SoFullPath * path; // to audit path
  SoNode * headnode; // to audit nodes in path
  SoPathSensor::TriggerFilter triggerfilter;
};


/*!
  Default constructor. Use setFunction() to set up a callback function
  later.
*/
SoPathSensor::SoPathSensor(void)
{
  this->commonConstructor();
}

/*!
  Constructor taking as parameters the sensor callback function and
  the userdata which will be passed to the callback.

  \sa setFunction(), setData()
 */
SoPathSensor::SoPathSensor(SoSensorCB * func, void * data)
  : inherited(func, data)
{
  this->commonConstructor();
}

void
SoPathSensor::commonConstructor(void)
{
  PRIVATE(this) = new SoPathSensorP;
  PRIVATE(this)->path = NULL;
  PRIVATE(this)->headnode = NULL;
  PRIVATE(this)->triggerfilter = PATH_AND_NODES;
}


/*!
  Destructor.
*/
SoPathSensor::~SoPathSensor(void)
{
  this->detach();
  delete PRIVATE(this);
}

/*!
  Attach sensor to a path. Whenever the path changes, the sensor will
  be triggered and call the callback function.

  When the SoPath instance is deleted, the sensor will automatically
  be detached.

  \sa detach()
 */
void
SoPathSensor::attach(SoPath * path)
{
  this->detach();

  PRIVATE(this)->path = (SoFullPath*) path;
  PRIVATE(this)->path->addAuditor(this, SoNotRec::SENSOR);

  PRIVATE(this)->headnode = path->getHead();
  if (PRIVATE(this)->headnode) {
    PRIVATE(this)->headnode->addAuditor(this, SoNotRec::SENSOR);
  }
}

/*!
  Detach sensor from path. As long as an SoPathSensor is detached, it
  will never invoke its callback function.

  \sa attach()
 */
void
SoPathSensor::detach(void)
{
  if (PRIVATE(this)->path) {
    PRIVATE(this)->path->removeAuditor(this, SoNotRec::SENSOR);
    PRIVATE(this)->path = NULL;
  }
  if (PRIVATE(this)->headnode) {
    PRIVATE(this)->headnode->removeAuditor(this, SoNotRec::SENSOR);
    PRIVATE(this)->headnode = NULL;
  }
  // unschedule so that we don't trigger a new callback
  if (this->isScheduled()) this->unschedule();
}

/*!
  Returns a pointer to the path connected to the sensor.

  \sa attach(), detach()
 */
SoPath *
SoPathSensor::getAttachedPath(void) const
{
  return PRIVATE(this)->path;
}

/*!
  Set the TriggerFilter for this sensor.

  The default is PATH_AND_NODES.

  \since Coin 2.0
*/
void
SoPathSensor::setTriggerFilter(const TriggerFilter filter)
{
  PRIVATE(this)->triggerfilter = filter;
}

/*!
  Return the TriggerFilter for this sensor.

  \since Coin 2.0
*/
SoPathSensor::TriggerFilter
SoPathSensor::getTriggerFilter(void) const
{
  return PRIVATE(this)->triggerfilter;
}

// Doc from superclass.
void
SoPathSensor::notify(SoNotList * l)
{
  SoBase * firstbase = l->getLastRec()->getBase();
  SoBase * lastbase = l->getFirstRec()->getBase();

  if ((lastbase != firstbase) && (lastbase == (SoBase*) PRIVATE(this)->path)) {
    // some node in/off the path was changed, wait for the
    // notification from the head node so that we only do the
    // processing/triggering once.
    return;
  }
  // if the path triggered the notification, we should always trigger
  if (firstbase == (SoBase*) (PRIVATE(this)->path)) {
    // check filter before triggering
    if (PRIVATE(this)->triggerfilter & PATH) {
      inherited::notify(l);
    }
  }
  else {
    // we came here because the root node notified us. Use
    // SoPath::isRelevantNotification() to figure out if we should
    // trigger.
    if (PRIVATE(this)->triggerfilter & NODES) {
      if (PRIVATE(this)->path->isRelevantNotification(l)) {
        inherited::notify(l);
      }
    }
  }
}

// Doc from superclass.
void
SoPathSensor::dyingReference(void)
{
  // store the attached path, and only detach if the delete callback
  // didn't attach this sensor to a new path.

  SoPath * deadpath = this->getAttachedPath();

  this->invokeDeleteCallback();

  if (this->getAttachedPath() == deadpath) {
    this->detach();
  }
}

#undef PRIVATE

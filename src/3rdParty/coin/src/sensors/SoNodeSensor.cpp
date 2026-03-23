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
  \class SoNodeSensor SoNodeSensor.h Inventor/sensors/SoNodeSensor.h
  \brief The SoNodeSensor class detects changes to nodes.

  \ingroup coin_sensors

  Attach a node to a sensor of this type to put it under surveillance,
  so you can act upon changes to the node.

  Any modification to the node's fields will trigger the sensor, as
  will changes to node's children (if any), including if nodes are
  added or removed as children below the node in the subgraph.

  An SoNodeSensor can also act for delete-callback purposes alone and
  does not need a regular notification-based callback.
*/

#include <Inventor/sensors/SoNodeSensor.h>
#include <Inventor/nodes/SoNode.h>
#include <Inventor/errors/SoDebugError.h>

/*!
  Default constructor.
*/
SoNodeSensor::SoNodeSensor(void)
{
  this->convict = NULL;
}

/*!
  Constructor taking as parameters the sensor callback function and
  the userdata which will be passed to the callback.

  \sa setFunction(), setData()
*/
SoNodeSensor::SoNodeSensor(SoSensorCB * func, void * data)
  : inherited(func, data)
{
  this->convict = NULL;
}

/*!
  Destructor.
*/
SoNodeSensor::~SoNodeSensor(void)
{
  if (this->convict) this->detach();
}

/*!
  Attach sensor to a node. Whenever any data in the node (or its
  children, if it is an SoGroup or SoGroup derived node) changes, the
  sensor will be triggered and call the callback function.

  Attaching a node sensor to a node will \e not increase the node's
  reference count (and conversely, detach()'ing the node sensor will
  not decrease the reference count, either).

  When the attached node is deleted, the sensor will be automatically
  detached().

  \sa detach()
*/
void
SoNodeSensor::attach(SoNode * node)
{
  if (this->convict != NULL) {
    this->detach();
    SoDebugError::postWarning("SoNodeSensor::attach", 
                              "Attaching node sensor that is already attached.", 
                              this);
  }
  this->convict = node;
  node->addAuditor(this, SoNotRec::SENSOR);
}

/*!
  Detach sensor from node. As long as an SoNodeSensor is detached, it
  will never call its callback function.

  \sa attach()
*/
void
SoNodeSensor::detach(void)
{
  if (this->convict) this->convict->removeAuditor(this, SoNotRec::SENSOR);
  this->convict = NULL;
}

/*!
  Returns a pointer to the node connected to the sensor.

  \sa attach(), detach()
*/
SoNode *
SoNodeSensor::getAttachedNode(void) const
{
  return this->convict;
}

// Doc from superclass.
void
SoNodeSensor::dyingReference(void)
{
  SoNode * dyingnode = this->getAttachedNode();
  this->invokeDeleteCallback();
  if (dyingnode == this->getAttachedNode()) {
    this->detach();
  }
}

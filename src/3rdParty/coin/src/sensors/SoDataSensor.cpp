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
  \class SoDataSensor SoDataSensor.h Inventor/sensors/SoDataSensor.h
  \brief The SoDataSensor class is the abstract base class for sensors monitoring changes in a scene graph.

  \ingroup coin_sensors

  If you need to know when a particular entity (as a field or a node)
  changes, subclasses of SoDataSensor can be used to monitor the
  entity and notify you when it changes.
*/

#include <Inventor/sensors/SoDataSensor.h>
#include <Inventor/SoPath.h>
#include <Inventor/misc/SoNotification.h>
#include <Inventor/nodes/SoNode.h>
#include <Inventor/fields/SoSFNode.h>

#if COIN_DEBUG
#include <Inventor/errors/SoDebugError.h>
#endif // COIN_DEBUG


/*!
  \fn void SoDataSensor::dyingReference(void)

  This method is called when the entity we are connected to is about
  to be deleted.
*/


/*!
  Default constructor.
*/
SoDataSensor::SoDataSensor(void)
  : cbfunc(NULL),
    cbdata(NULL),
    findpath(FALSE),
    triggerfield(NULL),
    triggernode(NULL),
    triggerpath(NULL),
    triggeroperationtype(SoNotRec::UNSPECIFIED),
    triggerindex(-1),
    triggerfieldnumindices(0),
    triggergroupchild(NULL),
    triggergroupprevchild(NULL)
{
}

/*!
  Constructor taking as parameters the sensor callback function and
  the userdata which will be passed to the callback.

  \sa setFunction(), setData()
*/
SoDataSensor::SoDataSensor(SoSensorCB * func, void * data)
  : inherited(func, data),
    cbfunc(NULL),
    cbdata(NULL),
    findpath(FALSE),
    triggerfield(NULL),
    triggernode(NULL),
    triggerpath(NULL),
    triggeroperationtype(SoNotRec::UNSPECIFIED),
    triggerindex(-1),
    triggerfieldnumindices(0),
    triggergroupchild(NULL),
    triggergroupprevchild(NULL)
{
}

/*!
  Destructor.
*/
SoDataSensor::~SoDataSensor(void)
{
  if (this->triggerpath) this->triggerpath->unref();
}

/*!
  If an object monitored by a data sensor is deleted, the given
  callback function will be called with the given userdata.

  The sensor priority setting does not affect the delete callback.
  It will be called immediately, before the object is deleted.
*/
void
SoDataSensor::setDeleteCallback(SoSensorCB * function, void * data)
{
  this->cbfunc = function;
  this->cbdata = data;
}

/*!
  Returns a pointer to the node causing the sensor to trigger, or \c
  NULL if there was no such node.

  \c NULL will also be returned for sensors which are not immediate
  sensors (i.e. with priority equal to 0), as the result could
  otherwise be misleading (non-immediate sensors could have been
  scheduled and rescheduled multiple times, so there wouldn't be a
  single node responsible for the sensor triggering).

  The result is only valid within the scope of a trigger(), so if you
  need to use the pointer outside your sensor callback, you must store
  it.

  \sa getTriggerField()
*/
SoNode *
SoDataSensor::getTriggerNode(void) const
{
  return this->triggernode;
}

/*!
  Returns a pointer to the field causing the sensor to trigger, or \c
  NULL if the change didn't start at a field.

  Only valid for immediate sensors (will return \c NULL otherwise),
  for the same reason as described for SoDataSensor::getTriggerNode().

  The result is only valid within the scope of a trigger(), so if you
  need to use the pointer outside your sensor callback, you must store
  it.
*/
SoField *
SoDataSensor::getTriggerField(void) const
{
  return this->triggerfield;
}

/*!
  Returns a pointer to the path from the node under the surveillance
  of this sensor (either directly or indirectly through a field
  watch) down to the node which caused the sensor to be triggered.

  Will only work for immediate mode sensors, for the same reason
  explained under getTriggerNode().

  The resulting path is only valid within the scope of trigger(), so
  if you need to use the path outside your sensor callback, you must
  store the pointer and call SoPath::ref() to avoid its destruction at
  the end of SoDataSensor::trigger().
*/
SoPath *
SoDataSensor::getTriggerPath(void) const
{
  return this->triggerpath;
}

/*!
  This flag indicates whether or not the path should be queried
  whenever a node triggers the data sensor.

  This flag is provided because finding a node path through a scene
  graph is an expensive operation.

  \sa getTriggerPathFlag(), getTriggerPath()
*/
void
SoDataSensor::setTriggerPathFlag(SbBool flag)
{
  this->findpath = flag;
}

/*!
  Returns whether or not any node induced trigger operations will make
  the sensor find the path of the node which caused it.

  \sa setTriggerPathFlag(), getTriggerPath()
*/
SbBool
SoDataSensor::getTriggerPathFlag(void) const
{
  return this->findpath;
}

/*!
  Returns the type of the scene graph operation on the node that caused
  the sensor to trigger.

  \sa getTriggerNode(), getTriggerField(), getTriggerGroupChild()
*/
SoNotRec::OperationType
SoDataSensor::getTriggerOperationType(void) const
{
  return this->triggeroperationtype;
}

/*!
  Returns the index of the child node or value in the node or
  multifield that caused the sensor to trigger.

  Please note that this method is an extension to the original SGI
  Inventor API.

  \sa getTriggerFieldNumIndices(), getTriggerGroupChild(), getTriggerNode(), getTriggerField()
*/
int
SoDataSensor::getTriggerIndex(void) const
{
  return this->triggerindex;
}

/*!
  Returns the number of indices of the multifield that caused the
  sensor to trigger.

  Please note that this method is an extension to the original SGI
  Inventor API.

  \sa getTriggerIndex(), getTriggerField()
*/
int
SoDataSensor::getTriggerFieldNumIndices(void) const
{
  return this->triggerfieldnumindices;
}

/*!
  Returns a pointer to the actual child node in the node that caused
  the sensor to trigger, or \c NULL if there was no such node.

  Please note that this method is an extension to the original SGI
  Inventor API.

  \sa getTriggerNode(), getTriggerReplacedGroupChild
*/
SoNode *
SoDataSensor::getTriggerGroupChild(void) const
{
  return this->triggergroupchild;
}

/*!
  Returns a pointer to the actual child node for a
  SoNotRec::GROUP_REPLACECHILD type of operation in the node that is
  about to be replaced and caused the sensor to trigger, or \c NULL if
  there was no such node.

  Please note that this method is an extension to the original SGI
  Inventor API.

  \sa getTriggerNode(), getTriggerGroupChild
*/
SoNode *
SoDataSensor::getTriggerReplacedGroupChild(void) const
{
  return this->triggergroupprevchild;
}

// Doc from superclass.
void
SoDataSensor::trigger(void)
{
  inherited::trigger();
  this->triggerfield = NULL;
  this->triggernode = NULL;
  if (this->triggerpath) this->triggerpath->unref();
  this->triggerpath = NULL;
  this->triggeroperationtype = SoNotRec::UNSPECIFIED;
  this->triggerindex = -1;
  this->triggerfieldnumindices = 0;
  this->triggergroupchild = NULL;
  this->triggergroupprevchild = NULL;
}

/*!
  Called from entity we are monitoring when it changes.

  If this is an immediate sensor, the field and node (if any) causing
  the change will be stored and can be fetched by getTriggerField()
  and getTriggerNode(). If the trigger path flag has been set, the path
  down to the node is also found and stored for later retrieval by
  getTriggerPath().

  \sa setTriggerPathFlag()
*/
void
SoDataSensor::notify(SoNotList * l)
{
  if (this->triggerpath != NULL) {
    this->triggerpath->unref();
    this->triggerpath = NULL;
  }
  this->triggerfield = NULL;
  this->triggernode = NULL;

  if (this->getPriority() == 0) {
    this->triggerfield = l->getLastField();
    SoNotRec * firstrecord = l->getFirstRecAtNode();
    this->triggernode = (SoNode *) (firstrecord ? firstrecord->getBase() : NULL);

    if (this->findpath && this->triggernode) {
      const SoNotRec * lastrecord = l->getLastRec();
      // find last record with node base (we know there's at least one
      // because of triggernode)
      while (!lastrecord->getBase()->isOfType(SoNode::getClassTypeId())) {
        lastrecord = lastrecord->getPrevious();
      }
      // create path down to triggernode
      this->triggerpath = new SoPath((SoNode*)lastrecord->getBase());
      this->triggerpath->ref();
      while (lastrecord->getBase() != this->triggernode) {
        lastrecord = lastrecord->getPrevious();
        this->triggerpath->append((SoNode*) lastrecord->getBase());
      }
    }

    this->triggeroperationtype = firstrecord ? firstrecord->getOperationType() : SoNotRec::UNSPECIFIED;
    this->triggerindex = firstrecord ? firstrecord->getIndex() : -1;
    this->triggerfieldnumindices = firstrecord ? firstrecord->getFieldNumIndices() : 0;
    this->triggergroupchild = (SoNode *) (firstrecord ? firstrecord->getGroupChild() : NULL);
    this->triggergroupprevchild = (SoNode *) (firstrecord ? firstrecord->getGroupPrevChild() : NULL);
  }
  this->schedule();
}

/*!
  Runs the callback set in setDeleteCallback().

  Called from subclasses when the entity we're monitoring is about to
  be deleted.
*/
void
SoDataSensor::invokeDeleteCallback(void)
{
  if (this->cbfunc) this->cbfunc(this->cbdata, this);
}

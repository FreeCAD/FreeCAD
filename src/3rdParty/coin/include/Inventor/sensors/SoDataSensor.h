#ifndef COIN_SODATASENSOR_H
#define COIN_SODATASENSOR_H

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

#include <Inventor/sensors/SoDelayQueueSensor.h>
#include <Inventor/misc/SoNotRec.h>
#include <stdlib.h> // for NULL definition

class SoNode;
class SoField;
class SoPath;
class SoNotList;

class COIN_DLL_API SoDataSensor : public SoDelayQueueSensor {
  typedef SoDelayQueueSensor inherited;

public:
  SoDataSensor(void);
  SoDataSensor(SoSensorCB * func, void * data);
  virtual ~SoDataSensor(void);

  void setDeleteCallback(SoSensorCB * function, void * data = NULL);
  SoNode * getTriggerNode(void) const;
  SoField * getTriggerField(void) const;
  SoPath * getTriggerPath(void) const;
  void setTriggerPathFlag(SbBool flag);
  SbBool getTriggerPathFlag(void) const;
  SoNotRec::OperationType getTriggerOperationType(void) const;
  int getTriggerIndex(void) const;
  int getTriggerFieldNumIndices(void) const;
  SoNode * getTriggerGroupChild(void) const;
  SoNode * getTriggerReplacedGroupChild(void) const;

  virtual void trigger(void);
  virtual void notify(SoNotList * l);
  virtual void dyingReference(void) = 0;

protected:
  void invokeDeleteCallback(void);

private:
  SoSensorCB * cbfunc;
  void * cbdata;
  SbBool findpath;
  SoField * triggerfield;
  SoNode * triggernode;
  SoPath * triggerpath;
  SoNotRec::OperationType triggeroperationtype;
  int triggerindex, triggerfieldnumindices;
  SoNode * triggergroupchild;
  SoNode * triggergroupprevchild;
};

#endif // !COIN_SODATASENSOR_H

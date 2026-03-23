#ifndef COIN_SOSENSORMANAGER_H
#define COIN_SOSENSORMANAGER_H

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

#include <Inventor/SbBasic.h>

class SoDelayQueueSensor;
class SoTimerQueueSensor;
class SoTimerSensor;
class SbTime;

class COIN_DLL_API SoSensorManager {
public:
  SoSensorManager(void);
  ~SoSensorManager();

  void insertDelaySensor(SoDelayQueueSensor * s);
  void insertTimerSensor(SoTimerQueueSensor * s);
  void removeDelaySensor(SoDelayQueueSensor * s);
  void removeTimerSensor(SoTimerQueueSensor * s);

  void setChangedCallback(void (*sensorQueueChangedCB)(void *), void * data);

  void rescheduleTimer(SoTimerSensor * s);
  void removeRescheduledTimer(SoTimerQueueSensor * s);

  void processDelayQueue(SbBool isidle);
  void processImmediateQueue(void);
  void processTimerQueue(void);

  SbBool isDelaySensorPending(void);
  SbBool isTimerSensorPending(SbTime & tm);

  void setDelaySensorTimeout(const SbTime & t);
  const SbTime & getDelaySensorTimeout(void);

  int doSelect(int nfds, void * readfds, void * writefds,
               void * exceptfds, struct timeval * userTimeOut);

private:
  void notifyChanged(void);

  int mergeTimerQueues(void);
  int mergeDelayQueues(void);
  
  class SoSensorManagerP * pimpl;
  friend class SoSensorManagerP;
};

#endif // !COIN_SOSENSORMANAGER_H

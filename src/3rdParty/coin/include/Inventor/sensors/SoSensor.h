#ifndef COIN_SOSENSOR_H
#define COIN_SOSENSOR_H

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

class SoSensor;

typedef void SoSensorCB(void * data, SoSensor * sensor);
typedef SoSensorCB * SoSensorCBPtr;

class COIN_DLL_API SoSensor {
public:
  SoSensor(void);
  SoSensor(SoSensorCB * func, void * data);
  virtual ~SoSensor(void);

  void setFunction(SoSensorCB * callbackfunction);
  SoSensorCBPtr getFunction(void) const;
  void setData(void * callbackdata);
  void * getData(void) const;

  virtual void schedule(void) = 0;
  virtual void unschedule(void) = 0;
  virtual SbBool isScheduled(void) const = 0;

  virtual void trigger(void);

  virtual SbBool isBefore(const SoSensor * s) const = 0;
  void setNextInQueue(SoSensor * next);
  SoSensor * getNextInQueue(void) const;

  static void initClass(void);

protected:
  SoSensorCB * func;
  void * funcData;
};

#endif // !COIN_SOSENSOR_H

#ifndef COIN_SOPATHSENSOR_H
#define COIN_SOPATHSENSOR_H

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

#include <Inventor/sensors/SoDataSensor.h>

class SoPathSensorP;

class COIN_DLL_API SoPathSensor : public SoDataSensor {
  typedef SoDataSensor inherited;

public:
  SoPathSensor(void);
  SoPathSensor(SoSensorCB * func, void * data);
  virtual ~SoPathSensor(void);

  enum TriggerFilter {
    PATH =           0x1,
    NODES =          0x2,
    PATH_AND_NODES = 0x3
  };

  void setTriggerFilter(const TriggerFilter type);
  TriggerFilter getTriggerFilter(void) const;

  void attach(SoPath * path);
  void detach(void);
  SoPath * getAttachedPath(void) const;

protected:
  virtual void notify(SoNotList * l);

private:
  void commonConstructor(void);
  virtual void dyingReference(void);

  SoPathSensorP * pimpl;
};

#endif // !COIN_SOPATHSENSOR_H

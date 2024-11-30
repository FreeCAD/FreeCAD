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

#include "SensorManager.h"

#include <QTimer>

#include <Inventor/SbTime.h>
#include <Inventor/SoDB.h>
#include <Inventor/SoRenderManager.h>
#include <Inventor/C/threads/thread.h>

#include "SignalThread.h"


using namespace SIM::Coin3D::Quarter;

SensorManager::SensorManager()
  : inherited()
{
  this->mainthreadid = cc_thread_id();
  this->signalthread = new SignalThread();

  QObject::connect(this->signalthread, &SignalThread::triggerSignal,
                   this, &SensorManager::sensorQueueChanged);

  this->idletimer = new QTimer;
  this->delaytimer = new QTimer;
  this->timerqueuetimer = new QTimer;

  this->idletimer->setSingleShot(true);
  this->delaytimer->setSingleShot(true);
  this->timerqueuetimer->setSingleShot(true);

  this->connect(this->idletimer, &QTimer::timeout, this, &SensorManager::idleTimeout);
  this->connect(this->delaytimer, &QTimer::timeout, this, &SensorManager::delayTimeout);
  this->connect(this->timerqueuetimer, &QTimer::timeout, this, &SensorManager::timerQueueTimeout);

  SoDB::getSensorManager()->setChangedCallback(SensorManager::sensorQueueChangedCB, this);
  this->timerEpsilon = 1.0 / 5000.0;

  SoDB::setRealTimeInterval(1.0 / 25.0);
  SoRenderManager::enableRealTimeUpdate(false);
}

SensorManager::~SensorManager()
{
  // remove the Coin callback before shutting down
  SoDB::getSensorManager()->setChangedCallback(nullptr, nullptr);

  if (this->signalthread->isRunning()) {
    this->signalthread->stopThread();
    this->signalthread->wait();
  }
  delete this->signalthread;
  delete this->idletimer;
  delete this->delaytimer;
  delete this->timerqueuetimer;
}

void
SensorManager::sensorQueueChangedCB(void * closure)
{
  SensorManager * thisp = (SensorManager * ) closure;

  // if we get a callback from another thread, route the callback
  // through SignalThread so that we receive the callback in the
  // QApplication thread (needed since QTimer isn't thread safe)
  if (cc_thread_id() != thisp->mainthreadid) {
    if (!thisp->signalthread->isRunning()) thisp->signalthread->start();
    thisp->signalthread->trigger();
  }
  else {
    thisp->sensorQueueChanged();
  }
}

void
SensorManager::sensorQueueChanged()
{
  SoSensorManager * sensormanager = SoDB::getSensorManager();
  assert(sensormanager);

  SbTime interval;
  if (sensormanager->isTimerSensorPending(interval)) {
    interval -= SbTime::getTimeOfDay();
    if (interval.getValue() < this->timerEpsilon) {
      interval.setValue(this->timerEpsilon);
    }
    if (!this->timerqueuetimer->isActive()) {
      this->timerqueuetimer->start(interval.getMsecValue());
    } else {
      this->timerqueuetimer->setInterval(interval.getMsecValue());
    }
  } else if (this->timerqueuetimer->isActive()) {
    this->timerqueuetimer->stop();
  }

  if (sensormanager->isDelaySensorPending()) {
    this->idletimer->start(0);

    if (!this->delaytimer->isActive()) {
      SbTime time = SoDB::getDelaySensorTimeout();
      if (time != SbTime::zero()) {
        this->delaytimer->start(time.getMsecValue());
      }
    }
  } else {
    if (this->idletimer->isActive()) {
      this->idletimer->stop();
    }
    if (this->delaytimer->isActive()) {
      this->delaytimer->stop();
    }
  }
}

void
SensorManager::idleTimeout()
{
  SoDB::getSensorManager()->processTimerQueue();
  SoDB::getSensorManager()->processDelayQueue(true);
  this->sensorQueueChanged();
}

void
SensorManager::timerQueueTimeout()
{
  SoDB::getSensorManager()->processTimerQueue();
  this->sensorQueueChanged();
}

void
SensorManager::delayTimeout()
{
  SoDB::getSensorManager()->processTimerQueue();
  SoDB::getSensorManager()->processDelayQueue(false);
  this->sensorQueueChanged();
}

void
SensorManager::setTimerEpsilon(double sec)
{
  this->timerEpsilon = sec;
}

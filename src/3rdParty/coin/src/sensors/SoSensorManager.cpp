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
  \class SoSensorManager SoSensorManager.h Inventor/sensors/SoSensorManager.h
  \brief The SoSensorManager class handles the sensor queues.

  \ingroup coin_sensors

  There are two major sensor types in Coin, "delay" sensors and
  "timer" sensors:

  \li Delay sensors trigger when the application is otherwise idle. In
      addition, to avoid starvation in applications that are
      continually busy, the delay sensor queue also has a timeout
      which, when reached, will empty the queue anyhow.

  \li Timer sensors are set up to trigger at specific, absolute times.

  Each of these two types has its own queue, which is handled by the
  SoSensorManager. The queues are kept in sorted order by
  SoSensorManager, either according to trigger time (for
  timer sensors) or by priority (for delay sensors).

  The SoSensorManager provides methods for managing these queues, by
  insertion and removal of sensors, and processing (emptying) of the
  queues.

  The sensor mechanism is crucial in Coin for a number of important
  features, most notably automatic scheduling redraw upon changes,
  and for making it possible to set up animations in the scene graph
  which does \e not need any additional book-keeping from the
  application code.

  SoSensorManager should usually be considered as an internal class in
  the Coin API. It is only interesting for application programmers
  when \e implementing new window system specific libraries (like
  Kongsberg Oil & Gas Technologies SoQt, SoXt, SoGtk, SoWin or Sc21)
  or wrappers. Then one has to set up code to empty the queues at the correct
  intervals. For detailed information on how to do that, we would
  advise you to look at the implementation of said mechanisms in the
  So*-libraries which SIM provides.

  Please note that before Coin 2.3.1, sensors with equal priority (or
  the same trigger time for SoTimerQueue sensors) were processed LIFO.
  This has now been changed to FIFO to be conformant to SGI Inventor.

  \sa SoSensor SoTimerQueueSensor SoDelayQueueSensor
  \sa SoTimerSensor SoAlarmSensor
  \sa SoIdleSensor SoDataSensor SoOneShotSensor
  \sa SoPathSensor SoFieldSensor SoNodeSensor
*/

// *************************************************************************

#include <Inventor/sensors/SoSensorManager.h>

#include <cassert>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H

// FIXME: find fd_set definition properly through something configure
// based.  19991214 mortene.  (Note: fd_set is in time.h under AIX?)
#ifdef HAVE_UNISTD_H
#include <unistd.h> // fd_set (?)
#endif // HAVE_UNISTD_H

#include <Inventor/sensors/SoDelayQueueSensor.h>
#include <Inventor/sensors/SoTimerSensor.h>
#include <Inventor/sensors/SoAlarmSensor.h>
#include <Inventor/lists/SbList.h>
#include <Inventor/SbTime.h>
#include <Inventor/errors/SoDebugError.h>

#ifdef COIN_THREADSAFE
#include <Inventor/threads/SbMutex.h>
#endif // COIN_THREADSAFE

#include "misc/SbHash.h"
#include "coindefs.h" // COIN_STUB()

// *************************************************************************

// Keep these around. Even though the SoSensorManager code seems to be
// working as it should now, a lot of other stuff around in the Coin
// library depends on getting the "local" sensor handling correct, and
// enabling the debuginfo in this class can help immensely. --mortene.
#define DEBUG_DELAY_SENSORHANDLING 0
#define DEBUG_TIMER_SENSORHANDLING 0

// This can be any "magic" bitpattern of 32 bits which seems unlikely
// to be randomly assigned to a memory word upon destruction.
//
// The 32 bits allocated for the "alive" bitpattern are used to try to
// detect when the instance has been prematurely destructed. This
// should prove useful to catch errors related to when SoSensorManager
// is destructed (on exit) while there are still live SoSensor-derived
// instances in the system, which then subsequently tries to
// unschedule themselves.
//
// <mortene@sim.no>
#define ALIVE_PATTERN 0x600dc0de /* spells "goodcode" */

// *************************************************************************

class SoSensorManagerP {
public:
  SoSensorManagerP(void) : alive(ALIVE_PATTERN) { }
  ~SoSensorManagerP() { this->alive = 0xdeadbeef; /* set to whatever != ALIVE_PATTERN */ }

  SbBool processingtimerqueue, processingdelayqueue;
  SbBool processingimmediatequeue;

  // immediatequeue - stores SoDelayQueueSensors with priority 0. FIFO.
  // delayqueue   - stores SoDelayQueueSensor's in sorted order.
  // timerqueue - stores SoTimerSensors in sorted order.

  SbList <SoDelayQueueSensor *> immediatequeue;
  SbList <SoDelayQueueSensor *> delayqueue;
  SbList <SoTimerQueueSensor *> timerqueue;
  SbList <SoTimerSensor*> reschedulelist;

  // FIXME: from what I can see, the two dicts below are simply used
  // as sets. Should implement a set datatype and use that
  // instead. 20050520 mortene.

  // stores sensors that has been triggered in processDelayQueue().
  SbHash<SoDelayQueueSensor *, SoDelayQueueSensor *> triggerdict;
  // temporary storage for idle sensors during processing
  SbHash<SoDelayQueueSensor *, SoDelayQueueSensor *> reinsertdict;

  void (*queueChangedCB)(void *);
  void * queueChangedCBData;

  SbTime delaysensortimeout;
  SoAlarmSensor * timeoutsensor;

  uint32_t alive;
  static void assertAlive(SoSensorManagerP * that);

#ifdef COIN_THREADSAFE
  SbMutex timermutex;
  SbMutex delaymutex;
  SbMutex immediatemutex;
  SbMutex reschedulemutex;
#endif // COIN_THREADSAFE
};

// The reason this is useful to keep around is that it is good for
// catching errors in the order we bring down the internal "services"
// of the Coin library, and the order we free up resources. If a
// sensor is lingering around after the SoSensorManager has been
// deallocated, for instance, we should expect this to hit.
void
SoSensorManagerP::assertAlive(SoSensorManagerP * that)
{
  if (that->alive != ALIVE_PATTERN) {
    SoDebugError::post("SoSensorManagerP::assertAlive",
                       "Detected an attempt to access SoSensorManager "
                       "instance after it was destructed!  "
                       "This is most likely to be the result of some grave "
                       "programming error in the internal library code. "
                       "Please report this problem");
    assert(FALSE && "SoSensorManager-object no longer alive!");
  }
}

#ifdef COIN_THREADSAFE

#define LOCK_TIMER_QUEUE(_mgr_) \
  _mgr_->pimpl->timermutex.lock();

#define UNLOCK_TIMER_QUEUE(_mgr_) \
  _mgr_->pimpl->timermutex.unlock();

#define LOCK_DELAY_QUEUE(_mgr_) \
  _mgr_->pimpl->delaymutex.lock();

#define UNLOCK_DELAY_QUEUE(_mgr_) \
  _mgr_->pimpl->delaymutex.unlock();

#define LOCK_IMMEDIATE_QUEUE(_mgr_) \
  _mgr_->pimpl->immediatemutex.lock();

#define UNLOCK_IMMEDIATE_QUEUE(_mgr_) \
  _mgr_->pimpl->immediatemutex.unlock();

#define LOCK_RESCHEDULE_LIST(_mgr_) \
  _mgr_->pimpl->immediatemutex.lock();

#define UNLOCK_RESCHEDULE_LIST(_mgr_) \
  _mgr_->pimpl->immediatemutex.unlock();

#else // COIN_THREADSAFE

#define LOCK_TIMER_QUEUE(_mgr_)
#define UNLOCK_TIMER_QUEUE(_mgr_)
#define LOCK_DELAY_QUEUE(_mgr_)
#define UNLOCK_DELAY_QUEUE(_mgr_)
#define LOCK_IMMEDIATE_QUEUE(_mgr_)
#define UNLOCK_IMMEDIATE_QUEUE(_mgr_)
#define LOCK_RESCHEDULE_LIST(_mgr_)
#define UNLOCK_RESCHEDULE_LIST(_mgr_)

#endif // ! COIN_THREADSAFE

#define PRIVATE(obj) ((obj)->pimpl)

// Callback called whenever the timeoutsensor triggers
// because the system hasn't been idle for a while.
static void
timeoutsensor_cb(void * userdata, SoSensor *)
{
  SoSensorManager * thisp = (SoSensorManager *)userdata;
  thisp->processDelayQueue(FALSE);
}

/*!
  Constructor.
 */
SoSensorManager::SoSensorManager(void)
{
  PRIVATE(this) = new SoSensorManagerP;

  PRIVATE(this)->queueChangedCB = NULL;
  PRIVATE(this)->queueChangedCBData = NULL;

  PRIVATE(this)->processingtimerqueue = FALSE;
  PRIVATE(this)->processingdelayqueue = FALSE;
  PRIVATE(this)->processingimmediatequeue = FALSE;

  PRIVATE(this)->delaysensortimeout.setValue(1.0/12.0);
  PRIVATE(this)->timeoutsensor = new SoAlarmSensor(timeoutsensor_cb, this);
}

/*!
  Destructor.
 */
SoSensorManager::~SoSensorManager()
{
  delete PRIVATE(this)->timeoutsensor;

  if(PRIVATE(this)->delayqueue.getLength() != 0) {
    // FIXME: remove entries. 19990225 mortene.
  }
  if(PRIVATE(this)->timerqueue.getLength() != 0) {
    // FIXME: remove entries. 19990225 mortene.
  }

  delete PRIVATE(this);
}

/*!
  Add a new entry to the queue of delay sensors.

  \sa removeFromQueue()
 */
void
SoSensorManager::insertDelaySensor(SoDelayQueueSensor * newentry)
{
  SoSensorManagerP::assertAlive(PRIVATE(this));
  assert(newentry);

  // immediate sensors are stored in a separate list. We don't need to
  // sort them based on SoSensor::isBefore(), but just use a FIFO
  // strategy.
  if (newentry->getPriority() == 0) {
    LOCK_IMMEDIATE_QUEUE(this);
    PRIVATE(this)->immediatequeue.append(newentry);
    UNLOCK_IMMEDIATE_QUEUE(this);
  }
  else {
    if (!PRIVATE(this)->timeoutsensor->isScheduled() &&
        PRIVATE(this)->delaysensortimeout != SbTime::zero()) {
      PRIVATE(this)->timeoutsensor->setTimeFromNow(PRIVATE(this)->delaysensortimeout);
      PRIVATE(this)->timeoutsensor->schedule();
    }

    LOCK_DELAY_QUEUE(this);
    SbList <SoDelayQueueSensor *> & delayqueue = PRIVATE(this)->delayqueue;

    // <= in test since the sensors should be processed FIFO for
    // sensors with equal priority
    uint32_t newsensorpriority = newentry->getPriority();
    int pos = 0;
    while((pos < delayqueue.getLength()) &&
          (delayqueue[pos]->getPriority() <= newsensorpriority)) {
      pos++;
    }
    delayqueue.insert(newentry, pos);
    UNLOCK_DELAY_QUEUE(this);
    this->notifyChanged();
  }

#if DEBUG_DELAY_SENSORHANDLING // debug
  SoDebugError::postInfo("SoSensorManager::insertDelaySensor",
                         "inserted delay sensor #%d -- %p -- "
                         "%sprocessing queue",
                         PRIVATE(this)->delayqueue.getLength() +
                         PRIVATE(this)->delaywaitqueue.getLength() - 1,
                         newentry,
                         PRIVATE(this)->processingdelayqueue ? "" : "not ");
#endif // debug
}

/*!
  Add a new entry to the queue of timer sensors. The queue will be sorted in
  order of supposed trigger time.

  \sa removeFromQueue()
 */
void
SoSensorManager::insertTimerSensor(SoTimerQueueSensor * newentry)
{
  SoSensorManagerP::assertAlive(PRIVATE(this));
  assert(newentry);

  SbList <SoTimerQueueSensor *> & timerqueue = PRIVATE(this)->timerqueue;

  LOCK_TIMER_QUEUE(this);
  int i = 0;

  // <= in test since the sensors should be processed FIFO for sensors
  // with the same trigger time
  double newtime = newentry->getTriggerTime().getValue();
  while ((i < timerqueue.getLength()) &&
         (timerqueue[i]->getTriggerTime().getValue() <= newtime)) {
    i++;
  }
  timerqueue.insert(newentry, i);

  UNLOCK_TIMER_QUEUE(this);

#if DEBUG_TIMER_SENSORHANDLING || 0 // debug
  SoDebugError::postInfo("SoSensorManager::insertTimerSensor",
                         "inserted timer sensor #%d -- %p "
                         "(triggertime %f) -- "
                         "%sprocessing queue",
                         PRIVATE(this)->timerqueue.getLength() +
                         PRIVATE(this)->timerwaitqueue.getLength() - 1,
                         newentry, newentry->getTriggerTime().getValue(),
                         PRIVATE(this)->processingtimerqueue ? "" : "not ");
#endif // debug

  if (!PRIVATE(this)->processingtimerqueue) {
    this->notifyChanged();
  }
}

/*!
  Remove an entry from the queue of prioritized sensors.

  \sa addToQueue()
 */
void
SoSensorManager::removeDelaySensor(SoDelayQueueSensor * entry)
{
  SoSensorManagerP::assertAlive(PRIVATE(this));

  LOCK_DELAY_QUEUE(this);
  // Check "real" queue first..
  int idx = PRIVATE(this)->delayqueue.find(entry);
  if (idx != -1) PRIVATE(this)->delayqueue.remove(idx);
  UNLOCK_DELAY_QUEUE(this);

  // ..then the immediate queue.
  if (idx == -1) {
    LOCK_IMMEDIATE_QUEUE(this);
    idx = PRIVATE(this)->immediatequeue.find(entry);
    if (idx != -1) PRIVATE(this)->immediatequeue.remove(idx);
    UNLOCK_IMMEDIATE_QUEUE(this);
  }
  // ..then the reinsert list
  if (idx == -1) {
    if (PRIVATE(this)->reinsertdict.erase(entry)) {
      idx = 0; // make sure notifyChanged() is called.
    }
  }

  if (idx != -1) this->notifyChanged();

#if COIN_DEBUG
  if (idx == -1) {
    SoDebugError::postWarning("SoSensorManager::removeDelaySensor",
                              "trying to remove element not in list");
  }
#endif // COIN_DEBUG
}

/*!
  Remove an entry from the queue of timer sensors.
 */
void
SoSensorManager::removeTimerSensor(SoTimerQueueSensor * entry)
{
  SoSensorManagerP::assertAlive(PRIVATE(this));

  LOCK_TIMER_QUEUE(this);
  int idx = PRIVATE(this)->timerqueue.find(entry);
  if (idx != -1) {
    PRIVATE(this)->timerqueue.remove(idx);
    UNLOCK_TIMER_QUEUE(this);
    this->notifyChanged();
  }
  else {
    UNLOCK_TIMER_QUEUE(this);
#if COIN_DEBUG
    SoDebugError::postWarning("SoSensorManager::removeTimerSensor",
                              "trying to remove element not in list");
#endif // COIN_DEBUG
  }
}

/*!
  Trigger all the timers which have expired.
 */
void
SoSensorManager::processTimerQueue(void)
{
  SoSensorManagerP::assertAlive(PRIVATE(this));

  if (PRIVATE(this)->processingtimerqueue || PRIVATE(this)->timerqueue.getLength() == 0)
    return;

#if DEBUG_TIMER_SENSORHANDLING // debug
  SoDebugError::postInfo("SoSensorManager::processTimerQueue",
                         "start: %d elements", PRIVATE(this)->timerqueue.getLength());
#endif // debug

  // Make sure that the PRIVATE(this)->processingtimerqueue
  // is reset even when the function is left after an unhandled exception
  // from the sensor's callback function.
  class FlagReset {
  public:
    FlagReset(SbBool& flag) : myflag(flag) {}
    ~FlagReset() {
      if (this->myflag) {
        SoDebugError::post("SoSensorManager::processTimerQueue",
                           "Unexpected function exit. Unhandled Exception?");
        this->myflag = FALSE;
      }
    }
    SbBool& myflag;
  };

  assert(PRIVATE(this)->reschedulelist.getLength() == 0);
  PRIVATE(this)->processingtimerqueue = TRUE;
  FlagReset fr(PRIVATE(this)->processingtimerqueue);

  LOCK_TIMER_QUEUE(this);

  SbTime currenttime = SbTime::getTimeOfDay();
  while (PRIVATE(this)->timerqueue.getLength() > 0 &&
         PRIVATE(this)->timerqueue[0]->getTriggerTime() <= currenttime) {
#if DEBUG_TIMER_SENSORHANDLING // debug
    SoDebugError::postInfo("SoSensorManager::processTimerQueue",
                           "process element with triggertime %s",
                           PRIVATE(this)->timerqueue[0]->getTriggerTime().format().getString());
#endif // debug
    SoSensor * sensor = PRIVATE(this)->timerqueue[0];
    PRIVATE(this)->timerqueue.remove(0);
    UNLOCK_TIMER_QUEUE(this);
    sensor->trigger();
    LOCK_TIMER_QUEUE(this);
  }

  UNLOCK_TIMER_QUEUE(this);

#if DEBUG_TIMER_SENSORHANDLING // debug
  SoDebugError::postInfo("SoSensorManager::processTimerQueue",
                         "end, before merge: %d elements",
                         PRIVATE(this)->timerqueue.getLength());
#endif // debug

  LOCK_RESCHEDULE_LIST(this);
  int n = PRIVATE(this)->reschedulelist.getLength();
  if (n) {
    SbTime time = SbTime::getTimeOfDay();
    for (int i = 0; i < n; i++) {
      PRIVATE(this)->reschedulelist[i]->reschedule(time);
    }
    PRIVATE(this)->reschedulelist.truncate(0);
  }
  UNLOCK_RESCHEDULE_LIST(this);

  PRIVATE(this)->processingtimerqueue = FALSE;

#if DEBUG_TIMER_SENSORHANDLING // debug
  SoDebugError::postInfo("SoSensorManager::processTimerQueue",
                         "end, after merge: %d elements",
                         PRIVATE(this)->timerqueue.getLength());
#endif // debug
}

/*!
  Trigger all delay queue entries in priority order.

  The \a isidle flag indicates whether or not the processing happens
  because the application is idle or because the delay queue timeout
  was reached.

  A delay queue sensor with priority > 0 can only be triggered once
  during a call to this function. If a delay sensor is rescheduled
  during processDelayQueue(), it is not processed until the next time
  this function is called. This is done to avoid an infinite loop
  while processing the sensors.

  A delay queue sensor with priority 0 is called an immediate sensor.

  \sa SoDB::setDelaySensorTimeout()
  \sa SoSensorManager::processImmediateQueue()
*/
void
SoSensorManager::processDelayQueue(SbBool isidle)
{
  SoSensorManagerP::assertAlive(PRIVATE(this));

  this->processImmediateQueue();

  if (PRIVATE(this)->processingdelayqueue || PRIVATE(this)->delayqueue.getLength() == 0)
    return;

#if DEBUG_DELAY_SENSORHANDLING // debug
  SoDebugError::postInfo("SoSensorManager::processDelayQueue",
                         "start: %d elements", PRIVATE(this)->delayqueue.getLength());
#endif // debug

  // Make sure that the PRIVATE(this)->processingdelayqueue
  // is reset even when the function is left after an unhandled exception
  // from the sensor's callback function.
  class FlagReset {
  public:
    FlagReset(SbBool& flag) : myflag(flag) {}
    ~FlagReset() {
      if (this->myflag) {
        SoDebugError::post("SoSensorManager::processDelayQueue",
                           "Unexpected function exit. Unhandled Exception?");
        this->myflag = FALSE;
      }
    }
    SbBool& myflag;
  };

  PRIVATE(this)->processingdelayqueue = TRUE;
  FlagReset fr(PRIVATE(this)->processingdelayqueue);

  // triggerdict is used to store sensors that have already been
  // triggered. A sensor should only be triggered once during a call
  // to processDelayQueue(), otherwise we might risk never returning
  // from this function. E.g. SoSceneManager::scheduleRedraw()
  // triggers a delay sensor, which again triggers a redraw. During
  // the redraw, SoSceneManager::scheduleRedraw() might be called
  // again, etc...
  PRIVATE(this)->triggerdict.clear();

  LOCK_DELAY_QUEUE(this);

  // Sensors with higher priorities are triggered first.
  while (PRIVATE(this)->delayqueue.getLength()) {
#if DEBUG_DELAY_SENSORHANDLING // debug
    SoDebugError::postInfo("SoSensorManager::processDelayQueue",
                           "treat element with pri %d",
                           PRIVATE(this)->delayqueue[0]->getPriority());
#endif // debug

    SoDelayQueueSensor * sensor = PRIVATE(this)->delayqueue[0];
    PRIVATE(this)->delayqueue.remove(0);
    UNLOCK_DELAY_QUEUE(this);

    if (!isidle && sensor->isIdleOnly()) {
      // move sensor to another temporary list. It will be reinserted
      // at the end of this function. We do this to be able to always
      // remove the first list element. We avoid searching for the
      // first non-idle sensor.
      (void) PRIVATE(this)->reinsertdict.put(sensor, sensor);
    }
    else {
      // only trigger sensor once per processing loop
      if (PRIVATE(this)->triggerdict.put(sensor, sensor)) {
        sensor->trigger();
      }
      else {
        // Reuse the "reinsert" list to store the sensor. It will be
        // reinserted at the end of this function.
        (void) PRIVATE(this)->reinsertdict.put(sensor, sensor);
      }
    }
    LOCK_DELAY_QUEUE(this);
  }
  UNLOCK_DELAY_QUEUE(this);

  // reinsert sensors that couldn't be triggered, either because it
  // was an idle sensor, or because the sensor had already been
  // triggered
  for(
      SbHash<SoDelayQueueSensor *, SoDelayQueueSensor *>::const_iterator iter =
       PRIVATE(this)->reinsertdict.const_begin();
      iter!=PRIVATE(this)->reinsertdict.const_end();
      ++iter
      ) {
    this->insertDelaySensor(iter->obj);
  }
  PRIVATE(this)->reinsertdict.clear();
  PRIVATE(this)->processingdelayqueue = FALSE;

  // If we still have pending sensors and the timeoutsensor
  // isn't currently scheduled, schedule it.
  if (PRIVATE(this)->delayqueue.getLength() && !PRIVATE(this)->timeoutsensor->isScheduled()) {
    PRIVATE(this)->timeoutsensor->setTimeFromNow(PRIVATE(this)->delaysensortimeout);
    PRIVATE(this)->timeoutsensor->schedule();
  }
}

/*!
  Process all immediate sensors (delay sensors with priority 0).

  Be aware that you might risk an infinite loop using immediate
  sensors. Unlike delay queue sensors, immediate sensors can be
  rescheduled and triggered multiple times during immediate queue
  processing.

  \sa SoDelayQueueSensor::setPriority() */
void
SoSensorManager::processImmediateQueue(void)
{
  SoSensorManagerP::assertAlive(PRIVATE(this));

  if (PRIVATE(this)->processingimmediatequeue) return;

#if DEBUG_DELAY_SENSORHANDLING || 0 // debug
  SoDebugError::postInfo("SoSensorManager::processImmediateQueue",
                         "start: %d elements in full immediate queue",
                         PRIVATE(this)->immediatequeue.getLength());
#endif // debug

  // Make sure that the PRIVATE(this)->processingimmediatequeue
  // is reset even when the function is left after an unhandled exception
  // from the sensor's callback function.
  class FlagReset {
  public:
    FlagReset(SbBool& flag) : myflag(flag) {}
    ~FlagReset() {
      if (this->myflag) {
        SoDebugError::post("SoSensorManager::processImmediateQueue",
                           "Unexpected function exit. Unhandled Exception?");
        this->myflag = FALSE;
      }
    }
    SbBool& myflag;
  };

  PRIVATE(this)->processingimmediatequeue = TRUE;
  FlagReset fr(PRIVATE(this)->processingimmediatequeue);

  // FIXME: implement some better logic to break out of the
  // processing loop. Right now we break out if more than 10000
  // immediate sensors are processed. pederb, 2002-01-30
  int triggercnt = 0;

  LOCK_IMMEDIATE_QUEUE(this);

  while (PRIVATE(this)->immediatequeue.getLength()) {
#if DEBUG_DELAY_SENSORHANDLING || 0 // debug
    SoDebugError::postInfo("SoSensorManager::processImmediateQueue",
                           "trigger element");
#endif // debug
    SoSensor * sensor = PRIVATE(this)->immediatequeue[0];
    PRIVATE(this)->immediatequeue.remove(0);
    UNLOCK_IMMEDIATE_QUEUE(this);

    sensor->trigger();

    LOCK_IMMEDIATE_QUEUE(this);
    triggercnt++;
    if (triggercnt > 10000) break;
  }
  if (PRIVATE(this)->immediatequeue.getLength()) {
#if COIN_DEBUG
    SoDebugError::postWarning("SoSensorManager::processImmediateQueue",
                              "Infinite loop detected. Breaking out.");
#endif // COIN_DEBUG
  }
  UNLOCK_IMMEDIATE_QUEUE(this);

  PRIVATE(this)->processingimmediatequeue = FALSE;
}

/*!
  \COININTERNAL
*/
void
SoSensorManager::rescheduleTimer(SoTimerSensor * s)
{
  SoSensorManagerP::assertAlive(PRIVATE(this));

  LOCK_RESCHEDULE_LIST(this);
  PRIVATE(this)->reschedulelist.append(s);
  UNLOCK_RESCHEDULE_LIST(this);
}

/*!
  \COININTERNAL
*/
void
SoSensorManager::removeRescheduledTimer(SoTimerQueueSensor * s)
{
  SoSensorManagerP::assertAlive(PRIVATE(this));

  LOCK_RESCHEDULE_LIST(this);
  int idx = PRIVATE(this)->reschedulelist.find((SoTimerSensor*)s);
  if (idx >= 0) {
    PRIVATE(this)->reschedulelist.remove(idx);
    UNLOCK_RESCHEDULE_LIST(this);
  }
  else {
    UNLOCK_RESCHEDULE_LIST(this);
    this->removeTimerSensor(s);
  }
}

/*!
  Returns \c TRUE if at least one delay sensor or immediate sensor is
  present in the respective queue, otherwise \c FALSE.
*/
SbBool
SoSensorManager::isDelaySensorPending(void)
{
  SoSensorManagerP::assertAlive(PRIVATE(this));

  return (PRIVATE(this)->delayqueue.getLength() ||
          PRIVATE(this)->immediatequeue.getLength()) ? TRUE : FALSE;
}

/*!
  Returns \c TRUE if at least one timer sensor is present in the
  queue, otherwise \c FALSE.

  If sensors are pending, the time interval until the next one should
  be triggered will be put in the \a tm variable.
*/
SbBool
SoSensorManager::isTimerSensorPending(SbTime & tm)
{
  SoSensorManagerP::assertAlive(PRIVATE(this));

  LOCK_TIMER_QUEUE(this);
  if (PRIVATE(this)->timerqueue.getLength() > 0) {
    tm = PRIVATE(this)->timerqueue[0]->getTriggerTime();
    UNLOCK_TIMER_QUEUE(this);
    return TRUE;
  }

  UNLOCK_TIMER_QUEUE(this);
  return FALSE;
}

/*!
  Delay sensors are usually triggered only when the system is
  idle. But when there are continuous updates to the scene graph,
  there's a possibility that the delay queue will starve and sensors
  are never triggered. To make sure this won't happen, this is a
  timeout value for the delay queue. When this timeout has been reached,
  the sensors in the delay queue gets processed before other sensors and
  events. This method will let the user set this timeout value.

  The default value is 1/12 of a second.

  Specifying a zero time will disable the timeout, opening for
  potential delay queue starvation.

  \sa getDelaySensorTimeout(), SoDelayQueueSensor
*/
void
SoSensorManager::setDelaySensorTimeout(const SbTime & t)
{
  SoSensorManagerP::assertAlive(PRIVATE(this));

#if COIN_DEBUG
  if(t < SbTime::zero()) {
    SoDebugError::postWarning("SoDB::setDelaySensorTimeout",
                              "Tried to set negative interval.");
    return;
  }
#endif // COIN_DEBUG

  PRIVATE(this)->delaysensortimeout = t;

  if (t == SbTime::zero() && PRIVATE(this)->timeoutsensor->isScheduled()) {
    PRIVATE(this)->timeoutsensor->unschedule();
  }
  else if (PRIVATE(this)->delayqueue.getLength()) {
    PRIVATE(this)->timeoutsensor->setTimeFromNow(t);
    PRIVATE(this)->timeoutsensor->schedule();
  }
}

/*!
  Returns the timeout value for sensors in the delay queue.

  \sa setDelaySensorTimeout(), SoDelayQueueSensor
 */
const SbTime &
SoSensorManager::getDelaySensorTimeout(void)
{
  SoSensorManagerP::assertAlive(PRIVATE(this));

  return PRIVATE(this)->delaysensortimeout;
}

/*!
  For setting up a callback function to be invoked whenever any of the
  sensor queues are changed.

  This callback should typically be responsible for updating the
  client-side mechanism which is used for processing the queues.
*/
void
SoSensorManager::setChangedCallback(void (*func)(void *), void * data)
{
  SoSensorManagerP::assertAlive(PRIVATE(this));

  PRIVATE(this)->queueChangedCB = func;
  PRIVATE(this)->queueChangedCBData = data;
}

void
SoSensorManager::notifyChanged(void)
{
  SoSensorManagerP::assertAlive(PRIVATE(this));

  if (PRIVATE(this)->queueChangedCB &&
      !PRIVATE(this)->processingtimerqueue &&
      !PRIVATE(this)->processingdelayqueue &&
      !PRIVATE(this)->processingimmediatequeue) {
    PRIVATE(this)->queueChangedCB(PRIVATE(this)->queueChangedCBData);
  }
}

/*!
  NOTE: THIS METHOD IS OBSOLETED. DON'T USE IT.

  This is a wrapper around the standard select(2) call, which will
  make sure the sensor queues are updated while waiting for any action
  to happen on the given file descriptors.

  The void* arguments must be valid pointers to fd_set
  structures. We've changed this from the original SGI Inventor API to
  avoid messing up the header file with system specific includes.

  NOTE: THIS METHOD IS OBSOLETED. DON'T USE IT.
*/
int
SoSensorManager::doSelect(int COIN_UNUSED_ARG(nfds), void * COIN_UNUSED_ARG(readfds), void * COIN_UNUSED_ARG(writefds),
                          void * COIN_UNUSED_ARG(exceptfds), struct timeval * COIN_UNUSED_ARG(usertimeout))
{
  assert(FALSE && "obsoleted method");
  return 0;
}

int
SoSensorManager::mergeTimerQueues(void)
{
  assert(0 && "obsoleted");
  return 0;
}

int
SoSensorManager::mergeDelayQueues(void)
{
  assert(0 && "obsoleted");
  return 0;
}


#undef DEBUG_DELAY_SENSORHANDLING
#undef DEBUG_TIMER_SENSORHANDLING
#undef ALIVE_PATTERN
#undef LOCK_TIMER_QUEUE
#undef UNLOCK_TIMER_QUEUE
#undef LOCK_DELAY_QUEUE
#undef UNLOCK_DELAY_QUEUE
#undef LOCK_IMMEDIATE_QUEUE
#undef UNLOCK_IMMEDIATE_QUEUE
#undef LOCK_RESCHEDULE_LIST
#undef UNLOCK_RESCHEDULE_LIST
#undef PRIVATE

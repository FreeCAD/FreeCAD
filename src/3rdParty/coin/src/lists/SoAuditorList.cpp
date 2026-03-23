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
  \class SoAuditorList SoAuditorList.h Inventor/lists/SoAuditorList.h
  \brief The SoAuditorList class is used to keep track of auditors for certain object classes.

  \ingroup coin_general

  This class is mainly for internal use (from SoBase) and it should
  not be necessary to be familiar with it for "ordinary" Coin use.
*/


#include <Inventor/fields/SoField.h>
#include <Inventor/fields/SoFieldContainer.h>
#include <Inventor/sensors/SoDataSensor.h>
#if COIN_DEBUG
#include <Inventor/errors/SoDebugError.h>
#endif // COIN_DEBUG

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H

#ifdef COIN_THREADSAFE
#include "threads/recmutexp.h"
// we need this lock to avoid that auditors are added/removed by one
// thread while another thread is notifying
#define NOTIFY_LOCK (void) cc_recmutex_internal_notify_lock()
#define NOTIFY_UNLOCK (void) cc_recmutex_internal_notify_unlock()
#else // COIN_THREADSAFE
#define NOTIFY_LOCK
#define NOTIFY_UNLOCK
#endif // !COIN_THREADSAFE

/*!
  Default constructor.
*/
SoAuditorList::SoAuditorList(void)
  : SbPList(8)
{
}

/*!
  Destructor.
*/
SoAuditorList::~SoAuditorList()
{
}

/*!
  Append an \a auditor of \a type to the list.
*/
void
SoAuditorList::append(void * const auditor, const SoNotRec::Type type)
{
  NOTIFY_LOCK;
  SbPList::append(auditor);
  SbPList::append((void *)type);
  NOTIFY_UNLOCK;
}

/*!
  Set \a auditor pointer and auditor \a type in list at \a index.
*/
void
SoAuditorList::set(const int index,
                   void * const auditor, const SoNotRec::Type type)
{
  NOTIFY_LOCK;
  assert(index >= 0 && index < this->getLength());

  SbPList::set(index * 2, auditor);
  SbPList::set(index * 2 + 1, (void *)type);
  NOTIFY_UNLOCK;
}

/*!
  Returns number of elements in list.
*/
int
SoAuditorList::getLength(void) const
{
  return SbPList::getLength() / 2;
}

/*!
  Find \a auditor of \a type in list and return index. Returns -1 if
  \a auditor is not in the list.
*/
int
SoAuditorList::find(void * const auditor, const SoNotRec::Type type) const
{
  const int num = this->getLength();
  for (int i = 0; i < num; i++) {
    if (this->getObject(i) == auditor && this->getType(i) == type)
      return i;
  }
  return -1;
}

/*!
  Returns auditor pointer at \a index.
*/
void *
SoAuditorList::getObject(const int index) const
{
  return SbPList::operator[](index * 2);
}

/*!
  Returns auditor type at \a index.
*/
SoNotRec::Type
SoAuditorList::getType(const int index) const
{
  const uintptr_t tmp = (uintptr_t)(SbPList::operator[](index*2+1));
  return (SoNotRec::Type)tmp;
}

/*!
  Remove auditor at \a index.
*/
void
SoAuditorList::remove(const int index)
{
  NOTIFY_LOCK;
  assert(index >= 0 && index < this->getLength());
  SbPList::remove(index * 2); // ptr
  SbPList::remove(index * 2); // type
  NOTIFY_UNLOCK;
}

/*!
  Remove \a auditor of \a type from list.
*/
void
SoAuditorList::remove(void * const auditor, const SoNotRec::Type type)
{
  this->remove(this->find(auditor, type));
}

/*!
  Send notification to all our auditors.
*/
void
SoAuditorList::notify(SoNotList * l)
{
  const int num = this->getLength();
  if (num == 1) { // fast path for common case
    this->doNotify(l, this->getObject(0), this->getType(0));
  }
  // handle multiple auditors by copying the list, in case any one of
  // the notifications we're sending out changes the list
  // mid-traversal (that's also why we take special care of the
  // 1-auditor case above -- so we don't have to copy the list for the
  // common case)
  else if (num > 1) {
    // FIXME: should perhaps use a more general mechanism to detect when
    // to ignore notification? (In SoFieldContainer::notify() -- based
    // on SoNotList::getTimeStamp()?) 20000304 mortene.
    SbPList notified(num);

    for (int i = 0; i < num; i++) {
      void * auditor = this->getObject(i);
      if (notified.find(auditor) == -1) {
        // use a copy of 'l', since the notification list might change
        // when auditors are notified
        SoNotList listcopy(l);
        this->doNotify(&listcopy, auditor, this->getType(i));
        notified.append(auditor);
      }
    }

    // FIXME: it should be possible for the application programmer to
    // do this (it is for instance useful and tempting to do it upon
    // changes in engines). pederb, 2001-11-06
    assert(num == this->getLength() &&
           "auditors cannot be removed during the notification loop");
  }
}

//
// Private method used to propagate 'l' to the 'auditor' of type 'type'
//
void
SoAuditorList::doNotify(SoNotList * l, const void * auditor, const SoNotRec::Type type)
{
  l->setLastType(type);

  switch (type) {
  case SoNotRec::CONTAINER:
  case SoNotRec::PARENT:
    {
      SoFieldContainer * obj = (SoFieldContainer *)auditor;
      obj->notify(l);
    }
    break;

  case SoNotRec::SENSOR:
    {
      SoDataSensor * obj = (SoDataSensor *)auditor;
#if COIN_DEBUG && 0 // debug
      SoDebugError::postInfo("SoAuditorList::notify",
                             "notify and schedule sensor: %p", obj);
#endif // debug
      // don't schedule the sensor here. The sensor instance will do
      // that in notify() (it might also choose _not_ to schedule),
      obj->notify(l);
    }
    break;

  case SoNotRec::FIELD:
  case SoNotRec::ENGINE:
    {
      // We used to check whether or not the fields was already
      // dirty before we transmitted the notification
      // message. This is _not_ correct (the dirty flag is
      // conceptually only relevant for whether or not to do
      // re-evaluation), so don't try to "optimize" the
      // notification mechanism by re-introducing that "feature".
      // :^/
      ((SoField *)auditor)->notify(l);
    }
    break;

  default:
    assert(0 && "Unknown auditor type");
  }
}

#undef NOTIFY_LOCK
#undef NOTIFY_UNLOCK

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
  \class SoNotRec SoNotRec.h Inventor/misc/SoNotification.h
  \brief The SoNotRec class specifies records for notification lists.

  \ingroup coin_general

  \sa SoNotification
*/

/*!
  \enum SoNotRec::Type

  This enum is used to specify the type of the notification source
  within the record.
*/

/*!
  \enum SoNotRec::OperationType

  This enum is used to specify the type of the scene graph operation
  causing the notification.
*/

#include <Inventor/misc/SoNotRec.h>
#include <Inventor/errors/SoDebugError.h>
#include <cassert>

#if COIN_DEBUG  // for SoNotRec::print() method
#include <Inventor/misc/SoBase.h>
#include <Inventor/SbName.h>
#endif // COIN_DEBUG


/*!
  Constructor. Initializes the record with \a notifbase pointer.
*/
SoNotRec::SoNotRec(SoBase * const notifbase) :
  type((SoNotRec::Type)-1), base(notifbase), prev(NULL), index(-1),
  fieldNumIndices(0), operationType(UNSPECIFIED), groupChild(NULL), groupPrevChild(NULL)
{
}

/*!
  Set the \a type of the notification source of this record.
*/
void
SoNotRec::setType(const SoNotRec::Type typearg)
{
  this->type = typearg;
}

/*!
  Returns the notification source within this record.
*/
SoBase *
SoNotRec::getBase(void) const
{
  return this->base;
}

/*!
  Returns the type of the notification source within this record.
*/
SoNotRec::Type
SoNotRec::getType(void) const
{
  return this->type;
}

/*!
  Returns the previous notification source (i.e. the source that the
  base within this record was auditing).
*/
const SoNotRec *
SoNotRec::getPrevious(void) const
{
#if 0 // OBSOLETED: see comment on setPrevious(). 20000304 mortene.
  assert(this != this->prev);
#endif // OBSOLETED
  return this->prev;
}

/*!
  Set pointer to the previous notification record.
*/
void
SoNotRec::setPrevious(const SoNotRec * const prevptr)
{
#if 0 // OBSOLETED: looks like this can be allowed (and need to be
      // allowed under the current circumstances, as it hits under certain
      // conditions). 20000304 mortene
  assert(this != prevptr);
#endif // OBSOLETED
  this->prev = prevptr;
}

/*!
  Prints debug information.
*/
void
SoNotRec::print(FILE * const file) const
{
#if COIN_DEBUG
  (void)fprintf(file, "\tSoNotRec %p: type ", this);
  switch (this->type) {
  case CONTAINER:  (void)fprintf(file, "CONTAINER"); break;
  case PARENT:     (void)fprintf(file, "PARENT"); break;
  case SENSOR:     (void)fprintf(file, "SENSOR"); break;
  case FIELD:      (void)fprintf(file, "FIELD"); break;
  case ENGINE:     (void)fprintf(file, "ENGINE"); break;
  default:         (void)fprintf(file, "UNSET"); break;
  }
  if (this->base) {
    (void)fprintf(file, ", base %p (type %s, \"%s\")\n",
                  this->base, this->base->getTypeId().getName().getString(),
                  this->base->getName().getString());
  }
  else {
    (void)fprintf(file," base is NULL\n");
  }
#endif // COIN_DEBUG
}

SoNotRec::OperationType
SoNotRec::getOperationType(void) const
{
  return operationType;
}

int
SoNotRec::getIndex(void) const
{
  return index;
}

int
SoNotRec::getFieldNumIndices(void) const
{
  return fieldNumIndices;
}

const SoBase *
SoNotRec::getGroupChild(void) const
{
  return groupChild;
}

const SoBase *
SoNotRec::getGroupPrevChild(void) const
{
  return groupPrevChild;
}

void
SoNotRec::setOperationType(const SoNotRec::OperationType opType)
{
  this->operationType = opType;
}

void
SoNotRec::setIndex(const int idx)
{
  this->index = idx;
}

void
SoNotRec::setFieldNumIndices(const int fldnumind)
{
  this->fieldNumIndices = fldnumind;
}

void
SoNotRec::setGroupChild(const SoBase * const gc)
{
  this->groupChild = gc;
}

void
SoNotRec::setGroupPrevChild(const SoBase * const pc)
{
  this->groupPrevChild = pc;
}

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
  \class SoNotList SoNotification.h Inventor/misc/SoNotification.h
  \brief The SoNotList class is a list of SoNotRec notification records.

  \ingroup coin_general
*/

#include <Inventor/misc/SoNotification.h>
#include <Inventor/misc/SoNotRec.h>
#include <Inventor/errors/SoDebugError.h>
#include <Inventor/nodes/SoNode.h>
#include <Inventor/SbName.h>
#include <cassert>
#include <ctime>


/*!
  Initialize list.
*/
SoNotList::SoNotList(void) :
  head(NULL), tail(NULL), firstnoderec(NULL), lastfield(NULL), lastengine(NULL),
  // this is used in SoNode::notify() to stop a notification
  // when a node has already been notified.
  stamp(SoNode::getNextNodeId())
{
}

/*!
  Copy constructor. Does a bitwise copy of the \a nl object (no
  duplication of list elements).
*/
SoNotList::SoNotList(const SoNotList * nl)
{
  *this = *nl;
}

/*!
  Append \a rec notification source to the list.
*/
void
SoNotList::append(SoNotRec * const rec)
{
#if COIN_DEBUG && 0 // debug
  if (rec->getBase()) {
    SoDebugError::postInfo("SoNotList::append", "%p - %p (base: %p %s \"%s\")",
                           this, rec, rec->getBase(),
                           rec->getBase()->getTypeId().getName().getString(),
                           rec->getBase()->getName().getString());
  }
  else {
    SoDebugError::postInfo("SoNotList::append", "%p - %p (base is NULL)",
                           this, rec);
  }
#endif // debug

  // Link into list.
  rec->setPrevious(this->tail);
  this->tail = rec;
  if (!this->head) this->head = rec;

  if (!this->firstnoderec && rec->getBase() && rec->getBase()->isOfType(SoNode::getClassTypeId()))
    this->firstnoderec = rec;
}

/*!
  Append \a rec notification source to the list, setting \a field as
  the last field having been influenced by the notification process.
*/
void
SoNotList::append(SoNotRec * const rec, SoField * const field)
{
#if COIN_DEBUG && 0 // debug
  SoDebugError::postInfo("SoNotList::append", "field %p", field);
#endif // debug
  assert(field);
  this->lastfield = field;
  this->append(rec);
}

/*!
  Append \a rec notification source to the list, setting \a engineout
  as the last engine output field having been influenced by the
  notification process.
*/
void
SoNotList::append(SoNotRec * const rec, SoEngineOutput * const engineout)
{
  assert(engineout);
  this->lastengine = engineout;
  this->append(rec);
}

/*!
  Set the \a type of the last notification record in the list.
*/
void
SoNotList::setLastType(const SoNotRec::Type type)
{
  assert(this->tail);
  this->tail->setType(type);

  switch (type) {
  case SoNotRec::FIELD:
  case SoNotRec::ENGINE:
    this->firstnoderec = NULL;
  default:
    break;
  }
}

/*!
  Returns the first record in the list.
*/
SoNotRec *
SoNotList::getFirstRec(void) const
{
  return this->head;
}

/*!
  Returns the last record in the list.
*/
SoNotRec *
SoNotList::getLastRec(void) const
{
  return this->tail;
}

/*!
  Returns the first record in the list which is derived from SoBase.
*/
SoNotRec *
SoNotList::getFirstRecAtNode(void) const
{
  return this->firstnoderec;
}

/*!
  Returns the last field touched by notification.
*/
SoField *
SoNotList::getLastField(void) const
{
  return this->lastfield;
}

/*!
  Returns the last engine output field touched by notification.
*/
SoEngineOutput *
SoNotList::getLastEngineOutput(void) const
{
  return this->lastengine;
}

/*!
  Returns the time stamp when the notification started.
*/
SbUniqueId
SoNotList::getTimeStamp(void) const
{
  return this->stamp;
}

/*!
  Dump contents of list from tail record and backwards. Only available
  if compiled with debug information on.
*/
void
SoNotList::print(FILE * const file) const
{
#if COIN_DEBUG
  (void)fprintf(file, "SoNotList: %p\n", this);
  const SoNotRec * ptr = this->tail;
  while (ptr) {
    ptr->print(file);
    ptr = ptr->getPrevious();
  }

  const SoBase * n = this->getFirstRecAtNode()->getBase();
  const SoField * f = this->getLastField();
  SbName fname("");
  if (n && n->isOfType(SoNode::getClassTypeId()) && f) {
    (void)((SoNode *)n)->getFieldName(f, fname);
  }

  (void)fprintf(file, "\tfirstAtNode = %p, lastField = %p (\"%s\")\n",
                this->getFirstRecAtNode(),
                this->getLastField(), fname.getString());
#endif // COIN_DEBUG
}

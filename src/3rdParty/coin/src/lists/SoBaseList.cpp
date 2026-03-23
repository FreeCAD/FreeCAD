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
  \class SoBaseList SoBaseList.h Inventor/lists/SoBaseList.h
  \brief The SoBaseList class is a container for pointers to SoBase derived objects.

  \ingroup coin_general

  The additional capability of the SoBaseList class over its parent
  class, SbPList, is to automatically handle referencing and
  dereferencing of items as they are added or removed from the lists.
*/

#include <Inventor/lists/SoBaseList.h>
#include <Inventor/misc/SoBase.h>
#include <cassert>

// Convenience macro for casting array element from void * to SoBase *.
#define GET_BASEPTR(idx) ((SoBase *)SbPList::get(idx))


/*!
  Default constructor.
*/
SoBaseList::SoBaseList(void)
  : SbPList(), referencing(TRUE)
{
}

/*!
  Constructor with a hint about the maximum number of pointers in the
  list.

  \sa SbPList::SbPList(const int)
*/
SoBaseList::SoBaseList(const int size)
  : SbPList(size), referencing(TRUE)
{
}

/*!
  Copy constructor.

  Shallow copy the items of \a l into this list, adding to the item
  reference counts if the reference flag of \a l is \c TRUE.

  \sa SbPList::SbPList(const SbPList &)
*/
SoBaseList::SoBaseList(const SoBaseList & l)
  : SbPList(l) // copies list items "raw"
{
  this->referencing = l.referencing;
  if (this->referencing) {
    const int n = this->getLength();
    for (int i = 0; i < n; i++) GET_BASEPTR(i)->ref();
  }
}

/*!
  Destructor. Dereferences items before freeing resources.
*/
SoBaseList::~SoBaseList()
{
  this->truncate(0); // truncate() does unref-ing
}

/*!
  Append \a ptr to list, adding to the reference count of the object
  (unless addReferences() has been set to \c FALSE).

  Overloaded from parent to support reference counting on the SoBase object.

  \sa SbPList::append()
*/
void
SoBaseList::append(SoBase * ptr)
{
  if (this->referencing && ptr) ptr->ref();
  SbPList::append(ptr);
}

/*!
  Insert \a ptr in the list at position \a addbefore, adding to the
  reference count of the object (unless addReferences() has been set
  to \c FALSE).

  Overloaded from parent to support reference counting on the SoBase object.

  \sa SbPList::insert()
*/
void
SoBaseList::insert(SoBase * ptr, const int addbefore)
{
  if (this->referencing && ptr) ptr->ref();
  SbPList::insert(ptr, addbefore);
}

/*!
  Removes item at \a index from the list, dereferencing the object
  (unless addReferences() has been set to \c FALSE).

  Overloaded from parent to support reference counting on the SoBase object.

  \sa SbPList::remove()
*/
void
SoBaseList::remove(const int index)
{
  assert(index >= 0 && index < this->getLength());
  if (this->referencing && GET_BASEPTR(index) != NULL)
    GET_BASEPTR(index)->unref();
  SbPList::remove(index);
}

/*!
  Removes \a item from the list, dereferencing the object (unless
  addReferences() has been set to \c FALSE).

  Overloaded from parent to support reference counting on the SoBase object.

  \sa SbPList::removeItem()
*/
void
SoBaseList::removeItem(SoBase * item)
{
  // We need to override the removeItem() function from our SbPList
  // superclass, or invocations of it will not cause a decrease of the
  // reference count.

  // Just forward call to remove(), which takes care of the
  // dereferencing.
  this->remove(this->find(item));
}

/*!
  Makes the list contain only the \a length first items, removing all
  items from index \a length and onwards to the end of the
  list. Dereferences the objects to be removed (unless addReferences()
  has been set to \c FALSE).

  Overloaded from parent to support reference counting on the SoBase object.

  \sa SbPList::truncate()
*/
void
SoBaseList::truncate(const int length)
{
  if (this->referencing) {
    const int n = this->getLength();
    for (int i = length; i < n; i++)
      if (GET_BASEPTR(i) != NULL) GET_BASEPTR(i)->unref();
  }
  SbPList::truncate(length);
}

/*!
  Shallow copy of the item pointers of \a l list into this one, first
  removing all items in this list.
*/
void
SoBaseList::copy(const SoBaseList & l)
{
  if (this == &l) return;

  this->truncate(0);

  this->referencing = l.referencing;
  const int n = l.getLength();
  for (int i = 0; i < n; i++) this->append(l[i]); // handles ref'ing
}

/*!
  Shallow copy of the SoBase pointers from \a l into this one,
  returning a pointer to ourself.

  \sa copy()
*/
SoBaseList &
SoBaseList::operator=(const SoBaseList & l)
{
  this->copy(l);
  return *this;
}

/*!
  \copydetails SbPList::operator[](const int idx) const

  Overloaded from parent to return an SoBase pointer.

  \sa SbPList::operator[]()
*/
SoBase *
SoBaseList::operator[](const int idx) const
{
  // Overridden from superclass to cast from the \c void pointer
  // actually stored.

  return (SoBase *)SbPList::operator[](idx);
}

/*!
  Decide whether or not the SoBase items should be automatically
  referenced and dereferenced as they are added and removed from the
  list.

  Default setting is to do referencing.
*/
void
SoBaseList::addReferences(const SbBool flag)
{
#if 0 // OBSOLETED: don't do this, it looks like it could give the
      // user some nasty surprises. 20000228 mortene.

  // this method should probably never be called when there are items in
  // the list, but I think the code below should handle that case also.
  // If refing changes from on to off, all items are unref'ed, since
  // they were ref'ed when inserted. If state changes from off to on, all
  // items are ref'ed, since they will be unref'ed when removed from list.

  if (flag == this->referencing) return; // no change

  const int n = this->getLength();
  for (int i = 0; i < n; i++) {
    SoBase * item = GET_BASEPTR(i);
    if (item) {
      if (flag) item->ref();
      else item->unref();
    }
  }

#else

  assert(this->getLength() == 0);

#endif

  this->referencing = flag;
}

/*!
  Return whether the SoBase instances are automatically referenced
  and dereferenced when they are added and removed from the list.

  \COIN_FUNCTION_EXTENSION

  \sa addReferences()
  \since Coin 2.0
*/
SbBool 
SoBaseList::isReferencing(void) const
{
  return this->referencing;
}

/*!
  \copydetails SbPList::set(const int idx, void * item)

  Overloaded from parent to support reference counting on the SoBase object.

  \sa SbPList::set(const int idx, void * item)
*/
void
SoBaseList::set(const int i, SoBase * const ptr)
{
  if (this->referencing) {
    // Note: it's important to ref() before we unref(), in case the
    // value of ptr is equal to the old value (otherwise we could
    // unref() it to zero, which causes premature destruction).
    if (ptr) ptr->ref();
    if (GET_BASEPTR(i)) GET_BASEPTR(i)->unref();
  }
  SbPList::set(i, (void *)ptr);
}


#undef GET_BASEPTR

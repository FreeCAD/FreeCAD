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

#include <Inventor/lists/SoTypeList.h>

/*!
  \class SoTypeList SoTypeList.h Inventor/lists/SoTypeList.h
  \brief The SoTypeList class is a container class for arrays of SoType objects.

  \ingroup coin_general

  \sa SbList
*/

/*!
  \fn SoTypeList::SoTypeList(void)

  Default constructor.
*/

/*!
  \fn SoTypeList::SoTypeList(const int sizehint)

  This constructor initializes the internal allocated size for the
  list to \a sizehint. Note that the list will still initially contain
  zero items.

  \sa SbList::SbList(const int sizehint)
*/

/*!
  \fn SoTypeList::SoTypeList(const SoTypeList & l)

  Copy constructor.

  \sa SbList::SbList(const SbList<Type> & l)
*/

/*!
  \copydetails SbPList::append(void * item)

  Overloaded from parent to accept an SoType pointer argument.

  \sa SbPList::append()
*/
void
SoTypeList::append(const SoType type)
{
  // need a temporary variable since not all compilers will let you
  // cast directly from an int16 to a void *.
  const uintptr_t tmp = (uintptr_t)type.getKey();
  SbPList::append((void*) tmp);
}

/*!
  \copydetails SbPList::find(const void * item) const

  Overloaded from parent to accept an SoType argument.

  \sa SbPList::find()
*/
int
SoTypeList::find(const SoType type) const
{
  // need a temporary variable since not all compilers will let you
  // cast directly from an int16 to a void *.
  const uintptr_t tmp = (uintptr_t)type.getKey();
  return SbPList::find((void*) tmp);
}

/*!
  \copydetails SbPList::insert(void * item, const int insertbefore)

  Overloaded from parent to accept an SoType argument.

  \sa SbPList::insert()
*/
void
SoTypeList::insert(const SoType type, const int insertbefore)
{
  // need a temporary variable since not all compilers will let you
  // cast directly from an int16 to a void *.
  const uintptr_t tmp = (uintptr_t)type.getKey();
  SbPList::insert((void*) tmp, insertbefore);
}

/*!
  \copydetails SbPList::operator[](const int index) const

  Overloaded from parent to return an SoType instance.

  \sa SbPList::operator[]()
*/
SoType
SoTypeList::operator[](const int idx) const
{
  // need a temporary variable since not all compilers will let you
  // cast directly from a pointer to an int16.
  const uintptr_t tmp = (uintptr_t) SbPList::operator[](idx);

  return SoType::fromKey((int16_t) tmp);
}

/*!
  \copydetails SbPList::set(const int index, void * item)

  Overloaded from parent to accept an SoType argument.

  \sa SbPList::set()
*/
void
SoTypeList::set(const int index, const SoType item)
{
  // need a temporary variable since not all compilers will let you
  // cast directly from an int16 to a void *.
  const uintptr_t tmp = (uintptr_t) item.getKey();
  SbPList::set(index, (void*) tmp);
}

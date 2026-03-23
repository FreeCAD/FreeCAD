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

#include <Inventor/lists/SbVec3fList.h>

/*!
  \class SbVec3fList SbVec3fList.h Inventor/lists/SbVec3fList.h
  \brief The SbVec3fList class is a container for pointers to SbVec3f objects.

  \ingroup coin_base

  Note that upon using the equality and inequality operators, the
  SbVec3f objects themselves are not compared, only the pointer
  values.

  Note also that all calls to append() and insert() will cause the
  list to allocate a new SbVec3f object. These objects are freed
  when the list is destructed.

  \sa SbList
*/



/*!
  \fn SbVec3fList::SbVec3fList(void)

  Default constructor.
*/

/*!
  Destructor.
*/

SbVec3fList::~SbVec3fList()
{
  for (int i = 0; i < this->getLength(); i++) {
    delete (*this)[i];
  }
}

/*!
  \fn void SbVec3fList::append(const SbVec3f * item)

  \copydetails SbPList::append()

  Overloaded from parent to allocate a new SbVec3f instance when
  called.

  \sa SbPList::append()
*/

/*!
  \fn void SbVec3fList::insert(const SbVec3f * item, const int insertbefore)

  \copydetails SbPList::insert(void * item, const int insertbefore)

  Overloaded from parent to allocate a new SbVec3f instance when
  called.

  \sa SbPList::insert()
*/

/*!
  \fn SbVec3f * SbVec3fList::operator[](const int idx) const

  \copydetails SbPList::operator[](const int index) const

  Overloaded from parent to return an SbVec3f pointer.

  \sa SbPList::operator[]()
*/

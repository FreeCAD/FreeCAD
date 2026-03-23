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
  \class SbStringList SbStringList.h Inventor/lists/SbStringList.h
  \brief The SbStringList class is a container for pointers to SbString objects.

  \ingroup coin_base

  Note that upon using the equality and inequality operators, the
  strings themselves are not compared, only the pointer values.  
  
  This class does not allocate or deallocate strings. It's the callers
  responsibility to allocate/deallocate the SbString instances.

  \sa SbPList
*/

#include <Inventor/lists/SbStringList.h>

/*!
  \fn SbStringList::SbStringList(void)

  Default constructor.
*/

/*!
  \fn SbStringList::SbStringList(const int sizehint)

  This constructor initializes the internal allocated size for the
  list to \a sizehint. Note that the list will still initially contain
  zero items.

  \sa SPbList::SbList(const int sizehint)
*/

/*!
  \fn void SbStringList::append(SbString * string)

  \copydetails SbPList::append(void * item)

  Overloaded from parent to accept an SbString pointer argument.

  \sa SbPList::append()
*/

/*!
  \fn int SbStringList::find(SbString * string) const

  \copydetails SbPList::find(const void * item) const

  Overloaded from parent to accept an SbString pointer argument.

  \sa SbPList::find()
*/

/*!
  \fn void SbStringList::insert(SbString * string, int insertbefore)

  \copydetails SbPList::insert(void * item, const int insertbefore)

  Overloaded from parent to accept an SbString pointer argument.

  \sa SbPList::insert()
*/

/*!
  \fn SbString *& SbStringList::operator[](const int idx) const

  \copydetails SbPList::operator[](const int index) const

  Overloaded from parent to return an SbString pointer.

  \sa SbPList::operator[]()
*/ 

/*!
  \fn const SbString ** SbStringList::getArrayPtr(void) const 

  \copydetails SbPList::getArrayPtr(const int start = 0) const

  Overloaded from parent to return an SbString pointer array.

  \sa SbPList::getArrayPtr()
*/

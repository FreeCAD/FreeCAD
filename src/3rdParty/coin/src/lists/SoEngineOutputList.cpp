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

#include <Inventor/lists/SoEngineOutputList.h>

/*!
  \class SoEngineOutputList SoEngineOutputList.h Inventor/lists/SoEngineOutputList.h
  \brief The SoEngineOutputList class is a container for SoEngineOutput objects.

  \ingroup coin_engines

  \sa SbList
*/

/*!
  \fn SoEngineOutputList::SoEngineOutputList(void)

  Default constructor.
*/

/*!
  \fn SoEngineOutputList::SoEngineOutputList(const int sizehint)

  This constructor initializes the internal allocated size for the
  list to \a sizehint. Note that the list will still initially contain
  zero items.
*/

/*!
  \fn SoEngineOutputList::SoEngineOutputList(const SoEngineOutputList & l)

  Copy constructor.
*/

/*!
  \fn void SoEngineOutputList::append(SoEngineOutput * output)

  \copydetails SbPList::append(void * item)

  Overloaded from parent to accept an SoEngineOutput pointer argument.

  \sa SbPList::append()
*/

/*!
  \fn void SoEngineOutputList::insert(SoEngineOutput * output, const int insertbefore)

  \copydetails SbPList::insert(void * item, const int insertbefore)

  Overloaded from parent to accept an SoEngineOutput pointer argument.

  \sa SbPList::insert()
*/

/*!
  \fn SoEngineOutput * SoEngineOutputList::operator[](const int idx) const

  \copydetails SbPList::operator[](const int index) const

  Overloaded from parent to return an SoEngineOutput pointer.

  \sa SbPList::operator[]()
*/


/*!
  \fn void SoEngineOutputList::set(const int index, SoEngineOutput * item)

  \copydetails SbPList::set(const int index, void * item)

  Overloaded from parent to destruct the replaced item.

  \sa SbPList::set()
*/

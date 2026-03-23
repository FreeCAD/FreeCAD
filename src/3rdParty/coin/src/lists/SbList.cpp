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
  \class SbList SbList.h Inventor/lists/SbList.h
  \brief The SbList class is a template container class for lists.

  \ingroup coin_base

  SbList is an extension of the Coin library versus the original Open
  Inventor API. Open Inventor handles most list classes by inheriting
  the SbPList class, which contains an array of generic \c void*
  pointers. By using this template-based class instead, we can share
  more code and make the list handling code more typesafe.

  Care has been taken to make sure the list classes which are part of
  the Open Inventor API to still be compatible with their original
  interfaces, as derived from the SbPList base class. But if you still
  bump into any problems when porting your Open Inventor applications,
  let us know and we'll do our best to sort them out.

  A feature with this class is that the list object arrays grow
  dynamically as you append() more items to the list.  The actual
  growing technique used is to double the list size when it becomes
  too small.

  There are also other array-related convenience methods; e.g. finding
  item indices, inserting items at any position, removing items (and
  shrink the array), copying of arrays, etc.

  \sa SbPList
*/


// FIXME: all methods on this class is now inlined. This probably adds
// quite a few (hundred) kBytes to the total size of the
// library. Several methods on this class should therefore be
// "de-inlined". The problem with this is that compilers seems to
// differ on whether or not subclasses or template instances then need
// to explicitly "declare themselves".  This is not too hard to fix,
// but it involves _some_ pain as it needs some nifty configure
// checking. 20000227 mortene.


/*!
  \fn SbList<Type>::SbList(const int sizehint)

  Default constructor.

  The \a sizehint argument hints about how many elements the list will
  contain, so memory allocation can be done efficiently.

  Important note: explicitly specifying an \a sizehint value does \e
  not mean that the list will initially contain this number of values.
  After construction, the list will contain zero items, just as for
  the default constructor. Here's a good example on how to give
  yourself hard to find bugs:

  \code
  SbList<SbBool> flags(2); // Assume we need only 2 elements. Note
                           // that the list is still 0 elements long.
  flags[0] = TRUE;         // Ouch. List is still 0 elements long.
  \endcode

  Since this conceptual misunderstanding is so easy to make, you're
  probably better (or at least safer) off leaving the \a sizehint
  argument to its default value by not explicitly specifying it.

  It improves performance if you know the approximate total size of
  the list in advance before adding list elements, as the number of
  reallocations will be minimized.
 */

/*!
  \fn SbList<Type>::SbList(const SbList<Type> & l)

  Copy constructor. Creates a complete copy of the given list.
 */

/*!
\fn SbList<Type>::~SbList()

  Destructor, frees all internal resources used by the list container.
*/

/*!
  \fn void SbList<Type>::copy(const SbList<Type> & l)

  Make this list a copy of \a l.
 */

/*!
  \fn SbList<Type> & SbList<Type>::operator=(const SbList<Type> & l)

  Make this list a copy of \a l.
 */

/*!
  \fn void SbList<Type>::fit(void)

  Fit the allocated array exactly around the length of the list,
  discarding memory spent on unused pre-allocated array cells.

  You should normally not need or want to call this method, and it is
  only available for the sake of having the option to optimize memory
  usage for the unlikely event that you should throw around huge
  SbList objects within your application.
 */

/*!
  \fn void SbList<Type>::append(const Type item)

  Append the \a item at the end of list, expanding the list array by
  one.
 */

/*!
  \fn int SbList<Type>::find(const Type item) const

  Return index of first occurrence of \a item in the list, or -1 if \a
  item is not present.
*/

/*!
  \fn void SbList<Type>::insert(const Type item, const int insertbefore)

  Insert \a item at index \a insertbefore.

  \a insertbefore should not be larger than the current number of
  items in the list.
 */


/*!
  \fn void SbList<Type>::removeItem(const Type item)

  Removes an \a item from the list. If there are several items with
  the same value, removes the \a item with the lowest index.
*/

/*!
  \fn void SbList<Type>::remove(const int index)

  Remove the item at \a index, moving all subsequent items downwards
  one place in the list.
*/

/*!
  \fn void SbList<Type>::removeFast(const int index)

  Remove the item at \a index, moving the last item into its place and
  truncating the list.
*/

/*!
  \fn int SbList<Type>::getLength(void) const

  Returns number of items in the list.
*/

/*!
 \fn void SbList<Type>::truncate(const int length, const int fit)

 Shorten the list to contain \a length elements, removing items from
 \e index \a length and onwards.

 If \a fit is non-zero, will also shrink the internal size of the
 allocated array. Note that this is much less efficient than not
 re-fitting the array size.
*/

/*!
  \fn void SbList<Type>::push(const Type item)

  This appends \a item at the end of the list in the same fashion as
  append() does. Provided as an abstraction for using the list class
  as a stack.
*/

/*!
  \fn Type SbList<Type>::pop(void)

  Pops off the last element of the list and returns it.
*/

/*!
  \fn const Type * SbList<Type>::getArrayPtr(const int start = 0) const

  Returns pointer to a non-modifiable array of the lists elements.
  \a start specifies an index into the array.

  The caller is \e not responsible for freeing up the array, as it is
  just a pointer into the internal array used by the list.
*/

/*!
  \fn Type SbList<Type>::operator[](const int index) const

  Returns a copy of item at \a index.
*/

/*!
  \fn Type & SbList<Type>::operator[](const int index)

  Returns a reference to item at \a index.
*/

/*!
  \fn SbBool SbList<Type>::operator==(const SbList<Type> & l) const

  Equality operator. Returns \c TRUE if this list and \a l are
  identical, containing the exact same set of elements.
*/

/*!
  \fn SbBool SbList<Type>::operator!=(const SbList<Type> & l) const

  Inequality operator. Returns \c TRUE if this list and \a l are not
  equal.
*/

/*!
  \fn void SbList<Type>::expand(const int size)

  Expand the list to contain \a size items. The new items added at the
  end have undefined value.
*/

/*!
  \fn int SbList<Type>::getArraySize(void) const

  Return number of items there's allocated space for in the array.

  \sa getLength()
*/

/*!
  \fn void SbList<Type>::ensureCapacity(const int size)

  Ensure that the internal buffer can hold at least \a size
  elements. SbList will automatically resize itself to make room for
  new elements, but this method can be used to improve performance
  (and avoid memory fragmentation) if you know approximately the
  number of elements that is going to be added to the list.
  
  \since Coin 2.5
*/

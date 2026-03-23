#ifndef COIN_SBLAZYPIMPLPTR_HPP
#define COIN_SBLAZYPIMPLPTR_HPP

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

#ifndef COIN_SBLAZYPIMPLPTR_H
#error do not include Inventor/tools/SbLazyPimplPtr.hpp directly, use Inventor/tools/SbLazyPimplPtr.h
#endif // !COIN_SBLAZYPIMPLPTR_H

/* ********************************************************************** */

template <typename T>
SbLazyPimplPtr<T>::SbLazyPimplPtr(void)
: ptr(NULL)
{
}

template <typename T>
SbLazyPimplPtr<T>::SbLazyPimplPtr(T * initial)
{
  this->ptr = initial;
}

template <typename T>
SbLazyPimplPtr<T>::SbLazyPimplPtr(const SbLazyPimplPtr<T> & copy)
{
  *this = copy;
}

template <typename T>
SbLazyPimplPtr<T>::~SbLazyPimplPtr(void)
{
  this->set(NULL);
}

template <typename T>
void
SbLazyPimplPtr<T>::set(T * value)
{
  delete this->ptr;
  this->ptr = value;
}

template <typename T>
T &
SbLazyPimplPtr<T>::get(void) const
{
  if (this->ptr == NULL) {
    this->ptr = this->getNew();
  }
  return *(this->ptr);
}

template <typename T>
T *
SbLazyPimplPtr<T>::getNew(void) const
{
  return new T;
}

template <typename T>
SbLazyPimplPtr<T> &
SbLazyPimplPtr<T>::operator = (const SbLazyPimplPtr<T> & copy)
{
  this->get() = copy.get();
  return *this;
}

template <typename T>
SbBool
SbLazyPimplPtr<T>::operator == (const SbLazyPimplPtr<T> & rhs) const
{
  return this->get() == rhs.get();
}

template <typename T>
SbBool
SbLazyPimplPtr<T>::operator != (const SbLazyPimplPtr<T> & rhs) const
{
  return !(*this == rhs);
}

template <typename T>
const T *
SbLazyPimplPtr<T>::operator -> (void) const
{
  return &(this->get());
}

template <typename T>
T *
SbLazyPimplPtr<T>::operator -> (void)
{
  return &(this->get());
}

/* ********************************************************************** */

#endif // !COIN_SBPIMPLPTR_HPP

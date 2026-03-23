#ifndef COIN_SCXMLCOMMONP_H
#define COIN_SCXMLCOMMONP_H

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

#include <cstring>
#include <vector>
#include <algorithm>

#include "coindefs.h"
#include "SbBasicP.h"

// *************************************************************************

#define SCXML__SET_ATTRIBUTE_VALUE(_ptr_, _name_, _value_)              \
  do {                                                                  \
    if ((_ptr_ != NULL) &&                                              \
        (_ptr_ != this->getXMLAttribute(_name_))) {                     \
      delete [] _ptr_;                                                  \
    }                                                                   \
    _ptr_ = NULL;                                                       \
    if (_value_ != NULL) {                                              \
      if (_value_ == this->getXMLAttribute(_name_)) {                   \
        _ptr_ = const_cast<char *>(_value_);                            \
      } else {                                                          \
        _ptr_ = new char [std::strlen(_value_) + 1];                    \
        std::strcpy(_ptr_, _value_);                                    \
      }                                                                 \
    }                                                                   \
  } while (FALSE)

#define SCXML__CLEAR_STD_VECTOR(_listobj_, _elementtype_)               \
  do {                                                                  \
    std::vector<_elementtype_>::iterator it = _listobj_.begin();        \
    while (it != _listobj_.end()) {                                     \
      delete *it;                                                       \
      ++it;                                                             \
    }                                                                   \
    _listobj_.clear();                                                  \
  } while ( FALSE )


#define SCXML_SINGLE_OBJECT_API_IMPL(classname, objtype, pointer, singular) \
void                                                                    \
classname::SO__CONCAT(set,singular)(objtype * obj)                      \
{                                                                       \
  pointer.reset(obj);                                                   \
  obj->setContainer(this);                                              \
}                                                                       \
                                                                        \
objtype *                                                               \
classname::SO__CONCAT(get,singular)(void) const                         \
{                                                                       \
  return pointer.get();                                                 \
}

#define SCXML_LIST_OBJECT_API_IMPL(classname, objtype, objlist, singular, plural) \
                                                                        \
int                                                                     \
classname::SO__CONCAT(getNum,plural)(void) const                        \
{                                                                       \
  return static_cast<int>(objlist.size());                              \
}                                                                       \
                                                                        \
objtype *                                                               \
classname::SO__CONCAT(get,singular)(int idx) const                      \
{                                                                       \
  assert(idx >= 0 && idx < static_cast<int>(objlist.size()));           \
  return objlist.at(idx);                                               \
}                                                                       \
                                                                        \
void                                                                    \
classname::SO__CONCAT(add,singular)(objtype * obj)                      \
{                                                                       \
  objlist.push_back(obj);                                               \
  obj->setContainer(this);                                              \
}                                                                       \
                                                                        \
void                                                                    \
classname::SO__CONCAT(remove,singular)(objtype * obj)                   \
{                                                                       \
  std::vector<objtype *>::iterator it =                                 \
    std::find(objlist.begin(), objlist.end(), obj);                     \
  assert(it != objlist.end());                                          \
  objlist.erase(it);                                                    \
  obj->setContainer(NULL);                                              \
}                                                                       \
                                                                        \
void                                                                    \
classname::SO__CONCAT(clearAll,plural)(void)                            \
{                                                                       \
  std::vector<objtype *>::iterator it = objlist.begin();                \
  while (it != objlist.end()) {                                         \
    (*it)->setContainer(NULL);                                          \
    ++it;                                                               \
  }                                                                     \
  objlist.clear();                                                      \
}

#endif // !COIN_SCXMLCOMMONP_H

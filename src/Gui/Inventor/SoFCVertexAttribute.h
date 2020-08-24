/****************************************************************************
 *   Copyright (c) 2020 Zheng, Lei (realthunder) <realthunder.dev@gmail.com>*
 *                                                                          *
 *   This file is part of the FreeCAD CAx development system.               *
 *                                                                          *
 *   This library is free software; you can redistribute it and/or          *
 *   modify it under the terms of the GNU Library General Public            *
 *   License as published by the Free Software Foundation; either           *
 *   version 2 of the License, or (at your option) any later version.       *
 *                                                                          *
 *   This library  is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *   GNU Library General Public License for more details.                   *
 *                                                                          *
 *   You should have received a copy of the GNU Library General Public      *
 *   License along with this library; see the file COPYING.LIB. If not,     *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,          *
 *   Suite 330, Boston, MA  02111-1307, USA                                 *
 *                                                                          *
 ****************************************************************************/

#ifndef FC_VERTEX_ATTRIBUTE_H
#define FC_VERTEX_ATTRIBUTE_H

#include <algorithm>
#include "SoFCVBO.h"

template<class T>
class SoFCVertexAttribute : protected SoFCVBO 
{
  typedef Gui::CoinPtr<SoFCVertexAttribute<T> > Pointer;
  typedef std::unordered_map<SbFCUniqueId, std::vector<SoFCVertexAttribute*> > Table;

public:
  SoFCVertexAttribute(SbFCUniqueId dataid,
                      SoFCVertexAttribute<T> *proxy,
                      const GLenum target = GL_ARRAY_BUFFER,
                      const GLenum usage = GL_STATIC_DRAW)
      :SoFCVBO(target, usage),
       refcount(0),
       len(0),
       attachtypesize(0)
  {
    this->dataid = dataid;
    if (proxy) {
      while (proxy->proxy)
        proxy = proxy->proxy;
      if (proxy->isAttached())
        this->proxy = proxy;
    }
  }

  SoFCVertexAttribute(const SoFCVertexAttribute & rhs) = delete;
  SoFCVertexAttribute & operator = (const SoFCVertexAttribute & rhs) = delete;

  SbFCUniqueId getBufferDataId(void) const { return this->dataid; }

  SbBool isAttached() const { return this->data != nullptr; }

  void ref(void) { ++refcount; }

  void unref(void) {
    assert(refcount);
    if (--refcount == 0)
      delete this;
  }

  void bindBuffer(SoState *state, uint32_t contextid) {
    assert(isAttached());
    if (this->proxy)
      this->proxy->bindBuffer(state, contextid);
    else
      this->SoFCVBO::bindBuffer(state, contextid);
  }

  int getLength() const { return len; }

  const T * getArrayPtr(const int start = 0) const {
    if (this->proxy)
      return this->proxy->getArrayPtr(start);
    return &this->array[start];
  }

  T * getWritableArrayPtr(int start = 0) {
    assert (!isAttached());
    if (this->proxy) {
      this->len = this->proxy->getLength();
      this->array = this->proxy->array;
      this->proxy.reset();
    }
    return &this->array[start];
  }

  const T & operator[](const int index) const {
    if (this->proxy)
      return (*this->proxy)[index];
    return this->array[index];
  }

  T & operator[](const int index) {
    if (this->proxy)
      return (*this->proxy)[index];
    return this->array[index];
  }

  bool operator==(const SoFCVertexAttribute<T> & l) const {
    if (this == &l)
      return true;
    if (this->len != l.len
        || this->target != l.target
        || this->usage != l.usage
        || this->attachtypesize != l.attachtypesize)
      return false;
    if (this->proxy) {
      if (l.proxy)
        return this->proxy->array == l.proxy->array;
      return this->proxy->array == l.array;
    }
    else if (l.proxy)
      return this->array == l.proxy->array;
    return this->array == l.array;
  }

  int operator!=(const SoFCVertexAttribute<T> & l) const {
    return !(*this == l);
  }

  void copyFromProxy()
  {
    assert(!isAttached());
    if (!this->proxy)
      return;
    assert(this->array.empty());
    auto it = this->proxy->array.begin();
    this->array.insert(this->array.end(), it, it + this->len);
    this->proxy.reset();
  }

  void append(const T & item)
  {
    if (this->proxy) {
      if (this->proxy->len > this->len
          && (*this->proxy)[this->len] == item)
      {
        ++this->len;
        return;
      }
      copyFromProxy();
    }
    ++this->len;
    this->array.push_back(item);
  }

  SoFCVertexAttribute<T> * attach()
  {
    assert(!isAttached());
    if (this->proxy) {
      assert(this->proxy->isAttached());
      if (this->proxy->attachtypesize != sizeof(T)) {
        copyFromProxy();
        return attach();
      }
      this->attachtypesize = sizeof(T);
    }
    else {
      this->attachtypesize = sizeof(T);
      SoFCVertexAttribute<T> * res = find();
      if (res)
        return res;
    }
    registerAndAttach();
    return this;
  }

  template<class L>
  SoFCVertexAttribute<T> * attachAndConvert()
  {
    assert(!isAttached());
    if (this->proxy) {
      assert(this->proxy->isAttached());
      if (this->proxy->attachtypesize == sizeof(L)) {
        this->attachtypesize = sizeof(L);
        registerAndAttach();
        return this;
      }
      copyFromProxy();
    }
    this->attachtypesize = sizeof(L);
    SoFCVertexAttribute<T> * res = find();
    if (res)
      return res;
    L * dst = (L *)this->allocBufferData(this->getLength() * sizeof(L), this->dataid);
    const T * src = this->getArrayPtr();
    for (int i=0; i<this->getLength(); ++i)
      dst[i] = static_cast<L>(src[i]);
    this->table[this->dataid].emplace_back(this);
    return this;
  }

private:
  void registerAndAttach() {
    this->setBufferData(this->getArrayPtr(), this->len * this->attachtypesize, this->dataid);
    this->table[this->dataid].emplace_back(this);
  }

  SoFCVertexAttribute<T> * find() {
    for (auto & p : this->table[this->dataid]) {
      if (*p == *this)
        return p;
    }
    return nullptr;
  }

  const GLvoid *getBufferData() const {
    if (this->proxy)
      return this->proxy->getBufferData();
    return this->data;
  }

  // support for CoinPtr
  friend inline void intrusive_ptr_add_ref(SoFCVertexAttribute<T> * obj) { obj->ref(); }
  friend inline void intrusive_ptr_release(SoFCVertexAttribute<T> * obj) { obj->unref(); }

  virtual ~SoFCVertexAttribute() {
    assert(refcount == 0);
    if (!this->data)
      return;
    auto it = this->table.find(this->dataid);
    if (it == this->table.end())
      return;
    for (auto iter=it->second.begin(); iter!=it->second.end(); ++iter) {
      if (*iter == this) {
        it->second.erase(iter);
        if (it->second.empty())
          this->table.erase(it);
        break;
      }
    }
  }

private:
  int refcount;
  Pointer proxy;
  std::vector<T> array;
  int len;
  int attachtypesize;
  static FC_COIN_THREAD_LOCAL Table table;
};

template<class T>
typename SoFCVertexAttribute<T>::Table SoFCVertexAttribute<T>::table;

#endif //FC_VERTEX_ATTRIBUTE_H
// vim: noai:ts=2:sw=2

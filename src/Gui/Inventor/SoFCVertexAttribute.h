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

#include <cstring>
#include <algorithm>
#include "COWData.h"
#include "SoFCVBO.h"

template<class T, class L=T>
class SoFCVertexAttribute 
{
  struct VBOKey {
    GLenum target;
    GLenum usage;
    const void *data;
    intptr_t size;

    VBOKey(GLenum t, GLenum u, const void *d, intptr_t s)
      :target(t), usage(u), data(d), size(s)
    {}

    bool operator<(const VBOKey &other) const {
      if (target < other.target)
        return true;
      if (target > other.target)
        return false;
      if (usage < other.usage)
        return true;
      if (usage > other.usage)
        return false;
      if (size < other.size)
        return true;
      if (size > other.size)
        return false;
      if (data == other.data)
        return false;
      return std::memcmp(data, other.data, size) < 0;
    }
  };

  struct VBOEntry {
    SoFCVBOData<T, L> *vbo;
    FC_COIN_COUNTER(int) refcount;
    VBOEntry()
      :vbo(nullptr)
      ,refcount(0)
    {}
  };
  typedef SbFCMap<VBOKey, VBOEntry> Table;

public:
  SoFCVertexAttribute(const GLenum target = GL_ARRAY_BUFFER,
                      const GLenum usage = GL_STATIC_DRAW)
      :len(0),
       key(target, usage, nullptr, 0),
       entry(nullptr)
  {
  }

  SoFCVertexAttribute(const SoFCVertexAttribute & other)
    :array(other.array)
    ,len(other.len)
    ,key(other.key)
    ,entry(other.entry)
  {
    if (this->entry)
      ++this->entry->refcount;
  }

  SoFCVertexAttribute & operator=(const SoFCVertexAttribute & other) {
    if (this == &other)
      return *this;
    detach();
    this->array = other.array;
    this->len = other.len;
    this->key = other.key;
    this->entry = other.entry;
    if (this->entry)
      ++this->entry->refcount;
    return *this;
  }

  virtual ~SoFCVertexAttribute() {
    detach();
  }

  GLenum getTarget() const {
    return this->key.target;
  }

  GLenum getUsage() const {
    return this->key.usage;
  }

  void init(const SoFCVertexAttribute & other) {
    *this = other;
    this->len = 0;
  }

  SbBool isAttached() const {
    return this->len && this->entry;
  }

  void detach() {
    if (!this->entry) return;
    if (--this->entry->refcount == 0) {
      auto vbo = this->entry->vbo;
      table.erase(this->key);
      // make sure to delete vbo after erase, because key.data may actually be
      // stored in the vbo.
      delete vbo;
    }
    this->entry = nullptr;
  }

  void bindBuffer(SoState *state, uint32_t contextid) {
    attach();
    if (this->entry) {
      this->entry->vbo->bindBuffer(state, contextid);
    }
  }

  int getLength() const {
    return this->len;
  }

  explicit operator bool() const {
    return this->len>0;
  }

  void truncate(int len = 0) {
    if (len == this->len)
      return;
    detach();
    if (len < 0)
      len = 0;
    if (len <= this->array.size())
      this->len = len;
    else
      this->len = this->array.size();
  }

  typename SbFCVector<T>::const_iterator begin() const {
    return this->array.getData().begin();
  }

  typename SbFCVector<T>::const_iterator end() const {
    assert(this->len <= this->array.size());
    return this->array.getData().begin() + this->len;
  }

  const T * getArrayPtr(const int start = 0) const {
    return &this->array[start];
  }

  T * getWritableArrayPtr(int start = 0) {
    detach();
    if (!this->len)
      this->len = this->array.size();
    else {
      assert(this->len > 0 && this->len <= this->array.size());
      this->array.resize(this->len);
    }
    return this->array.at(start);
  }

  const T & operator[](const int index) const {
    return this->array[index];
  }

  void set(int index, const T & item) {
    if (index == this->array.size()) {
      append(item);
      return;
    }
    assert(index >= 0 && index < this->array.size());
    if (this->array[index] != item) {
      detach();
      this->array.set(index, item);
    }
    if (this->len <= index)
      this->len = index + 1;
  }

  void append(const T & item)
  {
    assert(this->len <= this->array.size());
    if (this->len == this->array.size()) {
      detach();
      this->array.append(item);
    } else if (this->array[this->len] != item) {
      detach();
      this->array.set(this->len, item);
    }
    ++this->len;
  }

  void append(const SoFCVertexAttribute<T, L> &other) {
    if (!other)
      return;
    assert(this != &other);
    if (this->len == this->array.size()
        && other.len == other.array.size())
    {
      detach();
      this->array.append(other.array);
      this->len += other.array.size();
      return;
    }
    reserve(this->len + other.len);
    for (auto &v : other)
      append(v);
  }

  void reserve(int len) {
    if (this->array.reserve(len))
      detach();
  }

  bool attach(bool convert = false)
  {
    if (this->entry)
      return this->entry->vbo->isConverted();
     
    if (!this->len)
      return false;

    this->array.resize(this->len);
    this->key.data = &this->array[0];
    this->key.size = this->len * sizeof(T);
    auto it = table.insert(std::make_pair(this->key, VBOEntry())).first;
    this->key = it->first;
    auto &entry = it->second;
    ++entry.refcount;
    if (!entry.vbo) {
      entry.vbo = new SoFCVBOData<T, L>(this->key.target, this->key.usage);
      entry.vbo->setBufferData(this->array, convert);
    } else
      this->array = entry.vbo->getBufferData();
    this->entry = &entry;
    return entry.vbo->isConverted();
  }

private:
  COWVector<T> array;
  int len;
  VBOKey key;
  VBOEntry *entry;
  static FC_COIN_THREAD_LOCAL Table table;
};

template<class T, class L>
typename SoFCVertexAttribute<T, L>::Table SoFCVertexAttribute<T, L>::table;

#endif //FC_VERTEX_ATTRIBUTE_H
// vim: noai:ts=2:sw=2

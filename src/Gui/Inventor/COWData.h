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

#ifndef FC_COWDATA_H
#define FC_COWDATA_H

#include <memory>
#include <set>
#include <map>
#include <vector>
#include <typeinfo>
#include <typeindex>

// -------------------------------------------------------------

#define _FC_RENDER_MEM_TRACE
#ifdef _FC_RENDER_MEM_TRACE
struct SbFCMemUnit {
  int count;
  int maxcount;
};

struct SbFCMemUnitStats {
  static std::map<std::type_index, SbFCMemUnit> _MemUnits;
  static int64_t _MemSize;
  static int64_t _MemMaxSize;
};

template<typename T>
struct SoFCAllocator : std::allocator<T>, SbFCMemUnitStats {
  typedef typename std::allocator<T>::pointer pointer;
  typedef typename std::allocator<T>::size_type size_type;
  template<typename U> struct rebind { typedef SoFCAllocator<U> other; };

  SoFCAllocator() {}

  template<typename U>
  SoFCAllocator(const SoFCAllocator<U>& u) : std::allocator<T>(u) {}

  pointer allocate(size_type size, std::allocator<void>::const_pointer = 0) {
    void* p = std::malloc(size * sizeof(T));
    if(p == 0)
      throw std::bad_alloc();
    _MemSize += size * sizeof(T);
    if (_MemSize > _MemMaxSize)
      _MemMaxSize = _MemSize;
    auto &unit = _MemUnits[std::type_index(typeid(T))];
    unit.count += size;
    if (unit.count > unit.maxcount)
      unit.maxcount = unit.count;
    return static_cast<pointer>(p);
  }
  void deallocate(pointer p, size_type size) {
    _MemSize -= size * sizeof(T);
    _MemUnits[std::type_index(typeid(T))].count -= size;
    std::free(p);
  }
};

template<class KeyT, class ValueT>
using SbFCMap = std::map<KeyT, ValueT, std::less<KeyT>, SoFCAllocator<std::pair<KeyT,ValueT>>>;

template<class ValueT>
using SbFCSet = std::set<ValueT, std::less<ValueT>, SoFCAllocator<ValueT>>;

template<class ValueT>
using SbFCVector = std::vector<ValueT, SoFCAllocator<ValueT>>;

#else // _FC_RENDER_MEM_TRACE

template<class KeyT, class ValueT>
using SbFCMap = std::map<KeyT, ValueT>;

template<class ValueT>
using SbFCSet = std::set<ValueT>;

template<class ValueT>
using SbFCVector = std::vector<ValueT>;

template<class T>
using SoFCAllocator = std::allocator<T>;

#endif //_FC_RENDER_MEM_TRACE

// -------------------------------------------------------------
// simple copy on write template container, NOT thread safe
template<class DataT, class ValueT>
class COWData {
public:
  typedef SoFCAllocator<DataT> AllocatorT;

  int getNum() const {
    return this->data ? static_cast<int>(this->data->size()) : 0;
  }

  int size() const {
      return getNum();
  }

  bool empty() const {
      return getNum() == 0;
  }

  explicit operator bool() const {
      return getNum() != 0;
  }

  void clear() {
    if (!this->data) return;
    if (this->data.use_count() == 1) this->data->clear();
    else this->data.reset();
  }

  void reset() {
    this->data.reset();
  }

  void detach() {
    if (!this->data) return;
    if (this->data.use_count() > 1)
      this->data = std::allocate_shared<DataT>(AllocatorT(), *this->data);
  }

  bool operator<(const COWData<DataT,ValueT> & other) const {
    if (size() < other.size())
      return true;
    if (size() > other.size())
      return false;
    if (this->data == other.data)
      return false;
    if (!this->data)
      return true;
    if (!other.data)
      return false;
    return *this->data < *other.data;
  }

  bool operator>(const COWData<DataT,ValueT> & other) const {
    if (size() < other.size())
      return false;
    if (size() > other.size())
      return true;
    if (this->data == other.data)
      return false;
    if (!this->data)
      return false;
    if (!other.data)
      return true;
    return *this->data > *other.data;
  }

  bool operator==(const COWData<DataT,ValueT> & other) const {
    if (this->data == other.data)
      return true;
    if (!this->data || !other.data)
      return false;
    return *this->data == *other.data;
  }

  bool operator!=(const COWData<DataT,ValueT> & other) const {
    return !operator==(other);
  }

  void copy(const DataT & other) {
    if (this->data.get() != &other) {
      if (!this->data || this->data.use_count() > 1)
        this->data = std::allocate_shared<DataT>(AllocatorT(), other);
      else
        *this->data = other;
    }
  }

  void move(DataT && other) {
    if (this->data.get() != &other) {
      if (!this->data || this->data.use_count() > 1)
        this->data = std::allocate_shared<DataT>(AllocatorT());
      *this->data = std::move(other);
    }
  }

  const DataT & getData() const {
    assert(this->data);
    return *this->data;
  }

protected:
  std::shared_ptr<DataT> data;
};


// -------------------------------------------------------------
// simple copy on write template map, NOT thread safe
template<class KeyT, class ValueT, class MapT = SbFCMap<KeyT, ValueT>>
class COWMap: public COWData<MapT, ValueT> {
public:
  typedef SoFCAllocator<MapT> AllocatorT;

  const ValueT * get(const KeyT &key) const {
    if (!this->data)
        return nullptr;
    auto it = this->data->find(key);
    if (it == this->data->end())
      return nullptr;
    return &it->second;
  }

  void erase(const KeyT &key) {
    if (!this->data) return;
    auto it = this->data->find(key);
    if (it == this->data->end()) return;
    if (this->data.use_count() == 1) {
      this->data->erase(key);
      return;
    }
    if (this->data->size() == 1) {
      this->data.reset();
      return;
    }
    auto begin = this->data->begin();
    auto end = this->data->end();
    this->data = std::allocate_shared<MapT>(AllocatorT());
    this->data->insert(begin, it);
    this->data->insert(it, end);
  }

  void set(const KeyT &key, const ValueT &value, bool overwrite=true) {
    if (!this->data)
      this->data = std::allocate_shared<MapT>(AllocatorT());
    else {
      auto it = this->data->find(key);
      if (it != this->data->end() && (!overwrite || it->second == value)) return;
      if (this->data.use_count() > 1)
        this->data = std::allocate_shared<MapT>(AllocatorT(),*this->data);
      else if (it != this->data->end()) {
        it->second = value;
        return;
      }
    }
    this->data->operator[](key) = value;
  }

  void add(const COWMap<KeyT, ValueT, MapT> &other, bool overwrite=true) {
    if (!other.data || other.data == this->data) return;
    for (auto & v : *other.data)
      set(v.first, v.second, overwrite);
  }

  void combine(const KeyT &key, const ValueT &value) {
    bool found = false;
    if (!this->data) 
      this->data = std::allocate_shared<MapT>(AllocatorT());
    else {
      auto it = this->data->find(key);
      found = it != this->data->end();
      if (this->data.use_count() > 1)
        this->data = std::allocate_shared<MapT>(AllocatorT(),*this->data);
      else if (!found) {
        it->second.combine(value);
        return;
      }
    }
    if (found)
      this->data->operator[](key).combine(value);
    else
      this->data->emplace(key, value);
  }

  void combine(const COWMap<KeyT, ValueT, MapT> &other) {
    if (!other.data || other.data == this->data) return;
    for (auto & v : *other.data)
      combine(v.first, v.second);
  }
};

// -------------------------------------------------------------
// simple copy on write vector, NOT thread safe
template<class ValueT, class VectorT = SbFCVector<ValueT>>
class COWVector: public COWData<VectorT, ValueT> {
public:
  typedef SoFCAllocator<VectorT> AllocatorT;

  const VectorT &getData() const {
    static VectorT dummy;
    return this->data ? *this->data : dummy;
  }

  typename VectorT::const_iterator begin() const {
    return this->getData().begin();
  }

  typename VectorT::const_iterator end() const {
    return this->getData().end();
  }

  const ValueT & get(int idx) const {
    assert(idx >= 0 && idx < this->size());
    return (*this->data)[idx];
  }

  ValueT * at(int idx) {
    assert(idx >= 0 && idx < this->size());
    if (this->data.use_count() > 1)
      this->data = std::allocate_shared<VectorT>(AllocatorT(), *this->data);
    return &this->data->at(idx);
  }

  const ValueT & operator[](int idx) const {
    return this->get(idx);
  }

  void set(int idx, const ValueT &v) {
    if (this->data.use_count() > 1)
      this->data = std::allocate_shared<VectorT>(AllocatorT(), *this->data);
    (*this->data)[idx] = v;
  }

  bool compareAndSet(int idx, const ValueT &v) {
    if (idx == this->size()) {
      append(v);
      return true;
    }
    assert(idx >= 0 && idx < this->size());
    if (get(idx) != v) {
      set(idx, v);
      return true;
    }
    return false;
  }

  void erase(int idx) {
    assert(idx >= 0 && idx < this->size());
    if (this->data.use_count() > 1)
      this->data = std::allocate_shared<VectorT>(AllocatorT(), *this->data);
    this->data->erase(this->data->begin() + idx);
  }

  void append(const ValueT &value) {
    if (!this->data)
      this->data = std::allocate_shared<VectorT>(AllocatorT());
    else if (this->data.use_count() > 1)
      this->data = std::allocate_shared<VectorT>(AllocatorT(), *this->data);
    this->data->push_back(value);
  }

  void push_back(const ValueT &value) {
    append(value);
  }

  const ValueT back() const {
    assert(!this->empty());
    return this->data->back();
  }

  const ValueT front() const {
    assert(!this->empty());
    return this->data->front();
  }

  bool reserve(int size) {
    if (size <= 0)
      return false;
    if (!this->data)
      this->data = std::allocate_shared<VectorT>(AllocatorT());
    else if (size <= (int)this->data->capacity())
      return false;
    else if (this->data.use_count() > 1)
      this->data = std::allocate_shared<VectorT>(AllocatorT(), *this->data);
    this->data->reserve(size);
    return true;
  }

  void resize(int size) {
    if (size < 0)
      return;
    if (!this->data) {
      if (!size) return;
      this->data = std::allocate_shared<VectorT>(AllocatorT());
    } else if (size == (int)this->data->size())
      return;
    else if (this->data.use_count() > 1)
      this->data = std::allocate_shared<VectorT>(AllocatorT(), *this->data);
    this->data->resize(size);
  }

  void append(const COWVector<ValueT, VectorT> &other) {
    if (!other.data) return;
    if (!this->data) {
      this->data = other.data;
      return;
    }
    if (this->data.use_count() > 1)
      this->data = std::allocate_shared<VectorT>(AllocatorT(), *this->data);
    this->data->insert(this->data->end(), other.data->begin(), other.data->end());
  }
};

#endif // FC_COWDATA_H
// vim: noai:ts=2:sw=2

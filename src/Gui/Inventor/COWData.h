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

// -------------------------------------------------------------
// simple copy on write template container, NOT thread safe
template<class DataT, class ValueT>
class COWData {
public:
  int getNum() const {
    return this->data ? static_cast<int>(this->data->size()) : 0;
  }

  void clear() {
    if (!this->data) return;
    if (this->data.use_count() == 1) this->data->clear();
    else this->data.reset();
  }

  void detach() {
    if (!this->data) return;
    if (this->data.use_count() > 1)
      this->data = std::make_shared<DataT>(*this->data);
  }

  bool operator<(const COWData<DataT,ValueT> & other) const {
    if (this->data == other.data)
      return false;
    if (!this->data)
      return true;
    if (!other.data)
      return false;
    return *this->data < *other.data;
  }

  bool operator>(const COWData<DataT,ValueT> & other) const {
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

  COWData<DataT,ValueT> & operator=(const COWData<DataT,ValueT> & other) {
    this->data = other.data;
    return *this;
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
template<class MapT>
class COWMap: public COWData<MapT, typename MapT::mapped_type> {
public:
  typedef typename MapT::key_type KeyT;
  typedef typename MapT::mapped_type ValueT;

  const ValueT * get(int unit) const {
    auto it = this->data->find(unit);
    if (it == this->data->end())
      return NULL;
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
    this->data = std::make_shared<MapT>();
    this->data->insert(begin, it);
    this->data->insert(it, end);
  }

  void set(const KeyT &key, const ValueT &value, bool overwrite=true) {
    if (!this->data)
      this->data = std::make_shared<MapT>();
    else {
      auto it = this->data->find(key);
      if (it != this->data->end() && (!overwrite || it->second == value)) return;
      if (this->data.use_count() > 1)
        this->data = std::make_shared<MapT>(*this->data);
      else if (it != this->data->end()) {
        it->second = value;
        return;
      }
    }
    this->data->operator[](key) = value;
  }

  void add(const COWMap<MapT> &other, bool overwrite=true) {
    if (!other.data || other.data == this->data) return;
    for (auto & v : *other.data)
      set(v.first, v.second, overwrite);
  }

  void combine(const KeyT &key, const ValueT &value) {
    bool found = false;
    if (!this->data) 
      this->data = std::make_shared<MapT>();
    else {
      auto it = this->data->find(key);
      found = it != this->data->end();
      if (this->data.use_count() > 1)
        this->data = std::make_shared<MapT>(*this->data);
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

  void combine(const COWMap<MapT> &other) {
    if (!other.data || other.data == this->data) return;
    for (auto & v : *other.data)
      combine(v.first, v.second);
  }
};

// -------------------------------------------------------------
// simple copy on write vector, NOT thread safe
template<class VectorT>
class COWVector: public COWData<VectorT, typename VectorT::value_type> {
public:
  typedef typename VectorT::value_type ValueT;

  const ValueT & get(int idx) const {
    return this->data[idx];
  }

  void erase(int idx) const {
    if (!this->data) return;
    if (idx < 0 || idx >= this->getNum()) return;
    if (this->data.use_count() > 1)
      this->data = std::make_shared<VectorT>(*this->data);
    this->data->erase(this->data->begin() + idx);
  }

  void append(const ValueT &value) {
    if (!this->data)
      this->data = std::make_shared<VectorT>();
    else if (this->data.use_count() > 1)
      this->data = std::make_shared<VectorT>(*this->data);
    this->data->push_back(value);
  }

  void append(const COWVector<VectorT> &other) {
    if (!other.data || this->data == other.data) return;
    if (!this->data) {
      this->data = other.data;
      return;
    }
    if (this->data.use_count() > 1)
      this->data = std::make_shared<VectorT>(*this->data);
    this->data->insert(this->data->end(), other.data->begin(), other.data->end());
  }
};

#endif // FC_COWDATA_H

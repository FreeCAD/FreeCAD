#ifndef CCommandTreeNode_HPP_INCLUDED
#define CCommandTreeNode_HPP_INCLUDED
// <copyright file="CCommandTreeNode.hpp" company="3Dconnexion">
// ------------------------------------------------------------------------------------------------
// This file is part of the FreeCAD CAx development system.
//
// Copyright (c) 2014-2023 3Dconnexion.
//
// This source code is released under the GNU Library General Public License, (see "LICENSE").
// ------------------------------------------------------------------------------------------------
// </copyright>
// <history>
// ************************************************************************************************
// File History
//
// $Id: CCommandTreeNode.hpp 16056 2019-04-10 13:42:31Z mbonk $
//
// </history>
#include <SpaceMouse/CActionNode.hpp>

// stdlib
#include <exception>
#include <stdexcept>
#include <iterator>
#include <memory>

#ifndef _MSC_VER
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshadow"
#endif

namespace TDx {
template <typename Ty_, bool doubled_linked = false> class raw_linkedlist_iterator {
public:
  typedef std::input_iterator_tag iterator_category;
  typedef Ty_ value_type;
  typedef ptrdiff_t difference_type;
  typedef Ty_ *pointer;
  typedef Ty_ &reference;

public:
  raw_linkedlist_iterator(Ty_ *ptr_ = nullptr) : MyPtr_(ptr_) {
  }

  raw_linkedlist_iterator(const raw_linkedlist_iterator<Ty_> &_other) : MyPtr_(_other.MyPtr_) {
  }

  raw_linkedlist_iterator<Ty_> &operator=(const raw_linkedlist_iterator<Ty_> &_other) {
    MyPtr_ = _other.MyPtr_;
    return *this;
  }

  raw_linkedlist_iterator<Ty_> &operator=(Ty_ *ptr_) {
    MyPtr_ = ptr_;
    return *this;
  }

  // accessors
  Ty_ &operator*() {
    return *MyPtr_;
  }
  Ty_ const &operator*() const {
    return *MyPtr_;
  }
  Ty_ *operator->() {
    return MyPtr_;
  }
  Ty_ const *operator->() const {
    return MyPtr_;
  }

  raw_linkedlist_iterator<Ty_> &operator++() {
    if (MyPtr_)
      MyPtr_ = MyPtr_->GetNext();
    return *this;
  }

  raw_linkedlist_iterator<Ty_> &operator--() {
    if (doubled_linked) {
      if (MyPtr_)
        MyPtr_ = MyPtr_->GetPrevious();
    } else {
      throw std::logic_error("Not Supported");
    }
    return *this;
  }

  bool operator<(raw_linkedlist_iterator<Ty_> const &rhs) const {
    if (!MyPtr_)
      return false;
    else if (!rhs.MyPtr_)
      return true;
    return (MyPtr_ < rhs.MyPtr_);
  }

  bool operator<=(raw_linkedlist_iterator<Ty_> const &rhs) const {
    if (MyPtr_ == rhs.MyPtr_)
      return true;
    return operator<(rhs);
  }

  bool operator>(raw_linkedlist_iterator<Ty_> const &rhs) const {
    return !operator<=(rhs);
  }

  bool operator>=(raw_linkedlist_iterator<Ty_> const &rhs) const {
    return !operator<(rhs);
  }

  operator bool() const {
    return MyPtr_ != nullptr;
  }

protected:
  Ty_ *MyPtr_;
};

/// <summary>
/// Tree container for CActionNode.
/// </summary>
/// <remarks>The tree is implemented as a singularly linked list.</remarks>
class CCommandTree : public CActionNode {
public:
  typedef CActionNode base_type;
  typedef CActionNode const const_base_type;
  typedef CActionNode const &const_base_ref_type;
  typedef CActionNode const *const_base_ptr_type;
  typedef CCommandTree self_type;

public:
  typedef CCommandTree &reference;
  typedef CCommandTree const &const_reference;

  typedef raw_linkedlist_iterator<base_type> iterator;
  typedef raw_linkedlist_iterator<const base_type> const_iterator;

  CCommandTree() {
  }

  explicit CCommandTree(std::string id, std::string label, std::string description,
                        SiActionNodeType_t type)
      : base_type(std::move(id), std::move(label), std::move(description), type) {
  }

  explicit CCommandTree(std::string id, std::string label, SiActionNodeType_t type)
      : base_type(std::move(id), std::move(label), type) {
  }

#if defined(_MSC_VER) && _MSC_VER < 1900
  CCommandTree(CCommandTree &&other) : base_type(std::forward<base_type>(other)) {
  }
  CCommandTree &operator=(CCommandTree &&other) {
    base_type::operator=(std::forward<base_type>(other));
    return *this;
  }

private:
  CCommandTree(const CCommandTree &) {
  }
  const CCommandTree &operator=(const CCommandTree &){};
#else
  CCommandTree(CCommandTree &&) = default;
  CCommandTree &operator=(CCommandTree &&) = default;
  // No copying
  CCommandTree(const CCommandTree &) = delete;
  const CCommandTree &operator=(const CCommandTree &) = delete;
#endif // defined(_MSC_VER) && _MSC_VER<1900

public:
  void push_back(base_type &&value) {
#if (defined(_MSC_VER) && _MSC_VER < 1900)
    std::unique_ptr<base_type> node(new base_type(std::forward<base_type>(value)));
#else
    std::unique_ptr<base_type> node = std::make_unique<base_type>(std::forward<base_type>(value));
#endif
    push_back(std::move(node));
  }

  template <class T>
  void
  push_back(std::unique_ptr<T> &&value,
            typename std::enable_if<std::is_base_of<CActionNode, T>::value>::type * = nullptr) {
    base_type *last = this->GetChild();
    if (!last) {
      PutChild(std::unique_ptr<base_type>(static_cast<base_type *>(value.release())));
    } else {
      while (last->GetNext() != nullptr) {
        last = last->GetNext();
      }
      last->PutNext(std::unique_ptr<base_type>(static_cast<base_type *>(value.release())));
    }
  }

  void push_front(base_type &&value) {
#if (defined(_MSC_VER) && _MSC_VER < 1900)
    std::unique_ptr<base_type> node(new base_type(std::forward<base_type>(value)));
#else
    std::unique_ptr<base_type> node = std::make_unique<base_type>(std::forward<base_type>(value));
#endif
    push_front(std::move(node));
  }

  void push_front(std::unique_ptr<base_type> &&value) {
    value->PutNext(std::unique_ptr<base_type>(DetachChild()));
    PutChild(std::forward<std::unique_ptr<base_type>>(value));
  }

  const_reference back() const {
    const base_type *last = this->GetChild();
    if (!last) {
      return *this;
    }

    while (last->GetNext() != nullptr) {
      last = last->GetNext();
    }
    return *static_cast<const self_type *>(last);
  }

  reference back() {
    base_type *last = this->GetChild();
    if (!last) {
      return *this;
    }

    while (last->GetNext() != nullptr) {
      last = last->GetNext();
    }
    return *static_cast<self_type *>(last);
  }

  void clear() {
    base_type *head = this->DetachChild();
    if (head) {
      head->clear();
    }
  }

  const_reference front() const {
    const base_type *head = this->GetChild();
    if (!head) {
      return *this;
    }
    return *static_cast<const self_type *>(head);
  }

  reference front() {
    base_type *head = this->GetChild();
    if (!head) {
      return *this;
    }
    return *static_cast<self_type *>(head);
  }

  const_iterator begin() const {
    if (empty()) {
      return end();
    }
    return const_iterator(this->GetChild());
  }

  iterator begin() {
    if (empty()) {
      return end();
    }
    return iterator(this->GetChild());
  }

  bool empty() const {
    return (this->GetChild() == nullptr);
  }

  const_iterator end() const {
    return nullptr;
  }

  iterator end() {
    return nullptr;
  }
};

typedef CCommandTree CCommandTreeNode;
} // namespace TDx

#ifndef _MSC_VER
#pragma GCC diagnostic pop
#endif

#endif // CCommandTreeNode_HPP_INCLUDED

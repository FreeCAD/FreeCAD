// Formatting library for C++ - mock allocator
//
// Copyright (c) 2012 - present, Victor Zverovich
// All rights reserved.
//
// For the license information refer to format.h.

#ifndef FMT_MOCK_ALLOCATOR_H_
#define FMT_MOCK_ALLOCATOR_H_

#include <assert.h>  // assert
#include <stddef.h>  // size_t

#include <memory>  // std::allocator_traits

#include "gmock/gmock.h"

template <typename T> class mock_allocator {
 public:
  mock_allocator() {}
  mock_allocator(const mock_allocator&) {}
  using value_type = T;
  MOCK_METHOD1_T(allocate, T*(size_t n));
  MOCK_METHOD2_T(deallocate, void(T* p, size_t n));
};

template <typename Allocator> class allocator_ref {
 private:
  Allocator* alloc_;

  void move(allocator_ref& other) {
    alloc_ = other.alloc_;
    other.alloc_ = nullptr;
  }

 public:
  using value_type = typename Allocator::value_type;

  explicit allocator_ref(Allocator* alloc = nullptr) : alloc_(alloc) {}

  allocator_ref(const allocator_ref& other) : alloc_(other.alloc_) {}
  allocator_ref(allocator_ref&& other) { move(other); }

  allocator_ref& operator=(allocator_ref&& other) {
    assert(this != &other);
    move(other);
    return *this;
  }

  allocator_ref& operator=(const allocator_ref& other) {
    alloc_ = other.alloc_;
    return *this;
  }

 public:
  Allocator* get() const { return alloc_; }

  value_type* allocate(size_t n) {
    return std::allocator_traits<Allocator>::allocate(*alloc_, n);
  }
  void deallocate(value_type* p, size_t n) { alloc_->deallocate(p, n); }
};

#endif  // FMT_MOCK_ALLOCATOR_H_

/***************************************************************************
    scoped_ptr - a not too smart pointer for exception safe heap objects
                       -------------------------
    begin                : May 2019
    copyright            : (C) 2019 Intermodalics
 
 ***************************************************************************
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Lesser General Public            *
 *   License as published by the Free Software Foundation; either          *
 *   version 2.1 of the License, or (at your option) any later version.    *
 *                                                                         *
 *   This library is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU     *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with this library; if not, write to the Free Software   *
 *   Foundation, Inc., 59 Temple Place,                                    *
 *   Suite 330, Boston, MA  02111-1307  USA                                *
 *                                                                         *
 ***************************************************************************/

#if (__cplusplus > 199711L)
#include <utility>
#else
#include <algorithm>
#endif

namespace KDL {

template<typename T> class scoped_ptr;

template<typename T> void swap(scoped_ptr<T>&, scoped_ptr<T>&);

template<typename T>
class scoped_ptr {
 public:
  scoped_ptr() : ptr_(0) { }
  explicit scoped_ptr(T* p) : ptr_(p) { }

  ~scoped_ptr() { delete ptr_; }

  T* operator->() { return ptr_; }
  const T* operator->() const { return ptr_; }

  T* get() const { return ptr_; }

  void reset(T* p = 0) {
    T* old = ptr_;
    ptr_ = p;
    delete old;
  }

  T* release() {
    T* old = ptr_;
    ptr_ = 0;
    return old;
  }

  friend void swap<>(scoped_ptr<T>& a, scoped_ptr<T>& b);

 private:
  scoped_ptr(const scoped_ptr&);  // not-copyable
  scoped_ptr& operator=(const scoped_ptr&); // not-copyable

  T* ptr_;
};

template<typename T>
void swap(scoped_ptr<T>& a, scoped_ptr<T>& b) {
  using std::swap;
  swap(a.ptr_, b.ptr_);
}


} // namespace KDL

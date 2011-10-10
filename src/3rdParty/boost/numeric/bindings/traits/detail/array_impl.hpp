/*
 * 
 * Copyright (c) Kresimir Fresl 2003
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * Author acknowledges the support of the Faculty of Civil Engineering, 
 * University of Zagreb, Croatia.
 *
 */

#ifndef BOOST_NUMERIC_BINDINGS_TRAITS_DETAIL_ARRAY_IMPL_HPP
#define BOOST_NUMERIC_BINDINGS_TRAITS_DETAIL_ARRAY_IMPL_HPP

/*
 very simple dynamic array class which is used in `higher level' 
 bindings functions for pivot and work arrays 

 Namely, there are (at least) two versions of all bindings functions 
 where called LAPACK function expects work and/or pivot array, e.g.
 
      `lower' level (user should provide work and pivot arrays):
           int sysv (SymmA& a, IVec& i, MatrB& b, Work& w);
 
      `higher' level (with `internal' work and pivot arrays):
           int sysv (SymmA& a, MatrB& b);
 
 Probably you ask why I didn't use std::vector. There are two reasons. 
 First is efficiency -- std::vector's constructor initialises vector 
 elements. Second is consistency. LAPACK functions use `info' parameter 
 as an error indicator. On the other hand, std::vector's allocator can
 throw an exception if memory allocation fails. detail::array's 
 constructor uses `new (nothrow)' which returns 0 if allocation fails. 
 So I can check whether array::storage == 0 and return appropriate error 
 in `info'.
 */

#include <new>
#include <boost/noncopyable.hpp> 

namespace boost { namespace numeric { namespace bindings { 

  namespace traits { namespace detail {

    template <typename T> 
    class array : private noncopyable {
    public:

      array (int n) {
        stg = new (std::nothrow) T[n]; 
        sz = (stg != 0) ? n : 0; 
      }
      ~array() { delete[] stg; }

      int size() const { return sz; }
      bool valid() const { return stg != 0; } 
      void resize (int n) {
        delete[] stg; 
        stg = new (std::nothrow) T[n]; 
        sz = (stg != 0) ? n : 0; 
      }

      T* storage() { return stg; }
      T const* storage() const { return stg; }

      T& operator[] (int i) { return stg[i]; }
      T const& operator[] (int i) const { return stg[i]; }

    private:
      int sz; 
      T* stg; 
    };

  }}

}}}

#endif 

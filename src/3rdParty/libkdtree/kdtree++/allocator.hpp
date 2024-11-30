/** \file
 * Defines the allocator interface as used by the KDTree class.
 *
 * \author Martin F. Krafft <libkdtree@pobox.madduck.net>
 */

#ifndef INCLUDE_KDTREE_ALLOCATOR_HPP
#define INCLUDE_KDTREE_ALLOCATOR_HPP

#include <cstddef>

#include "node.hpp"

namespace KDTree
{

  template <typename _Tp, typename _Alloc>
    class _Alloc_base
    {
    public:
      typedef _Node<_Tp> _Node_;
      typedef typename _Node_::_Base_ptr _Base_ptr;
      typedef _Alloc allocator_type;

      _Alloc_base(allocator_type const& __A)
        : _M_node_allocator(__A) {}

      allocator_type
      get_allocator() const
      {
        return _M_node_allocator;
      }


      class NoLeakAlloc
      {
         _Alloc_base * base;
         _Node_ * new_node;

      public:
         NoLeakAlloc(_Alloc_base * b) : base(b), new_node(base->_M_allocate_node()) {}

         _Node_ * get() { return new_node; }
         void disconnect() { new_node = NULL; }

         ~NoLeakAlloc() { if (new_node) base->_M_deallocate_node(new_node); }
      };


    protected:
      allocator_type _M_node_allocator;
      
      _Node_*
      _M_allocate_node()
      {
        return _M_node_allocator.allocate(1);
      }

      void
      _M_deallocate_node(_Node_* const __P)
      {
        return _M_node_allocator.deallocate(__P, 1);
      }

      void
      _M_construct_node(_Node_* __p, _Tp const __V = _Tp(),
                        _Base_ptr const __PARENT = NULL,
                        _Base_ptr const __LEFT = NULL,
                        _Base_ptr const __RIGHT = NULL)
      {
        new (__p) _Node_(__V, __PARENT, __LEFT, __RIGHT);
      }

      void
      _M_destroy_node(_Node_* __p)
      {
#if __cplusplus >= 201703L
        std::allocator_traits<allocator_type>::destroy(_M_node_allocator,__p);
#else
        _M_node_allocator.destroy(__p);
#endif
      }
    };

} // namespace KDTree

#endif // include guard

/* COPYRIGHT --
 *
 * This file is part of libkdtree++, a C++ template KD-Tree sorting container.
 * libkdtree++ is (c) 2004-2007 Martin F. Krafft <libkdtree@pobox.madduck.net>
 * and Sylvain Bougerel <sylvain.bougerel.devel@gmail.com> distributed under the
 * terms of the Artistic License 2.0. See the ./COPYING file in the source tree
 * root for more information.
 *
 * THIS PACKAGE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES
 * OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

/** \file
 * Defines interfaces for iterators as used by the KDTree class.
 *
 * \author Martin F. Krafft <libkdtree@pobox.madduck.net>
 */

#ifndef INCLUDE_KDTREE_ITERATOR_HPP
#define INCLUDE_KDTREE_ITERATOR_HPP

#include <iterator>

#include "node.hpp"

namespace KDTree
{
  template <typename _Val, typename _Ref, typename _Ptr>
    class _Iterator;

  template<typename _Val, typename _Ref, typename _Ptr>
    inline bool
    operator==(_Iterator<_Val, _Ref, _Ptr> const&,
               _Iterator<_Val, _Ref, _Ptr> const&);

  template<typename _Val>
    inline bool
    operator==(_Iterator<_Val, const _Val&, const _Val*> const&,
               _Iterator<_Val, _Val&, _Val*> const&);

  template<typename _Val>
    inline bool
    operator==(_Iterator<_Val, _Val&, _Val*> const&,
               _Iterator<_Val, const _Val&, const _Val*> const&);

  template<typename _Val, typename _Ref, typename _Ptr>
    inline bool
    operator!=(_Iterator<_Val, _Ref, _Ptr> const&,
               _Iterator<_Val, _Ref, _Ptr> const&);

  template<typename _Val>
    inline bool
    operator!=(_Iterator<_Val, const _Val&, const _Val*> const&,
               _Iterator<_Val, _Val&, _Val*> const&);

  template<typename _Val>
    inline bool
    operator!=(_Iterator<_Val, _Val&, _Val*> const&,
               _Iterator<_Val, const _Val&, const _Val*> const&);

  class _Base_iterator
  {
  protected:
    typedef _Node_base::_Base_const_ptr _Base_const_ptr;
    _Base_const_ptr _M_node;

    inline _Base_iterator(_Base_const_ptr const __N = NULL)
      : _M_node(__N) {}
    //inline _Base_iterator(_Base_iterator const& __THAT)
    //  : _M_node(__THAT._M_node) {}

    inline void
    _M_increment()
    {
      if (_M_node->_M_right)
      {
            _M_node = _M_node->_M_right;
            while (_M_node->_M_left) _M_node = _M_node->_M_left;
      }
      else
        {
          _Base_const_ptr __p = _M_node->_M_parent;
          while (__p && _M_node == __p->_M_right)
            {
              _M_node = __p;
              __p = _M_node->_M_parent;
            }
          if (__p) // (__p) provide undetermined behavior on end()++ rather
		   // than a seg fault, similar to standard iterator.
            _M_node = __p;
        }
    }

    inline void
    _M_decrement()
    {
      if (!_M_node->_M_parent) // clearly identify the header node
	{
	  _M_node = _M_node->_M_right;
	}
      else if (_M_node->_M_left)
	{
	  _Base_const_ptr x = _M_node->_M_left;
	  while (x->_M_right) x = x->_M_right;
	  _M_node = x;
	}
      else
	{
	  _Base_const_ptr __p = _M_node->_M_parent;
	  while (__p && _M_node == __p->_M_left) // see below
	    {
	      _M_node = __p;
	      __p = _M_node->_M_parent;
	    }
	  if (__p) // (__p) provide undetermined behavior on rend()++ rather
		   // than a seg fault, similar to standard iterator.
	    _M_node = __p;
	}
    }

    template <size_t const __K, typename _Val, typename _Acc,
	      typename _Dist, typename _Cmp, typename _Alloc>
      friend class KDTree;
  };

  template <typename _Val, typename _Ref, typename _Ptr>
    class _Iterator : protected _Base_iterator
    {
    public:
      typedef _Val value_type;
      typedef _Ref reference;
      typedef _Ptr pointer;
      typedef _Iterator<_Val, _Val&, _Val*> iterator;
      typedef _Iterator<_Val, _Val const&, _Val const*> const_iterator;
      typedef _Iterator<_Val, _Ref, _Ptr> _Self;
      typedef _Node<_Val> const* _Link_const_type;
      typedef std::bidirectional_iterator_tag iterator_category;
      typedef ptrdiff_t difference_type;

      inline _Iterator()
        : _Base_iterator() {}
      inline _Iterator(_Link_const_type const __N)
        : _Base_iterator(__N) {}
      inline _Iterator(iterator const& __THAT)
        : _Base_iterator(__THAT) {}

      _Link_const_type get_raw_node() const
      {
         return _Link_const_type(_M_node);
      }

      reference
      operator*() const
      {
        return _Link_const_type(_M_node)->_M_value;
      }

      pointer
      operator->() const
      {
        return &(operator*());
      }

      _Self
      operator++()
      {
        _M_increment();
        return *this;
      }

      _Self
      operator++(int)
      {
        _Self ret = *this;
        _M_increment();
        return ret;
      }

      _Self&
      operator--()
      {
        _M_decrement();
        return *this;
      }

      _Self
      operator--(int)
      {
        _Self ret = *this;
        _M_decrement();
        return ret;
      }

      friend bool
      operator== <>(_Iterator<_Val, _Ref, _Ptr> const&,
                    _Iterator<_Val, _Ref, _Ptr> const&);

      friend bool
      operator== <>(_Iterator<_Val, const _Val&, const _Val*> const&,
                    _Iterator<_Val, _Val&, _Val*> const&);

      friend bool
      operator== <>(_Iterator<_Val, _Val&, _Val*> const&,
                    _Iterator<_Val, const _Val&, const _Val*> const&);

      friend bool
      operator!= <>(_Iterator<_Val, _Ref, _Ptr> const&,
                    _Iterator<_Val, _Ref, _Ptr> const&);

      friend bool
      operator!= <>(_Iterator<_Val, const _Val&, const _Val*> const&,
                    _Iterator<_Val, _Val&, _Val*> const&);

      friend bool
      operator!= <>(_Iterator<_Val, _Val&, _Val*> const&,
                    _Iterator<_Val, const _Val&, const _Val*> const&);
    };

  template<typename _Val, typename _Ref, typename _Ptr>
    bool
    operator==(_Iterator<_Val, _Ref, _Ptr> const& __X,
               _Iterator<_Val, _Ref, _Ptr> const& __Y)
    { return __X._M_node == __Y._M_node; }

  template<typename _Val>
    bool
    operator==(_Iterator<_Val, const _Val&, const _Val*> const& __X,
               _Iterator<_Val, _Val&, _Val*> const& __Y)
    { return __X._M_node == __Y._M_node; }

  template<typename _Val>
    bool
    operator==(_Iterator<_Val, _Val&, _Val*> const& __X,
               _Iterator<_Val, const _Val&, const _Val*> const& __Y)
    { return __X._M_node == __Y._M_node; }

  template<typename _Val, typename _Ref, typename _Ptr>
    bool
    operator!=(_Iterator<_Val, _Ref, _Ptr> const& __X,
               _Iterator<_Val, _Ref, _Ptr> const& __Y)
    { return __X._M_node != __Y._M_node; }

  template<typename _Val>
    bool
    operator!=(_Iterator<_Val, const _Val&, const _Val*> const& __X,
               _Iterator<_Val, _Val&, _Val*> const& __Y)
    { return __X._M_node != __Y._M_node; }

  template<typename _Val>
    bool
    operator!=(_Iterator<_Val, _Val&, _Val*> const& __X,
               _Iterator<_Val, const _Val&, const _Val*> const& __Y)
    { return __X._M_node != __Y._M_node; }

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

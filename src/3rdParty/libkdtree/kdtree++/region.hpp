/** \file
 * Defines the interface of the _Region class.
 *
 * \author Martin F. Krafft <libkdtree@pobox.madduck.net>
 */

#ifndef INCLUDE_KDTREE_REGION_HPP
#define INCLUDE_KDTREE_REGION_HPP

#include <cstddef>

#include "node.hpp"

namespace KDTree
{

  template <size_t const __K, typename _Val, typename _SubVal,
            typename _Acc, typename _Cmp>
    struct _Region
    {
      typedef _Val value_type;
      typedef _SubVal subvalue_type;

      // special typedef for checking against a fuzzy point (for find_nearest)
      // Note the region (first) component is not supposed to have an area, its
      // bounds should all be set to a specific point.
      typedef std::pair<_Region,_SubVal> _CenterPt;

      _Region(_Acc const& __acc=_Acc(), const _Cmp& __cmp=_Cmp())
	: _M_cmp(__cmp), _M_acc(__acc) {}

      template <typename Val>
      _Region(Val const& __V,
	      _Acc const& __acc=_Acc(), const _Cmp& __cmp=_Cmp())
	: _M_acc(__acc), _M_cmp(__cmp)
      {
        for (size_t __i = 0; __i != __K; ++__i)
          {
             _M_low_bounds[__i] = _M_high_bounds[__i] = _M_acc(__V,__i);
          }
      }

      template <typename Val>
      _Region(Val const& __V, subvalue_type const& __R,
	      _Acc const& __acc=_Acc(), const _Cmp& __cmp=_Cmp())
	: _M_acc(__acc), _M_cmp(__cmp)
      {
        for (size_t __i = 0; __i != __K; ++__i)
          {
             _M_low_bounds[__i] = _M_acc(__V,__i) - __R;
             _M_high_bounds[__i] = _M_acc(__V,__i) + __R;
          }
      }

      bool
      intersects_with(_CenterPt const& __THAT) const
      {
        for (size_t __i = 0; __i != __K; ++__i)
          {
             // does it fall outside the bounds? 
             // ! low-tolerance <= x <= high+tolerance
             // ! (low-tol <= x and x <= high+tol)
             // !low-tol<=x or !x<=high+tol
             // low-tol>x or x>high+tol
             // x<low-tol or high+tol<x
            if (_M_cmp(__THAT.first._M_low_bounds[__i], _M_low_bounds[__i] - __THAT.second)
             || _M_cmp(_M_high_bounds[__i] + __THAT.second, __THAT.first._M_low_bounds[__i]))
              return false;
          }
        return true;
      }

      bool
      intersects_with(_Region const& __THAT) const
      {
        for (size_t __i = 0; __i != __K; ++__i)
          {
            if (_M_cmp(__THAT._M_high_bounds[__i], _M_low_bounds[__i])
             || _M_cmp(_M_high_bounds[__i], __THAT._M_low_bounds[__i]))
              return false;
          }
        return true;
      }

      bool
      encloses(value_type const& __V) const
      {
        for (size_t __i = 0; __i != __K; ++__i)
          {
            if (_M_cmp(_M_acc(__V, __i), _M_low_bounds[__i])
             || _M_cmp(_M_high_bounds[__i], _M_acc(__V, __i)))
              return false;
          }
        return true;
      }

      _Region&
      set_high_bound(value_type const& __V, size_t const __L)
      {
        _M_high_bounds[__L % __K] = _M_acc(__V, __L % __K);
        return *this;
      }

      _Region&
      set_low_bound(value_type const& __V, size_t const __L)
      {
        _M_low_bounds[__L % __K] = _M_acc(__V, __L % __K);
        return *this;
      }

      subvalue_type _M_low_bounds[__K], _M_high_bounds[__K];
      _Acc _M_acc;
      _Cmp _M_cmp;
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

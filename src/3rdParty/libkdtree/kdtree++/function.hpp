/** \file
 * Defines the various functors and interfaces used for KDTree.
 *
 * \author Martin F. Krafft <libkdtree@pobox.madduck.net>
 * \author Sylvain Bougerel <sylvain.bougerel.devel@gmail.com>
 */

#ifndef INCLUDE_KDTREE_ACCESSOR_HPP
#define INCLUDE_KDTREE_ACCESSOR_HPP

#include <cstddef>

namespace KDTree
{
  template <typename _Val>
  struct _Bracket_accessor
  {
    typedef typename _Val::value_type result_type;

    result_type
    operator()(_Val const& V, size_t const N) const
    {
      return V[N];
    }
  };

  template <typename _Tp>
  struct always_true
  {
    bool operator() (const _Tp& ) const { return true; }
  };

  template <typename _Tp, typename _Dist>
  struct squared_difference
  {
    typedef _Dist distance_type;

    distance_type
    operator() (const _Tp& __a, const _Tp& __b) const
    {
      distance_type d=__a - __b;
      return d*d;
    }
  };

  template <typename _Tp, typename _Dist>
  struct squared_difference_counted
  {
    typedef _Dist distance_type;

    squared_difference_counted()
      : _M_count(0)
    { }

    void reset ()
    { _M_count = 0; }

    long&
    count () const
    { return _M_count; }

    distance_type
    operator() (const _Tp& __a, const _Tp& __b) const
    {
      distance_type d=__a - __b;
      ++_M_count;
      return d*d;
    }

  private:
    mutable long _M_count;
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

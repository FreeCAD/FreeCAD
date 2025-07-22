#define KDTREE_DEFINE_OSTREAM_OPERATORS

#include <kdtree++/kdtree.hpp>

#include <iostream>
#include <vector>

#include "py-kdtree.hpp"

int main()
{


  KDTree_2Int t;

  RECORD_2il c0 = { {5, 4} }; t.add(c0);
  RECORD_2il c1 = { {4, 2} }; t.add(c1);
  RECORD_2il c2 = { {7, 6} }; t.add(c2);
  RECORD_2il c3 = { {2, 2} }; t.add(c3);
  RECORD_2il c4 = { {8, 0} }; t.add(c4);
  RECORD_2il c5 = { {5, 7} }; t.add(c5);
  RECORD_2il c6 = { {3, 3} }; t.add(c6);
  RECORD_2il c7 = { {9, 7} }; t.add(c7);
  RECORD_2il c8 = { {2, 2} }; t.add(c8);
  RECORD_2il c9 = { {2, 0} }; t.add(c9);

  std::cout << t.tree << std::endl;

  t.remove(c0);
  t.remove(c1);
  t.remove(c3);
  t.remove(c5);

  t.optimize();

  std::cout << std::endl << t.tree << std::endl;

  int i=0;
  for (KDTREE_TYPE_2il::const_iterator iter=t.tree.begin(); iter!=t.tree.end(); ++iter, ++i);
  std::cout << "iterator walked through " << i << " nodes in total" << std::endl;
  if (i!=6)
    {
      std::cerr << "Error: does not tally with the expected number of nodes (6)" << std::endl;
      return 1;
    }
  i=0;
  for (KDTREE_TYPE_2il::const_reverse_iterator iter=t.tree.rbegin(); iter!=t.tree.rend(); ++iter, ++i);
  std::cout << "reverse_iterator walked through " << i << " nodes in total" << std::endl;
  if (i!=6)
    {
      std::cerr << "Error: does not tally with the expected number of nodes (6)" << std::endl;
      return 1;
    }

  RECORD_2il::point_t s = {5, 4};
  std::vector<RECORD_2il> v;
  unsigned int const RANGE = 3;

  size_t count = t.count_within_range(s, RANGE);
  std::cout << "counted " << count
	    << " nodes within range " << RANGE << " of " << s << ".\n";
  v = t.find_within_range(s, RANGE);

  std::cout << "found   " << v.size() << " nodes within range " << RANGE
	    << " of " << s << ":\n";
  std::vector<RECORD_2il>::const_iterator ci = v.begin();
  for (; ci != v.end(); ++ci)
    std::cout << *ci << " ";
  std::cout << "\n" << std::endl;

  std::cout << "Nearest to " << s << ": " <<
     t.find_nearest(s) << std::endl;

  RECORD_2il::point_t s2 = { 10, 10};
  std::cout << "Nearest to " << s2 << ": " <<
     t.find_nearest(s2) << std::endl;

  std::cout << std::endl;

  std::cout << t.tree << std::endl;

  return 0;
}

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

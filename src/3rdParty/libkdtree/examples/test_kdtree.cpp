#define KDTREE_DEFINE_OSTREAM_OPERATORS

// Make SURE all our asserts() are checked
#undef NDEBUG

#include <kdtree++/kdtree.hpp>

#include <deque>
#include <iostream>
#include <vector>
#include <limits>
#include <functional>
#include <set>

// used to ensure all triplets that are accessed via the operator<< are initialised.
std::set<const void*> registered;

struct triplet
{
  typedef double value_type;

  triplet(value_type a, value_type b, value_type c)
  {
    d[0] = a;
    d[1] = b;
    d[2] = c;
    bool reg_ok = (registered.find(this) == registered.end());
    assert(reg_ok);
    bool reg_inserted_ok = registered.insert(this).second;
    assert(reg_inserted_ok);
  }

  triplet(const triplet & x)
  {
    d[0] = x.d[0];
    d[1] = x.d[1];
    d[2] = x.d[2];
    bool reg_ok = (registered.find(this) == registered.end());
    assert(reg_ok);
    bool reg_inserted_ok = registered.insert(this).second;
    assert(reg_inserted_ok);
  }

  ~triplet()
  {
    bool unreg_ok = (registered.find(this) != registered.end());
    assert(unreg_ok);
    registered.erase(this);
  }

  double distance_to(triplet const& x) const
  {
     double dist = 0;
     for (int i = 0; i != 3; ++i)
        dist += (d[i]-x.d[i])*(d[i]-x.d[i]);
     return std::sqrt(dist);
  }

  inline value_type operator[](size_t const N) const { return d[N]; }

  value_type d[3];
};



// same as triplet, except with the values reversed.
struct alternate_triplet
{
  typedef double value_type;

  alternate_triplet(const triplet & x)
  {
    d[0] = x.d[2];
    d[1] = x.d[1];
    d[2] = x.d[0];
  }

  inline value_type operator[](size_t const N) const { return d[2-N]; }

  value_type d[3];
};

inline bool operator==(triplet const& A, triplet const& B) {
  return A.d[0] == B.d[0] && A.d[1] == B.d[1] && A.d[2] == B.d[2];
}

std::ostream& operator<<(std::ostream& out, triplet const& T)
{
  assert(registered.find(&T) != registered.end());
  return out << '(' << T.d[0] << ',' << T.d[1] << ',' << T.d[2] << ')';
}

inline double tac( triplet t, size_t k ) { return t[k]; }

// use tac as a class instead of a function,
// can access more than one type with just 1 definition.
struct alternate_tac
{
   typedef double result_type;
   double operator()( triplet const& t, size_t k ) const { return t[k]; }
   double operator()( alternate_triplet const& t, size_t k ) const { return t[k]; }
};


typedef KDTree::KDTree<3, triplet, std::pointer_to_binary_function<triplet,size_t,double> > tree_type;

struct Predicate
{
   bool operator()( triplet const& t ) const
   {
      return t[0] > 3;  // anything, we are currently testing that it compiles.
   }
};

// never finds anything
struct FalsePredicate
{
   bool operator()( triplet const& t ) const { return false; }
};

int main()
{
   // check that it'll find nodes exactly MAX away
   {
      tree_type exact_dist(std::ptr_fun(tac));
        triplet c0(5, 4, 0);
        exact_dist.insert(c0);
        triplet target(7,4,0);

      std::pair<tree_type::const_iterator,double> found = exact_dist.find_nearest(target,2);
      assert(found.first != exact_dist.end());
      assert(found.second == 2);
      std::cout << "Test find_nearest(), found at exact distance away from " << target << ", found " << *found.first << std::endl;
   }

   // do the same test, except use alternate_triplet as the search key
   {
      // NOTE: stores triplet, but we search with alternate_triplet
      typedef KDTree::KDTree<3, triplet, alternate_tac> alt_tree;

      triplet actual_target(7,0,0);

      alt_tree tree;
      tree.insert( triplet(0, 0, 7) );
      tree.insert( triplet(0, 0, 7) );
      tree.insert( triplet(0, 0, 7) );
      tree.insert( triplet(3, 0, 0) );
      tree.insert( actual_target );
      tree.optimise();

      alternate_triplet target( actual_target );

      std::pair<alt_tree::const_iterator,double> found = tree.find_nearest(target);
      assert(found.first != tree.end());
      std::cout << "Test with alternate search type, found: " << *found.first << ", wanted " << actual_target << std::endl;
      assert(found.second == 0);
      assert(*found.first == actual_target);
   }


   {
      tree_type exact_dist(std::ptr_fun(tac));
        triplet c0(5, 2, 0);
        exact_dist.insert(c0);
        triplet target(7,4,0);

        // call find_nearest without a range value - it found a compile error earlier.
      std::pair<tree_type::const_iterator,double> found = exact_dist.find_nearest(target);
      assert(found.first != exact_dist.end());
      std::cout << "Test find_nearest(), found at exact distance away from " << target << ", found " << *found.first << " @ " << found.second << " should be " << std::sqrt(8) << std::endl;
      assert(found.second == std::sqrt(8));
   }

   {
      tree_type exact_dist(std::ptr_fun(tac));
        triplet c0(5, 2, 0);
        exact_dist.insert(c0);
        triplet target(7,4,0);

      std::pair<tree_type::const_iterator,double> found = exact_dist.find_nearest(target,std::sqrt(8));
      assert(found.first != exact_dist.end());
      std::cout << "Test find_nearest(), found at exact distance away from " << target << ", found " << *found.first << " @ " << found.second << " should be " << std::sqrt(8) << std::endl;
      assert(found.second == std::sqrt(8));
   }

  tree_type src(std::ptr_fun(tac));

  triplet c0(5, 4, 0); src.insert(c0);
  triplet c1(4, 2, 1); src.insert(c1);
  triplet c2(7, 6, 9); src.insert(c2);
  triplet c3(2, 2, 1); src.insert(c3);
  triplet c4(8, 0, 5); src.insert(c4);
  triplet c5(5, 7, 0); src.insert(c5);
  triplet c6(3, 3, 8); src.insert(c6);
  triplet c7(9, 7, 3); src.insert(c7);
  triplet c8(2, 2, 6); src.insert(c8);
  triplet c9(2, 0, 6); src.insert(c9);

  std::cout << src << std::endl;

  src.erase(c0);
  src.erase(c1);
  src.erase(c3);
  src.erase(c5);

  src.optimise();


  // test the efficient_replace_and_optimise()
  tree_type eff_repl = src;
  {
     std::vector<triplet> vec;
     // erased above as part of test vec.push_back(triplet(5, 4, 0));
     // erased above as part of test vec.push_back(triplet(4, 2, 1));
     vec.push_back(triplet(7, 6, 9));
     // erased above as part of test vec.push_back(triplet(2, 2, 1));
     vec.push_back(triplet(8, 0, 5));
     // erased above as part of test vec.push_back(triplet(5, 7, 0));
     vec.push_back(triplet(3, 3, 8));
     vec.push_back(triplet(9, 7, 3));
     vec.push_back(triplet(2, 2, 6));
     vec.push_back(triplet(2, 0, 6));

     eff_repl.clear();
     eff_repl.efficient_replace_and_optimise(vec);
  }


  std::cout << std::endl << src << std::endl;

  tree_type copied(src);
  std::cout << copied << std::endl;
  tree_type assigned = src;
  std::cout << assigned << std::endl;

  for (int loop = 0; loop != 4; ++loop)
    {
      tree_type * target;
      switch (loop)
	{
	case 0: std::cout << "Testing plain construction" << std::endl;
	  target = &src;
	  break;

	case 1: std::cout << "Testing copy-construction" << std::endl;
	  target = &copied;
	  break;

	case 2: std::cout << "Testing assign-construction" << std::endl;
	  target = &assigned;
	  break;

   default:
	case 4: std::cout << "Testing efficient-replace-and-optimise" << std::endl;
	  target = &eff_repl;
	  break;
	}
      tree_type & t = *target;

      int i=0;
      for (tree_type::const_iterator iter=t.begin(); iter!=t.end(); ++iter, ++i);
      std::cout << "iterator walked through " << i << " nodes in total" << std::endl;
      if (i!=6)
	{
	  std::cerr << "Error: does not tally with the expected number of nodes (6)" << std::endl;
	  return 1;
	}
      i=0;
      for (tree_type::const_reverse_iterator iter=t.rbegin(); iter!=t.rend(); ++iter, ++i);
      std::cout << "reverse_iterator walked through " << i << " nodes in total" << std::endl;
      if (i!=6)
	{
	  std::cerr << "Error: does not tally with the expected number of nodes (6)" << std::endl;
	  return 1;
	}

      triplet s(5, 4, 3);
      std::vector<triplet> v;
      unsigned int const RANGE = 3;

      size_t count = t.count_within_range(s, RANGE);
      std::cout << "counted " << count
		<< " nodes within range " << RANGE << " of " << s << ".\n";
      t.find_within_range(s, RANGE, std::back_inserter(v));

      std::cout << "found   " << v.size() << " nodes within range " << RANGE
		<< " of " << s << ":\n";
      std::vector<triplet>::const_iterator ci = v.begin();
      for (; ci != v.end(); ++ci)
	std::cout << *ci << " ";
      std::cout << "\n" << std::endl;

      std::cout << std::endl << t << std::endl;

      // search for all the nodes at exactly 0 dist away
      for (tree_type::const_iterator target = t.begin(); target != t.end(); ++target)
      {
         std::pair<tree_type::const_iterator,double> found = t.find_nearest(*target,0);
         assert(found.first != t.end());
         assert(*found.first == *target);
         std::cout << "Test find_nearest(), found at exact distance away from " << *target << ", found " << *found.first << std::endl;
      }

      {
         const double small_dist = 0.0001;
         std::pair<tree_type::const_iterator,double> notfound = t.find_nearest(s,small_dist);
         std::cout << "Test find_nearest(), nearest to " << s << " within " << small_dist << " should not be found" << std::endl;

         if (notfound.first != t.end())
         {
            std::cout << "ERROR found a node at dist " << notfound.second << " : " << *notfound.first << std::endl;
            std::cout << "Actual distance = " << s.distance_to(*notfound.first) << std::endl;
         }

         assert(notfound.first == t.end());
      }

      {
         std::pair<tree_type::const_iterator,double> nif = t.find_nearest_if(s,std::numeric_limits<double>::max(),Predicate());
         std::cout << "Test find_nearest_if(), nearest to " << s << " @ " << nif.second << ": " << *nif.first << std::endl;

         std::pair<tree_type::const_iterator,double> cantfind = t.find_nearest_if(s,std::numeric_limits<double>::max(),FalsePredicate());
         std::cout << "Test find_nearest_if(), nearest to " << s << " should never be found (predicate too strong)" << std::endl;
         assert(cantfind.first == t.end());
      }




      {
      std::pair<tree_type::const_iterator,double> found = t.find_nearest(s,std::numeric_limits<double>::max());
      std::cout << "Nearest to " << s << " @ " << found.second << " " << *found.first << std::endl;
      std::cout << "Should be " << found.first->distance_to(s) << std::endl;
      // NOTE: the assert does not check for an exact match, as it is not exact when -O2 or -O3 is
      // switched on.  Some sort of optimisation makes the math inexact.
      assert( fabs(found.second - found.first->distance_to(s)) < std::numeric_limits<double>::epsilon() );
      }

      {
      triplet s2(10, 10, 2);
      std::pair<tree_type::const_iterator,double> found = t.find_nearest(s2,std::numeric_limits<double>::max());
      std::cout << "Nearest to " << s2 << " @ " << found.second << " " << *found.first << std::endl;
      std::cout << "Should be " << found.first->distance_to(s2) << std::endl;
      // NOTE: the assert does not check for an exact match, as it is not exact when -O2 or -O3 is
      // switched on.  Some sort of optimisation makes the math inexact.
      assert( fabs(found.second - found.first->distance_to(s2)) < std::numeric_limits<double>::epsilon() );
      }

      std::cout << std::endl;

      std::cout << t << std::endl;

      // Testing iterators
      {
	std::cout << "Testing iterators" << std::endl;

	t.erase(c2);
	t.erase(c4);
	t.erase(c6);
	t.erase(c7);
	t.erase(c8);
	//    t.erase(c9);

	std::cout << std::endl << t << std::endl;

	std::cout << "Forward iterator test..." << std::endl;
	std::vector<triplet> forwards;
	for (tree_type::iterator i = t.begin(); i != t.end(); ++i)
	  { std::cout << *i << " " << std::flush; forwards.push_back(*i); }
	std::cout << std::endl;
	std::cout << "Reverse iterator test..." << std::endl;
	std::vector<triplet> backwards;
	for (tree_type::reverse_iterator i = t.rbegin(); i != t.rend(); ++i)
	  { std::cout << *i << " " << std::flush; backwards.push_back(*i); }
	std::cout << std::endl;
	std::reverse(backwards.begin(),backwards.end());
	assert(backwards == forwards);
      }
    }


  // Walter reported that the find_within_range() wasn't giving results that were within
  // the specified range... this is the test.
  {
     tree_type tree(std::ptr_fun(tac));
     tree.insert( triplet(28.771200,16.921600,-2.665970) );
     tree.insert( triplet(28.553101,18.649700,-2.155560) );
     tree.insert( triplet(28.107500,20.341400,-1.188940) );
     tree.optimise();

     std::deque< triplet > vectors;
     triplet sv(18.892500,20.341400,-1.188940);
     tree.find_within_range(sv, 10.0f, std::back_inserter(vectors));

     std::cout << std::endl << "Test find_with_range( " << sv << ", 10.0f) found " << vectors.size() << " candidates." << std::endl;

     // double-check the ranges
     for (std::deque<triplet>::iterator v = vectors.begin(); v != vectors.end(); ++v)
     {
        double dist = sv.distance_to(*v);
        std::cout << "  " << *v << " dist=" << dist << std::endl;
        if (dist > 10.0f)
           std::cout << "    This point is too far! But that is by design, its within a 'box' with a 'radius' of 10, not a sphere with a radius of 10" << std::endl;
        // Not a valid test, it can be greater than 10 if the point is in the corners of the box.
        // assert(dist <= 10.0f);
     }
  }


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

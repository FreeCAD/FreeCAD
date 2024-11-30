/** \file
 * Defines the interface for the KDTree class.
 *
 * \author Martin F. Krafft <libkdtree@pobox.madduck.net>
 *
 * Paul Harris figured this stuff out (below)
 * Notes:
 * This is similar to a binary tree, but its not the same.
 * There are a few important differences:
 *
 *  * Each level is sorted by a different criteria (this is fundamental to the design).
 *
 *  * It is possible to have children IDENTICAL to its parent in BOTH branches
 *    This is different to a binary tree, where identical children are always to the right
 *    So, KDTree has the relationships:
 *    * The left branch is <= its parent (in binary tree, this relationship is a plain < )
 *    * The right branch is <= its parent (same as binary tree)
 *
 *    This is done for mostly for performance.
 *    Its a LOT easier to maintain a consistent tree if we use the <= relationship.
 *    Note that this relationship only makes a difference when searching for an exact
 *    item with find() or find_exact, other search, erase and insert functions don't notice
 *    the difference.
 *
 *    In the case of binary trees, you can safely assume that the next identical item
 *    will be the child leaf,
 *    but in the case of KDTree, the next identical item might
 *    be a long way down a subtree, because of the various different sort criteria.
 *
 *    So erase()ing a node from a KDTree could require serious and complicated
 *    tree rebalancing to maintain consistency... IF we required binary-tree-like relationships.
 *
 *    This has no effect on insert()s, a < test is good enough to keep consistency.
 *
 *    It has an effect on find() searches:
 *      * Instead of using compare(child,node) for a < relationship and following 1 branch,
 *        we must use !compare(node,child) for a <= relationship, and test BOTH branches, as
 *        we could potentially go down both branches.
 *
 *    It has no real effect on bounds-based searches (like find_nearest, find_within_range)
 *    as it compares vs a boundary and would follow both branches if required.
 *
 *    This has no real effect on erase()s, a < test is good enough to keep consistency.
 */

#ifndef INCLUDE_KDTREE_KDTREE_HPP
#define INCLUDE_KDTREE_KDTREE_HPP


//
//  This number is guarenteed to change with every release.
//
//  KDTREE_VERSION % 100 is the patch level
//  KDTREE_VERSION / 100 % 1000 is the minor version
//  KDTREE_VERSION / 100000 is the major version
#define KDTREE_VERSION 702
//
//  KDTREE_LIB_VERSION must be defined to be the same as KDTREE_VERSION
//  but as a *string* in the form "x_y[_z]" where x is the major version
//  number, y is the minor version number, and z is the patch level if not 0.
#define KDTREE_LIB_VERSION "0_7_2"


#include <vector>

#ifdef KDTREE_CHECK_PERFORMANCE_COUNTERS
#  include <map>
#endif
#include <algorithm>
#include <functional>

#ifdef KDTREE_DEFINE_OSTREAM_OPERATORS
#  include <ostream>
#  include <stack>
#endif

#include <cmath>
#include <cstddef>
#include <cassert>

#include "function.hpp"
#include "allocator.hpp"
#include "iterator.hpp"
#include "node.hpp"
#include "region.hpp"

namespace KDTree
{

#ifdef KDTREE_CHECK_PERFORMANCE
   unsigned long long num_dist_calcs = 0;
#endif

template <size_t const __K, typename _Val,
          typename _Acc = _Bracket_accessor<_Val>,
          typename _Dist = squared_difference<typename _Acc::result_type,
          typename _Acc::result_type>,
          typename _Cmp = std::less<typename _Acc::result_type>,
          typename _Alloc = std::allocator<_Node<_Val> > >
class KDTree : protected _Alloc_base<_Val, _Alloc>
{
protected:
  typedef _Alloc_base<_Val, _Alloc> _Base;
  typedef typename _Base::allocator_type allocator_type;

  typedef _Node_base* _Base_ptr;
  typedef _Node_base const* _Base_const_ptr;
  typedef _Node<_Val>* _Link_type;
  typedef _Node<_Val> const* _Link_const_type;

  typedef _Node_compare<_Val, _Acc, _Cmp> _Node_compare_;

public:
  typedef _Region<__K, _Val, typename _Acc::result_type, _Acc, _Cmp>
    _Region_;
  typedef _Val value_type;
  typedef value_type* pointer;
  typedef value_type const* const_pointer;
  typedef value_type& reference;
  typedef value_type const& const_reference;
  typedef typename _Acc::result_type subvalue_type;
  typedef typename _Dist::distance_type distance_type;
  typedef size_t size_type;
  typedef ptrdiff_t difference_type;

  KDTree(_Acc const& __acc = _Acc(), _Dist const& __dist = _Dist(),
         _Cmp const& __cmp = _Cmp(), const allocator_type& __a = allocator_type())
    : _Base(__a), _M_header(),
      _M_count(0), _M_acc(__acc), _M_cmp(__cmp), _M_dist(__dist)
  {
     _M_empty_initialise();
  }

  KDTree(const KDTree& __x)
     : _Base(__x.get_allocator()), _M_header(), _M_count(0),
       _M_acc(__x._M_acc), _M_cmp(__x._M_cmp), _M_dist(__x._M_dist)
  {
     _M_empty_initialise();
     // this is slow:
     // this->insert(begin(), __x.begin(), __x.end());
     // this->optimise();

     // this is much faster, as it skips a lot of useless work
     // do the optimisation before inserting
     // Needs to be stored in a vector first as _M_optimise()
     // sorts the data in the passed iterators directly.
     std::vector<value_type> temp;
     temp.reserve(__x.size());
     std::copy(__x.begin(),__x.end(),std::back_inserter(temp));
     _M_optimise(temp.begin(), temp.end(), 0);
  }

  template<typename _InputIterator>
    KDTree(_InputIterator __first, _InputIterator __last,
           _Acc const& acc = _Acc(), _Dist const& __dist = _Dist(),
           _Cmp const& __cmp = _Cmp(), const allocator_type& __a = allocator_type())
    : _Base(__a), _M_header(), _M_count(0),
      _M_acc(acc), _M_cmp(__cmp), _M_dist(__dist)
  {
     _M_empty_initialise();
     // this is slow:
     // this->insert(begin(), __first, __last);
     // this->optimise();

     // this is much faster, as it skips a lot of useless work
     // do the optimisation before inserting
     // Needs to be stored in a vector first as _M_optimise()
     // sorts the data in the passed iterators directly.
     std::vector<value_type> temp;
     temp.reserve(std::distance(__first,__last));
     std::copy(__first,__last,std::back_inserter(temp));
     _M_optimise(temp.begin(), temp.end(), 0);

     // NOTE: this will BREAK users that are passing in
     // read-once data via the iterator...
     // We increment __first all the way to __last once within
     // the distance() call, and again within the copy() call.
     //
     // This should end up using some funky C++ concepts or 
     // type traits to check that the iterators can be used in this way...
  }


  // this will CLEAR the tree and fill it with the contents
  // of 'writable_vector'.  it will use the passed vector directly,
  // and will basically resort the vector many times over while
  // optimising the tree.
  //
  // Paul: I use this when I have already built up a vector of data
  // that I want to add, and I don't mind if its contents get shuffled
  // by the kdtree optimise routine.
  void efficient_replace_and_optimise( std::vector<value_type> & writable_vector )
  {
     this->clear();
     _M_optimise(writable_vector.begin(), writable_vector.end(), 0);
  }



  KDTree&
  operator=(const KDTree& __x)
  {
  	if (this != &__x)
  	  {
  	    _M_acc = __x._M_acc;
  	    _M_dist = __x._M_dist;
  	    _M_cmp = __x._M_cmp;
           // this is slow:
           // this->insert(begin(), __x.begin(), __x.end());
           // this->optimise();

           // this is much faster, as it skips a lot of useless work
           // do the optimisation before inserting
           // Needs to be stored in a vector first as _M_optimise()
           // sorts the data in the passed iterators directly.
           std::vector<value_type> temp;
           temp.reserve(__x.size());
           std::copy(__x.begin(),__x.end(),std::back_inserter(temp));
           efficient_replace_and_optimise(temp);
  	  }
  	return *this;
  }

  ~KDTree()
  {
    this->clear();
  }

  allocator_type
  get_allocator() const
  {
    return _Base::get_allocator();
  }

  size_type
  size() const
  {
    return _M_count;
  }

  size_type
  max_size() const
  {
    return size_type(-1);
  }

  bool
  empty() const
  {
    return this->size() == 0;
  }

  void
  clear()
  {
    _M_erase_subtree(_M_get_root());
    _M_set_leftmost(&_M_header);
    _M_set_rightmost(&_M_header);
    _M_set_root(NULL);
    _M_count = 0;
  }

  /*! \brief Comparator for the values in the KDTree.

The comparator shall not be modified, it could invalidate the tree.
\return a copy of the comparator used by the KDTree.
   */
  _Cmp
  value_comp() const
  { return _M_cmp; }

  /*! \brief Accessor to the value's elements.

This accessor shall not be modified, it could invalidate the tree.
\return a copy of the accessor used by the KDTree.
   */
  _Acc
  value_acc() const
  { return _M_acc; }

  /*! \brief Distance calculator between 2 value's element.

This functor can be modified. It's modification will only affect the
behavior of the find and find_nearest functions.
\return a reference to the distance calculator used by the KDTree.
   */
  const _Dist&
  value_distance() const
  { return _M_dist; }

  _Dist&
  value_distance()
  { return _M_dist; }

  // typedef _Iterator<_Val, reference, pointer> iterator;
  typedef _Iterator<_Val, const_reference, const_pointer> const_iterator;
  // No mutable iterator at this stage
  typedef const_iterator iterator;
  typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
  typedef std::reverse_iterator<iterator> reverse_iterator;

  // Note: the static_cast in end() is invalid (_M_header is not convertable to a _Link_type), but
  // thats ok as it just means undefined behaviour if the user dereferences the end() iterator.

  const_iterator begin() const { return const_iterator(_M_get_leftmost()); }
  const_iterator end() const { return const_iterator(static_cast<_Link_const_type>(&_M_header)); }
  const_reverse_iterator rbegin() const { return const_reverse_iterator(end()); }
  const_reverse_iterator rend() const { return const_reverse_iterator(begin()); }

  iterator
  insert(iterator /* ignored */, const_reference __V)
  {
     return this->insert(__V);
  }

  iterator
  insert(const_reference __V)
  {
    if (!_M_get_root())
      {
        _Link_type __n = _M_new_node(__V, &_M_header);
        ++_M_count;
        _M_set_root(__n);
        _M_set_leftmost(__n);
        _M_set_rightmost(__n);
        return iterator(__n);
      }
    return _M_insert(_M_get_root(), __V, 0);
  }

  template <class _InputIterator>
  void insert(_InputIterator __first, _InputIterator __last) {
     for (; __first != __last; ++__first)
        this->insert(*__first);
  }

  void
  insert(iterator __pos, size_type __n, const value_type& __x)
  {
    for (; __n > 0; --__n)
      this->insert(__pos, __x);
  }

  template<typename _InputIterator>
  void
  insert(iterator __pos, _InputIterator __first, _InputIterator __last) {
     for (; __first != __last; ++__first)
        this->insert(__pos, *__first);
  }

  // Note: this uses the find() to location the item you want to erase.
  // find() compares by equivalence of location ONLY.  See the comments
  // above find_exact() for why you may not want this.
  //
  // If you want to erase ANY item that has the same location as __V,
  // then use this function.
  //
  // If you want to erase a PARTICULAR item, and not any other item
  // that might happen to have the same location, then you should use
  // erase_exact().
  void
  erase(const_reference __V) {
    const_iterator b = this->find(__V);
    this->erase(b);
  }

  void
  erase_exact(const_reference __V) {
    this->erase(this->find_exact(__V));
  }

  // note: kept as const because its easier to const-cast it away
  void
  erase(const_iterator const& __IT)
  {
     assert(__IT != this->end());
    _Link_const_type target = __IT.get_raw_node();
    _Link_const_type n = target;
    size_type level = 0;
    while ((n = _S_parent(n)) != &_M_header)
       ++level;
    _M_erase( const_cast<_Link_type>(target), level );
    _M_delete_node( const_cast<_Link_type>(target) );
    --_M_count;
  }

/* this does not work since erasure changes sort order
  void
  erase(const_iterator __A, const_iterator const& __B)
  {
    if (0 && __A == this->begin() && __B == this->end())
      {
        this->clear();
      }
    else
      {
        while (__A != __B)
          this->erase(__A++);
      }
  }
*/

  // compares via equivalence
  // so if you are looking for any item with the same location,
  // according to the standard accessor comparisions,
  // then this is the function for you.
  template <class SearchVal>
  const_iterator
  find(SearchVal const& __V) const
  {
    if (!_M_get_root()) return this->end();
    return _M_find(_M_get_root(), __V, 0);
  }

  // compares via equality
  // if you are looking for a particular item in the tree,
  // and (for example) it has an ID that is checked via an == comparison
  // eg
  // struct Item
  // {
  //    size_type unique_id;
  //    bool operator==(Item const& a, Item const& b) { return a.unique_id == b.unique_id; }
  //    Location location;
  // };
  // Two items may be equivalent in location.  find() would return
  // either one of them.  But no two items have the same ID, so
  // find_exact() would always return the item with the same location AND id.
  //
  template <class SearchVal>
  const_iterator
  find_exact(SearchVal const& __V) const
  {
    if (!_M_get_root()) return this->end();
    return _M_find_exact(_M_get_root(), __V, 0);
  }

  // NOTE: see notes on find_within_range().
  size_type
  count_within_range(const_reference __V, subvalue_type const __R) const
  {
    if (!_M_get_root()) return 0;
    _Region_ __region(__V, __R, _M_acc, _M_cmp);
    return this->count_within_range(__region);
  }

  size_type
  count_within_range(_Region_ const& __REGION) const
  {
    if (!_M_get_root()) return 0;

    _Region_ __bounds(__REGION);
    return _M_count_within_range(_M_get_root(),
                         __REGION, __bounds, 0);
  }

  // NOTE: see notes on find_within_range().
  template <typename SearchVal, class Visitor>
  Visitor
  visit_within_range(SearchVal const& V, subvalue_type const R, Visitor visitor) const
  {
    if (!_M_get_root()) return visitor;
    _Region_ region(V, R, _M_acc, _M_cmp);
    return this->visit_within_range(region, visitor);
  }

  template <class Visitor>
  Visitor
  visit_within_range(_Region_ const& REGION, Visitor visitor) const
  {
    if (_M_get_root())
      {
        _Region_ bounds(REGION);
        return _M_visit_within_range(visitor, _M_get_root(), REGION, bounds, 0);
      }
    return visitor;
  }

  // NOTE: this will visit points based on 'Manhattan distance' aka city-block distance
  // aka taxicab metric. Meaning it will find all points within:
  //    max(x_dist,max(y_dist,z_dist));
  //  AND NOT than what you would expect: sqrt(x_dist*x_dist + y_dist*y_dist + z_dist*z_dist)
  //
  // This is because it converts the distance into a bounding-box 'region' and compares
  // against that.
  //
  // If you want the sqrt() behaviour, ask on the mailing list for different options.
  //
  template <typename SearchVal, typename _OutputIterator>
  _OutputIterator
  find_within_range(SearchVal const& val, subvalue_type const range,
                    _OutputIterator out) const
  {
    if (!_M_get_root()) return out;
    _Region_ region(val, range, _M_acc, _M_cmp);
    return this->find_within_range(region, out);
  }

  template <typename _OutputIterator>
  _OutputIterator
  find_within_range(_Region_ const& region,
                    _OutputIterator out) const
  {
    if (_M_get_root())
      {
        _Region_ bounds(region);
        out = _M_find_within_range(out, _M_get_root(),
                             region, bounds, 0);
      }
    return out;
  }

  template <class SearchVal>
  std::pair<const_iterator, distance_type>
  find_nearest (SearchVal const& __val) const
  {
  	if (_M_get_root())
  	  {
  	    std::pair<const _Node<_Val>*,
  	      std::pair<size_type, typename _Acc::result_type> >
  	      best = _S_node_nearest (__K, 0, __val,
  				      _M_get_root(), &_M_header, _M_get_root(),
  				      std::sqrt(_S_accumulate_node_distance
  				      (__K, _M_dist, _M_acc, _M_get_root()->_M_value, __val)),
  				      _M_cmp, _M_acc, _M_dist,
  				      always_true<value_type>());
  	    return std::pair<const_iterator, distance_type>
  	      (best.first, best.second.second);
  	  }
  	return std::pair<const_iterator, distance_type>(end(), 0);
  }

  template <class SearchVal>
  std::pair<const_iterator, distance_type>
  find_nearest (SearchVal const& __val, distance_type __max) const
  {
  	if (_M_get_root())
  	  {
        bool root_is_candidate = false;
  	    const _Node<_Val>* node = _M_get_root();
        { // scope to ensure we don't use 'root_dist' anywhere else
      	  distance_type root_dist = std::sqrt(_S_accumulate_node_distance
    	      (__K, _M_dist, _M_acc, _M_get_root()->_M_value, __val));
    	    if (root_dist <= __max)
    	      {
                root_is_candidate = true;
                __max = root_dist;
    	      }
        }
  	    std::pair<const _Node<_Val>*,
  	      std::pair<size_type, typename _Acc::result_type> >
  	      best = _S_node_nearest (__K, 0, __val, _M_get_root(), &_M_header,
  				      node, __max, _M_cmp, _M_acc, _M_dist,
  				      always_true<value_type>());
         // make sure we didn't just get stuck with the root node...
         if (root_is_candidate || best.first != _M_get_root())
            return std::pair<const_iterator, distance_type>
              (best.first, best.second.second);
  	  }
  	return std::pair<const_iterator, distance_type>(end(), __max);
  }

  template <class SearchVal, class _Predicate>
  std::pair<const_iterator, distance_type>
  find_nearest_if (SearchVal const& __val, distance_type __max,
       _Predicate __p) const
  {
  	if (_M_get_root())
  	  {
        bool root_is_candidate = false;
  	    const _Node<_Val>* node = _M_get_root();
  	    if (__p(_M_get_root()->_M_value))
  	      {
              { // scope to ensure we don't use root_dist anywhere else
          	    distance_type root_dist = std::sqrt(_S_accumulate_node_distance
          		  (__K, _M_dist, _M_acc, _M_get_root()->_M_value, __val));
            		if (root_dist <= __max)
            		  {
                    root_is_candidate = true;
            		    root_dist = __max;
            		  }
              }
  	      }
  	    std::pair<const _Node<_Val>*,
  	      std::pair<size_type, typename _Acc::result_type> >
  	      best = _S_node_nearest (__K, 0, __val, _M_get_root(), &_M_header,
  				      node, __max, _M_cmp, _M_acc, _M_dist, __p);
         // make sure we didn't just get stuck with the root node...
         if (root_is_candidate || best.first != _M_get_root())
            return std::pair<const_iterator, distance_type>
              (best.first, best.second.second);
  	  }
  	return std::pair<const_iterator, distance_type>(end(), __max);
  }

  void
  optimise()
  {
    std::vector<value_type> __v(this->begin(),this->end());
    this->clear();
    _M_optimise(__v.begin(), __v.end(), 0);
  }

  void
  optimize()
  { // cater for people who cannot spell :)
    this->optimise();
  }

  void check_tree()
  {
     _M_check_node(_M_get_root(),0);
  }

protected:

  void _M_check_children( _Link_const_type child, _Link_const_type parent, size_type const level, bool to_the_left )
  {
     assert(parent);
     if (child)
     {
 _Node_compare_ compare(level % __K, _M_acc, _M_cmp);
        // REMEMBER! its a <= relationship for BOTH branches
        // for left-case (true), child<=node --> !(node<child)
        // for right-case (false), node<=child --> !(child<node)
        assert(!to_the_left || !compare(parent->_M_value,child->_M_value));  // check the left
        assert(to_the_left || !compare(child->_M_value,parent->_M_value));   // check the right
        // and recurse down the tree, checking everything
        _M_check_children(_S_left(child),parent,level,to_the_left);
        _M_check_children(_S_right(child),parent,level,to_the_left);
     }
  }

  void _M_check_node( _Link_const_type node, size_type const level )
  {
     if (node)
     {
        // (comparing on this level)
        // everything to the left of this node must be smaller than this
        _M_check_children( _S_left(node), node, level, true );
        // everything to the right of this node must be larger than this
        _M_check_children( _S_right(node), node, level, false );

        _M_check_node( _S_left(node), level+1 );
        _M_check_node( _S_right(node), level+1 );
     }
  }

  void _M_empty_initialise()
  {
    _M_set_leftmost(&_M_header);
    _M_set_rightmost(&_M_header);
    _M_header._M_parent = NULL;
    _M_set_root(NULL);
  }

  iterator
  _M_insert_left(_Link_type __N, const_reference __V)
  {
    _S_set_left(__N, _M_new_node(__V)); ++_M_count;
    _S_set_parent( _S_left(__N), __N );
    if (__N == _M_get_leftmost())
       _M_set_leftmost( _S_left(__N) );
    return iterator(_S_left(__N));
  }

  iterator
  _M_insert_right(_Link_type __N, const_reference __V)
  {
    _S_set_right(__N, _M_new_node(__V)); ++_M_count;
    _S_set_parent( _S_right(__N), __N );
    if (__N == _M_get_rightmost())
       _M_set_rightmost( _S_right(__N) );
    return iterator(_S_right(__N));
  }

  iterator
  _M_insert(_Link_type __N, const_reference __V,
         size_type const __L)
  {
    if (_Node_compare_(__L % __K, _M_acc, _M_cmp)(__V, __N->_M_value))
      {
        if (!_S_left(__N))
          return _M_insert_left(__N, __V);
        return _M_insert(_S_left(__N), __V, __L+1);
      }
    else
      {
        if (!_S_right(__N) || __N == _M_get_rightmost())
          return _M_insert_right(__N, __V);
        return _M_insert(_S_right(__N), __V, __L+1);
      }
  }

  _Link_type
  _M_erase(_Link_type dead_dad, size_type const level)
  {
     // find a new step_dad, he will become a drop-in replacement.
    _Link_type step_dad = _M_get_erase_replacement(dead_dad, level);

     // tell dead_dad's parent that his new child is step_dad
    if (dead_dad == _M_get_root())
       _M_set_root(step_dad);
    else if (_S_left(_S_parent(dead_dad)) == dead_dad)
        _S_set_left(_S_parent(dead_dad), step_dad);
    else
        _S_set_right(_S_parent(dead_dad), step_dad);

    // deal with the left and right edges of the tree...
    // if the dead_dad was at the edge, then substitude...
    // but if there IS no new dead, then left_most is the dead_dad's parent
     if (dead_dad == _M_get_leftmost())
       _M_set_leftmost( (step_dad ? step_dad : _S_parent(dead_dad)) );
     if (dead_dad == _M_get_rightmost())
       _M_set_rightmost( (step_dad ? step_dad : _S_parent(dead_dad)) );

    if (step_dad)
      {
         // step_dad gets dead_dad's parent
        _S_set_parent(step_dad, _S_parent(dead_dad));

        // first tell the children that step_dad is their new dad
        if (_S_left(dead_dad))
           _S_set_parent(_S_left(dead_dad), step_dad);
        if (_S_right(dead_dad))
           _S_set_parent(_S_right(dead_dad), step_dad);

        // step_dad gets dead_dad's children
        _S_set_left(step_dad, _S_left(dead_dad));
        _S_set_right(step_dad, _S_right(dead_dad));
      }

    return step_dad;
  }



  _Link_type
  _M_get_erase_replacement(_Link_type node, size_type const level)
  {
     // if 'node' is null, then we can't do any better
    if (_S_is_leaf(node))
       return NULL;

    std::pair<_Link_type,size_type> candidate;
    // if there is nothing to the left, find a candidate on the right tree
    if (!_S_left(node))
      candidate = _M_get_j_min( std::pair<_Link_type,size_type>(_S_right(node),level), level+1);
    // ditto for the right
    else if ((!_S_right(node)))
      candidate = _M_get_j_max( std::pair<_Link_type,size_type>(_S_left(node),level), level+1);
    // we have both children ...
    else
     {
        // we need to do a little more work in order to find a good candidate
        // this is actually a technique used to choose a node from either the
        // left or right branch RANDOMLY, so that the tree has a greater change of
        // staying balanced.
        // If this were a true binary tree, we would always hunt down the right branch.
        // See top for notes.
        _Node_compare_ compare(level % __K, _M_acc, _M_cmp);
        // compare the children based on this level's criteria...
        // (this gives virtually random results)
        if (compare(_S_right(node)->_M_value, _S_left(node)->_M_value))
           // the right is smaller, get our replacement from the SMALLEST on the right
           candidate = _M_get_j_min(std::pair<_Link_type,size_type>(_S_right(node),level), level+1);
        else
           candidate = _M_get_j_max( std::pair<_Link_type,size_type>(_S_left(node),level), level+1);
     }

    // we have a candidate replacement by now.
    // remove it from the tree, but don't delete it.
    // it must be disconnected before it can be reconnected.
    _Link_type parent = _S_parent(candidate.first);
    if (_S_left(parent) == candidate.first)
       _S_set_left(parent, _M_erase(candidate.first, candidate.second));
    else
       _S_set_right(parent, _M_erase(candidate.first, candidate.second));

    return candidate.first;
  }



  std::pair<_Link_type,size_type>
  _M_get_j_min( std::pair<_Link_type,size_type> const node, size_type const level)
  {
    typedef std::pair<_Link_type,size_type> Result;
    if (_S_is_leaf(node.first))
        return Result(node.first,level);

    _Node_compare_ compare(node.second % __K, _M_acc, _M_cmp);
    Result candidate = node;
    if (_S_left(node.first))
      {
        Result left = _M_get_j_min(Result(_S_left(node.first), node.second), level+1);
        if (compare(left.first->_M_value, candidate.first->_M_value))
            candidate = left;
      }
    if (_S_right(node.first))
      {
        Result right = _M_get_j_min( Result(_S_right(node.first),node.second), level+1);
        if (compare(right.first->_M_value, candidate.first->_M_value))
            candidate = right;
      }
    if (candidate.first == node.first)
       return Result(candidate.first,level);

    return candidate;
  }



  std::pair<_Link_type,size_type>
  _M_get_j_max( std::pair<_Link_type,size_type> const node, size_type const level)
  {
    typedef std::pair<_Link_type,size_type> Result;

    if (_S_is_leaf(node.first))
        return Result(node.first,level);

    _Node_compare_ compare(node.second % __K, _M_acc, _M_cmp);
    Result candidate = node;
    if (_S_left(node.first))
      {
        Result left = _M_get_j_max( Result(_S_left(node.first),node.second), level+1);
        if (compare(candidate.first->_M_value, left.first->_M_value))
            candidate = left;
      }
    if (_S_right(node.first))
      {
        Result right = _M_get_j_max(Result(_S_right(node.first),node.second), level+1);
        if (compare(candidate.first->_M_value, right.first->_M_value))
            candidate = right;
      }

    if (candidate.first == node.first)
       return Result(candidate.first,level);

    return candidate;
  }


  void
  _M_erase_subtree(_Link_type __n)
  {
    while (__n)
      {
        _M_erase_subtree(_S_right(__n));
        _Link_type __t = _S_left(__n);
        _M_delete_node(__n);
        __n = __t;
      }
  }

  const_iterator
  _M_find(_Link_const_type node, const_reference value, size_type const level) const
  {
     // be aware! This is very different to normal binary searches, because of the <=
     // relationship used. See top for notes.
     // Basically we have to check ALL branches, as we may have an identical node
     // in different branches.
      const_iterator found = this->end();

    _Node_compare_ compare(level % __K, _M_acc, _M_cmp);
    if (!compare(node->_M_value,value))   // note, this is a <= test
      {
       // this line is the only difference between _M_find_exact() and _M_find()
        if (_M_matches_node(node, value, level))
          return const_iterator(node);   // return right away
        if (_S_left(node))
           found = _M_find(_S_left(node), value, level+1);
      }
    if ( _S_right(node) && found == this->end() && !compare(value,node->_M_value))   // note, this is a <= test
        found = _M_find(_S_right(node), value, level+1);
    return found;
  }

  const_iterator
  _M_find_exact(_Link_const_type node, const_reference value, size_type const level) const
  {
     // be aware! This is very different to normal binary searches, because of the <=
     // relationship used. See top for notes.
     // Basically we have to check ALL branches, as we may have an identical node
     // in different branches.
      const_iterator found = this->end();

    _Node_compare_ compare(level % __K, _M_acc, _M_cmp);
    if (!compare(node->_M_value,value))  // note, this is a <= test
    {
       // this line is the only difference between _M_find_exact() and _M_find()
        if (value == *const_iterator(node))
          return const_iterator(node);   // return right away
       if (_S_left(node))
        found = _M_find_exact(_S_left(node), value, level+1);
    }

    // note: no else!  items that are identical can be down both branches
    if ( _S_right(node) && found == this->end() && !compare(value,node->_M_value))   // note, this is a <= test
        found = _M_find_exact(_S_right(node), value, level+1);
    return found;
  }

  bool
  _M_matches_node_in_d(_Link_const_type __N, const_reference __V,
                       size_type const __L) const
  {
    _Node_compare_ compare(__L % __K, _M_acc, _M_cmp);
    return !(compare(__N->_M_value, __V) || compare(__V, __N->_M_value));
  }

  bool
  _M_matches_node_in_other_ds(_Link_const_type __N, const_reference __V,
                              size_type const __L = 0) const
  {
    size_type __i = __L;
    while ((__i = (__i + 1) % __K) != __L % __K)
      if (!_M_matches_node_in_d(__N, __V, __i)) return false;
    return true;
  }

  bool
  _M_matches_node(_Link_const_type __N, const_reference __V,
                  size_type __L = 0) const
  {
    return _M_matches_node_in_d(__N, __V, __L)
      && _M_matches_node_in_other_ds(__N, __V, __L);
  }

  size_type
  _M_count_within_range(_Link_const_type __N, _Region_ const& __REGION,
                       _Region_ const& __BOUNDS,
                       size_type const __L) const
    {
       size_type count = 0;
      if (__REGION.encloses(_S_value(__N)))
        {
           ++count;
        }
      if (_S_left(__N))
        {
          _Region_ __bounds(__BOUNDS);
          __bounds.set_high_bound(_S_value(__N), __L);
          if (__REGION.intersects_with(__bounds))
            count += _M_count_within_range(_S_left(__N),
                                 __REGION, __bounds, __L+1);
        }
      if (_S_right(__N))
        {
          _Region_ __bounds(__BOUNDS);
          __bounds.set_low_bound(_S_value(__N), __L);
          if (__REGION.intersects_with(__bounds))
            count += _M_count_within_range(_S_right(__N),
                                 __REGION, __bounds, __L+1);
        }

      return count;
    }


  template <class Visitor>
  Visitor
  _M_visit_within_range(Visitor visitor,
                       _Link_const_type N, _Region_ const& REGION,
                       _Region_ const& BOUNDS,
                       size_type const L) const
    {
      if (REGION.encloses(_S_value(N)))
        {
          visitor(_S_value(N));
        }
      if (_S_left(N))
        {
          _Region_ bounds(BOUNDS);
          bounds.set_high_bound(_S_value(N), L);
          if (REGION.intersects_with(bounds))
            visitor = _M_visit_within_range(visitor, _S_left(N),
                                 REGION, bounds, L+1);
        }
      if (_S_right(N))
        {
          _Region_ bounds(BOUNDS);
          bounds.set_low_bound(_S_value(N), L);
          if (REGION.intersects_with(bounds))
            visitor = _M_visit_within_range(visitor, _S_right(N),
                                 REGION, bounds, L+1);
        }

      return visitor;
    }



  template <typename _OutputIterator>
  _OutputIterator
  _M_find_within_range(_OutputIterator out,
                       _Link_const_type __N, _Region_ const& __REGION,
                       _Region_ const& __BOUNDS,
                       size_type const __L) const
    {
      if (__REGION.encloses(_S_value(__N)))
        {
          *out++ = _S_value(__N);
        }
      if (_S_left(__N))
        {
          _Region_ __bounds(__BOUNDS);
          __bounds.set_high_bound(_S_value(__N), __L);
          if (__REGION.intersects_with(__bounds))
            out = _M_find_within_range(out, _S_left(__N),
                                 __REGION, __bounds, __L+1);
        }
      if (_S_right(__N))
        {
          _Region_ __bounds(__BOUNDS);
          __bounds.set_low_bound(_S_value(__N), __L);
          if (__REGION.intersects_with(__bounds))
            out = _M_find_within_range(out, _S_right(__N),
                                 __REGION, __bounds, __L+1);
        }

      return out;
    }


  template <typename _Iter>
  void
  _M_optimise(_Iter const& __A, _Iter const& __B,
              size_type const __L)
  {
    if (__A == __B) return;
    _Node_compare_ compare(__L % __K, _M_acc, _M_cmp);
    _Iter __m = __A + (__B - __A) / 2;
    std::nth_element(__A, __m, __B, compare);
    this->insert(*__m);
    if (__m != __A) _M_optimise(__A, __m, __L+1);
    if (++__m != __B) _M_optimise(__m, __B, __L+1);
  }

  _Link_const_type
  _M_get_root() const
  {
     return const_cast<_Link_const_type>(_M_root);
  }

  _Link_type
  _M_get_root()
  {
     return _M_root;
  }

  void _M_set_root(_Link_type n)
  {
     _M_root = n;
  }

  _Link_const_type
  _M_get_leftmost() const
  {
    return static_cast<_Link_type>(_M_header._M_left);
  }

  void
  _M_set_leftmost( _Node_base * a )
  {
     _M_header._M_left = a;
  }

  _Link_const_type
  _M_get_rightmost() const
  {
    return static_cast<_Link_type>( _M_header._M_right );
  }

  void
  _M_set_rightmost( _Node_base * a )
  {
     _M_header._M_right = a;
  }

  static _Link_type
  _S_parent(_Base_ptr N)
  {
    return static_cast<_Link_type>( N->_M_parent );
  }

  static _Link_const_type
  _S_parent(_Base_const_ptr N)
  {
    return static_cast<_Link_const_type>( N->_M_parent );
  }

  static void
  _S_set_parent(_Base_ptr N, _Base_ptr p)
  {
    N->_M_parent = p;
  }

  static void
  _S_set_left(_Base_ptr N, _Base_ptr l)
  {
    N->_M_left = l;
  }

  static _Link_type
  _S_left(_Base_ptr N)
  {
    return static_cast<_Link_type>( N->_M_left );
  }

  static _Link_const_type
  _S_left(_Base_const_ptr N)
  {
    return static_cast<_Link_const_type>( N->_M_left );
  }

  static void
  _S_set_right(_Base_ptr N, _Base_ptr r)
  {
    N->_M_right = r;
  }

  static _Link_type
  _S_right(_Base_ptr N)
  {
    return static_cast<_Link_type>( N->_M_right );
  }

  static _Link_const_type
  _S_right(_Base_const_ptr N)
  {
    return static_cast<_Link_const_type>( N->_M_right );
  }

  static bool
  _S_is_leaf(_Base_const_ptr N)
  {
    return !_S_left(N) && !_S_right(N);
  }

  static const_reference
  _S_value(_Link_const_type N)
  {
    return N->_M_value;
  }

  static const_reference
  _S_value(_Base_const_ptr N)
  {
    return static_cast<_Link_const_type>(N)->_M_value;
  }

  static _Link_const_type
  _S_minimum(_Link_const_type __X)
  {
    return static_cast<_Link_const_type> ( _Node_base::_S_minimum(__X) );
  }

  static _Link_const_type
  _S_maximum(_Link_const_type __X)
  {
    return static_cast<_Link_const_type>( _Node_base::_S_maximum(__X) );
  }


  _Link_type
  _M_new_node(const_reference __V, //  = value_type(),
              _Base_ptr const __PARENT = NULL,
              _Base_ptr const __LEFT = NULL,
              _Base_ptr const __RIGHT = NULL)
  {
     typename _Base::NoLeakAlloc noleak(this);
     _Link_type new_node = noleak.get();
     _Base::_M_construct_node(new_node, __V, __PARENT, __LEFT, __RIGHT);
     noleak.disconnect();
     return new_node;
  }

  /* WHAT was this for?
  _Link_type
  _M_clone_node(_Link_const_type __X)
  {
    _Link_type __ret = _M_allocate_node(__X->_M_value);
    // TODO
    return __ret;
  }
  */

  void
  _M_delete_node(_Link_type __p)
  {
    _Base::_M_destroy_node(__p);
    _Base::_M_deallocate_node(__p);
  }

  _Link_type _M_root;
  _Node_base _M_header;
  size_type _M_count;
  _Acc _M_acc;
  _Cmp _M_cmp;
  _Dist _M_dist;

#ifdef KDTREE_DEFINE_OSTREAM_OPERATORS
  friend std::ostream&
  operator<<(std::ostream& o,
	 KDTree<__K, _Val, _Acc, _Dist, _Cmp, _Alloc> const& tree)
  {
    o << "meta node:   " << tree._M_header << std::endl;
    o << "root node:   " << tree._M_root << std::endl;

    if (tree.empty())
      return o << "[empty " << __K << "d-tree " << &tree << "]";

    o << "nodes total: " << tree.size() << std::endl;
    o << "dimensions:  " << __K << std::endl;

    typedef KDTree<__K, _Val, _Acc, _Dist, _Cmp, _Alloc> _Tree;
    typedef typename _Tree::_Link_type _Link_type;

    std::stack<_Link_const_type> s;
    s.push(tree._M_get_root());

    while (!s.empty())
      {
        _Link_const_type n = s.top();
        s.pop();
        o << *n << std::endl;
        if (_Tree::_S_left(n)) s.push(_Tree::_S_left(n));
        if (_Tree::_S_right(n)) s.push(_Tree::_S_right(n));
      }

    return o;
  }
#endif

}; // class KDTree


} // namespace KDTree

#endif // include guard

/* COPYRIGHT --
 *
 * This file is part of libkdtree++, a C++ template KD-Tree sorting container.
 * libkdtree++ is (c) 2004-2007 Martin F. Krafft <libkdtree@pobox.madduck.net>
 * and Sylvain Bougerel <sylvain.bougerel.devel@gmail.com> distributed under the
 * terms of the Artistic License 2.0. See the ./COPYING file in the source tree
 * root for more information.
 * Parts of this file are (c) 2004-2007 Paul Harris <paulharris@computer.org>.
 *
 * THIS PACKAGE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES
 * OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

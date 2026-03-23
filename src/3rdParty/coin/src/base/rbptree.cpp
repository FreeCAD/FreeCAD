/**************************************************************************\
 * Copyright (c) Kongsberg Oil & Gas Technologies AS
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 
 * Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * 
 * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 * 
 * Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
\**************************************************************************/

/*
 * This implementation is heavily based on the Red-Black Tree
 * description in "Introduction to Algorithms" by Thomas H. Cormen et.al.
 * Some functions have a page number in the function description. This
 * page number refers to the corresponding pseude-code in the book.
 */

#include <Inventor/C/base/rbptree.h>

#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <cstdio>

#include <Inventor/C/base/string.h>
#include <Inventor/C/errors/debugerror.h>

#include "coindefs.h"
#include "tidbitsp.h"
#include "threads/threadsutilp.h"

#ifndef COIN_WORKAROUND_NO_USING_STD_FUNCS
using std::malloc;
using std::free;
#endif // !COIN_WORKAROUND_NO_USING_STD_FUNCS

/* ********************************************************************** */

/*!
  \typedef struct cc_rbptree_node cc_rbptree_node

  The type definition for the rbptree node structure.
*/

/*!
  \struct cc_rbptree rbptree.h Inventor/C/base/rbptree.h

  The structure for the whole rbptree.
*/

/*!
  \typedef struct cc_rbptree cc_rbptree

  The type definition for the whole rbptree structure.
*/

/*!
  \typedef void cc_rbptree_traversecb(void * p, void * data, void * closure)

  The type definition for a tree traversal callback function.
*/

static const int RBPTREE_RED = 0;
static const int RBPTREE_BLACK = 1;

struct cc_rbptree_node {
  struct cc_rbptree_node * left;
  struct cc_rbptree_node * right;
  struct cc_rbptree_node * parent;
  char * pointer;
  void * data;
  unsigned char color;
};

static cc_rbptree_node rbptree_sentinel;
static SbBool rbptree_isinitialized = FALSE;

extern "C" {

static void
rbptree_atexit_cleanup(void)
{
  rbptree_isinitialized = FALSE;
}

}  // extern "C"

/*
 * left-rotate the subgrap under node \c x.
 */
static void
rbptree_left_rotate(cc_rbptree * t, cc_rbptree_node * x)
{
  /* page 266 */
  cc_rbptree_node * y, * nil;

  nil = &rbptree_sentinel;

  assert(x != nil);
  y = x->right;
  assert(y != nil);

  x->right = y->left;

  if (y->left != nil) {
    y->left->parent = x;
  }
  y->parent = x->parent;
  if (x->parent == nil) {
    t->root = y;
  }
  else if (x == x->parent->left) {
    x->parent->left = y;
  }
  else {
    x->parent->right = y;
  }
  y->left = x;
  x->parent = y;
}

/*
 * right-rotate the subgrap under node \c y.
 */
static void
rbptree_right_rotate(cc_rbptree * t, cc_rbptree_node * y)
{
  /* page 266 (inverted) */
  cc_rbptree_node * nil;
  cc_rbptree_node * x;

  nil = &rbptree_sentinel;

  assert(y != nil);
  x = y->left;
  assert(x != nil);
  y->left = x->right;

  if (x->right != nil) {
    x->right->parent = y;
  }
  x->parent = y->parent;
  if (y->parent == nil) {
    t->root = x;
  }
  else if (y == y->parent->right) {
    y->parent->right = x;
  }
  else {
    y->parent->left = x;
  }
  x->right = y;
  y->parent = x;
}

/*
 * return the node with the minimum value.
 */
static cc_rbptree_node *
rbptree_tree_minimum(cc_rbptree * COIN_UNUSED_ARG(t), cc_rbptree_node * x)
{
  /* page 248 */
  cc_rbptree_node * nil = &rbptree_sentinel;
  while (x->left != nil) {
    x = x->left;
  }
  return x;
}

/*
 * return the node with the smallest pointer greater than x->pointer
 */
static cc_rbptree_node *
rbptree_tree_successor(cc_rbptree * t, cc_rbptree_node * x)
{
  /* page 249 */
  cc_rbptree_node *y, * nil = &rbptree_sentinel;
  if (x->right != nil) {
    return rbptree_tree_minimum(t, x->right);
  }
  y = x->parent;
  while (y != nil && x == y->right) {
    x = y;
    y = y->parent;
  }
  return y;
}

/*
 * regular (non red-black) bintree insert
 */
static void
rbptree_bintree_insert(cc_rbptree * t, cc_rbptree_node * z)
{
  cc_rbptree_node * x, * y, * nil;
  nil = &rbptree_sentinel;

  y = nil;
  x = t->root;
  nil = &rbptree_sentinel;

  while (x != nil) {
    y = x;
    if (z->pointer < x->pointer) {
      x = x->left;
    }
    else {
      x = x->right;
    }
  }

  z->parent = y;
  if (y == nil) {
    t->root = z;
  }
  else if (z->pointer < y->pointer) {
    y->left = z;
  }
  else {
    y->right = z;
  }
}

/*
 * allocate a new rbptree node. Initialize pointer values
 * to point to the sentinel.
 */
static cc_rbptree_node *
rbptree_new_node(cc_rbptree * COIN_UNUSED_ARG(t))
{
  cc_rbptree_node * x = static_cast<cc_rbptree_node*>(
    malloc(sizeof(cc_rbptree_node)));

  x->left = &rbptree_sentinel;
  x->right = &rbptree_sentinel;
  x->parent = &rbptree_sentinel;
  return x;
}

/*
 * rebalance the tree after a node has been removed.
 */
static void
rbptree_delete_fixup(cc_rbptree * t, cc_rbptree_node * x)
{
  /* page 274 */
  cc_rbptree_node * w, * nil;

  nil = &rbptree_sentinel;

  while (x != t->root && x->color == RBPTREE_BLACK) {
    if (x == x->parent->left) { /* x is left child */
      w = x->parent->right;
      assert(w != nil);
      if (w->color == RBPTREE_RED) {
        w->color = RBPTREE_BLACK;
        x->parent->color = RBPTREE_RED;
        rbptree_left_rotate(t, x->parent);
        w = x->parent->right;
      }
      if (w->left->color == RBPTREE_BLACK && w->right->color == RBPTREE_BLACK) {
        w->color = RBPTREE_RED;
        x = x->parent;
      }
      else {
        if (w->right->color == RBPTREE_BLACK) {
          w->left->color = RBPTREE_BLACK;
          w->color = RBPTREE_RED;
          rbptree_right_rotate(t, w);
          w = x->parent->right;
        }
        w->color = x->parent->color;
        x->parent->color = RBPTREE_BLACK;
        w->right->color = RBPTREE_BLACK;
        rbptree_left_rotate(t, x->parent);
        x = t->root;
      }
    }
    else { /* x is right child */
      w = x->parent->left;
      assert(w != nil);
      if (w->color == RBPTREE_RED) {
        w->color = RBPTREE_BLACK;
        x->parent->color = RBPTREE_RED;
        rbptree_right_rotate(t, x->parent);
        w = x->parent->left;
      }
      if (w->right->color == RBPTREE_BLACK && w->left->color == RBPTREE_BLACK) {
        w->color = RBPTREE_RED;
        x = x->parent;
      }
      else {
        if (w->left->color == RBPTREE_BLACK) {
          w->right->color = RBPTREE_BLACK;
          w->color = RBPTREE_RED;
          rbptree_left_rotate(t, w);
          w = x->parent->left;
        }
        w->color = x->parent->color;
        x->parent->color = RBPTREE_BLACK;
        w->left->color = RBPTREE_BLACK;
        rbptree_right_rotate(t, x->parent);
        x = t->root;
      }
    }
  }
  x->color = RBPTREE_BLACK;
}


/*
 * remove a node from the tree. the tree is rebalanced
 * when necessary.
 */
static void
rbptree_remove_node(cc_rbptree * t, cc_rbptree_node * z)
{
  cc_rbptree_node * x, * y, * nil;
  nil = &rbptree_sentinel;

  /* page 273 */
  if (z->left == nil || z->right == nil) {
    y = z;
  }
  else {
    y = rbptree_tree_successor(t, z);
  }
  if (y->left != nil) {
    x = y->left;
  }
  else {
    x = y->right;
  }
  x->parent = y->parent;

  if (y->parent == nil) {
    t->root = x;
  }
  else {
    if (y == y->parent->left) {
      y->parent->left = x;
    }
    else {
      y->parent->right =x;
    }
  }
  if (y != z) {
    z->pointer = y->pointer;
    z->data = y->data;
  }
  if (y->color == RBPTREE_BLACK) {
    rbptree_delete_fixup(t, x);
  }
  /* free memory for node */
  free(static_cast<void*>(y));
  t->counter--;
}

/*!
 * Initialize \c t. This is needed before making any operations
 * on the tree.
 */
void
cc_rbptree_init(cc_rbptree * t)
{
  CC_GLOBAL_LOCK;
  if (!rbptree_isinitialized) {
    rbptree_sentinel.left = NULL;
    rbptree_sentinel.right = NULL;
    rbptree_sentinel.parent = NULL;
    rbptree_sentinel.pointer = NULL;
    rbptree_sentinel.color = RBPTREE_BLACK;
    rbptree_isinitialized = TRUE;
    coin_atexit(static_cast<coin_atexit_f*>(rbptree_atexit_cleanup), CC_ATEXIT_NORMAL);
  }
  CC_GLOBAL_UNLOCK;

  t->root = &rbptree_sentinel;
  t->counter = 0;
}

/*
 * recursively delete the \c x subgraph.
 */
static void
rbptree_recursive_clean(cc_rbptree_node * x)
{
  cc_rbptree_node * nil = &rbptree_sentinel;
  assert(x != nil);

  if (x->left != nil) rbptree_recursive_clean(x->left);
  if (x->right != nil) rbptree_recursive_clean(x->right);
  free(static_cast<void*>(x));
}

/*!
 * Delete all nodes in \c t. After this call, the tree will
 * be reinitialized to an empty tree.
 */
void
cc_rbptree_clean(cc_rbptree * t)
{
  if (t->root != &rbptree_sentinel) {
    rbptree_recursive_clean(t->root);
    cc_rbptree_init(t);
  }
}



/*!
 * Insert a new value \c p into \c t.
 */
void
cc_rbptree_insert(cc_rbptree * t, void * p, void * data)
{
  cc_rbptree_node * x, * y;

  if (t->counter < 2) {
    /* put value is an inline value */
    t->inlinepointer[t->counter] = p;
    t->inlinedata[t->counter] = data;
    t->counter++;
    return;
  }

  /* page 268 */
  x = rbptree_new_node(t);
  x->pointer = static_cast<char*>(p);
  x->data = data;

  /* do normal bintree insert */
  rbptree_bintree_insert(t, x);
  x->color = RBPTREE_RED;

  /* fix up the modified tree */
  while (x != t->root && x->parent->color == RBPTREE_RED) {
    if (x->parent == x->parent->parent->left) {
      assert(x->parent != t->root);
      y = x->parent->parent->right;
      if (y->color == RBPTREE_RED) {
        x->parent->color = RBPTREE_BLACK;
        y->color = RBPTREE_BLACK;
        x->parent->parent->color = RBPTREE_RED;
        x = x->parent->parent;
      }
      else {
        if (x == x->parent->right) {
          x = x->parent;
          rbptree_left_rotate(t, x);
        }
        x->parent->color = RBPTREE_BLACK;
        x->parent->parent->color = RBPTREE_RED;
        rbptree_right_rotate(t, x->parent->parent);
      }
    }
    else {
      assert(x->parent != t->root);
      y = x->parent->parent->left;
      if (y->color == RBPTREE_RED) {
        x->parent->color = RBPTREE_BLACK;
        y->color = RBPTREE_BLACK;
        x->parent->parent->color = RBPTREE_RED;
        x = x->parent->parent;
      }
      else {
        if (x == x->parent->left) {
          x = x->parent;
          rbptree_right_rotate(t, x);
        }
        x->parent->color = RBPTREE_BLACK;
        x->parent->parent->color = RBPTREE_RED;
        rbptree_left_rotate(t, x->parent->parent);
      }
    }
  }
  t->root->color = RBPTREE_BLACK;
  t->counter++;
}

/*
 * binary search tree for the pointer value. Returns rbptree_sentinel
 * if pointer is not found.
*/
static cc_rbptree_node *
rbptree_find(cc_rbptree * t, void * pointer)
{
  /* page 248 */
  cc_rbptree_node * x, * nil;
  char * p = static_cast<char*>(pointer);

  x = t->root;
  nil = &rbptree_sentinel;

  while (x != nil && x->pointer != p) {
    if (p < x->pointer) {
      x = x->left;
    }
    else {
      x = x->right;
    }
  }
  return x;
}


static void
rbptree_remove_inline(cc_rbptree * t, const int idx)
{
  if (idx == 0) {
    if (t->counter > 1) {
      /* move slot 1 into slot 0 */
      t->inlinepointer[0] = t->inlinepointer[1];
      t->inlinedata[0] = t->inlinedata[1];
    }
  }
  /* test if some node should be moved into inline slot 1 */
  if (t->counter > 2) {
    /* copy root node data into slot 1 */
    t->inlinepointer[1] = static_cast<void*>(t->root->pointer);
    t->inlinedata[1] = t->root->data;

    /* remove root node from tree. This will decrement t->counter */
    rbptree_remove_node(t, t->root);
  }
  else {
    /* just decrement counter, no need to copy */
    t->counter--;
  }
}

/*!
 * Remove the (first) node with value \c p. Returns \e TRUE if \c p
 * is found and removed, \e FALSE otherwise.
 */
SbBool
cc_rbptree_remove(cc_rbptree * t, void * p)
{
  cc_rbptree_node *z, * nil;
  nil = &rbptree_sentinel;

  if (t->counter == 0) return FALSE;
  if (t->inlinepointer[0] == p) {
    rbptree_remove_inline(t, 0);
    return TRUE;
  }
  if (t->counter > 1 && t->inlinepointer[1] == p) {
    rbptree_remove_inline(t, 1);
    return TRUE;
  }

  z = rbptree_find(t, p);
  if (z == nil) {
    return FALSE;
  }
  assert(z->pointer == static_cast<char *>(p));
  /* remove node from tree */
  rbptree_remove_node(t, z);
  return TRUE;
}

/*!
  Returns the total numbers of items in the tree
*/
uint32_t
cc_rbptree_size(const cc_rbptree * t)
{
  return t->counter;
}

static void
rbptree_rec_traverse(cc_rbptree_node * x, cc_rbptree_traversecb * func, void * closure)
{
  cc_rbptree_node * nil = &rbptree_sentinel;

  func(static_cast<void*>(x->pointer), x->data, closure);
  if (x->left != nil) rbptree_rec_traverse(x->left, func, closure);
  if (x->right != nil) rbptree_rec_traverse(x->right, func, closure);
}

/*!
  Traverse the tree \c t
*/
void
cc_rbptree_traverse(const cc_rbptree * t, cc_rbptree_traversecb * func, void * closure)
{
  if (t->counter > 0) {
    func(static_cast<void*>(t->inlinepointer[0]), t->inlinedata[0], closure);
    if (t->counter > 1) {
      func(static_cast<void*>(t->inlinepointer[1]), t->inlinedata[1], closure);
    }
  }
  if (t->root != &rbptree_sentinel) {
    rbptree_rec_traverse(t->root, func, closure);
  }
}

static void
rbptree_debug(const cc_rbptree_node * x, const int idx)
{
  int i;
  cc_string str;

  cc_string_construct(&str);

  for (i = 0; i < idx; i++) { cc_string_append_text(&str, " "); }
  cc_string_sprintf(&str, "level %d ", idx/2);
  if (x->parent == &rbptree_sentinel) { cc_string_append_text(&str, "(head)"); }
  else if (x->parent->left == x) { cc_string_append_text(&str, "(left)"); }
  else { cc_string_append_text(&str, "(right)"); }

  cc_debugerror_postinfo("rbptree_debug", ": %p ==> %s\n", x->pointer,
                         x->color == RBPTREE_BLACK ? "black" : "red");

  if (x->left != &rbptree_sentinel) rbptree_debug(x->left, idx + 2);
  if (x->right != &rbptree_sentinel) rbptree_debug(x->right, idx + 2);

  cc_string_clean(&str);
}

/*!
 * for debugging only. Prints the tree to stdout.
 */
void
cc_rbptree_debug(const cc_rbptree * t)
{
  const cc_rbptree_node * x = t->root;

  if (x != &rbptree_sentinel) rbptree_debug(x, 0);

}

#ifdef COIN_TEST_SUITE

#include <cmath>
#include <Inventor/lists/SbList.h>

#define FILL_TIMES (50)
#define FILL_COUNT (10000)

BOOST_AUTO_TEST_CASE(rbptree_stress)
{
  srand(123);
  cc_rbptree tree;
  cc_rbptree_init(&tree);

  int i;
  for (int c = 0; c < FILL_TIMES; ++c) {
    SbList<void *> values;
    for (i = 0; i < FILL_COUNT; ++i) {
      void * entry = reinterpret_cast<void *>(static_cast<uintptr_t>(rand()));
      cc_rbptree_insert(&tree, entry, NULL);
      values.append(entry);
    }
    BOOST_ASSERT(cc_rbptree_size(&tree) == FILL_COUNT);

    if ((c & 1) == 0) {
      for (i = (FILL_COUNT - 1); i >= 0; --i) cc_rbptree_remove(&tree, values[i]);
    } else {
      for (i = 0; i < FILL_COUNT; ++i) cc_rbptree_remove(&tree, values[i]);
    }
    BOOST_ASSERT(cc_rbptree_size(&tree) == 0);
  }

  for (int c = 0; c < FILL_TIMES; ++c) {
    SbList<void *> values;
    for (i = 0; i < FILL_COUNT; ++i) {
      void * entry = reinterpret_cast<void *>(static_cast<uintptr_t>(((c & 2) == 0) ? i : (FILL_COUNT - i)));
      cc_rbptree_insert(&tree, entry, NULL);
      values.append(entry);
    }
    BOOST_ASSERT(cc_rbptree_size(&tree) == FILL_COUNT);

    if ((c & 1) == 0) {
      for (i = (FILL_COUNT - 1); i >= 0; --i) cc_rbptree_remove(&tree, values[i]);
    } else {
      for (i = 0; i < FILL_COUNT; ++i) cc_rbptree_remove(&tree, values[i]);
    }
    BOOST_ASSERT(cc_rbptree_size(&tree) == 0);

  }


  cc_rbptree_clean(&tree);
}

#endif // COIN_TEST_SUITE
